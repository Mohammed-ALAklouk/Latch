#pragma once
#include "Pin.h"

struct WireNode
{
	PinRef Pin;
	Vector2 Position;
	enum NodeType { PIN, JUNCTION } Type;
};

struct WireSegment
{
	int StartID;
	int EndID;
};

struct Wire 
{
public:

	Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos)
		: ID(id), Value(LogicLevel::UNDEFINED)
	{
		Nodes.push_back({ source, sourcePos, WireNode::PIN });


		auto type = destination.ComponentID == -1 ? WireNode::PIN : WireNode::JUNCTION;
		int x_diff = destPos.x - sourcePos.x;
		int y_diff = destPos.y - sourcePos.y;

		if (!x_diff || !y_diff) {
			Nodes.push_back({ destination, destPos, WireNode::PIN });
			Segments.push_back({ 0, 1 });
			return;
		}

		Vector2 junction_pos = { destPos.x, sourcePos.y };
		if (x_diff > y_diff)
			junction_pos = { sourcePos.x, destPos.y };
		
		Nodes.push_back({ {-1, -1}, junction_pos, type });
		Nodes.push_back({ destination, destPos, WireNode::PIN });
		

		Segments.push_back({ 0, 1 });
		Segments.push_back({ 1, 2 });
	}


	void draw() {
		Color wire_color = LogicLevelColors[Value];
		
		for (auto& segment: Segments)
		{
			Vector2 start_pos = Nodes[segment.StartID].Position;
			Vector2 end_pos = Nodes[segment.EndID].Position;
			DrawLineEx(start_pos, end_pos, 3, wire_color);
		}

		for (auto& node : Nodes) {
			if (node.Type == WireNode::JUNCTION) {
				DrawCircleV(node.Position, 5, wire_color);
			} else
				{
				DrawRectangleLines(node.Position.x - 6, node.Position.y - 6, 12, 12, WHITE);
			}
		}
	}


	LogicLevel Value;
	int ID;

	std::vector<WireNode> Nodes;
	std::vector<WireSegment> Segments;
};
