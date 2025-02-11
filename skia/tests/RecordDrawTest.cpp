/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Test.h"
#include "RecordTestUtils.h"

#include "SkDebugCanvas.h"
#include "SkDrawPictureCallback.h"
#include "SkDropShadowImageFilter.h"
#include "SkImagePriv.h"
#include "SkRecord.h"
#include "SkRecordDraw.h"
#include "SkRecordOpts.h"
#include "SkRecorder.h"
#include "SkRecords.h"
#include "SkSurface.h"

static const int W = 1920, H = 1080;

class JustOneDraw : public SkDrawPictureCallback {
public:
    JustOneDraw() : fCalls(0) {}

    virtual bool abortDrawing() SK_OVERRIDE { return fCalls++ > 0; }
private:
    int fCalls;
};

DEF_TEST(RecordDraw_Abort, r) {
    // Record two commands.
    SkRecord record;
    SkRecorder recorder(&record, W, H);
    recorder.drawRect(SkRect::MakeWH(200, 300), SkPaint());
    recorder.clipRect(SkRect::MakeWH(100, 200));

    SkRecord rerecord;
    SkRecorder canvas(&rerecord, W, H);

    JustOneDraw callback;
    SkRecordDraw(record, &canvas, NULL/*bbh*/, &callback);

    REPORTER_ASSERT(r, 3 == rerecord.count());
    assert_type<SkRecords::Save>    (r, rerecord, 0);
    assert_type<SkRecords::DrawRect>(r, rerecord, 1);
    assert_type<SkRecords::Restore> (r, rerecord, 2);
}

DEF_TEST(RecordDraw_Unbalanced, r) {
    SkRecord record;
    SkRecorder recorder(&record, W, H);
    recorder.save();  // We won't balance this, but SkRecordDraw will for us.

    SkRecord rerecord;
    SkRecorder canvas(&rerecord, W, H);
    SkRecordDraw(record, &canvas, NULL/*bbh*/, NULL/*callback*/);

    REPORTER_ASSERT(r, 4 == rerecord.count());
    assert_type<SkRecords::Save>    (r, rerecord, 0);
    assert_type<SkRecords::Save>    (r, rerecord, 1);
    assert_type<SkRecords::Restore> (r, rerecord, 2);
    assert_type<SkRecords::Restore> (r, rerecord, 3);
}

DEF_TEST(RecordDraw_SetMatrixClobber, r) {
    // Set up an SkRecord that just scales by 2x,3x.
    SkRecord scaleRecord;
    SkRecorder scaleCanvas(&scaleRecord, W, H);
    SkMatrix scale;
    scale.setScale(2, 3);
    scaleCanvas.setMatrix(scale);

    // Set up an SkRecord with an initial +20, +20 translate.
    SkRecord translateRecord;
    SkRecorder translateCanvas(&translateRecord, W, H);
    SkMatrix translate;
    translate.setTranslate(20, 20);
    translateCanvas.setMatrix(translate);

    SkRecordDraw(scaleRecord, &translateCanvas, NULL/*bbh*/, NULL/*callback*/);
    REPORTER_ASSERT(r, 4 == translateRecord.count());
    assert_type<SkRecords::SetMatrix>(r, translateRecord, 0);
    assert_type<SkRecords::Save>     (r, translateRecord, 1);
    assert_type<SkRecords::SetMatrix>(r, translateRecord, 2);
    assert_type<SkRecords::Restore>  (r, translateRecord, 3);

    // When we look at translateRecord now, it should have its first +20,+20 translate,
    // then a 2x,3x scale that's been concatted with that +20,+20 translate.
    const SkRecords::SetMatrix* setMatrix;
    setMatrix = assert_type<SkRecords::SetMatrix>(r, translateRecord, 0);
    REPORTER_ASSERT(r, setMatrix->matrix == translate);

    setMatrix = assert_type<SkRecords::SetMatrix>(r, translateRecord, 2);
    SkMatrix expected = scale;
    expected.postConcat(translate);
    REPORTER_ASSERT(r, setMatrix->matrix == expected);
}

struct TestBBH : public SkBBoxHierarchy {
    virtual void insert(SkAutoTMalloc<SkRect>* boundsArray, int N) SK_OVERRIDE {
        fEntries.setCount(N);
        for (int i = 0; i < N; i++) {
            Entry e = { (unsigned)i, (*boundsArray)[i] };
            fEntries[i] = e;
        }
    }

    virtual void search(const SkRect& query, SkTDArray<unsigned>* results) const SK_OVERRIDE {}

    struct Entry {
        unsigned opIndex;
        SkRect bounds;
    };
    SkTDArray<Entry> fEntries;
};

// Like a==b, with a little slop recognizing that float equality can be weird.
static bool sloppy_rect_eq(SkRect a, SkRect b) {
    SkRect inset(a), outset(a);
    inset.inset(1, 1);
    outset.outset(1, 1);
    return outset.contains(b) && !inset.contains(b);
}

// This test is not meant to make total sense yet.  It's testing the status quo
// of SkRecordFillBounds(), which itself doesn't make total sense yet.
DEF_TEST(RecordDraw_BBH, r) {
    SkRecord record;
    SkRecorder recorder(&record, W, H);
    recorder.save();
        recorder.clipRect(SkRect::MakeWH(400, 500));
        recorder.scale(2, 2);
        recorder.drawRect(SkRect::MakeWH(320, 240), SkPaint());
    recorder.restore();

    TestBBH bbh;
    SkRecordFillBounds(SkRect::MakeWH(SkIntToScalar(W), SkIntToScalar(H)), record, &bbh);

    REPORTER_ASSERT(r, bbh.fEntries.count() == 5);
    for (int i = 0; i < bbh.fEntries.count(); i++) {
        REPORTER_ASSERT(r, bbh.fEntries[i].opIndex == (unsigned)i);

        REPORTER_ASSERT(r, sloppy_rect_eq(SkRect::MakeWH(400, 480), bbh.fEntries[i].bounds));
    }
}

// A regression test for crbug.com/409110.
DEF_TEST(RecordDraw_TextBounds, r) {
    SkRecord record;
    SkRecorder recorder(&record, W, H);

    // Two Chinese characters in UTF-8.
    const char text[] = { '\xe6', '\xbc', '\xa2', '\xe5', '\xad', '\x97' };
    const size_t bytes = SK_ARRAY_COUNT(text);

    const SkScalar xpos[] = { 10, 20 };
    recorder.drawPosTextH(text, bytes, xpos, 30, SkPaint());

    const SkPoint pos[] = { {40, 50}, {60, 70} };
    recorder.drawPosText(text, bytes, pos, SkPaint());

    TestBBH bbh;
    SkRecordFillBounds(SkRect::MakeWH(SkIntToScalar(W), SkIntToScalar(H)), record, &bbh);
    REPORTER_ASSERT(r, bbh.fEntries.count() == 2);

    // We can make these next assertions confidently because SkRecordFillBounds
    // builds its bounds by overestimating font metrics in a platform-independent way.
    // If that changes, these tests will need to be more flexible.
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[0].bounds, SkRect::MakeLTRB(0,  0, 140, 60)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[1].bounds, SkRect::MakeLTRB(0, 20, 180, 100)));
}

// Base test to ensure start/stop range is respected
DEF_TEST(RecordDraw_PartialStartStop, r) {
    static const int kWidth = 10, kHeight = 10;

    SkRect r1 = { 0, 0, kWidth,   kHeight };
    SkRect r2 = { 0, 0, kWidth,   kHeight/2 };
    SkRect r3 = { 0, 0, kWidth/2, kHeight };
    SkPaint p;

    SkRecord record;
    SkRecorder recorder(&record, kWidth, kHeight);
    recorder.drawRect(r1, p);
    recorder.drawRect(r2, p);
    recorder.drawRect(r3, p);

    SkRecord rerecord;
    SkRecorder canvas(&rerecord, kWidth, kHeight);
    SkRecordPartialDraw(record, &canvas, r1, 1, 2, SkMatrix::I()); // replay just drawRect of r2

    REPORTER_ASSERT(r, 3 == rerecord.count());
    assert_type<SkRecords::Save>     (r, rerecord, 0);
    assert_type<SkRecords::DrawRect> (r, rerecord, 1);
    assert_type<SkRecords::Restore>  (r, rerecord, 2);

    const SkRecords::DrawRect* drawRect = assert_type<SkRecords::DrawRect>(r, rerecord, 1);
    REPORTER_ASSERT(r, drawRect->rect == r2);
}

// Check that clears are converted to drawRects
DEF_TEST(RecordDraw_PartialClear, r) {
    static const int kWidth = 10, kHeight = 10;

    SkRect rect = { 0, 0, kWidth, kHeight };

    SkRecord record;
    SkRecorder recorder(&record, kWidth, kHeight);
    recorder.clear(SK_ColorRED);

    SkRecord rerecord;
    SkRecorder canvas(&rerecord, kWidth, kHeight);
    SkRecordPartialDraw(record, &canvas, rect, 0, 1, SkMatrix::I()); // replay just the clear

    REPORTER_ASSERT(r, 3 == rerecord.count());
    assert_type<SkRecords::Save>    (r, rerecord, 0);
    assert_type<SkRecords::DrawRect>(r, rerecord, 1);
    assert_type<SkRecords::Restore> (r, rerecord, 2);

    const SkRecords::DrawRect* drawRect = assert_type<SkRecords::DrawRect>(r, rerecord, 1);
    REPORTER_ASSERT(r, drawRect->rect == rect);
    REPORTER_ASSERT(r, drawRect->paint.getColor() == SK_ColorRED);
}

// A regression test for crbug.com/415468 and skbug.com/2957.
//
// This also now serves as a regression test for crbug.com/418417.  We used to adjust the
// bounds for the saveLayer, clip, and restore to be greater than the bounds of the picture.
// (We were applying the saveLayer paint to the bounds after restore, which makes no sense.)
DEF_TEST(RecordDraw_SaveLayerAffectsClipBounds, r) {
    SkRecord record;
    SkRecorder recorder(&record, 50, 50);

    // We draw a rectangle with a long drop shadow.  We used to not update the clip
    // bounds based on SaveLayer paints, so the drop shadow could be cut off.
    SkPaint paint;
    paint.setImageFilter(SkDropShadowImageFilter::Create(20, 0, 0, 0, SK_ColorBLACK,
                         SkDropShadowImageFilter::kDrawShadowAndForeground_ShadowMode))->unref();

    recorder.saveLayer(NULL, &paint);
        recorder.clipRect(SkRect::MakeWH(20, 40));
        recorder.drawRect(SkRect::MakeWH(20, 40), SkPaint());
    recorder.restore();

    // Under the original bug, the right edge value of the drawRect would be 20 less than asserted
    // here because we intersected it with a clip that had not been adjusted for the drop shadow.
    //
    // The second bug showed up as adjusting the picture bounds (0,0,50,50) by the drop shadow too.
    // The saveLayer, clipRect, and restore bounds were incorrectly (0,0,70,50).
    TestBBH bbh;
    SkRecordFillBounds(SkRect::MakeWH(50, 50), record, &bbh);
    REPORTER_ASSERT(r, bbh.fEntries.count() == 4);
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[0].bounds, SkRect::MakeLTRB(0, 0, 50, 50)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[1].bounds, SkRect::MakeLTRB(0, 0, 50, 50)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[2].bounds, SkRect::MakeLTRB(0, 0, 40, 40)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[3].bounds, SkRect::MakeLTRB(0, 0, 50, 50)));
}

// When a saveLayer provides an explicit bound and has a complex paint (e.g., one that
// affects transparent black), that bound should serve to shrink the area of the required
// backing store.
DEF_TEST(RecordDraw_SaveLayerBoundsAffectsClipBounds, r) {
    SkRecord record;
    SkRecorder recorder(&record, 50, 50);

    SkPaint p;
    p.setXfermodeMode(SkXfermode::kSrc_Mode);

    SkRect bounds = SkRect::MakeLTRB(10, 10, 40, 40);
    recorder.saveLayer(&bounds, &p);
    recorder.drawRect(SkRect::MakeLTRB(20, 20, 30, 30), SkPaint());
    recorder.restore();

    TestBBH bbh;
    SkRecordFillBounds(SkRect::MakeWH(50, 50), record, &bbh);
    REPORTER_ASSERT(r, bbh.fEntries.count() == 3);
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[0].bounds, SkRect::MakeLTRB(10, 10, 40, 40)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[1].bounds, SkRect::MakeLTRB(20, 20, 30, 30)));
    REPORTER_ASSERT(r, sloppy_rect_eq(bbh.fEntries[2].bounds, SkRect::MakeLTRB(10, 10, 40, 40)));
}

DEF_TEST(RecordDraw_drawImage, r){
    class SkCanvasMock : public SkCanvas {
    public:
        SkCanvasMock(int width, int height) : INHERITED(width, height) {
            this->resetTestValues();
        }
        virtual ~SkCanvasMock() {}
        virtual void drawImage(const SkImage* image, SkScalar left, SkScalar top,
                               const SkPaint* paint = NULL) SK_OVERRIDE {

            fDrawImageCalled = true;
        }

        virtual void drawImageRect(const SkImage* image, const SkRect* src,
                                   const SkRect& dst,
                                   const SkPaint* paint = NULL) SK_OVERRIDE {
            fDrawImageRectCalled = true;
        }

        void resetTestValues() {
            fDrawImageCalled = fDrawImageRectCalled = false;
        }

        bool fDrawImageCalled;
        bool fDrawImageRectCalled;
    private:
        typedef SkCanvas INHERITED;
    };

    SkAutoTUnref<SkSurface> surface(SkSurface::NewRasterPMColor(10, 10));
    surface->getCanvas()->clear(SK_ColorGREEN);
    SkAutoTUnref<SkImage> image(surface->newImageSnapshot());

    SkCanvasMock canvas(10, 10);

    {
        SkRecord record;
        SkRecorder recorder(&record, 10, 10);
        recorder.drawImage(image, 0, 0);
        SkRecordDraw(record, &canvas, 0, 0);
    }
    REPORTER_ASSERT(r, canvas.fDrawImageCalled);
    canvas.resetTestValues();

    {
        SkRecord record;
        SkRecorder recorder(&record, 10, 10);
        recorder.drawImageRect(image, 0, SkRect::MakeWH(10, 10));
        SkRecordDraw(record, &canvas, 0, 0);
    }
    REPORTER_ASSERT(r, canvas.fDrawImageRectCalled);

}
