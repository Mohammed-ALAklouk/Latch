#pragma once
#include <vector>
#include <string>
#include "Pin.h"
#include "NodeInfo.h"

class Component
{
public:
	

private:

	Component(NodeInfo::Type type, std::string name, std::vector<int> input_wires, Pin output_pin)
		: m_type(type), m_name(name), m_input_wires(input_wires), m_output_pin(output_pin)
	{
	}

	static LogicLevel* LookupTables[];
	static Component BaseComponents[];

	static LogicLevel lookup2D(NodeInfo::Type type, LogicLevel input1, LogicLevel input2) {
		return LookupTables[type][input1 * 3 + input2];
	}

	static LogicLevel AndLookupTable[9];
	static LogicLevel OrLookupTable[9];
	static LogicLevel NotLookupTable[3];
	static LogicLevel HighLookupTable[3];
	static LogicLevel LowLookupTable[3];
public:

	Component(NodeInfo::Type type)
	{
		*this = BaseComponents[static_cast<int>(type)];	
	}

	void evaluate(std::vector<LogicLevel>& input_values) {
		switch (m_type)
		{
		case NodeInfo::Type::NOT:
			m_output_pin.value = LookupTables[m_type][input_values[0]];
			break;
		case NodeInfo::Type::HIGH:
			m_output_pin.value = LookupTables[m_type][0];
			break;
		case NodeInfo::Type::LOW:
			m_output_pin.value = LookupTables[m_type][0];
			break;
		default:
			m_output_pin.value = lookup2D(m_type, input_values[0], input_values[1]);
			break;
		}
	}

	std::vector<int> m_input_wires;
	std::vector<int> m_output_wires;
	Pin m_output_pin;
	std::string m_name;
	NodeInfo::Type m_type;
};

