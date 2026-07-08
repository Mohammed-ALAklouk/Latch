#include "Wire.h"

Wire::Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos, Elbow first)
	: ID(id), Value(LogicLevel::UNDEFINED)
{
	SourceNodeID = addNode(source, sourcePos);
	extendTo(SourceNodeID, destination, destPos, first);
}

// ===========================================================================
//  Read-only accessors
// ===========================================================================

Vector2 Wire::positionOf(int id) const
{
	int idx = node_id_manager.getIndex(id);
	if (idx == -1) return { -1, -1 };
	return Nodes[idx].Position;
}

int Wire::degree(int id) const
{
	auto it = Adjacency.find(id);
	return it == Adjacency.end() ? 0 : (int)it->second.size();
}

std::vector<int> Wire::neighborsOf(int id) const
{
	auto it = Adjacency.find(id);
	return it == Adjacency.end() ? std::vector<int>{} : it->second;
}

// Are a, b, c on a single axis-aligned line (all same x, or all same y)?
bool Wire::collinear(int a, int b, int c) const
{
	Vector2 pa = positionOf(a), pb = positionOf(b), pc = positionOf(c);
	return (pa.x == pb.x && pb.x == pc.x) || (pa.y == pb.y && pb.y == pc.y);
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
		if (segmentContainsPoint(segment, pos))
			return segment;
	}

	return { -1, -1 };
}

Vector2 Wire::getNodePosition(int nodeID) const
{
	return positionOf(nodeID);
}

// ===========================================================================
//  Structural primitives (the only functions that mutate the graph)
// ===========================================================================

// Get-or-create a vertex at pos. Does NOT touch topology; splitting a segment
// that this node happens to land on is normalize()'s job, not addNode's.
int Wire::addNode(PinRef pin, Vector2 pos)
{
	int existing = findNodeAt(pos);
	if (existing != -1) return existing;

	int id = node_id_manager.getNextId();
	auto type = pin.ComponentID != -1 ? WireNode::PIN : WireNode::JUNCTION;
	Nodes.push_back({ pin, pos, type, id });
	node_id_manager.setIndex(id, (int)Nodes.size() - 1);
	Adjacency[id] = {};

	if (SourceNodeID != -1 && type == WireNode::PIN)
		DestinationNodeIDs.push_back(id);

	return id;
}

void Wire::removeNode(int id)
{
	int index = node_id_manager.getIndex(id);
	if (index == -1) return;

	// drop every edge touching this node (keeps neighbours' adjacency in sync)
	for (int neighbor : neighborsOf(id))
		removeSegment(id, neighbor);
	Adjacency.erase(id);

	if (Nodes[index].Type == WireNode::PIN)
		DestinationNodeIDs.erase(std::remove(DestinationNodeIDs.begin(), DestinationNodeIDs.end(), id), DestinationNodeIDs.end());

	node_id_manager.releaseId(id);

	int lastIndex = (int)Nodes.size() - 1;
	if (index != lastIndex) {
		Nodes[index] = Nodes[lastIndex];
		node_id_manager.setIndex(Nodes[index].ID, index);
	}
	Nodes.pop_back();
}

void Wire::addSegment(int a, int b)
{
	if (a == b || a == -1 || b == -1) return;

	for (auto& s : Segments)
		if ((s.first == a && s.second == b) || (s.first == b && s.second == a))
			return; // already connected

	Segments.push_back({ a, b });
	Adjacency[a].push_back(b);
	Adjacency[b].push_back(a);
}

void Wire::removeSegment(int a, int b)
{
	Segments.erase(std::remove_if(Segments.begin(), Segments.end(),
		[&](const WireSegment& s) {
			return (s.first == a && s.second == b) || (s.first == b && s.second == a);
		}), Segments.end());

	auto drop = [](std::vector<int>& v, int x) {
		v.erase(std::remove(v.begin(), v.end(), x), v.end());
	};
	auto ia = Adjacency.find(a); if (ia != Adjacency.end()) drop(ia->second, b);
	auto ib = Adjacency.find(b); if (ib != Adjacency.end()) drop(ib->second, a);
}

// ===========================================================================
//  Extending (position-facing; callers never name an internal id)
// ===========================================================================

void Wire::extendTo(int sourceNodeID, PinRef pin, Vector2 destPos, Elbow first)
{
	Vector2 sourcePos = positionOf(sourceNodeID);
	auto pts = routePoints(sourcePos, destPos, first);

	int prev = sourceNodeID;
	for (size_t i = 1; i < pts.size(); ++i) {
		PinRef p = (i == pts.size() - 1) ? pin : PinRef{ -1, -1 };
		int cur = addNode(p, pts[i]);
		addSegment(prev, cur);
		prev = cur;
	}

	normalize();
}

void Wire::extendTo(WireSegment segment, Vector2 source, PinRef targetPin, Vector2 dest, Elbow first)
{
	if (segmentContainsPoint(segment, dest)) return;

	// Drop a node on the segment body; normalize()'s split pass wires it into
	// the segment, then we branch off it toward the destination.
	int sourceNodeID = addNode({ -1, -1 }, source);
	extendTo(sourceNodeID, targetPin, dest, first);
}

std::vector<Vector2> Wire::routePoints(Vector2 from, Vector2 to, Elbow first)
{
	if (from.x == to.x || from.y == to.y)
		return { from, to }; // already straight; no elbow needed

	// VerticalFirst: leave the source vertically, so the leg touching the
	// destination is horizontal (elbow shares the source's x, the dest's y).
	Vector2 elbow = (first == Elbow::VerticalFirst) ? Vector2{ from.x, to.y }
	                                                : Vector2{ to.x, from.y };
	return { from, elbow, to };
}

// ===========================================================================
//  Removal (public)
// ===========================================================================

std::vector<WireNode> Wire::removeNodes(const std::vector<int>& nodeIDs)
{
	if (nodeIDs.empty()) return {};

	// Removing the source pin tears the whole wire down.
	for (int id : nodeIDs)
		if (id == SourceNodeID)
			return clearAll();

	std::vector<WireNode> removedPins;
	for (int id : nodeIDs) {
		int idx = node_id_manager.getIndex(id);
		if (idx != -1 && Nodes[idx].Type == WireNode::PIN)
			removedPins.push_back(Nodes[idx]);
		removeNode(id);
	}

	auto pruned = normalize();
	removedPins.insert(removedPins.end(), pruned.begin(), pruned.end());
	return removedPins;
}

std::vector<WireNode> Wire::removeSegments(const std::vector<WireSegment>& segments)
{
	for (auto& seg : segments)
		removeSegment(seg.first, seg.second);

	return normalize();
}

// ===========================================================================
//  normalize(): restore every geometric invariant after an edit
// ===========================================================================

std::vector<WireNode> Wire::normalize()
{
	splitSegmentsAtNodes();
	removeRedundantNodes();
	return pruneOrphans();
}

// Any node sitting strictly inside a segment's body means that segment should
// actually pass *through* it. Rebuild each segment as a chain through the nodes
// on its span. This is also what resolves overlaps: an overlapping segment's
// endpoint lands inside its neighbour, so both get subdivided at shared points,
// and addSegment() dedupes the pieces they share.
void Wire::splitSegmentsAtNodes()
{
	std::vector<WireSegment> toRemove;
	std::vector<WireSegment> toAdd;

	for (auto& seg : Segments) {
		Vector2 a = positionOf(seg.first);
		Vector2 b = positionOf(seg.second);
		bool horizontal = (a.y == b.y);
		bool vertical = (a.x == b.x);
		if (!horizontal && !vertical) continue;

		std::vector<int> chain = { seg.first, seg.second };
		for (auto& n : Nodes) {
			if (n.ID == seg.first || n.ID == seg.second) continue;
			Vector2 p = n.Position;
			bool inside = horizontal
				? (p.y == a.y && p.x > std::min(a.x, b.x) && p.x < std::max(a.x, b.x))
				: (p.x == a.x && p.y > std::min(a.y, b.y) && p.y < std::max(a.y, b.y));
			if (inside) chain.push_back(n.ID);
		}

		if (chain.size() == 2) continue; // nothing lands on this segment

		std::sort(chain.begin(), chain.end(), [&](int l, int r) {
			return horizontal ? positionOf(l).x < positionOf(r).x
							   : positionOf(l).y < positionOf(r).y;
		});

		toRemove.push_back(seg);
		for (size_t i = 1; i < chain.size(); ++i)
			toAdd.push_back({ chain[i - 1], chain[i] });
	}

	for (auto& s : toRemove) removeSegment(s.first, s.second);
	for (auto& s : toAdd)    addSegment(s.first, s.second);
}

// A junction whose only role is to sit mid-line between two collinear
// neighbours carries no information -- drop it and join its neighbours. Pins
// and real corners/branches (degree != 2, or non-collinear) are kept.
void Wire::removeRedundantNodes()
{
	bool changed = true;
	while (changed) {
		changed = false;
		for (auto& node : Nodes) {
			int id = node.ID;
			if (node.Type == WireNode::PIN) continue;
			if (degree(id) != 2) continue;

			auto nb = neighborsOf(id);
			if (!collinear(nb[0], id, nb[1])) continue;

			int a = nb[0], c = nb[1];
			removeNode(id); // also clears its two incident edges
			addSegment(a, c);
			changed = true;
			break; // Nodes was mutated (swap-pop); restart the scan
		}
	}
}

std::vector<WireNode> Wire::pruneOrphans()
{
	std::vector<WireNode> removedPins;
	if (Nodes.empty()) return removedPins;

	int root = (node_id_manager.getIndex(SourceNodeID) != -1) ? SourceNodeID : Nodes[0].ID;

	std::unordered_set<int> visited;
	std::stack<int> toVisit;
	visited.insert(root);
	toVisit.push(root);

	while (!toVisit.empty()) {
		int cur = toVisit.top();
		toVisit.pop();
		for (int neighbor : neighborsOf(cur))
			if (visited.insert(neighbor).second)
				toVisit.push(neighbor);
	}

	std::vector<int> unreachable;
	for (auto& n : Nodes)
		if (visited.count(n.ID) == 0)
			unreachable.push_back(n.ID);

	for (int id : unreachable) {
		int idx = node_id_manager.getIndex(id);
		if (idx != -1 && Nodes[idx].Type == WireNode::PIN)
			removedPins.push_back(Nodes[idx]);
		removeNode(id);
	}

	// No edges left means the wire is gone; report its remaining pins and clear.
	if (Segments.empty()) {
		for (auto& n : Nodes)
			if (n.Type == WireNode::PIN)
				removedPins.push_back(n);

		Nodes.clear();
		Adjacency.clear();
		node_id_manager.clear();
		DestinationNodeIDs.clear();
		SourceNodeID = -1;
	}

	return removedPins;
}

std::vector<WireNode> Wire::clearAll()
{
	std::vector<WireNode> pins;
	for (auto& n : Nodes)
		if (n.Type == WireNode::PIN)
			pins.push_back(n);

	Nodes.clear();
	Segments.clear();
	Adjacency.clear();
	node_id_manager.clear();
	DestinationNodeIDs.clear();
	SourceNodeID = -1;
	return pins;
}

// ===========================================================================
//  Rendering
// ===========================================================================

void Wire::draw(const std::vector<int>& selectedNodeIDs, const std::vector<WireSegment>& selectedSegments) const {
	Color wire_color = LogicLevelColors[Value];

	for (auto& segment : Segments)
	{
		wire_color = LogicLevelColors[Value];
		if (std::find(selectedSegments.begin(), selectedSegments.end(), segment) != selectedSegments.end())
			wire_color = SelectedColor;

		Vector2 start_pos = positionOf(segment.first);
		Vector2 end_pos = positionOf(segment.second);
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

Color Wire::SelectedColor = { 0, 150, 255, 255 };
float Wire::NodeRadius = 5.0f;
float Wire::SegmentThickness = 3.0f;
