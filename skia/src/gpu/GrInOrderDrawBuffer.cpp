/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GrInOrderDrawBuffer.h"

#include "GrBufferAllocPool.h"
#include "GrDrawTargetCaps.h"
#include "GrGpu.h"
#include "GrOptDrawState.h"
#include "GrTemplates.h"
#include "GrTextStrike.h"
#include "GrTexture.h"

GrInOrderDrawBuffer::GrInOrderDrawBuffer(GrGpu* gpu,
                                         GrVertexBufferAllocPool* vertexPool,
                                         GrIndexBufferAllocPool* indexPool)
    : INHERITED(gpu->getContext())
    , fCmdBuffer(kCmdBufferInitialSizeInBytes)
    , fLastState(NULL)
    , fDstGpu(gpu)
    , fVertexPool(*vertexPool)
    , fIndexPool(*indexPool)
    , fFlushing(false)
    , fDrawID(0) {

    fDstGpu->ref();
    fCaps.reset(SkRef(fDstGpu->caps()));

    SkASSERT(vertexPool);
    SkASSERT(indexPool);

    GeometryPoolState& poolState = fGeoPoolStateStack.push_back();
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fUsedPoolIndexBytes = 0;
#ifdef SK_DEBUG
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)~0;
    poolState.fPoolStartVertex = ~0;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)~0;
    poolState.fPoolStartIndex = ~0;
#endif
    this->reset();
}

GrInOrderDrawBuffer::~GrInOrderDrawBuffer() {
    this->reset();
    // This must be called by before the GrDrawTarget destructor
    this->releaseGeometry();
    fDstGpu->unref();
}

////////////////////////////////////////////////////////////////////////////////

namespace {
void get_vertex_bounds(const void* vertices,
                       size_t vertexSize,
                       int vertexCount,
                       SkRect* bounds) {
    SkASSERT(vertexSize >= sizeof(SkPoint));
    SkASSERT(vertexCount > 0);
    const SkPoint* point = static_cast<const SkPoint*>(vertices);
    bounds->fLeft = bounds->fRight = point->fX;
    bounds->fTop = bounds->fBottom = point->fY;
    for (int i = 1; i < vertexCount; ++i) {
        point = reinterpret_cast<SkPoint*>(reinterpret_cast<intptr_t>(point) + vertexSize);
        bounds->growToInclude(point->fX, point->fY);
    }
}
}


namespace {

extern const GrVertexAttrib kRectAttribs[] = {
    {kVec2f_GrVertexAttribType,  0,                               kPosition_GrVertexAttribBinding},
    {kVec4ub_GrVertexAttribType, sizeof(SkPoint),                 kColor_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType,  sizeof(SkPoint)+sizeof(GrColor), kLocalCoord_GrVertexAttribBinding},
};
}

/** We always use per-vertex colors so that rects can be batched across color changes. Sometimes we
    have explicit local coords and sometimes not. We *could* always provide explicit local coords
    and just duplicate the positions when the caller hasn't provided a local coord rect, but we
    haven't seen a use case which frequently switches between local rect and no local rect draws.

    The color param is used to determine whether the opaque hint can be set on the draw state.
    The caller must populate the vertex colors itself.

    The vertex attrib order is always pos, color, [local coords].
 */
static void set_vertex_attributes(GrDrawState* drawState, bool hasLocalCoords, GrColor color) {
    if (hasLocalCoords) {
        drawState->setVertexAttribs<kRectAttribs>(3, 2 * sizeof(SkPoint) + sizeof(SkColor));
    } else {
        drawState->setVertexAttribs<kRectAttribs>(2, sizeof(SkPoint) + sizeof(SkColor));
    }
    if (0xFF == GrColorUnpackA(color)) {
        drawState->setHint(GrDrawState::kVertexColorsAreOpaque_Hint, true);
    }
}

enum {
    kTraceCmdBit = 0x80,
    kCmdMask = 0x7f,
};

static inline uint8_t add_trace_bit(uint8_t cmd) { return cmd | kTraceCmdBit; }

static inline uint8_t strip_trace_bit(uint8_t cmd) { return cmd & kCmdMask; }

static inline bool cmd_has_trace_marker(uint8_t cmd) { return SkToBool(cmd & kTraceCmdBit); }

void GrInOrderDrawBuffer::onDrawRect(const SkRect& rect,
                                     const SkRect* localRect,
                                     const SkMatrix* localMatrix) {
    GrDrawState* drawState = this->drawState();

    GrColor color = drawState->getColor();

    set_vertex_attributes(drawState, SkToBool(localRect),  color);

    AutoReleaseGeometry geo(this, 4, 0);
    if (!geo.succeeded()) {
        SkDebugf("Failed to get space for vertices!\n");
        return;
    }

    // Go to device coords to allow batching across matrix changes
    SkMatrix matrix = drawState->getViewMatrix();

    // When the caller has provided an explicit source rect for a stage then we don't want to
    // modify that stage's matrix. Otherwise if the effect is generating its source rect from
    // the vertex positions then we have to account for the view matrix change.
    GrDrawState::AutoViewMatrixRestore avmr;
    if (!avmr.setIdentity(drawState)) {
        return;
    }

    size_t vstride = drawState->getVertexStride();

    geo.positions()->setRectFan(rect.fLeft, rect.fTop, rect.fRight, rect.fBottom, vstride);
    matrix.mapPointsWithStride(geo.positions(), vstride, 4);

    SkRect devBounds;
    // since we already computed the dev verts, set the bounds hint. This will help us avoid
    // unnecessary clipping in our onDraw().
    get_vertex_bounds(geo.vertices(), vstride, 4, &devBounds);

    if (localRect) {
        static const int kLocalOffset = sizeof(SkPoint) + sizeof(GrColor);
        SkPoint* coords = GrTCast<SkPoint*>(GrTCast<intptr_t>(geo.vertices()) + kLocalOffset);
        coords->setRectFan(localRect->fLeft, localRect->fTop,
                           localRect->fRight, localRect->fBottom,
                           vstride);
        if (localMatrix) {
            localMatrix->mapPointsWithStride(coords, vstride, 4);
        }
    }

    static const int kColorOffset = sizeof(SkPoint);
    GrColor* vertColor = GrTCast<GrColor*>(GrTCast<intptr_t>(geo.vertices()) + kColorOffset);
    for (int i = 0; i < 4; ++i) {
        *vertColor = color;
        vertColor = (GrColor*) ((intptr_t) vertColor + vstride);
    }

    this->setIndexSourceToBuffer(this->getContext()->getQuadIndexBuffer());
    this->drawIndexedInstances(kTriangles_GrPrimitiveType, 1, 4, 6, &devBounds);

    // to ensure that stashing the drawState ptr is valid
    SkASSERT(this->drawState() == drawState);
}

int GrInOrderDrawBuffer::concatInstancedDraw(const DrawInfo& info,
                                             const GrClipMaskManager::ScissorState& scissorState) {
    SkASSERT(!fCmdBuffer.empty());
    SkASSERT(info.isInstanced());

    const GeometrySrcState& geomSrc = this->getGeomSrc();
    const GrDrawState& drawState = this->getDrawState();

    // we only attempt to concat the case when reserved verts are used with a client-specified index
    // buffer. To make this work with client-specified VBs we'd need to know if the VB was updated
    // between draws.
    if (kReserved_GeometrySrcType != geomSrc.fVertexSrc ||
        kBuffer_GeometrySrcType != geomSrc.fIndexSrc) {
        return 0;
    }
    // Check if there is a draw info that is compatible that uses the same VB from the pool and
    // the same IB
    if (kDraw_Cmd != strip_trace_bit(fCmdBuffer.back().fType)) {
        return 0;
    }

    Draw* draw = static_cast<Draw*>(&fCmdBuffer.back());
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GrVertexBuffer* vertexBuffer = poolState.fPoolVertexBuffer;

    if (!draw->fInfo.isInstanced() ||
        draw->fInfo.verticesPerInstance() != info.verticesPerInstance() ||
        draw->fInfo.indicesPerInstance() != info.indicesPerInstance() ||
        draw->vertexBuffer() != vertexBuffer ||
        draw->indexBuffer() != geomSrc.fIndexBuffer ||
        draw->fScissorState != scissorState) {
        return 0;
    }
    // info does not yet account for the offset from the start of the pool's VB while the previous
    // draw record does.
    int adjustedStartVertex = poolState.fPoolStartVertex + info.startVertex();
    if (draw->fInfo.startVertex() + draw->fInfo.vertexCount() != adjustedStartVertex) {
        return 0;
    }

    SkASSERT(poolState.fPoolStartVertex == draw->fInfo.startVertex() + draw->fInfo.vertexCount());

    // how many instances can be concat'ed onto draw given the size of the index buffer
    int instancesToConcat = this->indexCountInCurrentSource() / info.indicesPerInstance();
    instancesToConcat -= draw->fInfo.instanceCount();
    instancesToConcat = SkTMin(instancesToConcat, info.instanceCount());

    // update the amount of reserved vertex data actually referenced in draws
    size_t vertexBytes = instancesToConcat * info.verticesPerInstance() *
                         drawState.getVertexStride();
    poolState.fUsedPoolVertexBytes = SkTMax(poolState.fUsedPoolVertexBytes, vertexBytes);

    draw->fInfo.adjustInstanceCount(instancesToConcat);

    // update last fGpuCmdMarkers to include any additional trace markers that have been added
    if (this->getActiveTraceMarkers().count() > 0) {
        if (cmd_has_trace_marker(draw->fType)) {
            fGpuCmdMarkers.back().addSet(this->getActiveTraceMarkers());
        } else {
            fGpuCmdMarkers.push_back(this->getActiveTraceMarkers());
            draw->fType = add_trace_bit(draw->fType);
        }
    }

    return instancesToConcat;
}

void GrInOrderDrawBuffer::onDraw(const DrawInfo& info,
                                 const GrClipMaskManager::ScissorState& scissorState) {

    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GrDrawState& drawState = this->getDrawState();

    this->recordStateIfNecessary(GrGpu::PrimTypeToDrawType(info.primitiveType()),
                                 info.getDstCopy());

    const GrVertexBuffer* vb;
    if (kBuffer_GeometrySrcType == this->getGeomSrc().fVertexSrc) {
        vb = this->getGeomSrc().fVertexBuffer;
    } else {
        vb = poolState.fPoolVertexBuffer;
    }

    const GrIndexBuffer* ib = NULL;
    if (info.isIndexed()) {
        if (kBuffer_GeometrySrcType == this->getGeomSrc().fIndexSrc) {
            ib = this->getGeomSrc().fIndexBuffer;
        } else {
            ib = poolState.fPoolIndexBuffer;
        }
    }

    Draw* draw;
    if (info.isInstanced()) {
        int instancesConcated = this->concatInstancedDraw(info, scissorState);
        if (info.instanceCount() > instancesConcated) {
            draw = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, Draw, (info, scissorState, vb, ib));
            draw->fInfo.adjustInstanceCount(-instancesConcated);
        } else {
            return;
        }
    } else {
        draw = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, Draw, (info, scissorState, vb, ib));
    }
    this->recordTraceMarkersIfNecessary();

    // Adjust the starting vertex and index when we are using reserved or array sources to
    // compensate for the fact that the data was inserted into a larger vb/ib owned by the pool.
    if (kBuffer_GeometrySrcType != this->getGeomSrc().fVertexSrc) {
        size_t bytes = (info.vertexCount() + info.startVertex()) * drawState.getVertexStride();
        poolState.fUsedPoolVertexBytes = SkTMax(poolState.fUsedPoolVertexBytes, bytes);
        draw->fInfo.adjustStartVertex(poolState.fPoolStartVertex);
    }
    
    if (info.isIndexed() && kBuffer_GeometrySrcType != this->getGeomSrc().fIndexSrc) {
        size_t bytes = (info.indexCount() + info.startIndex()) * sizeof(uint16_t);
        poolState.fUsedPoolIndexBytes = SkTMax(poolState.fUsedPoolIndexBytes, bytes);
        draw->fInfo.adjustStartIndex(poolState.fPoolStartIndex);
    }
}

void GrInOrderDrawBuffer::onStencilPath(const GrPath* path,
                                        const GrClipMaskManager::ScissorState& scissorState,
                                        const GrStencilSettings& stencilSettings) {
    // Only compare the subset of GrDrawState relevant to path stenciling?
    this->recordStateIfNecessary(GrGpu::kStencilPath_DrawType, NULL);
    StencilPath* sp = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, StencilPath, (path));
    sp->fScissorState = scissorState;
    sp->fStencilSettings = stencilSettings;
    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::onDrawPath(const GrPath* path,
                                     const GrClipMaskManager::ScissorState& scissorState,
                                     const GrStencilSettings& stencilSettings,
                                     const GrDeviceCoordTexture* dstCopy) {
    // TODO: Only compare the subset of GrDrawState relevant to path covering?
    this->recordStateIfNecessary(GrGpu::kDrawPath_DrawType, dstCopy);
    DrawPath* dp = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, DrawPath, (path));
    if (dstCopy) {
        dp->fDstCopy = *dstCopy;
    }
    dp->fScissorState = scissorState;
    dp->fStencilSettings = stencilSettings;
    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::onDrawPaths(const GrPathRange* pathRange,
                                      const uint32_t indices[],
                                      int count,
                                      const float transforms[],
                                      PathTransformType transformsType,
                                      const GrClipMaskManager::ScissorState& scissorState,
                                      const GrStencilSettings& stencilSettings,
                                      const GrDeviceCoordTexture* dstCopy) {
    SkASSERT(pathRange);
    SkASSERT(indices);
    SkASSERT(transforms);

    this->recordStateIfNecessary(GrGpu::kDrawPaths_DrawType, dstCopy);

    int sizeOfIndices = sizeof(uint32_t) * count;
    int sizeOfTransforms = sizeof(float) * count *
                           GrPathRendering::PathTransformSize(transformsType);

    DrawPaths* dp = GrNEW_APPEND_WITH_DATA_TO_RECORDER(fCmdBuffer, DrawPaths, (pathRange),
                                                       sizeOfIndices + sizeOfTransforms);
    memcpy(dp->indices(), indices, sizeOfIndices);
    dp->fCount = count;
    memcpy(dp->transforms(), transforms, sizeOfTransforms);
    dp->fTransformsType = transformsType;
    dp->fScissorState = scissorState;
    dp->fStencilSettings = stencilSettings;
    if (dstCopy) {
        dp->fDstCopy = *dstCopy;
    }

    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::onClear(const SkIRect* rect, GrColor color,
                                  bool canIgnoreRect, GrRenderTarget* renderTarget) {
    SkIRect r;
    if (NULL == renderTarget) {
        renderTarget = this->drawState()->getRenderTarget();
        SkASSERT(renderTarget);
    }
    if (NULL == rect) {
        // We could do something smart and remove previous draws and clears to
        // the current render target. If we get that smart we have to make sure
        // those draws aren't read before this clear (render-to-texture).
        r.setLTRB(0, 0, renderTarget->width(), renderTarget->height());
        rect = &r;
    }
    Clear* clr = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, Clear, (renderTarget));
    GrColorIsPMAssert(color);
    clr->fColor = color;
    clr->fRect = *rect;
    clr->fCanIgnoreRect = canIgnoreRect;
    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::clearStencilClip(const SkIRect& rect,
                                           bool insideClip,
                                           GrRenderTarget* renderTarget) {
    if (NULL == renderTarget) {
        renderTarget = this->drawState()->getRenderTarget();
        SkASSERT(renderTarget);
    }
    ClearStencilClip* clr = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, ClearStencilClip, (renderTarget));
    clr->fRect = rect;
    clr->fInsideClip = insideClip;
    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::discard(GrRenderTarget* renderTarget) {
    SkASSERT(renderTarget);
    if (!this->caps()->discardRenderTargetSupport()) {
        return;
    }
    Clear* clr = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, Clear, (renderTarget));
    clr->fColor = GrColor_ILLEGAL;
    this->recordTraceMarkersIfNecessary();
}

void GrInOrderDrawBuffer::reset() {
    SkASSERT(1 == fGeoPoolStateStack.count());
    this->resetVertexSource();
    this->resetIndexSource();

    fCmdBuffer.reset();
    fLastState = NULL;
    fVertexPool.reset();
    fIndexPool.reset();
    fGpuCmdMarkers.reset();
}

void GrInOrderDrawBuffer::flush() {
    if (fFlushing) {
        return;
    }

    this->getContext()->getFontCache()->updateTextures();

    SkASSERT(kReserved_GeometrySrcType != this->getGeomSrc().fVertexSrc);
    SkASSERT(kReserved_GeometrySrcType != this->getGeomSrc().fIndexSrc);

    if (fCmdBuffer.empty()) {
        return;
    }

    GrAutoTRestore<bool> flushRestore(&fFlushing);
    fFlushing = true;

    fVertexPool.unmap();
    fIndexPool.unmap();

    CmdBuffer::Iter iter(fCmdBuffer);

    int currCmdMarker = 0;
    fDstGpu->saveActiveTraceMarkers();

    // Gpu no longer maintains the current drawstate, so we track the setstate calls below.
    // NOTE: we always record a new drawstate at flush boundaries
    SkAutoTUnref<const GrOptDrawState> currentOptState;

    while (iter.next()) {
        GrGpuTraceMarker newMarker("", -1);
        SkString traceString;
        if (cmd_has_trace_marker(iter->fType)) {
            traceString = fGpuCmdMarkers[currCmdMarker].toString();
            newMarker.fMarker = traceString.c_str();
            fDstGpu->addGpuTraceMarker(&newMarker);
            ++currCmdMarker;
        }

        if (kSetState_Cmd == strip_trace_bit(iter->fType)) {
            const SetState* ss = reinterpret_cast<const SetState*>(iter.get());
            currentOptState.reset(GrOptDrawState::Create(ss->fState,
                                                         fDstGpu,
                                                         &ss->fDstCopy,
                                                         ss->fDrawType));
        } else {
            iter->execute(fDstGpu, currentOptState.get());
        }

        if (cmd_has_trace_marker(iter->fType)) {
            fDstGpu->removeGpuTraceMarker(&newMarker);
        }
    }

    fDstGpu->restoreActiveTraceMarkers();
    SkASSERT(fGpuCmdMarkers.count() == currCmdMarker);

    this->reset();
    ++fDrawID;
}

void GrInOrderDrawBuffer::Draw::execute(GrGpu* gpu, const GrOptDrawState* optState) {
    if (!optState) {
        return;
    }
    gpu->setVertexSourceToBuffer(this->vertexBuffer(), optState->getVertexStride());
    if (fInfo.isIndexed()) {
        gpu->setIndexSourceToBuffer(this->indexBuffer());
    }
    gpu->draw(*optState, fInfo, fScissorState);
}

void GrInOrderDrawBuffer::StencilPath::execute(GrGpu* gpu, const GrOptDrawState* optState) {
    if (!optState) {
        return;
    }
    gpu->stencilPath(*optState, this->path(), fScissorState, fStencilSettings);
}

void GrInOrderDrawBuffer::DrawPath::execute(GrGpu* gpu, const GrOptDrawState* optState) {
    if (!optState) {
        return;
    }
    gpu->drawPath(*optState, this->path(), fScissorState, fStencilSettings,
                  fDstCopy.texture() ? &fDstCopy : NULL);
}

void GrInOrderDrawBuffer::DrawPaths::execute(GrGpu* gpu, const GrOptDrawState* optState) {
    if (!optState) {
        return;
    }
    gpu->drawPaths(*optState, this->pathRange(), this->indices(), fCount, this->transforms(),
                   fTransformsType, fScissorState, fStencilSettings,
                   fDstCopy.texture() ? &fDstCopy : NULL);
}

void GrInOrderDrawBuffer::SetState::execute(GrGpu* gpu, const GrOptDrawState*) {
}

void GrInOrderDrawBuffer::Clear::execute(GrGpu* gpu, const GrOptDrawState*) {
    if (GrColor_ILLEGAL == fColor) {
        gpu->discard(this->renderTarget());
    } else {
        gpu->clear(&fRect, fColor, fCanIgnoreRect, this->renderTarget());
    }
}

void GrInOrderDrawBuffer::ClearStencilClip::execute(GrGpu* gpu, const GrOptDrawState*) {
    gpu->clearStencilClip(fRect, fInsideClip, this->renderTarget());
}

void GrInOrderDrawBuffer::CopySurface::execute(GrGpu* gpu, const GrOptDrawState*){
    gpu->copySurface(this->dst(), this->src(), fSrcRect, fDstPoint);
}

bool GrInOrderDrawBuffer::copySurface(GrSurface* dst,
                                      GrSurface* src,
                                      const SkIRect& srcRect,
                                      const SkIPoint& dstPoint) {
    if (fDstGpu->canCopySurface(dst, src, srcRect, dstPoint)) {
        CopySurface* cs = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, CopySurface, (dst, src));
        cs->fSrcRect = srcRect;
        cs->fDstPoint = dstPoint;
        this->recordTraceMarkersIfNecessary();
        return true;
    } else if (GrDrawTarget::canCopySurface(dst, src, srcRect, dstPoint)) {
        GrDrawTarget::copySurface(dst, src, srcRect, dstPoint);
        return true;
    } else {
        return false;
    }
}

bool GrInOrderDrawBuffer::canCopySurface(GrSurface* dst,
                                         GrSurface* src,
                                         const SkIRect& srcRect,
                                         const SkIPoint& dstPoint) {
    return fDstGpu->canCopySurface(dst, src, srcRect, dstPoint) ||
           GrDrawTarget::canCopySurface(dst, src, srcRect, dstPoint);
}

void GrInOrderDrawBuffer::initCopySurfaceDstDesc(const GrSurface* src, GrSurfaceDesc* desc) {
    fDstGpu->initCopySurfaceDstDesc(src, desc);
}

void GrInOrderDrawBuffer::willReserveVertexAndIndexSpace(int vertexCount,
                                                         int indexCount) {
    // We use geometryHints() to know whether to flush the draw buffer. We
    // can't flush if we are inside an unbalanced pushGeometrySource.
    // Moreover, flushing blows away vertex and index data that was
    // previously reserved. So if the vertex or index data is pulled from
    // reserved space and won't be released by this request then we can't
    // flush.
    bool insideGeoPush = fGeoPoolStateStack.count() > 1;

    bool unreleasedVertexSpace =
        !vertexCount &&
        kReserved_GeometrySrcType == this->getGeomSrc().fVertexSrc;

    bool unreleasedIndexSpace =
        !indexCount &&
        kReserved_GeometrySrcType == this->getGeomSrc().fIndexSrc;

    int vcount = vertexCount;
    int icount = indexCount;

    if (!insideGeoPush &&
        !unreleasedVertexSpace &&
        !unreleasedIndexSpace &&
        this->geometryHints(&vcount, &icount)) {
        this->flush();
    }
}

bool GrInOrderDrawBuffer::geometryHints(int* vertexCount,
                                        int* indexCount) const {
    // we will recommend a flush if the data could fit in a single
    // preallocated buffer but none are left and it can't fit
    // in the current buffer (which may not be prealloced).
    bool flush = false;
    if (indexCount) {
        int32_t currIndices = fIndexPool.currentBufferIndices();
        if (*indexCount > currIndices &&
            (!fIndexPool.preallocatedBuffersRemaining() &&
             *indexCount <= fIndexPool.preallocatedBufferIndices())) {

            flush = true;
        }
        *indexCount = currIndices;
    }
    if (vertexCount) {
        size_t vertexStride = this->getDrawState().getVertexStride();
        int32_t currVertices = fVertexPool.currentBufferVertices(vertexStride);
        if (*vertexCount > currVertices &&
            (!fVertexPool.preallocatedBuffersRemaining() &&
             *vertexCount <= fVertexPool.preallocatedBufferVertices(vertexStride))) {

            flush = true;
        }
        *vertexCount = currVertices;
    }
    return flush;
}

bool GrInOrderDrawBuffer::onReserveVertexSpace(size_t vertexSize,
                                               int vertexCount,
                                               void** vertices) {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    SkASSERT(vertexCount > 0);
    SkASSERT(vertices);
    SkASSERT(0 == poolState.fUsedPoolVertexBytes);

    *vertices = fVertexPool.makeSpace(vertexSize,
                                      vertexCount,
                                      &poolState.fPoolVertexBuffer,
                                      &poolState.fPoolStartVertex);
    return SkToBool(*vertices);
}

bool GrInOrderDrawBuffer::onReserveIndexSpace(int indexCount, void** indices) {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    SkASSERT(indexCount > 0);
    SkASSERT(indices);
    SkASSERT(0 == poolState.fUsedPoolIndexBytes);

    *indices = fIndexPool.makeSpace(indexCount,
                                    &poolState.fPoolIndexBuffer,
                                    &poolState.fPoolStartIndex);
    return SkToBool(*indices);
}

void GrInOrderDrawBuffer::releaseReservedVertexSpace() {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GeometrySrcState& geoSrc = this->getGeomSrc();

    // If we get a release vertex space call then our current source should either be reserved
    // or array (which we copied into reserved space).
    SkASSERT(kReserved_GeometrySrcType == geoSrc.fVertexSrc);

    // When the caller reserved vertex buffer space we gave it back a pointer
    // provided by the vertex buffer pool. At each draw we tracked the largest
    // offset into the pool's pointer that was referenced. Now we return to the
    // pool any portion at the tail of the allocation that no draw referenced.
    size_t reservedVertexBytes = geoSrc.fVertexSize * geoSrc.fVertexCount;
    fVertexPool.putBack(reservedVertexBytes - poolState.fUsedPoolVertexBytes);
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fPoolVertexBuffer = NULL;
    poolState.fPoolStartVertex = 0;
}

void GrInOrderDrawBuffer::releaseReservedIndexSpace() {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GeometrySrcState& geoSrc = this->getGeomSrc();

    // If we get a release index space call then our current source should either be reserved
    // or array (which we copied into reserved space).
    SkASSERT(kReserved_GeometrySrcType == geoSrc.fIndexSrc);

    // Similar to releaseReservedVertexSpace we return any unused portion at
    // the tail
    size_t reservedIndexBytes = sizeof(uint16_t) * geoSrc.fIndexCount;
    fIndexPool.putBack(reservedIndexBytes - poolState.fUsedPoolIndexBytes);
    poolState.fUsedPoolIndexBytes = 0;
    poolState.fPoolIndexBuffer = NULL;
    poolState.fPoolStartIndex = 0;
}

void GrInOrderDrawBuffer::geometrySourceWillPush() {
    GeometryPoolState& poolState = fGeoPoolStateStack.push_back();
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fUsedPoolIndexBytes = 0;
#ifdef SK_DEBUG
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)~0;
    poolState.fPoolStartVertex = ~0;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)~0;
    poolState.fPoolStartIndex = ~0;
#endif
}

void GrInOrderDrawBuffer::geometrySourceWillPop(const GeometrySrcState& restoredState) {
    SkASSERT(fGeoPoolStateStack.count() > 1);
    fGeoPoolStateStack.pop_back();
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    // we have to assume that any slack we had in our vertex/index data
    // is now unreleasable because data may have been appended later in the
    // pool.
    if (kReserved_GeometrySrcType == restoredState.fVertexSrc) {
        poolState.fUsedPoolVertexBytes = restoredState.fVertexSize * restoredState.fVertexCount;
    }
    if (kReserved_GeometrySrcType == restoredState.fIndexSrc) {
        poolState.fUsedPoolIndexBytes = sizeof(uint16_t) * restoredState.fIndexCount;
    }
}

void GrInOrderDrawBuffer::recordStateIfNecessary(GrGpu::DrawType drawType,
                                                 const GrDeviceCoordTexture* dstCopy) {
    if (!fLastState) {
        SetState* ss = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, SetState, (this->getDrawState()));
        fLastState = &ss->fState;
        if (dstCopy) {
            ss->fDstCopy = *dstCopy;
        }
        ss->fDrawType = drawType;
        this->convertDrawStateToPendingExec(fLastState);
        this->recordTraceMarkersIfNecessary();
        return;
    }
    const GrDrawState& curr = this->getDrawState();
    switch (GrDrawState::CombineIfPossible(*fLastState, curr, *this->caps())) {
        case GrDrawState::kIncompatible_CombinedState: {
            SetState* ss = GrNEW_APPEND_TO_RECORDER(fCmdBuffer, SetState, (curr));
            fLastState = &ss->fState;
            if (dstCopy) {
                ss->fDstCopy = *dstCopy;
            }
            ss->fDrawType = drawType;
            this->convertDrawStateToPendingExec(fLastState);
            this->recordTraceMarkersIfNecessary();
            break;
        }
        case GrDrawState::kA_CombinedState:
        case GrDrawState::kAOrB_CombinedState: // Treat the same as kA.
            break;
        case GrDrawState::kB_CombinedState:
            // prev has already been converted to pending execution. That is a one-way ticket.
            // So here we just destruct the previous state and reinit with a new copy of curr.
            // Note that this goes away when we move GrIODB over to taking optimized snapshots
            // of draw states.
            fLastState->~GrDrawState();
            SkNEW_PLACEMENT_ARGS(fLastState, GrDrawState, (curr));
            this->convertDrawStateToPendingExec(fLastState);
            break;
    }
}

void GrInOrderDrawBuffer::recordTraceMarkersIfNecessary() {
    SkASSERT(!fCmdBuffer.empty());
    SkASSERT(!cmd_has_trace_marker(fCmdBuffer.back().fType));
    const GrTraceMarkerSet& activeTraceMarkers = this->getActiveTraceMarkers();
    if (activeTraceMarkers.count() > 0) {
        fCmdBuffer.back().fType = add_trace_bit(fCmdBuffer.back().fType);
        fGpuCmdMarkers.push_back(activeTraceMarkers);
    }
}
