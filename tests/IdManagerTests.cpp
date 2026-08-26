// IdManager id<->index plumbing: hands out ids, maps them to storage indices,
// and recycles released ids via a free list. No raylib dependency.

#include <catch2/catch_test_macros.hpp>

#include "IdManager.h"

TEST_CASE("getNextId hands out fresh, distinct ids", "[idmanager]") {
    IdManager ids;
    int a = ids.getNextId();
    int b = ids.getNextId();
    REQUIRE(a != b);
    // A brand-new id maps to nothing until an index is assigned.
    REQUIRE(ids.getIndex(a) == -1);
}

TEST_CASE("setIndex / getIndex round-trip", "[idmanager]") {
    IdManager ids;
    int id = ids.getNextId();
    ids.setIndex(id, 7);
    REQUIRE(ids.getIndex(id) == 7);
}

TEST_CASE("getIndex returns -1 for out-of-range ids", "[idmanager]") {
    IdManager ids;
    REQUIRE(ids.getIndex(-1) == -1);
    REQUIRE(ids.getIndex(0) == -1);   // nothing allocated yet
    REQUIRE(ids.getIndex(999) == -1);
}

TEST_CASE("releaseId frees an id and getNextId reuses it", "[idmanager]") {
    IdManager ids;
    int a = ids.getNextId();
    ids.setIndex(a, 3);

    ids.releaseId(a);
    REQUIRE(ids.getIndex(a) == -1); // released ids no longer resolve

    int reused = ids.getNextId();
    REQUIRE(reused == a);           // the free list hands the same id back
}

TEST_CASE("reuseId reclaims a freed id and restores its index", "[idmanager]") {
    IdManager ids;
    int a = ids.getNextId();
    ids.releaseId(a);

    ids.reuseId(a, 5);
    REQUIRE(ids.getIndex(a) == 5);

    // Having been reclaimed, it is no longer on the free list to be reissued.
    int next = ids.getNextId();
    REQUIRE(next != a);
}

TEST_CASE("clear resets the manager", "[idmanager]") {
    IdManager ids;
    int a = ids.getNextId();
    ids.setIndex(a, 1);

    ids.clear();
    REQUIRE(ids.getIndex(a) == -1);
    REQUIRE(ids.getNextId() == 0); // numbering starts over from scratch
}
