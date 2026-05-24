#pragma once
#include <vector>
#include "raylib.h"

struct NodeInfo {
	enum Type
	{
		AND,
		OR,
		NOT,
		HIGH,
		LOW,
		COMPONENT_COUNT
	};

	Type type;
	Vector2 position;
	std::vector<int> input_components;
};