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
	Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos);

	int findNodeAt(Vector2 pos) const;
	WireSegment findSegmentAt(Vector2 pos) const;
	void draw(const std::vector<int>& selectedNodeIDs, const std::vector<WireSegment>& selectedSegments) const;
	Vector2 getNodePosition(int nodeID) const;

	void extendTo(int sourceNodeID, PinRef pin, Vector2 destPos);
	void extendTo(WireSegment segment, Vector2 source, PinRef targetPin, Vector2 dest);

	std::vector<WireNode> removeNodes(const std::vector<int>& nodeIDs);       // removes the nodes and anything left disconnected
	std::vector<WireNode> removeSegments(const std::vector<WireSegment>& segments);

	static std::vector<Vector2> routePoints(Vector2 from, Vector2 to);
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
