#include "LogicNode.h"

void LogicNode::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;

	DrawRectangleGradientV(rect.x, rect.y, rect.width, rect.height, body_top, body_bot);
	DrawRectangle(rect.x, rect.y, 3.0f, rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(rect, 0.1f, 4, 1.5f , borderColor);

	
	
	const char* label = m_component.m_name.c_str();
	int text_w = MeasureText(label, 12);
	float tx = rect.x + (rect.width - text_w) * 0.5f;
	float ty = rect.y + (rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);

	DrawCircle(getOutputPosition().x, getOutputPosition().y, PIN_RADIUS, LogicLevelColors[m_component.m_output_pin.value]);
	
	for (int i = 0; i < inputs.size(); i++) {
		Vector2 inputPosition = getInputPosition(i);
		DrawCircle(inputPosition.x, inputPosition.y, PIN_RADIUS, LogicLevelColors[inputs[i]]);
		if (inputs[i] == LogicLevel::UNDEFINED) DrawCircle(inputPosition.x, inputPosition.y, PIN_RADIUS + 1.5f, Fade(RED, 0.4f));
	}
}

int LogicNode::inputPinsContainPoint(Vector2 point) const
{
	for (int i = 0; i < m_component.m_input_wires.size(); ++i) {
		Vector2 inputPosition = getInputPosition(i);
		if (CheckCollisionCircles(point, PIN_RADIUS, inputPosition, PIN_RADIUS)) {
			return i;
		}
	}
	return -1;
}
