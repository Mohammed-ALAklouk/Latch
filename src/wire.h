#pragma once
#include "Pin.h"
#include "IdManager.h"

#include <raylib.h>

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
		int sourceNodeID = addNode(source, sourcePos);

		extendTo(sourceNodeID, destination, destPos);
	}

	int addNode(PinRef pin, Vector2 pos)
	{
		int new_node_id = node_id_manager.getNextId();
		auto type = pin.ComponentID != -1 ? WireNode::PIN : WireNode::JUNCTION;
		Nodes.push_back({ pin, pos, type, new_node_id });
		node_id_manager.setIndex(new_node_id, Nodes.size() - 1);
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


	void draw(std::vector<int>& selectedNodeIDs) {
		Color wire_color = LogicLevelColors[Value];
		Color selected_color = { 0, 150, 255, 255 };
		
		for (auto& segment: Segments)
		{
			Vector2 start_pos = Nodes[node_id_manager.getIndex(segment.StartID)].Position;
			Vector2 end_pos = Nodes[node_id_manager.getIndex(segment.EndID)].Position;
			DrawLineEx(start_pos, end_pos, 3, wire_color);
		}

		for (auto& node : Nodes) {
			if (node.Type == WireNode::JUNCTION) {
				bool selected = std::find(selectedNodeIDs.begin(), selectedNodeIDs.end(), node.ID) != selectedNodeIDs.end();
				wire_color = LogicLevelColors[Value];
				if (selected) wire_color = selected_color;
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

	static WireNode getExtentionResults(Vector2 sourcePos, Vector2 destPos)
	{
		int x_diff = destPos.x - sourcePos.x;
		int y_diff = destPos.y - sourcePos.y;
		if (!x_diff || !y_diff) {
			return { { -1, -1 }, destPos, WireNode::PIN, -1 };
		}
		
		if (x_diff > y_diff)
			return { { -1, -1 }, { sourcePos.x, destPos.y }, WireNode::JUNCTION, 1 };

		return { { -1, -1 }, { destPos.x, sourcePos.y }, WireNode::JUNCTION, 1 };
	}

	// removes the node and everything connected to it
	void removeNode(int id)
	{
		if (node_id_manager.getIndex(id) == -1) return;

		deleteNode(id);
		for (auto seg = Segments.begin(); seg != Segments.end();) {
			if (seg->StartID == id || seg->EndID == id)
				Segments.erase(seg);
			else
				seg++;
		}
		if (Nodes.size() == 0) return;

		std::vector<int> visitedNodes;
		
		std::vector<int> curr_nodes_to_check = { Nodes[0].ID };
		std::vector<int> next_nodes_to_check;

		while (curr_nodes_to_check.size()) {
			int nodeID = curr_nodes_to_check[0];

			for (auto& seg: Segments) {
				if (seg.StartID == nodeID) {
					bool is_visited = std::find(visitedNodes.begin(), visitedNodes.end(), seg.EndID) != visitedNodes.end();
					if (!is_visited) next_nodes_to_check.push_back(seg.EndID);
				}
				else if (seg.EndID == nodeID) {
					bool is_visited = std::find(visitedNodes.begin(), visitedNodes.end(), seg.StartID) != visitedNodes.end();
					if (!is_visited) next_nodes_to_check.push_back(seg.StartID);
				}
			}

			visitedNodes.push_back(nodeID);
			curr_nodes_to_check.erase(curr_nodes_to_check.begin());
			if (curr_nodes_to_check.size() == 0) {
				curr_nodes_to_check = next_nodes_to_check;
				next_nodes_to_check.clear();
			}

		}

		for (auto& node = Nodes.begin(); node != Nodes.end();) {
			bool should_delete = std::find(visitedNodes.begin(), visitedNodes.end(), node->ID) == visitedNodes.end();
			if (should_delete)
				Nodes.erase(node);
			else
				node++;
		}

		for (auto& seg = Segments.begin(); seg != Segments.end();) {
			bool startVisited = std::find(visitedNodes.begin(), visitedNodes.end(), seg->StartID) != visitedNodes.end();
			bool endVisited = std::find(visitedNodes.begin(), visitedNodes.end(), seg->EndID) != visitedNodes.end();

			if (!startVisited && !endVisited)
				Segments.erase(seg);
			else
				seg++;
		}

		if (Segments.size() == 0) Nodes.clear();
	}

	// removes only the given node
	void deleteNode(int id) {
		int index = node_id_manager.getIndex(id);
		if (index == -1) return;

		Nodes[index] = *(Nodes.end() - 1);
		Nodes.pop_back();
		node_id_manager.releaseId(id);
		node_id_manager.setIndex(Nodes[index].ID, index);
	}


	LogicLevel Value;
	int ID;

	std::vector<WireNode> Nodes;
	std::vector<WireSegment> Segments;
	IdManager node_id_manager;
};
