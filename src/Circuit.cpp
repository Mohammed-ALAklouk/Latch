#include "Circuit.h"

void Circuit::evaluate()
{
	for (auto& wire : m_wires)
	{
		int source_component_id = wire.Source.ComponentID;
		int source_output_index = wire.Source.PinIndex; 
		wire.Value = getComponent(wire.Source.ComponentID).get()->m_outputValues[source_output_index];
	}
	
	for (auto& component : m_components)
	{
		evaluateComponent(component);
	}
}

void Circuit::evaluateComponent(std::unique_ptr<Gate>& component)
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

	component->evaluate(input_values);
}

void Circuit::draw(const std::vector<int>& selectedComponentIDs, int hoveredComponentID)
{
	for (auto& component : m_components)
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

		bool selected = std::find(selectedComponentIDs.begin(), selectedComponentIDs.end(), m_component_ids.getIndex(component->m_id)) != selectedComponentIDs.end();
		bool highlighted = hoveredComponentID == m_component_ids.getIndex(component->m_id);
		component->draw(input_values, selected, highlighted);
	}

	for (auto& wire : m_wires)
	{
		auto& source_component = getComponent(wire.Source.ComponentID);
		auto& destination_component = getComponent(wire.Destination.ComponentID);
		Vector2 start_pos = source_component->getOutputPosition(wire.Source.PinIndex);
		Vector2 end_pos = destination_component->getInputPosition(wire.Destination.PinIndex);
		Color wire_color = Gate::LogicLevelColors[wire.Value];
		DrawLineEx(start_pos, end_pos, 3, wire_color);
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

int Circuit::addWire(PinRef source, PinRef destination)
{
	int id = m_wire_ids.getNextId();
	auto& source_component = getComponent(source.ComponentID);
	source_component->m_outputWireIds[source.PinIndex] = id;
	set_component_input_wire(destination.ComponentID, destination.PinIndex, id);

	m_wires.push_back({ source, destination, source_component->m_outputValues[source.PinIndex], id });
	m_wire_ids.setIndex(id, m_wires.size() - 1);
	return id;
}

void Circuit::removeComponent(int id)
{
	int index = m_component_ids.getIndex(id);
	if (index != -1) {
		auto& component = getComponent(id);
		auto input_wires_copy = component->m_inputWireIds;

		for (int id : input_wires_copy) removeWire(id);

		auto output_wires_copy = component->m_outputWireIds;
		for (int id : output_wires_copy) removeWire(id);

		int lastIndex = (int)m_components.size() - 1;
		if (index != lastIndex) {
			std::swap(m_components[index], m_components[lastIndex]);
			m_component_ids.setIndex(m_components[index]->m_id, index);
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
		input_component->m_outputWireIds.erase(
			std::remove(input_component->m_outputWireIds.begin(), input_component->m_outputWireIds.end(), id),
			input_component->m_outputWireIds.end());

		auto& output_component = getComponent(wire.Destination.ComponentID);
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

void Circuit::selectComponentsInArea(Rectangle selectionRect, std::vector<int>& selectedComponentIDs) const
{
	for (const auto& component : m_components) {
		if (CheckCollisionRecs(component->m_rect, selectionRect)) {
			selectedComponentIDs.push_back(component->m_id);
		}
	}
}

void Circuit::restoreComponent(int id, NodeInfo nodeInfo)
{
	if (nodeInfo.type < 0 || nodeInfo.type >= NodeInfo::COMPONENT_COUNT) return;

	std::unique_ptr<Gate> new_component = std::move(GateFactories[nodeInfo.type](id, nodeInfo.position));

	for (int i = 0; i < nodeInfo.input_components.size(); ++i) {
		if (nodeInfo.input_components[i] == -1) {
			new_component->m_inputWireIds[i] = -1;
			continue;
		}

		int wireId = addWire({ nodeInfo.input_components[i], 0 }, { id, i });
		new_component->m_inputWireIds[i] = wireId;
	}

	m_components.push_back(std::move(new_component));
	m_component_ids.reuseId(id, m_components.size() - 1);
}