#include <doctest/doctest.h>
#include "element_data.h"

#include <cstring>

// ===========================================================================
//  findElementByZ
// ===========================================================================

TEST_CASE("findElementByZ: known elements") {
    const ElementInfo* h = findElementByZ(1);
    REQUIRE(h != nullptr);
    CHECK(h->Z == 1);
    CHECK(std::strcmp(h->symbol, "H") == 0);
    CHECK(std::strcmp(h->name, "Hydrogen") == 0);

    const ElementInfo* og = findElementByZ(118);
    REQUIRE(og != nullptr);
    CHECK(og->Z == 118);
    CHECK(std::strcmp(og->symbol, "Og") == 0);
    CHECK(std::strcmp(og->name, "Oganesson") == 0);

    // Spot-check a mid-range element
    const ElementInfo* au = findElementByZ(79);
    REQUIRE(au != nullptr);
    CHECK(au->Z == 79);
    CHECK(std::strcmp(au->symbol, "Au") == 0);
}

TEST_CASE("findElementByZ: out-of-range returns nullptr") {
    CHECK(findElementByZ(0)    == nullptr);
    CHECK(findElementByZ(-1)   == nullptr);
    CHECK(findElementByZ(119)  == nullptr);
    CHECK(findElementByZ(1000) == nullptr);
}

// ===========================================================================
//  findElementBySymbol
// ===========================================================================

TEST_CASE("findElementBySymbol: known symbols") {
    const ElementInfo* h = findElementBySymbol("H");
    REQUIRE(h != nullptr);
    CHECK(h->Z == 1);

    const ElementInfo* he = findElementBySymbol("He");
    REQUIRE(he != nullptr);
    CHECK(he->Z == 2);

    const ElementInfo* fe = findElementBySymbol("Fe");
    REQUIRE(fe != nullptr);
    CHECK(fe->Z == 26);

    const ElementInfo* u = findElementBySymbol("U");
    REQUIRE(u != nullptr);
    CHECK(u->Z == 92);
}

TEST_CASE("findElementBySymbol: unknown or empty symbol returns nullptr") {
    CHECK(findElementBySymbol("Xx")       == nullptr);
    CHECK(findElementBySymbol("")         == nullptr);
    CHECK(findElementBySymbol("hydrogen") == nullptr);  // case-sensitive
    CHECK(findElementBySymbol("h")        == nullptr);  // case-sensitive
}

// ===========================================================================
//  ELEMENTS array data integrity
// ===========================================================================

TEST_CASE("ELEMENTS: Z matches array index for all 118 elements") {
    for (int i = 1; i <= 118; ++i) {
        CHECK(ELEMENTS[i].Z == i);
    }
}

TEST_CASE("ELEMENTS: Z_eff is positive for all real elements") {
    for (int i = 1; i <= 118; ++i) {
        CHECK(ELEMENTS[i].Z_eff > 0.0);
    }
}

TEST_CASE("ELEMENTS: outermost_n is in [1, 7]") {
    for (int i = 1; i <= 118; ++i) {
        CHECK(ELEMENTS[i].outermost_n >= 1);
        CHECK(ELEMENTS[i].outermost_n <= 7);
    }
}

TEST_CASE("ELEMENTS: outermost_l is in [0, 3]  (s/p/d/f)") {
    for (int i = 1; i <= 118; ++i) {
        CHECK(ELEMENTS[i].outermost_l >= 0);
        CHECK(ELEMENTS[i].outermost_l <= 3);
    }
}

TEST_CASE("ELEMENTS: all seven periods are populated") {
    bool hasPeriod[8] = {};
    for (int i = 1; i <= 118; ++i) {
        int p = ELEMENTS[i].period;
        REQUIRE(p >= 1);
        REQUIRE(p <= 7);
        hasPeriod[p] = true;
    }
    for (int p = 1; p <= 7; ++p) {
        CHECK(hasPeriod[p]);
    }
}

TEST_CASE("ELEMENTS: symbol and name fields are non-null and non-empty") {
    for (int i = 1; i <= 118; ++i) {
        REQUIRE(ELEMENTS[i].symbol != nullptr);
        REQUIRE(ELEMENTS[i].name   != nullptr);
        CHECK(std::strlen(ELEMENTS[i].symbol) > 0);
        CHECK(std::strlen(ELEMENTS[i].name)   > 0);
    }
}

// ===========================================================================
//  NUM_ELEMENTS
// ===========================================================================

TEST_CASE("NUM_ELEMENTS equals 118") {
    CHECK(NUM_ELEMENTS == 118);
}

// ===========================================================================
//  Consistency between findElementByZ and findElementBySymbol
// ===========================================================================

TEST_CASE("findElementByZ and findElementBySymbol are consistent") {
    const int spot_check[] = {1, 6, 8, 26, 47, 79, 92, 118};
    for (int Z : spot_check) {
        const ElementInfo* byZ = findElementByZ(Z);
        REQUIRE(byZ != nullptr);
        const ElementInfo* bySym = findElementBySymbol(byZ->symbol);
        REQUIRE(bySym != nullptr);
        CHECK(bySym->Z == byZ->Z);
        CHECK(bySym == byZ);  // should point to same entry
    }
}
