#include "Gate.h"

int Gate::inputPinsContainPoint(Vector2 point) const
{
	for (int i = 0; i < m_inputWireIds.size(); ++i) {
		Vector2 inputPosition = getInputPosition(i);
		if (CheckCollisionCircles(point, PinRadius, inputPosition, PinRadius)) {
			return i;
		}
	}
	return -1;
}

int Gate::outputPinContainsPoint(Vector2 point) const
{
	for (int i = 0; i < m_outputWireIds.size(); ++i) {
		Vector2 outputPosition = getOutputPosition(i);
		if (CheckCollisionCircles(point, PinRadius, outputPosition, PinRadius)) {
			return i;
		}
	}
	return -1;
}

void AndGate::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;

	DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
	DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);

	const char* label = getLabel();
	int text_w = MeasureText(label, 12);
	float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
	float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);

	DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);

	for (int i = 0; i < inputs.size(); i++) {
		Vector2 inputPosition = getInputPosition(i);
		DrawCircle(inputPosition.x, inputPosition.y, PinRadius, LogicLevelColors[inputs[i]]);
		if (inputs[i] == LogicLevel::UNDEFINED) DrawCircle(inputPosition.x, inputPosition.y, PinRadius + 1.5f, Fade(RED, 0.4f));
	}
}

void OrGate::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;
	
	DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
	DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);
	
	const char* label = getLabel();
	int text_w = MeasureText(label, 12);
	float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
	float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);
	
	DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);
	
	for (int i = 0; i < inputs.size(); i++) {
		Vector2 inputPosition = getInputPosition(i);
		DrawCircle(inputPosition.x, inputPosition.y, PinRadius, LogicLevelColors[inputs[i]]);
		if (inputs[i] == LogicLevel::UNDEFINED) DrawCircle(inputPosition.x, inputPosition.y, PinRadius + 1.5f, Fade(RED, 0.4f));
	}
}

void NotGate::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;
	
	DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
	DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);
	
	const char* label = getLabel();
	int text_w = MeasureText(label, 12);
	float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
	float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);
	
	DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);
	
	for (int i = 0; i < inputs.size(); i++) {
		Vector2 inputPosition = getInputPosition(i);
		DrawCircle(inputPosition.x, inputPosition.y, PinRadius, LogicLevelColors[inputs[i]]);
		if (inputs[i] == LogicLevel::UNDEFINED) DrawCircle(inputPosition.x, inputPosition.y, PinRadius + 1.5f, Fade(RED, 0.4f));
	}
}

void HighGate::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;
	
	DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
	DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);
	
	const char* label = getLabel();
	int text_w = MeasureText(label, 12);
	float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
	float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);
	
	DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);
}

void LowGate::draw(std::vector<LogicLevel> inputs, bool selected, bool highlighted) const
{
	Color borderColor = BorderColor;
	if (highlighted) borderColor = BorderHighlightColor;
	if (selected) borderColor = BorderSelectedColor;
	
	DrawRectangleGradientV(m_rect.x, m_rect.y, m_rect.width, m_rect.height, body_top, body_bot);
	DrawRectangle(m_rect.x, m_rect.y, 3.0f, m_rect.height, AccentColor);
	DrawRectangleRoundedLinesEx(m_rect, 0.1f, 4, 1.5f, borderColor);
	
	const char* label = getLabel();
	int text_w = MeasureText(label, 12);
	float tx = m_rect.x + (m_rect.width - text_w) * 0.5f;
	float ty = m_rect.y + (m_rect.height - 12) * 0.5f;
	DrawText(label, (int)tx, (int)ty, 12, WHITE);
	
	DrawCircle(getOutputPosition().x, getOutputPosition().y, PinRadius, LogicLevelColors[m_outputValues[0]]);
}
