#pragma once
#include "Pin.h"
#include "IdManager.h"

#include <raylib.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <algorithm>

typedef std::pair<int, int> WireSegment; // pair of node IDs

struct WireNode
{
	PinRef Pin;
	Vector2 Position;
	enum NodeType { PIN, JUNCTION } Type;
	int ID;
};

struct Wire
{
public:
	// Which axis the elbow of an L-shaped route runs first, from source to dest.
	enum class Elbow { HorizontalFirst, VerticalFirst };

	Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos, Elbow first = Elbow::HorizontalFirst);
	Wire(const int id, const Wire& other, const std::unordered_map<int, int>& oldToNewComponentIDMap, const Vector2 displacement);

	int findNodeAt(Vector2 pos) const;
	WireSegment findSegmentAt(Vector2 pos) const;
	void draw(const std::vector<int>& selectedNodeIDs, const std::vector<WireSegment>& selectedSegments, const std::vector<int>& hiddenNodeIDs = {}) const;
	Vector2 getNodePosition(int nodeID) const;
	void setNodePosition(int nodeID, Vector2 newPos) {
		int idx = node_id_manager.getIndex(nodeID);
		if (idx != -1) {
			Nodes[idx].Position = newPos;
			normalize(); // keep the wire tidy after moving a node
		}
	}

	int getNodeComponentID(int nodeID) const {
		int idx = node_id_manager.getIndex(nodeID);
		return (idx != -1) ? Nodes[idx].Pin.ComponentID : -1;
	}

	// The driving pin, resolved via SourceNodeID (not a fixed Nodes index).
	// Returns { -1, -1 } for a torn-down / sourceless wire.
	PinRef getSourcePin() const {
		int idx = node_id_manager.getIndex(SourceNodeID);
		return (idx != -1) ? Nodes[idx].Pin : PinRef{ -1, -1 };
	}

	void demoteNodeToJunction(int nodeID) {
		int idx = node_id_manager.getIndex(nodeID);
		if (idx != -1) {
			Nodes[idx].Pin = { -1, -1 };
			Nodes[idx].Type = WireNode::JUNCTION;
			DestinationNodeIDs.erase(std::remove(DestinationNodeIDs.begin(), DestinationNodeIDs.end(), nodeID), DestinationNodeIDs.end());
		}
	}

	void promoteNodeToPin(int nodeID, PinRef pin) {
		int idx = node_id_manager.getIndex(nodeID);
		if (idx != -1) {
			Nodes[idx].Pin = pin;
			Nodes[idx].Type = WireNode::PIN;
			DestinationNodeIDs.push_back(nodeID);
		}
	}

	// Drop a pin node onto an existing segment body; normalize()'s split pass
	// cuts the segment at it so the wire routes through the new pin.
	void splicePinIntoSegment(PinRef pin, Vector2 pos) {
		addNode(pin, pos);
		normalize();
	}

	// The legs moveNodeWithElbow() *would* draw for this move -- one polyline
	// per incident segment -- without touching the graph. Used for the drag
	// preview so it can't diverge from the committed reroute.
	// At commit time the selected wire nodes translate BEFORE the pin reroutes,
	// so a neighbor in `translatedNeighbors` anchors at its post-move position
	// (old + delta), not where it currently sits.
	std::vector<std::vector<Vector2>> previewMoveLegs(int nodeID, Vector2 newPos, Elbow first,
			const std::vector<int>& translatedNeighbors = {}, Vector2 delta = { 0, 0 }) const {
		std::vector<std::vector<Vector2>> legs;
		if (node_id_manager.getIndex(nodeID) == -1) return legs;
		for (int m : neighborsOf(nodeID)) {
			Vector2 anchor = positionOf(m);
			if (std::find(translatedNeighbors.begin(), translatedNeighbors.end(), m) != translatedNeighbors.end()) {
				anchor.x += delta.x; // this neighbor is dragged too; anchor where it will land
				anchor.y += delta.y;
			}
			legs.push_back(routePoints(anchor, newPos, first));
		}

		return legs;
	}

	// Move a node to newPos, re-routing each incident segment as an orthogonal
	// L (bending per `first`) instead of letting it go diagonal. normalize()
	// then merges any bend that ended up straight.
	void moveNodeWithElbow(int nodeID, Vector2 newPos, Elbow first) {
		int idx = node_id_manager.getIndex(nodeID);
		if (idx == -1) return;
		Vector2 oldPos = Nodes[idx].Position;
		if (oldPos.x == newPos.x && oldPos.y == newPos.y) return;

		auto neighbours = neighborsOf(nodeID); // copy before we start editing edges
		Nodes[idx].Position = newPos;          // the node relocates; its legs are rebuilt below

		for (int m : neighbours) {
			removeSegment(nodeID, m);
			auto pts = routePoints(positionOf(m), newPos, first);
			int prev = m;
			for (size_t k = 1; k < pts.size(); ++k) {
				int cur = addNode({ -1, -1 }, pts[k]); // last point == newPos == nodeID (dedups back to it)
				addSegment(prev, cur);
				prev = cur;
			}
		}
		normalize();
	}

	// Preview legs for moving a GROUP of nodes by delta: a segment fully inside
	// the group translates rigidly (drawn once), a segment crossing to a
	// stationary node reroutes with an elbow. Mirrors moveNodesWithElbow so the
	// preview matches the commit.
	std::vector<std::vector<Vector2>> previewMoveGroupLegs(const std::vector<int>& movingIDs, Vector2 delta, Elbow first) const {
		std::vector<std::vector<Vector2>> legs;
		auto isMoving = [&](int id) { return std::find(movingIDs.begin(), movingIDs.end(), id) != movingIDs.end(); };
		for (int a : movingIDs) {
			if (node_id_manager.getIndex(a) == -1) continue;
			Vector2 aOld = positionOf(a);
			Vector2 aNew = { aOld.x + delta.x, aOld.y + delta.y };
			for (int m : neighborsOf(a)) {
				if (isMoving(m)) {
					if (a < m) { // interior segment: both ends move, draw it once
						Vector2 mOld = positionOf(m);
						legs.push_back({ aNew, { mOld.x + delta.x, mOld.y + delta.y } });
					}
				}
				else {
					legs.push_back(routePoints(positionOf(m), aNew, first)); // boundary: reroute
				}
			}
		}
		return legs;
	}

	// Move a GROUP of nodes by delta. Interior segments translate with their
	// endpoints; boundary segments (to a stationary node) reroute as elbows.
	void moveNodesWithElbow(const std::vector<int>& movingIDs, Vector2 delta, Elbow first) {
		if (delta.x == 0 && delta.y == 0) return;
		auto isMoving = [&](int id) { return std::find(movingIDs.begin(), movingIDs.end(), id) != movingIDs.end(); };

		for (int a : movingIDs) { // 1. shift every moving node; interior segments follow
			int idx = node_id_manager.getIndex(a);
			if (idx != -1) { Nodes[idx].Position.x += delta.x; Nodes[idx].Position.y += delta.y; }
		}

		for (int a : movingIDs) { // 2. reroute only the boundary segments
			if (node_id_manager.getIndex(a) == -1) continue;
			Vector2 aNew = positionOf(a);
			auto neighbours = neighborsOf(a); // snapshot before editing edges
			for (int m : neighbours) {
				if (isMoving(m)) continue; // interior: already translated in step 1
				removeSegment(a, m);
				auto pts = routePoints(positionOf(m), aNew, first);
				int prev = m;
				for (size_t k = 1; k < pts.size(); ++k) {
					int cur = addNode({ -1, -1 }, pts[k]); // last point == aNew, dedups back to a
					addSegment(prev, cur);
					prev = cur;
				}
			}
		}
		normalize();
	}

	int findNodebyPin(PinRef pin) {
		for (int i = 0; i < Nodes.size(); ++i) {
			if (Nodes[i].Pin.ComponentID == pin.ComponentID && Nodes[i].Pin.PinIndex == pin.PinIndex) {
				return Nodes[i].ID;
			}
		}
		return -1;
	}

	void extendTo(int sourceNodeID, PinRef pin, Vector2 destPos, Elbow first = Elbow::HorizontalFirst);
	void extendTo(WireSegment segment, Vector2 source, PinRef targetPin, Vector2 dest, Elbow first = Elbow::HorizontalFirst);

	std::vector<WireNode> removeNodes(const std::vector<int>& nodeIDs);       // removes the nodes and anything left disconnected
	std::vector<WireNode> removeSegments(const std::vector<WireSegment>& segments);

	static std::vector<Vector2> routePoints(Vector2 from, Vector2 to, Elbow first = Elbow::HorizontalFirst);
	static Rectangle getSegmentRect(Vector2 start, Vector2 end) {
		return (start.x == end.x)
			? Rectangle{ start.x - SegmentThickness / 2, std::min(start.y, end.y), SegmentThickness, std::abs(end.y - start.y) }
			: Rectangle{ std::min(start.x, end.x), start.y - SegmentThickness / 2, std::abs(end.x - start.x), SegmentThickness };
	}

	bool segmentContainsPoint(const WireSegment& segment, Vector2 point) const {
		Rectangle rect = getSegmentRect(positionOf(segment.first), positionOf(segment.second));
		return CheckCollisionPointRec(point, rect);
	}

	static Color SelectedColor;
	static float NodeRadius;
	static float SegmentThickness;

	LogicLevel Value;
	int ID;

	int SourceNodeID = -1;
	std::vector <int> DestinationNodeIDs;
	std::vector<WireNode> Nodes;
	std::vector<WireSegment> Segments;
	IdManager node_id_manager;
	std::unordered_map<int, std::vector<int>> Adjacency; // nodeID -> neighbor nodeIDs

private:
	// ---- structural primitives: the ONLY things that mutate the graph ----
	int  addNode(PinRef pin, Vector2 pos); // get-or-create a vertex at pos (no topology surgery)
	void removeNode(int id);               // remove a vertex and its incident edges
	void addSegment(int a, int b);         // add an edge (deduped)
	void removeSegment(int a, int b);      // remove an edge

	// ---- read-only accessors over the graph ----
	Vector2 positionOf(int id) const;
	int degree(int id) const;
	std::vector<int> neighborsOf(int id) const;
	bool collinear(int a, int b, int c) const;

	// ---- invariant restoration; run after any structural edit ----
	std::vector<WireNode> normalize();     // split -> merge -> prune; returns removed PIN nodes
	void splitSegmentsAtNodes();           // any node lying on a segment body splits it
	void removeRedundantNodes();           // collapse collinear degree-2 pass-through junctions
	std::vector<WireNode> pruneOrphans();  // drop anything no longer reachable from the source
	std::vector<WireNode> clearAll();      // tear the whole wire down, reporting its PIN nodes
};
