/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrDefaultGeoProcFactory_DEFINED
#define GrDefaultGeoProcFactory_DEFINED

#include "GrGeometryProcessor.h"

class GrDrawState;

/*
 * A factory for creating default Geometry Processors which simply multiply position by the uniform
 * view matrix and wire through color, coverage, UV coords if requested.  Right now this is only
 * used in the creation of optimized draw states because adding default GPs to the drawstate can
 * interfere with batching due to updating the drawstate.
 * TODO When we track geometry state separately from the draw state, we should be able use a default
 * GP with every draw call
 */
class GrDefaultGeoProcFactory {
public:
    // Structs for adding vertex attributes
    struct PositionAttr {
        SkPoint fPosition;
    };

    struct PositionCoverageAttr {
        SkPoint fPosition;
        GrColor fCoverage;
    };

    struct PositionColorAttr {
        SkPoint fPosition;
        SkColor fColor;
    };

    struct PositionColorCoverageAttr {
        SkPoint fPosition;
        SkColor fColor;
        GrColor fCoverage;
    };

    struct PositionLocalCoordAttr {
        SkPoint fPosition;
        SkPoint fLocalCoord;
    };

    struct PositionLocalCoordCoverageAttr {
        SkPoint fPosition;
        SkPoint fLocalCoord;
        GrColor fCoverage;
    };

    struct PositionColorLocalCoordAttr {
        SkPoint fPosition;
        GrColor fColor;
        SkPoint fLocalCoord;
    };

    struct PositionColorLocalCoordCoverage {
        SkPoint fPosition;
        GrColor fColor;
        SkPoint fLocalCoord;
        GrColor fCoverage;
    };

    enum GPType {
        kPosition_GPType = 0x0, // we ALWAYS have position
        kColor_GPType = 0x01,
        kLocalCoord_GPType = 0x02,
        kCoverage_GPType= 0x04,
        kLastGPType = kCoverage_GPType
    };

    // YOU MUST UNREF
    static const GrGeometryProcessor* CreateAndSetAttribs(GrDrawState*, uint32_t GPTypeFlags);
    static const GrGeometryProcessor* Create();
};

#endif
