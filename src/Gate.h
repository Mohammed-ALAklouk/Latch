#pragma once
#include <vector>
#include <memory>
#include "Pin.h"
#include "ComponentInfo.h"

#define CELL_SIZE 10.0f

class Gate {
public:
	Gate() = delete;
	Gate(int id, Vector2 position, int numInputs, int numOutputs, float w)
		: m_id(id), m_rect({ position.x, position.y, w, (numInputs + 2) * CELL_SIZE })
	{
		m_inputWireIds.resize(numInputs, -1);
		m_outputWireIds.resize(numOutputs, -1);
		m_outputValues.resize(numOutputs, LogicLevel::UNDEFINED);
	}

	virtual ~Gate() = default;

	bool containsPoint(Vector2 point) const {
		return CheckCollisionPointRec(point, m_rect);
	}

	int getInputWireId(int input_index) const {
		if (input_index < 0 || input_index >= m_inputWireIds.size()) return -1;
		return m_inputWireIds[input_index];
	}

	componentInfo getComponentInfo() const {
		return { getNodeInfoType(), m_id, { m_rect.x, m_rect.y }, m_inputWireIds, m_outputWireIds };
	}

	virtual int inputPinsContainPoint(Vector2 point) const;
	virtual int outputPinContainsPoint(Vector2 point) const;

	virtual void evaluate(const std::vector<LogicLevel>& inputs) = 0;
	virtual void draw(std::vector<LogicLevel>& inputs, bool selected, bool highlighted) const;
	virtual const char* getLabel() const = 0;
	virtual componentInfo::Type getNodeInfoType() const = 0;
	virtual void onClick() {}

	virtual Vector2 getInputPosition(int input_index) const
	{
		return { m_rect.x, m_rect.y + CELL_SIZE + 2 * CELL_SIZE * (input_index) };
	}

	virtual Vector2 getOutputPosition(int output_index = 0) const {
		return { m_rect.x + m_rect.width, m_rect.y + m_rect.height / 2 };
	}


	constexpr static float PinRadius = 5.0f;

	Rectangle m_rect;
	Color body_top = { 45, 50, 62, 255 };
	Color body_bot = { 32, 36, 46, 255 };
	Color AccentColor{ 0, 180, 120, 255 };
	Color BorderColor{ 80, 85, 95, 255 };
	Color BorderHighlightColor{ 100, 160, 255, 255 };
	Color BorderSelectedColor{ 0, 150, 255, 255 };

	std::vector<int> m_inputWireIds;
	std::vector<int> m_outputWireIds;
	std::vector<LogicLevel> m_outputValues;
	int m_id;
};

class AndGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW				HIGH				UNDEFINED
		LogicLevel::LOW, LogicLevel::LOW,		LogicLevel::LOW, // LOW
		LogicLevel::LOW, LogicLevel::HIGH,		LogicLevel::UNDEFINED, // HIGH
		LogicLevel::LOW, LogicLevel::UNDEFINED,	LogicLevel::UNDEFINED // UNDEFINED
	};

	AndGate(int id, Vector2 position) : Gate(id, position, 2, 1, 4 * CELL_SIZE)	{	}

	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}

		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	
	const char* getLabel() const override { return "AND"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::AND; }
};

class OrGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW					HIGH				UNDEFINED
		LogicLevel::LOW,		LogicLevel::HIGH,	LogicLevel::UNDEFINED, // LOW
		LogicLevel::HIGH,		LogicLevel::HIGH,	LogicLevel::HIGH, // HIGH
		LogicLevel::UNDEFINED,	LogicLevel::HIGH,	LogicLevel::UNDEFINED // UNDEFINED
	};

	OrGate(int id, Vector2 position) : Gate(id, position, 2, 1,  4 * CELL_SIZE)	{	}

	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}


	const char* getLabel() const override { return "OR"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::OR; }
};

class NotGate : public Gate {
	public:
	constexpr static LogicLevel LookupTable[3] = {
		LogicLevel::HIGH, // LOW
		LogicLevel::LOW,  // HIGH
		LogicLevel::UNDEFINED // UNDEFINED
	};
	NotGate(int id, Vector2 position) : Gate(id, position, 1, 1, 4 * CELL_SIZE)	{	}

	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 1) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0]];
	}


	const char* getLabel() const override { return "NOT"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::NOT; }
};

class LedGate : public Gate {
public:
	LedGate(int id, Vector2 position) : Gate(id, position, 1, 0, 4 * CELL_SIZE) {
		m_rect.height =  4 * CELL_SIZE;
	}

	void draw(std::vector<LogicLevel>& inputs, bool selected, bool highlighted) const override
	{
		auto borderColor = BorderColor;
		if (highlighted) borderColor = BorderHighlightColor;
		if (selected) borderColor = BorderSelectedColor;

		DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
		DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
		DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);

		Color fillColor = LogicLevelColors[inputs[0]];
		DrawCircle(getInputPosition(0).x, getInputPosition(0).y, PinRadius, fillColor);
		if (inputs[0] == LogicLevel::UNDEFINED) fillColor = Fade(RED, 0.4f);

		DrawCircle(m_rect.x + m_rect.width / 2, m_rect.y + m_rect.height / 2, 20, fillColor);
		DrawCircleLines(m_rect.x + m_rect.width / 2, m_rect.y + m_rect.height / 2, 20, borderColor);
	}

	void evaluate(const std::vector<LogicLevel>& inputs) override{
	}
	const char* getLabel() const override { return "LED"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::LED; }
	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { 0, 0 }; // No output pins
	}
	int inputPinsContainPoint(Vector2 point) const override
	{
		Vector2 inputPosition = getInputPosition(0);
		if (CheckCollisionCircles(point, PinRadius, inputPosition, PinRadius)) {
			return 0;
		}
		return -1;
	}

};

class ToggleGate : public Gate {
	public:
	ToggleGate(int id, Vector2 position) : Gate(id, position, 0, 1, 4 * CELL_SIZE)
	{
		m_rect.height =  4 * CELL_SIZE;
		m_outputValues[0] = LogicLevel::LOW;
	}

	void evaluate(const std::vector<LogicLevel>& inputs) override {
	}

	const char* getLabel() const override { return "TOGGLE"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::TOGGLE; }
	
	void onClick() override {
		m_outputValues[0] = (m_outputValues[0] == LogicLevel::HIGH) ? LogicLevel::LOW : LogicLevel::HIGH;
	}

	void draw(std::vector<LogicLevel>& inputs, bool selected, bool highlighted) const override
	{
		auto borderColor = BorderColor;
		if (highlighted) borderColor = BorderHighlightColor;
		if (selected) borderColor = BorderSelectedColor;
		DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
		DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
		DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);
		
		Color fillColor = LogicLevelColors[m_outputValues[0]];
		DrawCircle(m_rect.x + m_rect.width / 2, m_rect.y + m_rect.height / 2, 20, fillColor);
		DrawCircleLines(m_rect.x + m_rect.width / 2, m_rect.y + m_rect.height / 2, 20, borderColor);
		
		const char* label = m_outputValues[0] == LogicLevel::HIGH ? "1" : (m_outputValues[0] == LogicLevel::LOW ? "0" : "");
		int text_w = MeasureText(label, 12);
		float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
		float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
		DrawText(label, (int)tx, (int)ty, 12, WHITE);

		DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);
	}
};

class NandGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW				HIGH				UNDEFINED
		LogicLevel::HIGH, LogicLevel::HIGH,	LogicLevel::UNDEFINED, // LOW
		LogicLevel::HIGH, LogicLevel::LOW,  LogicLevel::UNDEFINED, // HIGH
		LogicLevel::UNDEFINED, LogicLevel::UNDEFINED, LogicLevel::UNDEFINED // UNDEFINED
	};
	NandGate(int id, Vector2 position) : Gate(id, position, 2, 1, 4 * CELL_SIZE) { }
	void evaluate(const std::vector<LogicLevel>& inputs) override {
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	const char* getLabel() const override { return "NAND"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::NAND; } 
};

class NorGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW					HIGH				UNDEFINED
		LogicLevel::HIGH,		LogicLevel::LOW, LogicLevel::UNDEFINED, // LOW
		LogicLevel::LOW,		LogicLevel::LOW, LogicLevel::LOW, // HIGH
		LogicLevel::UNDEFINED,	LogicLevel::LOW, LogicLevel::UNDEFINED // UNDEFINED
	};
	NorGate(int id, Vector2 position) : Gate(id, position, 2, 1,  4 * CELL_SIZE) { }
	void evaluate(const std::vector<LogicLevel>& inputs) override {
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	const char* getLabel() const override { return "NOR"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::NOR; } 
};

class XorGate : public Gate {
	public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW					HIGH				UNDEFINED
		LogicLevel::LOW,	LogicLevel::HIGH,	LogicLevel::UNDEFINED, // LOW
		LogicLevel::HIGH,	LogicLevel::LOW,	LogicLevel::UNDEFINED, // HIGH
		LogicLevel::UNDEFINED, LogicLevel::UNDEFINED, LogicLevel::UNDEFINED // UNDEFINED
	};
	XorGate(int id, Vector2 position) : Gate(id, position, 2, 1,  4 * CELL_SIZE) { }
	void evaluate(const std::vector<LogicLevel>& inputs) override {
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	const char* getLabel() const override { return "XOR"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::XOR; } 
};

class XnorGate : public Gate {

	public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW					HIGH				UNDEFINED
		LogicLevel::HIGH,	LogicLevel::LOW,	LogicLevel::UNDEFINED, // LOW
		LogicLevel::LOW,	LogicLevel::HIGH,	LogicLevel::UNDEFINED, // HIGH
		LogicLevel::UNDEFINED, LogicLevel::UNDEFINED, LogicLevel::UNDEFINED // UNDEFINED
	};
	XnorGate(int id, Vector2 position) : Gate(id, position, 2, 1,  4 * CELL_SIZE) { }
	void evaluate(const std::vector<LogicLevel>& inputs) override {
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	const char* getLabel() const override { return "XNOR"; }
	componentInfo::Type getNodeInfoType() const override { return componentInfo::Type::XNOR; } 
};




using GateFactory = std::unique_ptr<Gate>(*)(int id, Vector2 position);

template<typename T>
std::unique_ptr<Gate> makeGate(int id, Vector2 position) {
	return std::make_unique<T>(id, position);
}

inline constexpr GateFactory GateFactories[componentInfo::COMPONENT_COUNT] = {
	makeGate<AndGate>,     // AND
	makeGate<NandGate>,    // NAND
	makeGate<OrGate>,      // OR
	makeGate<NorGate>,     // NOR
	makeGate<XorGate>,     // XOR
	makeGate<XnorGate>,    // XNOR
	makeGate<NotGate>,     // NOT
	makeGate<LedGate>,     // LED
	makeGate<ToggleGate>,  // TOGGLE
};