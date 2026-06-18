#include "Wire.h"

Wire::Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos)
	: ID(id), Value(LogicLevel::UNDEFINED)
{
	SourceNodeID = addNode(source, sourcePos);
	extendTo(SourceNodeID, destination, destPos);
}

int Wire::addNode(PinRef pin, Vector2 pos)
{
	int new_node_id = node_id_manager.getNextId();
	auto type = pin.ComponentID != -1 ? WireNode::PIN : WireNode::JUNCTION;
	Nodes.push_back({ pin, pos, type, new_node_id });
	node_id_manager.setIndex(new_node_id, Nodes.size() - 1);

	if (SourceNodeID != -1 && type == WireNode::PIN)
		DestinationNodeIDs.push_back(new_node_id);

	return new_node_id;
}

int Wire::findNodeAt(Vector2 pos) const
{
	for (const auto& node : Nodes) {
		float dx = node.Position.x - pos.x;
		float dy = node.Position.y - pos.y;
		if (dx * dx + dy * dy < NodeRadius * NodeRadius) { 
			return node.ID;
		}
	}

	return -1;
}

WireSegment Wire::findSegmentAt(Vector2 pos) const
{
	for (const auto& segment : Segments) {
		Vector2 start_pos = Nodes[node_id_manager.getIndex(segment.first)].Position;
		Vector2 end_pos = Nodes[node_id_manager.getIndex(segment.second)].Position;
		Rectangle rect = getSegmentRect(start_pos, end_pos);

		if (CheckCollisionPointRec(pos, rect)) 
			return segment;
	}

	return { -1, -1 };
}

void Wire::draw(const std::vector<int>& selectedNodeIDs, const std::vector<WireSegment>& selectedSegments) const {
	Color wire_color = LogicLevelColors[Value];

	for (auto& segment : Segments)
	{
		wire_color = LogicLevelColors[Value];
		if (std::find(selectedSegments.begin(), selectedSegments.end(), segment) != selectedSegments.end())
			wire_color = SelectedColor;

		Vector2 start_pos = Nodes[node_id_manager.getIndex(segment.first)].Position;
		Vector2 end_pos = Nodes[node_id_manager.getIndex(segment.second)].Position;
		DrawLineEx(start_pos, end_pos, 3, wire_color);
	}

	for (auto& node : Nodes) {
		bool selected = std::find(selectedNodeIDs.begin(), selectedNodeIDs.end(), node.ID) != selectedNodeIDs.end();
		wire_color = LogicLevelColors[Value];
		if (selected) wire_color = SelectedColor;

		if (node.Type == WireNode::JUNCTION) {
			DrawCircleV(node.Position, NodeRadius, wire_color);
		}
		else {
			DrawRectangleLines(node.Position.x - NodeRadius, node.Position.y - NodeRadius, NodeRadius * 2, NodeRadius * 2, wire_color);
		}
	}
}

Vector2 Wire::getNodePosition(int nodeID) const
{
	if (node_id_manager.getIndex(nodeID) == -1) return { -1, -1 };
	return Nodes[node_id_manager.getIndex(nodeID)].Position;
}

void Wire::extendTo(int sourceNodeID, PinRef pin, Vector2 destPos)
{
	auto pts = routePoints(getNodePosition(sourceNodeID), destPos);

	int prevID = sourceNodeID;                 // pts[0] is the existing source
	for (size_t i = 1; i < pts.size(); ++i) {
		bool last = (i == pts.size() - 1);
		int id = last ? addNode(pin, pts[i])           // endpoint is the pin
			: addNode({ -1, -1 }, pts[i]);    // intermediates are junctions
		Segments.push_back({ prevID, id });
		prevID = id;
	}
}

std::vector<Vector2> Wire::routePoints(Vector2 from, Vector2 to)
{
	int dx = to.x - from.x;
	int dy = to.y - from.y;

	if (dx == 0 || dy == 0)
		return { from, to };

	Vector2 elbow = (dx > dy) ? Vector2{ from.x, to.y }
	: Vector2{ to.x, from.y };
	return { from, elbow, to };
}

// removes the node and everything connected to it
std::vector<WireNode> Wire::removeNodes(const std::vector<int>& nodeIDs)
{
	if (nodeIDs.empty()) return {};

	for (int id : nodeIDs) {
		if (id == SourceNodeID) {
			std::vector<WireNode> removedPinNodes = { Nodes[node_id_manager.getIndex(id)] };
			for (int id : DestinationNodeIDs)
				removedPinNodes.push_back(Nodes[node_id_manager.getIndex(id)]);

			Nodes.clear();
			node_id_manager.clear();
			return removedPinNodes;
		}
	}

	std::unordered_set<int> ids_to_remove(nodeIDs.begin(), nodeIDs.end());

	for (auto seg = Segments.begin(); seg != Segments.end();) {
		if (ids_to_remove.count(seg->first) || ids_to_remove.count(seg->second))
			seg = Segments.erase(seg);
		else
			seg++;
	}

	auto removedPinNodes = pruneOrphens();
	
	return removedPinNodes;
}

std::vector<WireNode> Wire::removeSegments(const std::vector<WireSegment>& segments)
{
	for (auto& seg : segments) {
		auto it = std::find(Segments.begin(), Segments.end(), seg);
		if (it != Segments.end())
			Segments.erase(it);
	}

	return pruneOrphens();
}

std::vector<WireNode> Wire::pruneOrphens()
{
	std::unordered_set<int> visited;
	std::stack<int> nodes_to_check;

	visited.insert(Nodes[0].ID);
	nodes_to_check.push(Nodes[0].ID);

	while (nodes_to_check.size()) {
		int nodeID = nodes_to_check.top();
		nodes_to_check.pop();

		for (auto& seg : Segments) {
			int neighbor = -1;
			if (seg.first == nodeID)    neighbor = seg.second;
			else if (seg.second == nodeID) neighbor = seg.first;
			else continue;
			if (visited.insert(neighbor).second)
				nodes_to_check.push(neighbor);
		}
	}

	for (auto seg = Segments.begin(); seg != Segments.end();) {
		if (!visited.count(seg->first) || !visited.count(seg->second))
			seg = Segments.erase(seg);
		else
			seg++;
	}

	std::vector<WireNode> removedPinNodes;
	std::vector<int> toRemove;
	for (auto& n : Nodes)
		if (visited.count(n.ID) == 0)
			toRemove.push_back(n.ID);

	for (int rid : toRemove) {
		auto& node = Nodes[node_id_manager.getIndex(rid)];
		if (node.Type == WireNode::PIN)
			removedPinNodes.push_back(node);

		eraseNode(rid);
	}

	if (Segments.empty()) {
		for (auto& node : Nodes)
			removedPinNodes.push_back(node);

		Nodes.clear();
		node_id_manager.clear();
	}

	return removedPinNodes;
}

// removes only the given node
void Wire::eraseNode(int id) {
	int index = node_id_manager.getIndex(id);
	if (index == -1) return;

	if (Nodes[index].Type == WireNode::PIN)
		DestinationNodeIDs.erase(std::remove(DestinationNodeIDs.begin(), DestinationNodeIDs.end(), id), DestinationNodeIDs.end());

	node_id_manager.releaseId(id);

	int lastIndex = Nodes.size() - 1;
	if (index != lastIndex) {
		Nodes[index] = Nodes[lastIndex];
		node_id_manager.setIndex(Nodes[index].ID, index);
	}
	Nodes.pop_back();
}

Color Wire::SelectedColor = { 0, 150, 255, 255 };
float Wire::NodeRadius = 5.0f;
float Wire::SegmentThickness = 3.0f;