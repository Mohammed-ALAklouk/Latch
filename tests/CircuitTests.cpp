// The simulation itself: circuits built through the public façade
// (addComponent / addWire / evaluate) and asserted at steady state. Tests tick
// until settled and check the final values, never a per-tick schedule, so they
// stay correct across a change to the propagation engine.

#include <catch2/catch_test_macros.hpp>

#include "Circuit.h"
#include "ComponentInfo.h"
#include "TestHelpers.h"

namespace {
    constexpr LogicLevel L = LogicLevel::LOW;
    constexpr LogicLevel H = LogicLevel::HIGH;
    constexpr LogicLevel X = LogicLevel::UNDEFINED;

    // A TOGGLE only holds LOW or HIGH; one click flips it, so drive it to a
    // target level by flipping only when it isn't already there.
    void setToggle(Circuit& circuit, int id, LogicLevel want)
    {
        if (outputOf(circuit, id) != want)
            circuit.getComponent(id)->onClick();
    }
}

TEST_CASE("Two toggles drive an AND gate", "[circuit]") {
    Circuit c;
    int a = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int b = c.addComponent(componentInfo::TOGGLE, { 0, 100 });
    int g = c.addComponent(componentInfo::AND, { 200, 0 });
    connect(c, a, 0, g, 0);
    connect(c, b, 0, g, 1);

    struct Case { LogicLevel a, b, out; };
    const Case cases[] = {
        { L, L, L }, { L, H, L }, { H, L, L }, { H, H, H },
    };
    for (const auto& t : cases) {
        setToggle(c, a, t.a);
        setToggle(c, b, t.b);
        CAPTURE(t.a, t.b);
        REQUIRE(settle(c));
        REQUIRE(outputOf(c, g) == t.out);
    }
}

TEST_CASE("Signal propagates through a chain of gates", "[circuit]") {
    // TOGGLE -> NOT -> NOT should reproduce the toggle after settling (a two
    // level network the single-pass engine only resolves over multiple ticks).
    Circuit c;
    int src = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int n1 = c.addComponent(componentInfo::NOT, { 100, 0 });
    int n2 = c.addComponent(componentInfo::NOT, { 200, 0 });
    connect(c, src, 0, n1, 0);
    connect(c, n1, 0, n2, 0);

    setToggle(c, src, H);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, n1) == L);
    REQUIRE(outputOf(c, n2) == H);

    setToggle(c, src, L);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, n2) == L);
}

TEST_CASE("Half adder computes sum and carry", "[circuit]") {
    // sum = A XOR B, carry = A AND B, with each input fanning out to two gates.
    Circuit c;
    int a = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int b = c.addComponent(componentInfo::TOGGLE, { 0, 100 });
    int sum = c.addComponent(componentInfo::XOR, { 200, 0 });
    int carry = c.addComponent(componentInfo::AND, { 200, 100 });
    connect(c, a, 0, sum, 0);
    connect(c, b, 0, sum, 1);
    connect(c, a, 0, carry, 0);
    connect(c, b, 0, carry, 1);

    struct Case { LogicLevel a, b, sum, carry; };
    const Case cases[] = {
        { L, L, L, L }, { L, H, H, L }, { H, L, H, L }, { H, H, L, H },
    };
    for (const auto& t : cases) {
        setToggle(c, a, t.a);
        setToggle(c, b, t.b);
        CAPTURE(t.a, t.b);
        REQUIRE(settle(c));
        REQUIRE(outputOf(c, sum) == t.sum);
        REQUIRE(outputOf(c, carry) == t.carry);
    }
}

TEST_CASE("An unconnected input reads as UNDEFINED", "[circuit]") {
    Circuit c;
    int src = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int g = c.addComponent(componentInfo::AND, { 200, 0 });
    connect(c, src, 0, g, 0); // second AND input left floating

    setToggle(c, src, H);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, g) == X); // AND(HIGH, UNDEFINED) == UNDEFINED
}

TEST_CASE("addComponent returns valid ids and rejects bad types", "[circuit]") {
    Circuit c;
    int id = c.addComponent(componentInfo::OR, { 0, 0 });
    REQUIRE(id >= 0);
    REQUIRE(c.componentExists(id));

    REQUIRE(c.addComponent(componentInfo::COMPONENT_COUNT, { 0, 0 }) == -1);
    REQUIRE(c.addComponent(static_cast<componentInfo::Type>(-1), { 0, 0 }) == -1);
}

TEST_CASE("Removing a driving component degrades its consumers to UNDEFINED", "[circuit]") {
    Circuit c;
    int src = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int n = c.addComponent(componentInfo::NOT, { 100, 0 });
    int wire = connect(c, src, 0, n, 0);

    setToggle(c, src, H);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, n) == L);

    c.removeComponent(src);
    REQUIRE_FALSE(c.componentExists(src));
    REQUIRE_FALSE(c.wireExists(wire)); // the attached wire is torn down with it

    REQUIRE(settle(c)); // must not touch the dead component/wire
    REQUIRE(outputOf(c, n) == X);
}

TEST_CASE("Removing a wire leaves its consumer safely UNDEFINED", "[circuit]") {
    Circuit c;
    int src = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int n = c.addComponent(componentInfo::NOT, { 100, 0 });
    int wire = connect(c, src, 0, n, 0);

    setToggle(c, src, H);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, n) == L);

    c.removeWire(wire);
    REQUIRE_FALSE(c.wireExists(wire));

    REQUIRE(settle(c)); // stale wire id on the input must be handled, not crash
    REQUIRE(outputOf(c, n) == X);
}

TEST_CASE("Snapshot restores a circuit's evaluation", "[circuit][snapshot]") {
    Circuit c;
    int a = c.addComponent(componentInfo::TOGGLE, { 0, 0 });
    int b = c.addComponent(componentInfo::TOGGLE, { 0, 100 });
    int g = c.addComponent(componentInfo::AND, { 200, 0 });
    connect(c, a, 0, g, 0);
    connect(c, b, 0, g, 1);

    setToggle(c, a, H);
    setToggle(c, b, H);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, g) == H);

    auto snap = c.GetSnapshot("both high");

    setToggle(c, a, L);
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, g) == L);

    c.RestoreSnapshot(snap);
    REQUIRE(c.componentExists(a));
    REQUIRE(c.componentExists(g));
    REQUIRE(settle(c));
    REQUIRE(outputOf(c, g) == H);
}
