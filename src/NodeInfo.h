#pragma once
#include <vector>
#include "raylib.h"

struct NodeInfo {
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
	Vector2 position;
	std::vector<int> input_components;
};