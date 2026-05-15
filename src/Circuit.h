#pragma once
#include <vector>
#include "Component.h"
#include "Wire.h"
#include "LogicNode.h"
#include "IdManager.h"

struct NodeInfo {
	Component::Type type;
	Vector2 position;
	std::vector<int> input_components;
};


class Circuit
{
public:
	void evaluate();
	void evaluateComponent(Component& component);
	void draw(const std::vector<int>& selectedComponentIDs, int hoveredComponentID) ;
	int addComponent(Component::Type type, Vector2 position);
	void set_component_input_wire(int componentID, int input_index, int wire_ID);
	int addWire(PinRef input, PinRef output);
	void removeComponent(int id);
	void removeWire(int id);
	void selectComponentsInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs) const;
	void restoreComponent(int id, NodeInfo nodeInfo);

	LogicNode& getComponent(int id) {
		int index = m_component_ids.getIndex(id);	
		return m_components[index];
	}

	Wire& getWire(int id) {
		int index = m_wire_ids.getIndex(id);
		return m_wires[index];
	}

	std::vector<LogicNode> m_components;
	std::vector<Wire> m_wires;
	IdManager m_component_ids;
	IdManager m_wire_ids;
};