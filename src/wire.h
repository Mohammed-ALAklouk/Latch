#pragma once
#include "Pin.h"
#include "IdManager.h"

struct WireNode
{
	PinRef Pin;
	Vector2 Position;
	enum NodeType { PIN, JUNCTION } Type;
	int ID;
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
		addNode(source, sourcePos);

		int x_diff = destPos.x - sourcePos.x;
		int y_diff = destPos.y - sourcePos.y;

		if (!x_diff || !y_diff) {
			addNode(destination, destPos);
			Segments.push_back({ 0, 1 });
			return;
		}

		Vector2 junction_pos = { destPos.x, sourcePos.y };
		if (x_diff > y_diff)
			junction_pos = { sourcePos.x, destPos.y };
		
		addNode({-1, -1}, junction_pos);
		addNode(destination, destPos);

		Segments.push_back({ 0, 1 });
		Segments.push_back({ 1, 2 });
	}

	int addNode(PinRef pin, Vector2 pos)
	{
		int new_node_id = node_id_manager.getNextId();
		auto type = pin.ComponentID != -1 ? WireNode::PIN : WireNode::JUNCTION;
		Nodes.push_back({ pin, pos, type, new_node_id });
		return new_node_id;
	}

	int collidesWithNode(Vector2 pos) const
	{
		for (const auto& node : Nodes) {
			float dx = node.Position.x - pos.x;
			float dy = node.Position.y - pos.y;
			if (dx * dx + dy * dy < 100) { // Collision radius of 10
				return node.ID;
			}
		}

		return -1;
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

	Vector2 getNodePosition(int nodeID) const
	{
		for (const auto& node : Nodes) {
			if (node.ID == nodeID) {
				return node.Position;
			}
		}
		return { 0, 0 }; // Default return value if not found
	}

	void extendTo(int sourceNodeID, PinRef pin, Vector2 destPos)
	{
		auto sourcePos = getNodePosition(sourceNodeID);
		int x_diff = destPos.x - sourcePos.x;
		int y_diff = destPos.y - sourcePos.y;

		if (!x_diff || !y_diff) {
			int destID = addNode(pin, destPos);
			Segments.push_back({ sourceNodeID, destID });
			return;
		}

		Vector2 junction_pos = { destPos.x, sourcePos.y };
		if (x_diff > y_diff)
			junction_pos = { sourcePos.x, destPos.y };

		int junctionID = addNode({ -1, -1 }, junction_pos);
		int destID = addNode(pin, destPos);

		Segments.push_back({ sourceNodeID, junctionID });
		Segments.push_back({ junctionID, destID });
	}


	LogicLevel Value;
	int ID;

	std::vector<WireNode> Nodes;
	std::vector<WireSegment> Segments;
	IdManager node_id_manager;
};
