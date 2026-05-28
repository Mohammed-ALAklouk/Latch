#pragma once
#include <vector>
#include "Pin.h"
#include "NodeInfo.h"


class Gate {
public:
	Gate() = delete;
	Gate(int id, Vector2 position)
		: m_id(id), m_rect({ position.x, position.y, 0, 0 })	{	}
	virtual ~Gate() = default;

	bool containsPoint(Vector2 point) const {
		return CheckCollisionPointRec(point, m_rect);
	}

	int getInputWireId(int input_index) const {
		if (input_index < 0 || input_index >= m_inputWireIds.size()) return -1;
		return m_inputWireIds[input_index];
	}

	virtual int inputPinsContainPoint(Vector2 point) const;
	virtual int outputPinContainsPoint(Vector2 point) const;

	virtual void evaluate(const std::vector<LogicLevel>& inputs) = 0;
	virtual void draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const;
	virtual char* getLabel() const = 0;
	virtual NodeInfo::Type getNodeInfoType() const = 0;
	virtual Vector2 getInputPosition(int input_index) const = 0;
	virtual Vector2 getOutputPosition(int output_index = 0) const = 0;
	virtual void onClick() {}


	constexpr static float PinRadius = 5.0f;
	constexpr static Color LogicLevelColors[] = {
		{60, 60, 60, 255}, // LOW
		{0, 220, 80, 255}, // HIGH
		{255, 150, 0, 255} // UNDEFINED
	};

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

	AndGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 80;
		m_rect.height = 60;
		m_inputWireIds.resize(2, -1);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::UNDEFINED);
	}

	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}

		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}
	
	char* getLabel() const override { return "AND"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::AND; }

	Vector2 getInputPosition(int input_index) const override
	{
		float y = m_rect.y + m_rect.height / (m_inputWireIds.size() + 1) * (input_index + 1);
		return { m_rect.x, y };
	}

	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width, m_rect.y + m_rect.height / 2 };
	}
};

class OrGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[9] = {
		//		LOW					HIGH				UNDEFINED
		LogicLevel::LOW,		LogicLevel::HIGH,	LogicLevel::UNDEFINED, // LOW
		LogicLevel::HIGH,		LogicLevel::HIGH,	LogicLevel::HIGH, // HIGH
		LogicLevel::UNDEFINED,	LogicLevel::HIGH,	LogicLevel::UNDEFINED // UNDEFINED
	};

	OrGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 60;
		m_rect.height = 40;
		m_inputWireIds.resize(2, -1);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::UNDEFINED);
	}
	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 2) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0] * 3 + inputs[1]];
	}


	char* getLabel() const override { return "OR"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::OR; }
	Vector2 getInputPosition(int input_index) const override
	{
		float spacing = m_rect.height / (m_inputWireIds.size() + 1);
		return { m_rect.x - PinRadius * 2, m_rect.y + spacing * (input_index + 1) };
	}
	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width + PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
};

class NotGate : public Gate {
	public:
	constexpr static LogicLevel LookupTable[3] = {
		LogicLevel::HIGH, // LOW
		LogicLevel::LOW,  // HIGH
		LogicLevel::UNDEFINED // UNDEFINED
	};
	NotGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 80;
		m_rect.height = 60;
		m_inputWireIds.resize(1, -1);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::UNDEFINED);
	}
	void evaluate(const std::vector<LogicLevel>& inputs) override{
		if (inputs.size() < 1) {
			m_outputValues[0] = LogicLevel::UNDEFINED;
			return;
		}
		m_outputValues[0] = LookupTable[inputs[0]];
	}


	char* getLabel() const override { return "NOT"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::NOT; }
	Vector2 getInputPosition(int input_index) const override
	{
		return { m_rect.x - PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width + PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
};

class LedGate : public Gate {
public:
	LedGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 80;
		m_rect.height = 60;
		m_inputWireIds.resize(1, -1);
		m_outputWireIds.resize(0);
		m_outputValues.resize(0);
	}

	void draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const override
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
	char* getLabel() const override { return "LED"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::LED; }
	Vector2 getInputPosition(int input_index) const override
	{
		return { m_rect.x - PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
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

class HighGate : public Gate {
public:
	constexpr static LogicLevel LookupTable[3] = {
		LogicLevel::HIGH, // LOW
		LogicLevel::HIGH,  // HIGH
		LogicLevel::HIGH // UNDEFINED
	};
	HighGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 60;
		m_rect.height = 40;
		m_inputWireIds.resize(0);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::HIGH);
	}
	void evaluate(const std::vector<LogicLevel>& inputs) override{
		m_outputValues[0] = LogicLevel::HIGH;
	}


	char* getLabel() const override { return "HIGH"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::HIGH; }
	Vector2 getInputPosition(int input_index) const override
	{
		return { 0, 0 }; // No input pins
	}
	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width + PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}

	int inputPinsContainPoint(Vector2 point) const override {	return false;	}
};

class LowGate : public Gate {
public: 
	constexpr static LogicLevel LookupTable[3] = {
		LogicLevel::LOW, // LOW
		LogicLevel::LOW,  // HIGH
		LogicLevel::LOW // UNDEFINED
	};
	LowGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 60;
		m_rect.height = 40;
		m_inputWireIds.resize(0);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::LOW);
	}
	void evaluate(const std::vector<LogicLevel>& inputs) override{
		m_outputValues[0] = LogicLevel::LOW;
	}

	char* getLabel() const override { return "LOW"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::LOW; }
	Vector2 getInputPosition(int input_index) const override
	{
		return { 0, 0 }; // No input pins
	}
	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width + PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
	int inputPinsContainPoint(Vector2 point) const override { return false; }

};

class ToggleGate : public Gate {
	public:
	ToggleGate(int id, Vector2 position) : Gate(id, position)
	{
		m_rect.width = 80;
		m_rect.height = 60;
		m_inputWireIds.resize(0);
		m_outputWireIds.resize(1, -1);
		m_outputValues.resize(1, LogicLevel::LOW);
	}
	void evaluate(const std::vector<LogicLevel>& inputs) override {
	}

	char* getLabel() const override { return "TOGGLE"; }
	NodeInfo::Type getNodeInfoType() const override { return NodeInfo::Type::TOGGLE; }
	Vector2 getInputPosition(int input_index) const override
	{
		return { 0, 0 }; // No input pins
	}

	Vector2 getOutputPosition(int output_index = 0) const override
	{
		return { m_rect.x + m_rect.width + PinRadius * 2, m_rect.y + m_rect.height / 2 };
	}
	void onClick() override {
		m_outputValues[0] = (m_outputValues[0] == LogicLevel::HIGH) ? LogicLevel::LOW : LogicLevel::HIGH;
	}

	void draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const override
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