#pragma once
// Shared test utilities. Every place a test reaches into the implementation is
// funnelled through this header, so a structural change to the core updates one
// helper instead of hundreds of assertions.

#include <vector>
#include <string>

#include <catch2/catch_tostring.hpp>

#include "Circuit.h"
#include "Gate.h"
#include "Pin.h"

// --- reading results -------------------------------------------------------

// The value a component is currently driving on one of its output pins.
inline LogicLevel outputOf(Circuit& circuit, int componentID, int pin = 0)
{
    return circuit.getComponent(componentID)->m_outputValues[pin];
}

// --- building circuits -----------------------------------------------------

// Wire an output pin of one component to an input pin of another, using the
// components' real pin geometry so the wire's endpoints land on the pins.
// Returns the new wire id. Isolates the addWire() signature from the tests.
inline int connect(Circuit& circuit, int srcID, int srcPin, int dstID, int dstPin)
{
    auto& src = circuit.getComponent(srcID);
    auto& dst = circuit.getComponent(dstID);
    Vector2 srcPos = src->getOutputPosition(srcPin);
    Vector2 dstPos = dst->getInputPosition(dstPin);
    return circuit.addWire(PinRef{ srcID, srcPin }, srcPos,
                           PinRef{ dstID, dstPin }, dstPos);
}

// --- running the simulation ------------------------------------------------

// Tick the circuit until its component outputs stop changing, then return true.
// Returns false if it never stabilized within maxIter ticks (an oscillator).
// Steady-state is the engine-agnostic contract: it holds whether the engine
// advances one gate-level per tick or settles a whole network in one pass.
inline bool settle(Circuit& circuit, int maxIter = 64)
{
    auto snapshot = [&]() {
        std::vector<std::vector<LogicLevel>> values;
        values.reserve(circuit.m_components.size());
        for (auto& component : circuit.m_components)
            values.push_back(component->m_outputValues);
        return values;
    };

    auto previous = snapshot();
    for (int i = 0; i < maxIter; ++i) {
        circuit.evaluate();
        auto current = snapshot();
        if (current == previous)
            return true;
        previous = std::move(current);
    }
    return false;
}

// --- readable failure messages ---------------------------------------------

namespace Catch {
    template <>
    struct StringMaker<LogicLevel> {
        static std::string convert(LogicLevel value)
        {
            switch (value) {
                case LogicLevel::LOW:       return "LOW";
                case LogicLevel::HIGH:      return "HIGH";
                case LogicLevel::UNDEFINED: return "UNDEFINED";
            }
            return "LogicLevel(" + std::to_string(static_cast<int>(value)) + ")";
        }
    };
}
