#include "Circuit.h"

void Circuit::evaluate()
{
	for (auto& wire : m_wires)
	{
		int source_component_id = wire.Nodes[0].Pin.ComponentID;
		int source_output_index = wire.Nodes[0].Pin.PinIndex; 
		wire.Value = getComponent(source_component_id).get()->m_outputValues[source_output_index];
	}
	
	for (auto& component : m_components)
	{
		evaluateComponent(component);
	}
}

std::vector<LogicLevel> Circuit::getComponentInputValues(std::unique_ptr<Gate>& component) 
{
	std::vector<LogicLevel> input_values;
	for (int i = 0; i < component->m_inputWireIds.size(); ++i)
	{
		int wire_id = component->m_inputWireIds[i];
		if (wire_id == -1)
		{
			input_values.push_back(LogicLevel::UNDEFINED);
			continue;
		}
		input_values.push_back(getWire(wire_id).Value);
	}

	return input_values;
}

void Circuit::evaluateComponent(std::unique_ptr<Gate>& component)
{
	std::vector<LogicLevel> input_values = getComponentInputValues(component);
	component->evaluate(input_values);
}

void Circuit::draw(const std::vector<int>& selectedComponentIDs, int hoveredComponentID, std::unordered_map<int, std::vector<int>>& selectedWireIDs)
{
	for (auto& component : m_components)
	{
		std::vector<LogicLevel> input_values = getComponentInputValues(component);

		bool selected = std::find(selectedComponentIDs.begin(), selectedComponentIDs.end(), component->m_id) != selectedComponentIDs.end();
		bool highlighted = hoveredComponentID == component->m_id;
		component->draw(input_values, selected, highlighted);
	}

	for (auto& wire : m_wires)
	{
		if(selectedWireIDs.find(wire.ID) != selectedWireIDs.end())
			wire.draw(selectedWireIDs[wire.ID]);
		else 
			wire.draw(std::vector<int>());
	}
}

int Circuit::addComponent(NodeInfo::Type type, Vector2 position)
{
	if (type < 0 || type >= NodeInfo::COMPONENT_COUNT) return -1;

	int id = m_component_ids.getNextId();
	std::unique_ptr<Gate> new_component = std::move(GateFactories[type](id, position));

	m_component_ids.setIndex(id, m_components.size());
	m_components.push_back(std::move(new_component));
	return id;
}

void Circuit::set_component_input_wire(int componentID, int input_index, int wire_ID)
{
	auto& component = getComponent(componentID);
	if (component) {
		if (input_index >= 0 && input_index < component->m_inputWireIds.size()) {
			component->m_inputWireIds[input_index] = wire_ID;
		}
	}
}

int Circuit::addWire(PinRef source, Vector2 sourcePos, PinRef destination, Vector2 destinationPos)
{
	if (source.ComponentID == -1) {
		return -1;
	}

	int id = m_wire_ids.getNextId();
	auto& source_component = getComponent(source.ComponentID);
	source_component->m_outputWireIds[source.PinIndex] = id;

	if (destination.ComponentID != -1) {
		auto& destination_component = getComponent(destination.ComponentID);
		set_component_input_wire(destination.ComponentID, destination.PinIndex, id);
	}


	m_wires.push_back(Wire(id, source, sourcePos, destination, destinationPos));
	m_wire_ids.setIndex(id, m_wires.size() - 1);
	return id;
}

void Circuit::removeComponent(int id)
{
	/*
	* TODO: Rewrite to use the new wire system
	* 
	int index = m_component_ids.getIndex(id);
	if (index != -1) {
		auto& component = getComponent(id);
		auto input_wires_copy = component->m_inputWireIds;

		for (int input_id : input_wires_copy) removeWire(input_id);

		auto output_wires_copy = component->m_outputWireIds;
		for (int output_id : output_wires_copy) removeWire(output_id);

		int lastIndex = (int)m_components.size() - 1;
		if (index != lastIndex) {
			std::swap(m_components[index], m_components[lastIndex]);
			m_component_ids.setIndex(m_components[index]->m_id, index);
		}

		m_components.pop_back();
		m_component_ids.releaseId(id);
	}*/
}

void Circuit::removeWire(int id)
{
	int index = m_wire_ids.getIndex(id);
	if (index != -1) {
		auto& wire = getWire(id);
		auto& source_component = getComponent(wire.Nodes[0].Pin.ComponentID);
		for (auto& output_wire_id : source_component->m_outputWireIds) {
			if (output_wire_id == id) {
				output_wire_id = -1;
			}
		}

		auto& output_component = getComponent(wire.Nodes[1].Pin.ComponentID);
		for (auto& input_wire_id : output_component->m_inputWireIds) {
			if (input_wire_id == id) {
				input_wire_id = -1;
			}
		}


		int lastIndex = (int)m_wires.size() - 1;
		if (index != lastIndex) {
			std::swap(m_wires[index], m_wires[lastIndex]);
			m_wire_ids.setIndex(m_wires[index].ID, index);
		}

		m_wires.pop_back();
		m_wire_ids.releaseId(id);
	}
}

void Circuit::removeWireNode(int wireID, int nodeID)
{
	auto& wire = getWire(wireID);
	wire.removeNode(nodeID);

	if (!wire.Nodes.size())
		removeWire(wireID);
}


void Circuit::selectComponentsInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs, 
	std::unordered_map<int, std::vector<int>>& selectedWireNodes) const
{
	for (const auto& component : m_components) {
		if (CheckCollisionRecs(component->m_rect, selectionRect)) {
			selectedComponentIDs.push_back(component->m_id);
		}
	}

	for (const auto& wire : m_wires) {
		for (const auto& node : wire.Nodes) {
			if (CheckCollisionCircleRec(node.Position, 10, selectionRect)) {
				if (selectedWireNodes.find(wire.ID) == selectedWireNodes.end())
					selectedWireNodes[wire.ID] = {};
				
				selectedWireNodes[wire.ID].push_back(node.ID);
			}
		}
	}

}

void Circuit::restoreComponent(int id, NodeInfo nodeInfo)
{
	// TODO: Restore wires as well when restoring a component
	/*
	if (nodeInfo.type < 0 || nodeInfo.type >= NodeInfo::COMPONENT_COUNT) return;

	m_components.push_back(GateFactories[nodeInfo.type](id, nodeInfo.position));
	m_component_ids.reuseId(id, m_components.size() - 1);
	auto& new_component = m_components.back();

	for (int i = 0; i < nodeInfo.input_components.size(); ++i) {
		if (nodeInfo.input_components[i] == -1) {
			new_component->m_inputWireIds[i] = -1;
			continue;
		}

		int wireId = addWire({ nodeInfo.input_components[i], 0 }, { id, i });
	}
	*/
}

int Circuit::extendWireTo(int wireID, int sourceNodeID, PinRef targetPin, Vector2 targetPos)
{
	auto& wire = getWire(wireID);
	wire.extendTo(sourceNodeID, targetPin, targetPos);
	if (targetPin.ComponentID != -1) {
		auto& target_component = getComponent(targetPin.ComponentID);
		target_component->m_inputWireIds[targetPin.PinIndex] = wireID;
	}
	return wireID;
}
