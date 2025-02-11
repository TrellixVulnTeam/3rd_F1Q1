
/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "GrGpu.h"

#include "GrBufferAllocPool.h"
#include "GrContext.h"
#include "GrDrawTargetCaps.h"
#include "GrIndexBuffer.h"
#include "GrStencilBuffer.h"
#include "GrVertexBuffer.h"

////////////////////////////////////////////////////////////////////////////////

#define DEBUG_INVAL_BUFFER    0xdeadcafe
#define DEBUG_INVAL_START_IDX -1

GrGpu::GrGpu(GrContext* context)
    : fResetTimestamp(kExpiredTimestamp+1)
    , fResetBits(kAll_GrBackendState)
    , fQuadIndexBuffer(NULL)
    , fContext(context) {
}

GrGpu::~GrGpu() {
    SkSafeSetNull(fQuadIndexBuffer);
    SkSafeUnref(fGeoSrcState.fVertexBuffer);
    SkSafeUnref(fGeoSrcState.fIndexBuffer);
}

void GrGpu::contextAbandoned() {}

////////////////////////////////////////////////////////////////////////////////

GrTexture* GrGpu::createTexture(const GrSurfaceDesc& desc,
                                const void* srcData, size_t rowBytes) {
    if (!this->caps()->isConfigTexturable(desc.fConfig)) {
        return NULL;
    }

    if ((desc.fFlags & kRenderTarget_GrSurfaceFlag) &&
        !this->caps()->isConfigRenderable(desc.fConfig, desc.fSampleCnt > 0)) {
        return NULL;
    }

    GrTexture *tex = NULL;
    if (GrPixelConfigIsCompressed(desc.fConfig)) {
        // We shouldn't be rendering into this
        SkASSERT((desc.fFlags & kRenderTarget_GrSurfaceFlag) == 0);

        if (!this->caps()->npotTextureTileSupport() &&
            (!SkIsPow2(desc.fWidth) || !SkIsPow2(desc.fHeight))) {
            return NULL;
        }

        this->handleDirtyContext();
        tex = this->onCreateCompressedTexture(desc, srcData);
    } else {
        this->handleDirtyContext();
        tex = this->onCreateTexture(desc, srcData, rowBytes);
        if (tex &&
            (kRenderTarget_GrSurfaceFlag & desc.fFlags) &&
            !(kNoStencil_GrSurfaceFlag & desc.fFlags)) {
            SkASSERT(tex->asRenderTarget());
            // TODO: defer this and attach dynamically
            if (!this->attachStencilBufferToRenderTarget(tex->asRenderTarget())) {
                tex->unref();
                return NULL;
            }
        }
    }
    return tex;
}

bool GrGpu::attachStencilBufferToRenderTarget(GrRenderTarget* rt) {
    SkASSERT(NULL == rt->getStencilBuffer());
    SkAutoTUnref<GrStencilBuffer> sb(
        this->getContext()->findAndRefStencilBuffer(rt->width(), rt->height(), rt->numSamples()));
    if (sb) {
        rt->setStencilBuffer(sb);
        bool attached = this->attachStencilBufferToRenderTarget(sb, rt);
        if (!attached) {
            rt->setStencilBuffer(NULL);
        }
        return attached;
    }
    if (this->createStencilBufferForRenderTarget(rt, rt->width(), rt->height())) {
        // Right now we're clearing the stencil buffer here after it is
        // attached to an RT for the first time. When we start matching
        // stencil buffers with smaller color targets this will no longer
        // be correct because it won't be guaranteed to clear the entire
        // sb.
        // We used to clear down in the GL subclass using a special purpose
        // FBO. But iOS doesn't allow a stencil-only FBO. It reports unsupported
        // FBO status.
        this->clearStencil(rt);
        return true;
    } else {
        return false;
    }
}

GrTexture* GrGpu::wrapBackendTexture(const GrBackendTextureDesc& desc) {
    this->handleDirtyContext();
    GrTexture* tex = this->onWrapBackendTexture(desc);
    if (NULL == tex) {
        return NULL;
    }
    // TODO: defer this and attach dynamically
    GrRenderTarget* tgt = tex->asRenderTarget();
    if (tgt &&
        !this->attachStencilBufferToRenderTarget(tgt)) {
        tex->unref();
        return NULL;
    } else {
        return tex;
    }
}

GrRenderTarget* GrGpu::wrapBackendRenderTarget(const GrBackendRenderTargetDesc& desc) {
    this->handleDirtyContext();
    return this->onWrapBackendRenderTarget(desc);
}

GrVertexBuffer* GrGpu::createVertexBuffer(size_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateVertexBuffer(size, dynamic);
}

GrIndexBuffer* GrGpu::createIndexBuffer(size_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateIndexBuffer(size, dynamic);
}

GrIndexBuffer* GrGpu::createInstancedIndexBuffer(const uint16_t* pattern,
                                                 int patternSize,
                                                 int reps,
                                                 int vertCount,
                                                 bool isDynamic) {
    size_t bufferSize = patternSize * reps * sizeof(uint16_t);
    GrGpu* me = const_cast<GrGpu*>(this);
    GrIndexBuffer* buffer = me->createIndexBuffer(bufferSize, isDynamic);
    if (buffer) {
        uint16_t* data = (uint16_t*) buffer->map();
        bool useTempData = (NULL == data);
        if (useTempData) {
            data = SkNEW_ARRAY(uint16_t, reps * patternSize);
        }
        for (int i = 0; i < reps; ++i) {
            int baseIdx = i * patternSize;
            uint16_t baseVert = (uint16_t)(i * vertCount);
            for (int j = 0; j < patternSize; ++j) {
                data[baseIdx+j] = baseVert + pattern[j];
            }
        }
        if (useTempData) {
            if (!buffer->updateData(data, bufferSize)) {
                SkFAIL("Can't get indices into buffer!");
            }
            SkDELETE_ARRAY(data);
        } else {
            buffer->unmap();
        }
    }
    return buffer;
}

void GrGpu::clear(const SkIRect* rect,
                  GrColor color,
                  bool canIgnoreRect,
                  GrRenderTarget* renderTarget) {
    SkASSERT(renderTarget);
    this->handleDirtyContext();
    this->onClear(renderTarget, rect, color, canIgnoreRect);
}

void GrGpu::clearStencilClip(const SkIRect& rect,
                             bool insideClip,
                             GrRenderTarget* renderTarget) {
    SkASSERT(renderTarget);
    this->handleDirtyContext();
    this->onClearStencilClip(renderTarget, rect, insideClip);
}

bool GrGpu::readPixels(GrRenderTarget* target,
                       int left, int top, int width, int height,
                       GrPixelConfig config, void* buffer,
                       size_t rowBytes) {
    this->handleDirtyContext();
    return this->onReadPixels(target, left, top, width, height,
                              config, buffer, rowBytes);
}

bool GrGpu::writeTexturePixels(GrTexture* texture,
                               int left, int top, int width, int height,
                               GrPixelConfig config, const void* buffer,
                               size_t rowBytes) {
    this->handleDirtyContext();
    return this->onWriteTexturePixels(texture, left, top, width, height,
                                      config, buffer, rowBytes);
}

void GrGpu::resolveRenderTarget(GrRenderTarget* target) {
    SkASSERT(target);
    this->handleDirtyContext();
    this->onResolveRenderTarget(target);
}

void GrGpu::initCopySurfaceDstDesc(const GrSurface* src, GrSurfaceDesc* desc) {
    // Make the dst of the copy be a render target because the default copySurface draws to the dst.
    desc->fOrigin = kDefault_GrSurfaceOrigin;
    desc->fFlags = kRenderTarget_GrSurfaceFlag | kNoStencil_GrSurfaceFlag;
    desc->fConfig = src->config();
}

typedef GrTraceMarkerSet::Iter TMIter;
void GrGpu::saveActiveTraceMarkers() {
    if (this->caps()->gpuTracingSupport()) {
        SkASSERT(0 == fStoredTraceMarkers.count());
        fStoredTraceMarkers.addSet(fActiveTraceMarkers);
        for (TMIter iter = fStoredTraceMarkers.begin(); iter != fStoredTraceMarkers.end(); ++iter) {
            this->removeGpuTraceMarker(&(*iter));
        }
    }
}

void GrGpu::restoreActiveTraceMarkers() {
    if (this->caps()->gpuTracingSupport()) {
        SkASSERT(0 == fActiveTraceMarkers.count());
        for (TMIter iter = fStoredTraceMarkers.begin(); iter != fStoredTraceMarkers.end(); ++iter) {
            this->addGpuTraceMarker(&(*iter));
        }
        for (TMIter iter = fActiveTraceMarkers.begin(); iter != fActiveTraceMarkers.end(); ++iter) {
            this->fStoredTraceMarkers.remove(*iter);
        }
    }
}

void GrGpu::addGpuTraceMarker(const GrGpuTraceMarker* marker) {
    if (this->caps()->gpuTracingSupport()) {
        SkASSERT(fGpuTraceMarkerCount >= 0);
        this->fActiveTraceMarkers.add(*marker);
        this->didAddGpuTraceMarker();
        ++fGpuTraceMarkerCount;
    }
}

void GrGpu::removeGpuTraceMarker(const GrGpuTraceMarker* marker) {
    if (this->caps()->gpuTracingSupport()) {
        SkASSERT(fGpuTraceMarkerCount >= 1);
        this->fActiveTraceMarkers.remove(*marker);
        this->didRemoveGpuTraceMarker();
        --fGpuTraceMarkerCount;
    }
}

void GrGpu::setVertexSourceToBuffer(const GrVertexBuffer* buffer, size_t vertexStride) {
    SkSafeUnref(fGeoSrcState.fVertexBuffer);
    fGeoSrcState.fVertexBuffer = buffer;
    buffer->ref();
    fGeoSrcState.fVertexSize = vertexStride;
}

void GrGpu::setIndexSourceToBuffer(const GrIndexBuffer* buffer) {
    SkSafeUnref(fGeoSrcState.fIndexBuffer);
    fGeoSrcState.fIndexBuffer = buffer;
    buffer->ref();
}

////////////////////////////////////////////////////////////////////////////////

static const int MAX_QUADS = 1 << 12; // max possible: (1 << 14) - 1;

GR_STATIC_ASSERT(4 * MAX_QUADS <= 65535);

static const uint16_t gQuadIndexPattern[] = {
  0, 1, 2, 0, 2, 3
};

const GrIndexBuffer* GrGpu::getQuadIndexBuffer() const {
    if (NULL == fQuadIndexBuffer || fQuadIndexBuffer->wasDestroyed()) {
        SkSafeUnref(fQuadIndexBuffer);
        GrGpu* me = const_cast<GrGpu*>(this);
        fQuadIndexBuffer = me->createInstancedIndexBuffer(gQuadIndexPattern,
                                                          6,
                                                          MAX_QUADS,
                                                          4);
    }

    return fQuadIndexBuffer;
}

////////////////////////////////////////////////////////////////////////////////

void GrGpu::draw(const GrOptDrawState& ds,
                 const GrDrawTarget::DrawInfo& info,
                 const GrClipMaskManager::ScissorState& scissorState) {
    this->handleDirtyContext();
    if (!this->flushGraphicsState(ds,
                                  PrimTypeToDrawType(info.primitiveType()),
                                  scissorState,
                                  info.getDstCopy())) {
        return;
    }
    this->onDraw(ds, info);
}

void GrGpu::stencilPath(const GrOptDrawState& ds,
                        const GrPath* path,
                        const GrClipMaskManager::ScissorState& scissorState,
                        const GrStencilSettings& stencilSettings) {
    this->handleDirtyContext();

    if (!this->flushGraphicsState(ds, kStencilPath_DrawType, scissorState, NULL)) {
        return;
    }

    this->pathRendering()->stencilPath(path, stencilSettings);
}


void GrGpu::drawPath(const GrOptDrawState& ds,
                     const GrPath* path,
                     const GrClipMaskManager::ScissorState& scissorState,
                     const GrStencilSettings& stencilSettings,
                     const GrDeviceCoordTexture* dstCopy) {
    this->handleDirtyContext();

    if (!this->flushGraphicsState(ds, kDrawPath_DrawType, scissorState, dstCopy)) {
        return;
    }

    this->pathRendering()->drawPath(path, stencilSettings);
}

void GrGpu::drawPaths(const GrOptDrawState& ds,
                      const GrPathRange* pathRange,
                      const uint32_t indices[],
                      int count,
                      const float transforms[],
                      GrDrawTarget::PathTransformType transformsType,
                      const GrClipMaskManager::ScissorState& scissorState,
                      const GrStencilSettings& stencilSettings,
                      const GrDeviceCoordTexture* dstCopy) {
    this->handleDirtyContext();

    if (!this->flushGraphicsState(ds, kDrawPaths_DrawType, scissorState, dstCopy)) {
        return;
    }

    pathRange->willDrawPaths(indices, count);
    this->pathRendering()->drawPaths(pathRange, indices, count, transforms, transformsType,
                                     stencilSettings);
}
