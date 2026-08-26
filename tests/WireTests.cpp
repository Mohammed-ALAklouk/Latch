// Wire geometry contract: orthogonal routing and the normalize() invariants.
// These assert the *observable* shape of a route/wire (endpoints, orthogonality,
// reachability, absence of redundant corners) rather than internal node counts,
// so they describe what a wire must look like without freezing how it is stored.

#include <catch2/catch_test_macros.hpp>

#include <unordered_set>
#include <stack>
#include <vector>

#include "Wire.h"

namespace {
    bool samePoint(Vector2 a, Vector2 b) { return a.x == b.x && a.y == b.y; }

    // Every consecutive pair of a polyline shares an axis (no diagonal legs).
    bool isOrthogonal(const std::vector<Vector2>& pts)
    {
        for (size_t i = 1; i < pts.size(); ++i)
            if (pts[i - 1].x != pts[i].x && pts[i - 1].y != pts[i].y)
                return false;
        return true;
    }

    int junctionCount(const Wire& w)
    {
        int n = 0;
        for (const auto& node : w.Nodes)
            if (node.Type == WireNode::JUNCTION) ++n;
        return n;
    }

    // Is `to` reachable from `from` walking the wire's adjacency graph?
    bool reachable(const Wire& w, int from, int to)
    {
        if (from == -1 || to == -1) return false;
        std::unordered_set<int> seen{ from };
        std::stack<int> todo;
        todo.push(from);
        while (!todo.empty()) {
            int cur = todo.top();
            todo.pop();
            auto it = w.Adjacency.find(cur);
            if (it == w.Adjacency.end()) continue;
            for (int nb : it->second)
                if (seen.insert(nb).second) todo.push(nb);
        }
        return seen.count(to) > 0;
    }

    Wire straightWire()   { return Wire(1, { 10, 0 }, { 0, 0 }, { 11, 0 }, { 100, 0 }); }
}

TEST_CASE("routePoints returns a straight run when already axis-aligned", "[wire][route]") {
    auto horizontal = Wire::routePoints({ 0, 0 }, { 100, 0 });
    REQUIRE(horizontal.size() == 2);
    REQUIRE(samePoint(horizontal.front(), { 0, 0 }));
    REQUIRE(samePoint(horizontal.back(), { 100, 0 }));

    auto vertical = Wire::routePoints({ 0, 0 }, { 0, 100 });
    REQUIRE(vertical.size() == 2);
    REQUIRE(isOrthogonal(vertical));
}

TEST_CASE("routePoints bends once, orthogonally, honoring the elbow", "[wire][route]") {
    const Vector2 from{ 0, 0 };
    const Vector2 to{ 100, 50 };

    SECTION("horizontal first: elbow shares the destination's x, source's y") {
        auto pts = Wire::routePoints(from, to, Wire::Elbow::HorizontalFirst);
        REQUIRE(pts.size() == 3);
        REQUIRE(samePoint(pts.front(), from));
        REQUIRE(samePoint(pts.back(), to));
        REQUIRE(samePoint(pts[1], { 100, 0 }));
        REQUIRE(isOrthogonal(pts));
    }
    SECTION("vertical first: elbow shares the source's x, destination's y") {
        auto pts = Wire::routePoints(from, to, Wire::Elbow::VerticalFirst);
        REQUIRE(pts.size() == 3);
        REQUIRE(samePoint(pts[1], { 0, 50 }));
        REQUIRE(isOrthogonal(pts));
    }
}

TEST_CASE("A freshly built wire connects source to destination", "[wire]") {
    Wire w = straightWire();
    REQUIRE(w.getSourcePin().ComponentID == 10);

    int srcNode = w.findNodeAt({ 0, 0 });
    int dstNode = w.findNodeAt({ 100, 0 });
    REQUIRE(srcNode != -1);
    REQUIRE(dstNode != -1);
    REQUIRE(reachable(w, srcNode, dstNode));
    REQUIRE(junctionCount(w) == 0); // a straight run needs no corners
}

TEST_CASE("An L-shaped wire keeps exactly one corner", "[wire]") {
    Wire w(1, { 10, 0 }, { 0, 0 }, { 11, 0 }, { 100, 50 }, Wire::Elbow::HorizontalFirst);
    REQUIRE(junctionCount(w) == 1);
    REQUIRE(w.findNodeAt({ 100, 0 }) != -1); // corner where the elbow bends
    REQUIRE(reachable(w, w.findNodeAt({ 0, 0 }), w.findNodeAt({ 100, 50 })));
}

TEST_CASE("normalize drops a redundant collinear junction", "[wire][normalize]") {
    Wire w = straightWire();
    w.splicePinIntoSegment({ -1, -1 }, { 50, 0 }); // a junction mid-run carries no info

    REQUIRE(junctionCount(w) == 0); // collapsed away
    REQUIRE(reachable(w, w.findNodeAt({ 0, 0 }), w.findNodeAt({ 100, 0 })));
}

TEST_CASE("normalize keeps a pin spliced onto a segment", "[wire][normalize]") {
    Wire w = straightWire();
    w.splicePinIntoSegment({ 12, 0 }, { 50, 0 }); // a real pin must survive

    int mid = w.findNodeAt({ 50, 0 });
    REQUIRE(mid != -1);
    REQUIRE(w.getNodeComponentID(mid) == 12);
    REQUIRE(reachable(w, w.findNodeAt({ 0, 0 }), w.findNodeAt({ 100, 0 })));
}

TEST_CASE("Removing the source node tears the whole wire down", "[wire][normalize]") {
    Wire w = straightWire();
    int source = w.SourceNodeID;
    REQUIRE(source != -1);

    auto removedPins = w.removeNodes({ source });

    REQUIRE(w.Nodes.empty());
    REQUIRE(w.SourceNodeID == -1);
    REQUIRE(w.getSourcePin().ComponentID == -1);
    REQUIRE_FALSE(removedPins.empty()); // the freed pin nodes are reported back
}
