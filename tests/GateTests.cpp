// Gate-level logic contract: the three-valued (LOW / HIGH / UNDEFINED) truth
// tables and the special display/input components. These semantics are the
// domain truth of the simulator and should survive any restructuring, so the
// expected values are transcribed verbatim from the tables in Gate.h rather
// than re-derived (the X-propagation is deliberate, e.g. AND(LOW,X)=LOW but
// AND(HIGH,X)=X).

#include <catch2/catch_test_macros.hpp>

#include "Gate.h"
#include "TestHelpers.h"

namespace {
    constexpr LogicLevel L = LogicLevel::LOW;
    constexpr LogicLevel H = LogicLevel::HIGH;
    constexpr LogicLevel X = LogicLevel::UNDEFINED;
    constexpr LogicLevel levels[3] = { L, H, X };

    // Drive a 2-input gate over all 9 input combinations and compare against a
    // [firstInput][secondInput] expectation table.
    template <typename GateT>
    void checkTruthTable2(const LogicLevel expected[3][3])
    {
        for (int a = 0; a < 3; ++a) {
            for (int b = 0; b < 3; ++b) {
                GateT gate(0, { 0, 0 });
                gate.evaluate({ levels[a], levels[b] });
                CAPTURE(levels[a], levels[b]);
                REQUIRE(gate.m_outputValues[0] == expected[a][b]);
            }
        }
    }
}

TEST_CASE("AND truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { L, L, L },
        { L, H, X },
        { L, X, X },
    };
    checkTruthTable2<AndGate>(expected);
}

TEST_CASE("OR truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { L, H, X },
        { H, H, H },
        { X, H, X },
    };
    checkTruthTable2<OrGate>(expected);
}

TEST_CASE("NAND truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { H, H, X },
        { H, L, X },
        { X, X, X },
    };
    checkTruthTable2<NandGate>(expected);
}

TEST_CASE("NOR truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { H, L, X },
        { L, L, L },
        { X, L, X },
    };
    checkTruthTable2<NorGate>(expected);
}

TEST_CASE("XOR truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { L, H, X },
        { H, L, X },
        { X, X, X },
    };
    checkTruthTable2<XorGate>(expected);
}

TEST_CASE("XNOR truth table", "[gate]") {
    const LogicLevel expected[3][3] = {
        { H, L, X },
        { L, H, X },
        { X, X, X },
    };
    checkTruthTable2<XnorGate>(expected);
}

TEST_CASE("NOT truth table", "[gate]") {
    const LogicLevel expected[3] = { H, L, X }; // LOW->HIGH, HIGH->LOW, X->X
    for (int a = 0; a < 3; ++a) {
        NotGate gate(0, { 0, 0 });
        gate.evaluate({ levels[a] });
        CAPTURE(levels[a]);
        REQUIRE(gate.m_outputValues[0] == expected[a]);
    }
}

TEST_CASE("Gates yield UNDEFINED when under-driven", "[gate]") {
    SECTION("two-input gate with too few inputs") {
        AndGate gate(0, { 0, 0 });
        gate.evaluate({});          // no inputs
        REQUIRE(gate.m_outputValues[0] == X);
        gate.evaluate({ H });       // only one input
        REQUIRE(gate.m_outputValues[0] == X);
    }
    SECTION("NOT with no input") {
        NotGate gate(0, { 0, 0 });
        gate.evaluate({});
        REQUIRE(gate.m_outputValues[0] == X);
    }
}

TEST_CASE("TOGGLE starts LOW and flips on click", "[gate][toggle]") {
    ToggleGate toggle(0, { 0, 0 });
    REQUIRE(toggle.m_outputValues[0] == L);

    toggle.onClick();
    REQUIRE(toggle.m_outputValues[0] == H);

    toggle.onClick();
    REQUIRE(toggle.m_outputValues[0] == L);

    // evaluate() must not disturb a source's driven value.
    toggle.onClick();
    toggle.evaluate({});
    REQUIRE(toggle.m_outputValues[0] == H);
}

TEST_CASE("LED is a sink: one input, no outputs", "[gate][led]") {
    LedGate led(0, { 0, 0 });
    REQUIRE(led.m_inputWireIds.size() == 1);
    REQUIRE(led.m_outputValues.empty());
    REQUIRE(led.m_outputWireIds.empty());
    REQUIRE_NOTHROW(led.evaluate({ H })); // display-only no-op
}

TEST_CASE("Component pin counts match their role", "[gate]") {
    struct Case { componentInfo::Type type; size_t inputs; size_t outputs; };
    const Case cases[] = {
        { componentInfo::AND,    2, 1 },
        { componentInfo::NAND,   2, 1 },
        { componentInfo::OR,     2, 1 },
        { componentInfo::NOR,    2, 1 },
        { componentInfo::XOR,    2, 1 },
        { componentInfo::XNOR,   2, 1 },
        { componentInfo::NOT,    1, 1 },
        { componentInfo::LED,    1, 0 },
        { componentInfo::TOGGLE, 0, 1 },
    };
    for (const auto& c : cases) {
        auto gate = GateFactories[c.type](0, { 0, 0 });
        CAPTURE(gate->getLabel());
        REQUIRE(gate->m_inputWireIds.size() == c.inputs);
        REQUIRE(gate->m_outputValues.size() == c.outputs);
    }
}
