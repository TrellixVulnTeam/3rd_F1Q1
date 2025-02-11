/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GrDistanceFieldTextureEffect.h"
#include "gl/builders/GrGLProgramBuilder.h"
#include "gl/GrGLProcessor.h"
#include "gl/GrGLSL.h"
#include "gl/GrGLTexture.h"
#include "gl/GrGLGeometryProcessor.h"
#include "GrTBackendProcessorFactory.h"
#include "GrTexture.h"

#include "SkDistanceFieldGen.h"

// Assuming a radius of the diagonal of the fragment, hence a factor of sqrt(2)/2
#define SK_DistanceFieldAAFactor     "0.7071"

class GrGLDistanceFieldTextureEffect : public GrGLGeometryProcessor {
public:
    GrGLDistanceFieldTextureEffect(const GrBackendProcessorFactory& factory,
                                   const GrProcessor&)
        : INHERITED (factory)
        , fTextureSize(SkISize::Make(-1,-1))
#ifdef SK_GAMMA_APPLY_TO_A8
        , fLuminance(-1.0f)
#endif
        {}

    virtual void emitCode(const EmitArgs& args) SK_OVERRIDE {
        const GrDistanceFieldTextureEffect& dfTexEffect =
                args.fGP.cast<GrDistanceFieldTextureEffect>();
        SkASSERT(1 == dfTexEffect.getVertexAttribs().count());

        GrGLGPFragmentBuilder* fsBuilder = args.fPB->getFragmentShaderBuilder();
        SkAssertResult(fsBuilder->enableFeature(
                GrGLFragmentShaderBuilder::kStandardDerivatives_GLSLFeature));

        GrGLVertToFrag v(kVec2f_GrSLType);
        args.fPB->addVarying("TextureCoords", &v);

        GrGLVertexBuilder* vsBuilder = args.fPB->getVertexShaderBuilder();
        vsBuilder->codeAppendf("\t%s = %s;\n", v.vsOut(), dfTexEffect.inTextureCoords().c_str());

        // setup position varying
        vsBuilder->codeAppendf("%s = %s * vec3(%s, 1);", vsBuilder->glPosition(),
                               vsBuilder->uViewM(), vsBuilder->inPosition());

        const char* textureSizeUniName = NULL;
        fTextureSizeUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                               kVec2f_GrSLType, "TextureSize",
                                               &textureSizeUniName);

        fsBuilder->codeAppend("\tvec4 texColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[0],
                                       v.fsIn(),
                                       kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tfloat distance = "
                       SK_DistanceFieldMultiplier "*(texColor.r - " SK_DistanceFieldThreshold ");");

        // we adjust for the effect of the transformation on the distance by using
        // the length of the gradient of the texture coordinates. We use st coordinates
        // to ensure we're mapping 1:1 from texel space to pixel space.
        fsBuilder->codeAppendf("\tvec2 uv = %s;\n", v.fsIn());
        fsBuilder->codeAppendf("\tvec2 st = uv*%s;\n", textureSizeUniName);
        fsBuilder->codeAppend("\tfloat afwidth;\n");
        if (dfTexEffect.getFlags() & kSimilarity_DistanceFieldEffectFlag) {
            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("\tafwidth = abs(" SK_DistanceFieldAAFactor "*dFdx(st.x));\n");
        } else {
            fsBuilder->codeAppend("\tvec2 Jdx = dFdx(st);\n");
            fsBuilder->codeAppend("\tvec2 Jdy = dFdy(st);\n");

            fsBuilder->codeAppend("\tvec2 uv_grad;\n");
            if (args.fPB->ctxInfo().caps()->dropsTileOnZeroDivide()) {
                // this is to compensate for the Adreno, which likes to drop tiles on division by 0
                fsBuilder->codeAppend("\tfloat uv_len2 = dot(uv, uv);\n");
                fsBuilder->codeAppend("\tif (uv_len2 < 0.0001) {\n");
                fsBuilder->codeAppend("\t\tuv_grad = vec2(0.7071, 0.7071);\n");
                fsBuilder->codeAppend("\t} else {\n");
                fsBuilder->codeAppend("\t\tuv_grad = uv*inversesqrt(uv_len2);\n");
                fsBuilder->codeAppend("\t}\n");
            } else {
                fsBuilder->codeAppend("\tuv_grad = normalize(uv);\n");
            }
            fsBuilder->codeAppend("\tvec2 grad = vec2(uv_grad.x*Jdx.x + uv_grad.y*Jdy.x,\n");
            fsBuilder->codeAppend("\t                 uv_grad.x*Jdx.y + uv_grad.y*Jdy.y);\n");

            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("\tafwidth = " SK_DistanceFieldAAFactor "*length(grad);\n");
        }
        fsBuilder->codeAppend("\tfloat val = smoothstep(-afwidth, afwidth, distance);\n");

#ifdef SK_GAMMA_APPLY_TO_A8
        // adjust based on gamma
        const char* luminanceUniName = NULL;
        // width, height, 1/(3*width)
        fLuminanceUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                             kFloat_GrSLType, "Luminance",
                                             &luminanceUniName);

        fsBuilder->codeAppendf("\tuv = vec2(val, %s);\n", luminanceUniName);
        fsBuilder->codeAppend("\tvec4 gammaColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[1], "uv", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tval = gammaColor.r;\n");
#endif

        fsBuilder->codeAppendf("\t%s = %s;\n", args.fOutput,
                                   (GrGLSLExpr4(args.fInput) * GrGLSLExpr1("val")).c_str());
    }

    virtual void setData(const GrGLProgramDataManager& pdman,
                         const GrProcessor& effect) SK_OVERRIDE {
        SkASSERT(fTextureSizeUni.isValid());

        GrTexture* texture = effect.texture(0);
        if (texture->width() != fTextureSize.width() ||
            texture->height() != fTextureSize.height()) {
            fTextureSize = SkISize::Make(texture->width(), texture->height());
            pdman.set2f(fTextureSizeUni,
                        SkIntToScalar(fTextureSize.width()),
                        SkIntToScalar(fTextureSize.height()));
        }
#ifdef SK_GAMMA_APPLY_TO_A8
        const GrDistanceFieldTextureEffect& dfTexEffect =
                effect.cast<GrDistanceFieldTextureEffect>();
        float luminance = dfTexEffect.getLuminance();
        if (luminance != fLuminance) {
            pdman.set1f(fLuminanceUni, luminance);
            fLuminance = luminance;
        }
#endif
    }

    static inline void GenKey(const GrProcessor& processor, const GrGLCaps&,
                              GrProcessorKeyBuilder* b) {
        const GrDistanceFieldTextureEffect& dfTexEffect =
                processor.cast<GrDistanceFieldTextureEffect>();

        b->add32(dfTexEffect.getFlags());
    }

private:
    GrGLProgramDataManager::UniformHandle fTextureSizeUni;
    SkISize                               fTextureSize;
    GrGLProgramDataManager::UniformHandle fLuminanceUni;
    float                                 fLuminance;

    typedef GrGLGeometryProcessor INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

GrDistanceFieldTextureEffect::GrDistanceFieldTextureEffect(GrTexture* texture,
                                                           const GrTextureParams& params,
#ifdef SK_GAMMA_APPLY_TO_A8
                                                           GrTexture* gamma,
                                                           const GrTextureParams& gammaParams,
                                                           float luminance,
#endif
                                                           uint32_t flags)
    : fTextureAccess(texture, params)
#ifdef SK_GAMMA_APPLY_TO_A8
    , fGammaTextureAccess(gamma, gammaParams)
    , fLuminance(luminance)
#endif
    , fFlags(flags & kNonLCD_DistanceFieldEffectMask)
    , fInTextureCoords(this->addVertexAttrib(GrShaderVar("inTextureCoords",
                                                         kVec2f_GrSLType,
                                                         GrShaderVar::kAttribute_TypeModifier))) {
    SkASSERT(!(flags & ~kNonLCD_DistanceFieldEffectMask));
    this->addTextureAccess(&fTextureAccess);
#ifdef SK_GAMMA_APPLY_TO_A8
    this->addTextureAccess(&fGammaTextureAccess);
#endif
}

bool GrDistanceFieldTextureEffect::onIsEqual(const GrGeometryProcessor& other) const {
    const GrDistanceFieldTextureEffect& cte = other.cast<GrDistanceFieldTextureEffect>();
    return
#ifdef SK_GAMMA_APPLY_TO_A8
           fLuminance == cte.fLuminance &&
#endif
           fFlags == cte.fFlags;
}

void GrDistanceFieldTextureEffect::onComputeInvariantOutput(InvariantOutput* inout) const {
    inout->mulByUnknownAlpha();
}

const GrBackendGeometryProcessorFactory& GrDistanceFieldTextureEffect::getFactory() const {
    return GrTBackendGeometryProcessorFactory<GrDistanceFieldTextureEffect>::getInstance();
}

///////////////////////////////////////////////////////////////////////////////

GR_DEFINE_GEOMETRY_PROCESSOR_TEST(GrDistanceFieldTextureEffect);

GrGeometryProcessor* GrDistanceFieldTextureEffect::TestCreate(SkRandom* random,
                                                              GrContext*,
                                                              const GrDrawTargetCaps&,
                                                              GrTexture* textures[]) {
    int texIdx = random->nextBool() ? GrProcessorUnitTest::kSkiaPMTextureIdx :
                                      GrProcessorUnitTest::kAlphaTextureIdx;
#ifdef SK_GAMMA_APPLY_TO_A8
    int texIdx2 = random->nextBool() ? GrProcessorUnitTest::kSkiaPMTextureIdx :
                                       GrProcessorUnitTest::kAlphaTextureIdx;
#endif
    static const SkShader::TileMode kTileModes[] = {
        SkShader::kClamp_TileMode,
        SkShader::kRepeat_TileMode,
        SkShader::kMirror_TileMode,
    };
    SkShader::TileMode tileModes[] = {
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
    };
    GrTextureParams params(tileModes, random->nextBool() ? GrTextureParams::kBilerp_FilterMode :
                                                           GrTextureParams::kNone_FilterMode);
#ifdef SK_GAMMA_APPLY_TO_A8
    GrTextureParams params2(tileModes, random->nextBool() ? GrTextureParams::kBilerp_FilterMode :
                                                            GrTextureParams::kNone_FilterMode);
#endif

    return GrDistanceFieldTextureEffect::Create(textures[texIdx], params,
#ifdef SK_GAMMA_APPLY_TO_A8
                                                textures[texIdx2], params2,
                                                random->nextF(),
#endif
                                                random->nextBool() ?
                                                    kSimilarity_DistanceFieldEffectFlag : 0);
}

///////////////////////////////////////////////////////////////////////////////

class GrGLDistanceFieldNoGammaTextureEffect : public GrGLGeometryProcessor {
public:
    GrGLDistanceFieldNoGammaTextureEffect(const GrBackendProcessorFactory& factory,
                                          const GrProcessor& effect)
        : INHERITED(factory)
        , fTextureSize(SkISize::Make(-1, -1)) {}

    virtual void emitCode(const EmitArgs& args) SK_OVERRIDE {
        const GrDistanceFieldNoGammaTextureEffect& dfTexEffect =
                args.fGP.cast<GrDistanceFieldNoGammaTextureEffect>();
        SkASSERT(1 == dfTexEffect.getVertexAttribs().count());

        GrGLGPFragmentBuilder* fsBuilder = args.fPB->getFragmentShaderBuilder();
        SkAssertResult(fsBuilder->enableFeature(
                                     GrGLFragmentShaderBuilder::kStandardDerivatives_GLSLFeature));

        GrGLVertToFrag v(kVec2f_GrSLType);
        args.fPB->addVarying("TextureCoords", &v);

        GrGLVertexBuilder* vsBuilder = args.fPB->getVertexShaderBuilder();
        vsBuilder->codeAppendf("%s = %s;", v.vsOut(), dfTexEffect.inTextureCoords().c_str());

        // setup position varying
        vsBuilder->codeAppendf("%s = %s * vec3(%s, 1);", vsBuilder->glPosition(),
                               vsBuilder->uViewM(), vsBuilder->inPosition());

        const char* textureSizeUniName = NULL;
        fTextureSizeUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                              kVec2f_GrSLType, "TextureSize",
                                              &textureSizeUniName);

        fsBuilder->codeAppend("vec4 texColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[0],
                                       v.fsIn(),
                                       kVec2f_GrSLType);
        fsBuilder->codeAppend(";");
        fsBuilder->codeAppend("float distance = "
            SK_DistanceFieldMultiplier "*(texColor.r - " SK_DistanceFieldThreshold ");");

        // we adjust for the effect of the transformation on the distance by using
        // the length of the gradient of the texture coordinates. We use st coordinates
        // to ensure we're mapping 1:1 from texel space to pixel space.
        fsBuilder->codeAppendf("vec2 uv = %s;", v.fsIn());
        fsBuilder->codeAppendf("vec2 st = uv*%s;", textureSizeUniName);
        fsBuilder->codeAppend("float afwidth;");
        if (dfTexEffect.getFlags() & kSimilarity_DistanceFieldEffectFlag) {
            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("afwidth = abs(" SK_DistanceFieldAAFactor "*dFdx(st.x));");
        } else {
            fsBuilder->codeAppend("vec2 Jdx = dFdx(st);");
            fsBuilder->codeAppend("vec2 Jdy = dFdy(st);");

            fsBuilder->codeAppend("vec2 uv_grad;");
            if (args.fPB->ctxInfo().caps()->dropsTileOnZeroDivide()) {
                // this is to compensate for the Adreno, which likes to drop tiles on division by 0
                fsBuilder->codeAppend("float uv_len2 = dot(uv, uv);");
                fsBuilder->codeAppend("if (uv_len2 < 0.0001) {");
                fsBuilder->codeAppend("uv_grad = vec2(0.7071, 0.7071);");
                fsBuilder->codeAppend("} else {");
                fsBuilder->codeAppend("uv_grad = uv*inversesqrt(uv_len2);");
                fsBuilder->codeAppend("}");
            } else {
                fsBuilder->codeAppend("uv_grad = normalize(uv);");
            }
            fsBuilder->codeAppend("vec2 grad = vec2(uv_grad.x*Jdx.x + uv_grad.y*Jdy.x,");
            fsBuilder->codeAppend("                 uv_grad.x*Jdx.y + uv_grad.y*Jdy.y);");

            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("afwidth = " SK_DistanceFieldAAFactor "*length(grad);");
        }
        fsBuilder->codeAppend("float val = smoothstep(-afwidth, afwidth, distance);");

        fsBuilder->codeAppendf("%s = %s;", args.fOutput,
            (GrGLSLExpr4(args.fInput) * GrGLSLExpr1("val")).c_str());
    }

    virtual void setData(const GrGLProgramDataManager& pdman,
                         const GrProcessor& effect) SK_OVERRIDE {
        SkASSERT(fTextureSizeUni.isValid());

        GrTexture* texture = effect.texture(0);
        if (texture->width() != fTextureSize.width() || 
            texture->height() != fTextureSize.height()) {
            fTextureSize = SkISize::Make(texture->width(), texture->height());
            pdman.set2f(fTextureSizeUni,
                        SkIntToScalar(fTextureSize.width()),
                        SkIntToScalar(fTextureSize.height()));
        }
    }

    static inline void GenKey(const GrProcessor& effect, const GrGLCaps&,
                              GrProcessorKeyBuilder* b) {
        const GrDistanceFieldNoGammaTextureEffect& dfTexEffect =
            effect.cast<GrDistanceFieldNoGammaTextureEffect>();

        b->add32(dfTexEffect.getFlags());
    }

private:
    GrGLProgramDataManager::UniformHandle fTextureSizeUni;
    SkISize                               fTextureSize;

    typedef GrGLGeometryProcessor INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

GrDistanceFieldNoGammaTextureEffect::GrDistanceFieldNoGammaTextureEffect(GrTexture* texture,
                                                                    const GrTextureParams& params,
                                                                    uint32_t flags)
    : fTextureAccess(texture, params)
    , fFlags(flags & kNonLCD_DistanceFieldEffectMask)
    , fInTextureCoords(this->addVertexAttrib(GrShaderVar("inTextureCoords",
                       kVec2f_GrSLType,
                       GrShaderVar::kAttribute_TypeModifier))) {
    SkASSERT(!(flags & ~kNonLCD_DistanceFieldEffectMask));
    this->addTextureAccess(&fTextureAccess);
}

bool GrDistanceFieldNoGammaTextureEffect::onIsEqual(const GrGeometryProcessor& other) const {
    const GrDistanceFieldNoGammaTextureEffect& cte = 
                                                 other.cast<GrDistanceFieldNoGammaTextureEffect>();
    return fFlags == cte.fFlags;
}

void GrDistanceFieldNoGammaTextureEffect::onComputeInvariantOutput(InvariantOutput* inout) const {
    inout->mulByUnknownAlpha();
}

const GrBackendGeometryProcessorFactory& GrDistanceFieldNoGammaTextureEffect::getFactory() const {
    return GrTBackendGeometryProcessorFactory<GrDistanceFieldNoGammaTextureEffect>::getInstance();
}

///////////////////////////////////////////////////////////////////////////////

GR_DEFINE_GEOMETRY_PROCESSOR_TEST(GrDistanceFieldNoGammaTextureEffect);

GrGeometryProcessor* GrDistanceFieldNoGammaTextureEffect::TestCreate(SkRandom* random,
                                                                     GrContext*,
                                                                     const GrDrawTargetCaps&,
                                                                     GrTexture* textures[]) {
    int texIdx = random->nextBool() ? GrProcessorUnitTest::kSkiaPMTextureIdx 
                                    : GrProcessorUnitTest::kAlphaTextureIdx;
    static const SkShader::TileMode kTileModes[] = {
        SkShader::kClamp_TileMode,
        SkShader::kRepeat_TileMode,
        SkShader::kMirror_TileMode,
    };
    SkShader::TileMode tileModes[] = {
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
    };
    GrTextureParams params(tileModes, random->nextBool() ? GrTextureParams::kBilerp_FilterMode 
                                                         : GrTextureParams::kNone_FilterMode);

    return GrDistanceFieldNoGammaTextureEffect::Create(textures[texIdx], params,
        random->nextBool() ? kSimilarity_DistanceFieldEffectFlag : 0);
}

///////////////////////////////////////////////////////////////////////////////

class GrGLDistanceFieldLCDTextureEffect : public GrGLGeometryProcessor {
public:
    GrGLDistanceFieldLCDTextureEffect(const GrBackendProcessorFactory& factory,
                                      const GrProcessor&)
    : INHERITED (factory)
    , fTextureSize(SkISize::Make(-1,-1))
    , fTextColor(GrColor_ILLEGAL) {}

    virtual void emitCode(const EmitArgs& args) SK_OVERRIDE {
        const GrDistanceFieldLCDTextureEffect& dfTexEffect =
                args.fGP.cast<GrDistanceFieldLCDTextureEffect>();
        SkASSERT(1 == dfTexEffect.getVertexAttribs().count());

        GrGLVertToFrag v(kVec2f_GrSLType);
        args.fPB->addVarying("TextureCoords", &v);

        GrGLVertexBuilder* vsBuilder = args.fPB->getVertexShaderBuilder();
        vsBuilder->codeAppendf("\t%s = %s;\n", v.vsOut(), dfTexEffect.inTextureCoords().c_str());

        // setup position varying
        vsBuilder->codeAppendf("%s = %s * vec3(%s, 1);", vsBuilder->glPosition(),
                               vsBuilder->uViewM(), vsBuilder->inPosition());

        const char* textureSizeUniName = NULL;
        // width, height, 1/(3*width)
        fTextureSizeUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                              kVec3f_GrSLType, "TextureSize",
                                              &textureSizeUniName);

        GrGLGPFragmentBuilder* fsBuilder = args.fPB->getFragmentShaderBuilder();

        SkAssertResult(fsBuilder->enableFeature(
                GrGLFragmentShaderBuilder::kStandardDerivatives_GLSLFeature));

        // create LCD offset adjusted by inverse of transform
        fsBuilder->codeAppendf("\tvec2 uv = %s;\n", v.fsIn());
        fsBuilder->codeAppendf("\tvec2 st = uv*%s.xy;\n", textureSizeUniName);
        bool isUniformScale = !!(dfTexEffect.getFlags() & kUniformScale_DistanceFieldEffectMask);
        if (isUniformScale) {
            fsBuilder->codeAppend("\tfloat dx = dFdx(st.x);\n");
            fsBuilder->codeAppendf("\tvec2 offset = vec2(dx*%s.z, 0.0);\n", textureSizeUniName);
        } else {
            fsBuilder->codeAppend("\tvec2 Jdx = dFdx(st);\n");
            fsBuilder->codeAppend("\tvec2 Jdy = dFdy(st);\n");
            fsBuilder->codeAppendf("\tvec2 offset = %s.z*Jdx;\n", textureSizeUniName);
        }

        // green is distance to uv center
        fsBuilder->codeAppend("\tvec4 texColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[0], "uv", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tvec3 distance;\n");
        fsBuilder->codeAppend("\tdistance.y = texColor.r;\n");
        // red is distance to left offset
        fsBuilder->codeAppend("\tvec2 uv_adjusted = uv - offset;\n");
        fsBuilder->codeAppend("\ttexColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[0], "uv_adjusted", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tdistance.x = texColor.r;\n");
        // blue is distance to right offset
        fsBuilder->codeAppend("\tuv_adjusted = uv + offset;\n");
        fsBuilder->codeAppend("\ttexColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[0], "uv_adjusted", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tdistance.z = texColor.r;\n");

        fsBuilder->codeAppend("\tdistance = "
           "vec3(" SK_DistanceFieldMultiplier ")*(distance - vec3(" SK_DistanceFieldThreshold"));");

        // we adjust for the effect of the transformation on the distance by using
        // the length of the gradient of the texture coordinates. We use st coordinates
        // to ensure we're mapping 1:1 from texel space to pixel space.

        // To be strictly correct, we should compute the anti-aliasing factor separately
        // for each color component. However, this is only important when using perspective
        // transformations, and even then using a single factor seems like a reasonable
        // trade-off between quality and speed.
        fsBuilder->codeAppend("\tfloat afwidth;\n");
        if (isUniformScale) {
            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("\tafwidth = abs(" SK_DistanceFieldAAFactor "*dx);\n");
        } else {
            fsBuilder->codeAppend("\tvec2 uv_grad;\n");
            if (args.fPB->ctxInfo().caps()->dropsTileOnZeroDivide()) {
                // this is to compensate for the Adreno, which likes to drop tiles on division by 0
                fsBuilder->codeAppend("\tfloat uv_len2 = dot(uv, uv);\n");
                fsBuilder->codeAppend("\tif (uv_len2 < 0.0001) {\n");
                fsBuilder->codeAppend("\t\tuv_grad = vec2(0.7071, 0.7071);\n");
                fsBuilder->codeAppend("\t} else {\n");
                fsBuilder->codeAppend("\t\tuv_grad = uv*inversesqrt(uv_len2);\n");
                fsBuilder->codeAppend("\t}\n");
            } else {
                fsBuilder->codeAppend("\tuv_grad = normalize(uv);\n");
            }
            fsBuilder->codeAppend("\tvec2 grad = vec2(uv_grad.x*Jdx.x + uv_grad.y*Jdy.x,\n");
            fsBuilder->codeAppend("\t                 uv_grad.x*Jdx.y + uv_grad.y*Jdy.y);\n");

            // this gives us a smooth step across approximately one fragment
            fsBuilder->codeAppend("\tafwidth = " SK_DistanceFieldAAFactor "*length(grad);\n");
        }

        fsBuilder->codeAppend("\tvec4 val = vec4(smoothstep(vec3(-afwidth), vec3(afwidth), distance), 1.0);\n");

        // adjust based on gamma
        const char* textColorUniName = NULL;
        // width, height, 1/(3*width)
        fTextColorUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                             kVec3f_GrSLType, "TextColor",
                                             &textColorUniName);

        fsBuilder->codeAppendf("\tuv = vec2(val.x, %s.x);\n", textColorUniName);
        fsBuilder->codeAppend("\tvec4 gammaColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[1], "uv", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tval.x = gammaColor.r;\n");

        fsBuilder->codeAppendf("\tuv = vec2(val.y, %s.y);\n", textColorUniName);
        fsBuilder->codeAppend("\tgammaColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[1], "uv", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tval.y = gammaColor.r;\n");

        fsBuilder->codeAppendf("\tuv = vec2(val.z, %s.z);\n", textColorUniName);
        fsBuilder->codeAppend("\tgammaColor = ");
        fsBuilder->appendTextureLookup(args.fSamplers[1], "uv", kVec2f_GrSLType);
        fsBuilder->codeAppend(";\n");
        fsBuilder->codeAppend("\tval.z = gammaColor.r;\n");

        fsBuilder->codeAppendf("\t%s = %s;\n", args.fOutput,
                               (GrGLSLExpr4(args.fInput) * GrGLSLExpr4("val")).c_str());
    }

    virtual void setData(const GrGLProgramDataManager& pdman,
                         const GrProcessor& processor) SK_OVERRIDE {
        SkASSERT(fTextureSizeUni.isValid());
        SkASSERT(fTextColorUni.isValid());

        const GrDistanceFieldLCDTextureEffect& dfTexEffect =
                processor.cast<GrDistanceFieldLCDTextureEffect>();
        GrTexture* texture = processor.texture(0);
        if (texture->width() != fTextureSize.width() ||
            texture->height() != fTextureSize.height()) {
            fTextureSize = SkISize::Make(texture->width(), texture->height());
            float delta = 1.0f/(3.0f*texture->width());
            if (dfTexEffect.getFlags() & kBGR_DistanceFieldEffectFlag) {
                delta = -delta;
            }
            pdman.set3f(fTextureSizeUni,
                        SkIntToScalar(fTextureSize.width()),
                        SkIntToScalar(fTextureSize.height()),
                        delta);
        }

        GrColor textColor = dfTexEffect.getTextColor();
        if (textColor != fTextColor) {
            static const float ONE_OVER_255 = 1.f / 255.f;
            pdman.set3f(fTextColorUni,
                        GrColorUnpackR(textColor) * ONE_OVER_255,
                        GrColorUnpackG(textColor) * ONE_OVER_255,
                        GrColorUnpackB(textColor) * ONE_OVER_255);
            fTextColor = textColor;
        }
    }

    static inline void GenKey(const GrProcessor& processor, const GrGLCaps&,
                              GrProcessorKeyBuilder* b) {
        const GrDistanceFieldLCDTextureEffect& dfTexEffect =
                processor.cast<GrDistanceFieldLCDTextureEffect>();

        b->add32(dfTexEffect.getFlags());
    }

private:
    GrGLProgramDataManager::UniformHandle fTextureSizeUni;
    SkISize                               fTextureSize;
    GrGLProgramDataManager::UniformHandle fTextColorUni;
    SkColor                               fTextColor;

    typedef GrGLGeometryProcessor INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

GrDistanceFieldLCDTextureEffect::GrDistanceFieldLCDTextureEffect(
                                                  GrTexture* texture, const GrTextureParams& params,
                                                  GrTexture* gamma, const GrTextureParams& gParams,
                                                  SkColor textColor,
                                                  uint32_t flags)
    : fTextureAccess(texture, params)
    , fGammaTextureAccess(gamma, gParams)
    , fTextColor(textColor)
    , fFlags(flags & kLCD_DistanceFieldEffectMask)
    , fInTextureCoords(this->addVertexAttrib(GrShaderVar("inTextureCoords",
                                                         kVec2f_GrSLType,
                                                         GrShaderVar::kAttribute_TypeModifier))) {
    SkASSERT(!(flags & ~kLCD_DistanceFieldEffectMask) && (flags & kUseLCD_DistanceFieldEffectFlag));
        
    this->addTextureAccess(&fTextureAccess);
    this->addTextureAccess(&fGammaTextureAccess);
}

bool GrDistanceFieldLCDTextureEffect::onIsEqual(const GrGeometryProcessor& other) const {
    const GrDistanceFieldLCDTextureEffect& cte = other.cast<GrDistanceFieldLCDTextureEffect>();
    return (fTextColor == cte.fTextColor &&
            fFlags == cte.fFlags);
}

void GrDistanceFieldLCDTextureEffect::onComputeInvariantOutput(InvariantOutput* inout) const {
    inout->mulByUnknownColor();
}

const GrBackendGeometryProcessorFactory& GrDistanceFieldLCDTextureEffect::getFactory() const {
    return GrTBackendGeometryProcessorFactory<GrDistanceFieldLCDTextureEffect>::getInstance();
}

///////////////////////////////////////////////////////////////////////////////

GR_DEFINE_GEOMETRY_PROCESSOR_TEST(GrDistanceFieldLCDTextureEffect);

GrGeometryProcessor* GrDistanceFieldLCDTextureEffect::TestCreate(SkRandom* random,
                                                                 GrContext*,
                                                                 const GrDrawTargetCaps&,
                                                                 GrTexture* textures[]) {
    int texIdx = random->nextBool() ? GrProcessorUnitTest::kSkiaPMTextureIdx :
                                      GrProcessorUnitTest::kAlphaTextureIdx;
    int texIdx2 = random->nextBool() ? GrProcessorUnitTest::kSkiaPMTextureIdx :
                                       GrProcessorUnitTest::kAlphaTextureIdx;
    static const SkShader::TileMode kTileModes[] = {
        SkShader::kClamp_TileMode,
        SkShader::kRepeat_TileMode,
        SkShader::kMirror_TileMode,
    };
    SkShader::TileMode tileModes[] = {
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
        kTileModes[random->nextULessThan(SK_ARRAY_COUNT(kTileModes))],
    };
    GrTextureParams params(tileModes, random->nextBool() ? GrTextureParams::kBilerp_FilterMode :
                           GrTextureParams::kNone_FilterMode);
    GrTextureParams params2(tileModes, random->nextBool() ? GrTextureParams::kBilerp_FilterMode :
                           GrTextureParams::kNone_FilterMode);
    GrColor textColor = GrColorPackRGBA(random->nextULessThan(256),
                                        random->nextULessThan(256),
                                        random->nextULessThan(256),
                                        random->nextULessThan(256));
    uint32_t flags = kUseLCD_DistanceFieldEffectFlag;
    flags |= random->nextBool() ? kUniformScale_DistanceFieldEffectMask : 0;
    flags |= random->nextBool() ? kBGR_DistanceFieldEffectFlag : 0;
    return GrDistanceFieldLCDTextureEffect::Create(textures[texIdx], params,
                                                   textures[texIdx2], params2,
                                                   textColor,
                                                   flags);
}
