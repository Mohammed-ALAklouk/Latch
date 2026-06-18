#pragma once
#include "Pin.h"
#include "IdManager.h"

#include <raylib.h>
#include <unordered_set>
#include <stack>

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
private:
	// removes only the given node
	void eraseNode(int id);
public:

	Wire(int id, PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destPos);
	int addNode(PinRef pin, Vector2 pos);
	int findNodeAt(Vector2 pos) const;
	WireSegment findSegmentAt(Vector2 pos) const;
	void draw(const std::vector<int>& selectedNodeIDs, const std::vector<WireSegment>& selectedSegments) const;
	Vector2 getNodePosition(int nodeID) const;
	void extendTo(int sourceNodeID, PinRef pin, Vector2 destPos);
	std::vector<WireNode> removeNodes(const std::vector<int>& nodeIDs); // removes the node and everything connected to it
	std::vector<WireNode> removeSegments(const std::vector<WireSegment>& segments); // removes the node and everything connected to it
	std::vector<WireNode> pruneOrphens();
	
	static std::vector<Vector2> routePoints(Vector2 from, Vector2 to);
	
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
};
