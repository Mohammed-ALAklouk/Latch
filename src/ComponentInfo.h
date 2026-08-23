#pragma once
#include <vector>
#include "raylib.h"
#include "Pin.h"

struct componentInfo {
	enum Type
	{
		AND,
		NAND,
		OR,
		NOR,
		XOR,
		XNOR,
		NOT,
		LED,
		TOGGLE,

		COMPONENT_COUNT
	};

	Type type;
	int id;
	Vector2 position;
	std::vector<int> input_components;
	std::vector<int> output_components;
	std::vector<LogicLevel> output_value;
};