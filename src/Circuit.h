#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <string>
#include "Gate.h"
#include "Wire.h"
#include "IdManager.h"





class Circuit
{
public:
	struct CircuitSnapshot {
		std::vector<componentInfo> components;
		std::vector<Wire> wires;
		IdManager component_ids;
		IdManager wire_ids;
		std::string note;
	};

	void evaluate();
	std::vector<LogicLevel> getComponentInputValues(std::unique_ptr<Gate>& component);
	void evaluateComponent(std::unique_ptr<Gate>& component);
	void draw(const std::vector<int>& selectedComponentIDs, const int hoveredComponentID,
		const std::unordered_map<int, std::vector<int>>& selectedWireIDs, const std::unordered_map<int, std::vector<WireSegment>>& selectedWireSegments,
		const std::unordered_map<int, std::vector<int>>& hiddenWireNodes = {});
	int addComponent(componentInfo::Type type, Vector2 position);
	void set_component_input_wire(int componentID, int input_index, int wire_ID);
	int addWire(PinRef input, Vector2 inputPos, PinRef output, Vector2 outputPos, Wire::Elbow first = Wire::Elbow::HorizontalFirst);
	void removeComponent(int id);
	void removeWire(int id);
	void removeWireNodes(int wireID, const std::vector<int>& nodeIDs);
	void removeWireSegments(int wireID, const std::vector<WireSegment>& segments);
	void selectInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs, 
		std::unordered_map<int, std::vector<int>>& selectedWireNodes, std::unordered_map<int, std::vector<WireSegment>>& selectedWireSegments) const;
	void restoreComponent(componentInfo nodeInfo);
	int extendWireTo(int wireID, int sourceNodeID, PinRef targetPin, Vector2 targetPos, Wire::Elbow first = Wire::Elbow::HorizontalFirst);
	int extendWireTo(int wireID, WireSegment segment, Vector2 source, PinRef targetPin, Vector2 dest, Wire::Elbow first = Wire::Elbow::HorizontalFirst);
	
	void paste(const std::vector<Wire>& wires, const std::vector<componentInfo>& components, const Vector2 displacement);

	bool wireExists(int id) const {
		return m_wire_ids.getIndex(id) != -1;
	}

	bool componentExists(int id) const {
		return m_component_ids.getIndex(id) != -1;
	}

	// Callers must ensure the id is live (componentExists/wireExists). The
	// assert turns an otherwise silent m_x[-1] out-of-bounds read into a clear
	// failure in Debug builds.
	std::unique_ptr<Gate>& getComponent(int id) {
		int index = m_component_ids.getIndex(id);
		assert(index != -1 && "getComponent: invalid component id");
		return m_components[index];
	}

	Wire& getWire(int id) {
		int index = m_wire_ids.getIndex(id);
		assert(index != -1 && "getWire: invalid wire id");
		return m_wires[index];
	}

	void disconnectRemovedPins(int wireID, const std::vector<WireNode>& removedPinNodes);
	
	const CircuitSnapshot GetSnapshot(std::string note = "") const {
		CircuitSnapshot snapshot;
		snapshot.components.reserve(m_components.size());
		for (const auto& component : m_components) {
			snapshot.components.push_back(component->getComponentInfo());
		}
		snapshot.note = note;
		snapshot.wires = m_wires;
		snapshot.component_ids = m_component_ids;
		snapshot.wire_ids = m_wire_ids;
		return snapshot;
	}

	void RestoreSnapshot(const CircuitSnapshot& snapshot) {
		m_wires = snapshot.wires;
		m_wire_ids = snapshot.wire_ids;
		m_component_ids = snapshot.component_ids;
		
		m_components.clear();
		for (const auto& componentInfo : snapshot.components) {
			restoreComponent(componentInfo);
		}
	}

	std::vector<std::unique_ptr<Gate>> m_components;
	std::vector<Wire> m_wires;
	IdManager m_component_ids;
	IdManager m_wire_ids;
};