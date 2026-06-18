#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Gate.h"
#include "Wire.h"
#include "IdManager.h"




class Circuit
{
public:

	void evaluate();
	std::vector<LogicLevel> getComponentInputValues(std::unique_ptr<Gate>& component);
	void evaluateComponent(std::unique_ptr<Gate>& component);
	void draw(const std::vector<int>& selectedComponentIDs, int hoveredComponentID, std::unordered_map<int, std::vector<int>>& selectedWireIDs);
	int addComponent(NodeInfo::Type type, Vector2 position);
	void set_component_input_wire(int componentID, int input_index, int wire_ID);
	int addWire(PinRef input, Vector2 inputPos, PinRef output, Vector2 outputPos);
	void removeComponent(int id);
	void removeWire(int id);
	void removeWireNodes(int wireID, const std::vector<int>& nodeIDs);
	void selectComponentsInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs, 
		std::unordered_map<int, std::vector<int>>& selectedWireNodes) const;
	void restoreComponent(int id, NodeInfo nodeInfo);
	int extendWireTo(int wireID, int sourceNodeID, PinRef targetPin, Vector2 targetPos);

	std::unique_ptr<Gate>& getComponent(int id) {
		int index = m_component_ids.getIndex(id);
		return m_components[index];
	}

	Wire& getWire(int id) {
		int index = m_wire_ids.getIndex(id);
		return m_wires[index];
	}


	std::vector<std::unique_ptr<Gate>> m_components;
	std::vector<Wire> m_wires;
	IdManager m_component_ids;
	IdManager m_wire_ids;
};