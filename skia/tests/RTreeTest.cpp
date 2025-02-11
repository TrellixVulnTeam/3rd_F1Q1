/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkRTree.h"
#include "SkRandom.h"
#include "SkTSort.h"
#include "Test.h"

static const size_t MIN_CHILDREN = 6;
static const size_t MAX_CHILDREN = 11;

static const int NUM_RECTS = 200;
static const size_t NUM_ITERATIONS = 100;
static const size_t NUM_QUERIES = 50;

static SkRect random_rect(SkRandom& rand) {
    SkRect rect = {0,0,0,0};
    while (rect.isEmpty()) {
        rect.fLeft   = rand.nextRangeF(0, 1000);
        rect.fRight  = rand.nextRangeF(0, 1000);
        rect.fTop    = rand.nextRangeF(0, 1000);
        rect.fBottom = rand.nextRangeF(0, 1000);
        rect.sort();
    }
    return rect;
}

static bool verify_query(SkRect query, SkRect rects[], SkTDArray<unsigned>& found) {
    // TODO(mtklein): no need to do this after everything's SkRects
    query.roundOut();

    SkTDArray<unsigned> expected;

    // manually intersect with every rectangle
    for (int i = 0; i < NUM_RECTS; ++i) {
        if (SkRect::Intersects(query, rects[i])) {
            expected.push(i);
        }
    }

    if (expected.count() != found.count()) {
        return false;
    }

    if (0 == expected.count()) {
        return true;
    }

    // skia:2834.  RTree doesn't always return results in order.
    SkTQSort(expected.begin(), expected.end() -1);
    SkTQSort(found.begin(), found.end() -1);
    return found == expected;
}

static void run_queries(skiatest::Reporter* reporter, SkRandom& rand, SkRect rects[],
                       SkRTree& tree) {
    for (size_t i = 0; i < NUM_QUERIES; ++i) {
        SkTDArray<unsigned> hits;
        SkRect query = random_rect(rand);
        tree.search(query, &hits);
        REPORTER_ASSERT(reporter, verify_query(query, rects, hits));
    }
}

static void rtree_test_main(SkRTree* rtree, skiatest::Reporter* reporter) {
    SkASSERT(rtree);

    int expectedDepthMin = -1;
    int expectedDepthMax = -1;

    int tmp = NUM_RECTS;
    while (tmp > 0) {
        tmp -= static_cast<int>(pow(static_cast<double>(MAX_CHILDREN),
                                static_cast<double>(expectedDepthMin + 1)));
        ++expectedDepthMin;
    }

    tmp = NUM_RECTS;
    while (tmp > 0) {
        tmp -= static_cast<int>(pow(static_cast<double>(MIN_CHILDREN),
                                static_cast<double>(expectedDepthMax + 1)));
        ++expectedDepthMax;
    }

    SkRandom rand;
    SkAutoTMalloc<SkRect> rects(NUM_RECTS);
    for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
        rtree->clear();
        REPORTER_ASSERT(reporter, 0 == rtree->getCount());

        for (int j = 0; j < NUM_RECTS; j++) {
            rects[j] = random_rect(rand);
        }

        rtree->insert(&rects, NUM_RECTS);
        SkASSERT(rects);  // SkRTree doesn't take ownership of rects.

        run_queries(reporter, rand, rects, *rtree);
        REPORTER_ASSERT(reporter, NUM_RECTS == rtree->getCount());
        REPORTER_ASSERT(reporter, expectedDepthMin <= rtree->getDepth() &&
                                  expectedDepthMax >= rtree->getDepth());
    }
}

DEF_TEST(RTree, reporter) {
    SkRTree* rtree = SkRTree::Create(MIN_CHILDREN, MAX_CHILDREN);
    SkAutoUnref au(rtree);
    rtree_test_main(rtree, reporter);

    // Rtree that orders input rectangles on deferred insert.
    SkRTree* unsortedRtree = SkRTree::Create(MIN_CHILDREN, MAX_CHILDREN, 1, false);
    SkAutoUnref auo(unsortedRtree);
    rtree_test_main(unsortedRtree, reporter);
}
