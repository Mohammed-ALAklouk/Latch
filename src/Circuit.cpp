#include "Circuit.h"

void Circuit::evaluate()
{
	for (auto& wire : m_wires)
	{
		wire.Value = getComponent(wire.Source.ComponentID).m_component.m_output_pin.value;
	}
	
	for (auto& component : m_components)
	{
		evaluateComponent(component.m_component);
	}
}

void Circuit::evaluateComponent(Component& component)
{
	std::vector<LogicLevel> input_values;
	for (int i = 0; i < component.m_input_wires.size(); ++i)
	{
		int wire_id = component.m_input_wires[i];
		if (wire_id == -1)
		{
			input_values.push_back(LogicLevel::UNDEFINED);
			continue;
		}


		input_values.push_back(getWire(wire_id).Value);
	}

	component.evaluate(input_values);
}

void Circuit::draw(const std::vector<int>& selectedComponentIDs, int hoveredComponentID) 
{
	{
		for (const auto& component : m_components) {
			std::vector<LogicLevel> inputs;
			for (int i = 0; i < component.m_component.m_input_wires.size(); ++i) {
				int wire_id = component.m_component.m_input_wires[i];
				if (wire_id == -1) {
					inputs.push_back(LogicLevel::UNDEFINED);
				}
				else {
					inputs.push_back(getWire(wire_id).Value);
				}
			}

			bool highlighted = (component.id == hoveredComponentID);
			bool selected = (std::find(selectedComponentIDs.begin(), selectedComponentIDs.end(), component.id) != selectedComponentIDs.end());
			component.draw(inputs, highlighted, selected);
		}

		for (const auto& wire : m_wires) {
			auto start = getComponent(wire.Source.ComponentID).getOutputPosition();
			auto end = getComponent(wire.Destination.ComponentID).getInputPosition(wire.Destination.PinIndex);
			DrawLineEx(start, end, 3, LogicLevelColors[wire.Value]);
		}
	}
}

int Circuit::addComponent(NodeInfo::Type type, Vector2 position)
{
	int id = m_component_ids.getNextId();
	m_components.push_back(LogicNode(type, position, id));
	m_component_ids.setIndex(id, m_components.size() - 1);
	return id;
}

void Circuit::set_component_input_wire(int componentID, int input_index, int wire_ID)
{
	auto& component = getComponent(componentID);
	component.m_component.m_input_wires[input_index] = wire_ID;
}

int Circuit::addWire(PinRef input, PinRef output)
{
	int id = m_wire_ids.getNextId();
	auto& input_component = getComponent(input.ComponentID);
	input_component.m_component.m_output_wires.push_back(id);
	m_wires.push_back({ input, output, input_component.m_component.m_output_pin.value, id });
	m_wire_ids.setIndex(id, m_wires.size() - 1);

	return id;
}

void Circuit::removeComponent(int id)
{
	int index = m_component_ids.getIndex(id);
	if (index != -1) {
		auto& component = getComponent(id);
		auto input_wires_copy = component.m_component.m_input_wires;

		for (int id : input_wires_copy) removeWire(id);

		auto output_wires_copy = component.m_component.m_output_wires;
		for (int id : output_wires_copy) removeWire(id);

		int lastIndex = (int)m_components.size() - 1;
		if (index != lastIndex) {
			std::swap(m_components[index], m_components[lastIndex]);
			m_component_ids.setIndex(m_components[index].id, index);
		}

		m_components.pop_back();
		m_component_ids.releaseId(id);
	}
}

void Circuit::removeWire(int id)
{
	int index = m_wire_ids.getIndex(id);
	if (index != -1) {
		auto& wire = getWire(id);
		auto& input_component = getComponent(wire.Source.ComponentID);
		input_component.m_component.m_output_wires.erase(
			std::remove(input_component.m_component.m_output_wires.begin(), input_component.m_component.m_output_wires.end(), id),
			input_component.m_component.m_output_wires.end());

		auto& output_component = getComponent(wire.Destination.ComponentID);
		for (auto& input_wire_id : output_component.m_component.m_input_wires) {
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

void Circuit::selectComponentsInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs) const
{
	for (const auto& component : m_components) {
		if (CheckCollisionRecs(component.rect, selectionRect)) {
			selectedComponentIDs.push_back(component.id);
		}
	}
}

void Circuit::restoreComponent(int id, NodeInfo nodeInfo)
{
	LogicNode newNode(nodeInfo.type, nodeInfo.position, id);

	for (int i = 0; i < nodeInfo.input_components.size(); ++i) {
		if (nodeInfo.input_components[i] == -1) {
			newNode.m_component.m_input_wires[i] = -1;
			continue;
		}

		int wireId = addWire({ nodeInfo.input_components[i], 0 }, { id, i });
		newNode.m_component.m_input_wires[i] = wireId;
	}

	m_components.push_back(newNode);
	m_component_ids.reuseId(id, m_components.size() - 1);
}

