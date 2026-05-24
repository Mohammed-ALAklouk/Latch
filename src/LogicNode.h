#pragma once
#include "Component.h"
#include "NodeInfo.h"

#define PIN_RADIUS 4 

class LogicNode 
{

	public:
	LogicNode(NodeInfo::Type ComponentType, int x, int y, int id)
		: m_component(ComponentType), rect({ (float)(x / 20 * 20), (float)(y / 20 * 20), 80, 60 }), id(id)
	{
	}

	LogicNode(NodeInfo::Type ComponentType, Vector2 position, int id)
		: LogicNode(ComponentType, position.x, position.y, id)
	{
	}

	void draw(std::vector <LogicLevel> inputs, bool selected, bool highlighted) const;
	int inputPinsContainPoint(Vector2 point) const;

	Vector2 getInputPosition(int i) const {
		float y = rect.y + rect.height / (m_component.m_input_wires.size() + 1) * (i + 1);
		return { rect.x, y };
	}

	bool containsPoint(Vector2 point) const {
		return CheckCollisionPointRec(point, rect);
	}

	Vector2 getOutputPosition() const {
		return { rect.x + rect.width, rect.y + rect.height / 2 };
	}


	int getInputWireId(int pinIndex) const {
		if (pinIndex < 0 || pinIndex >= m_component.m_input_wires.size()) return -1;
		return m_component.m_input_wires[pinIndex];
	}

	bool outputPinContainsPoint(Vector2 point) const {
		return CheckCollisionCircles(point, PIN_RADIUS, getOutputPosition(), PIN_RADIUS);
	}

	Component m_component;
	Rectangle rect;
	Color body_top = { 45, 50, 62, 255 };
	Color body_bot = { 32, 36, 46, 255 };
	Color AccentColor{ 0, 180, 120, 255 };
	Color BorderColor{ 80, 85, 95, 255 };
	Color BorderHighlightColor{ 100, 160, 255, 255 };
	Color BorderSelectedColor{ 0, 150, 255, 255 };
	int id;
};