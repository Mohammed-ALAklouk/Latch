#pragma once
#include <raylib.h>

inline Color LogicLevelColors[] = {
	{60, 60, 60, 255}, // LOW
	{0, 220, 80, 255}, // HIGH
	{255, 150, 0, 255} // UNDEFINED
};

enum LogicLevel
{
	LOW,
	HIGH,
	UNDEFINED
};

struct Pin
{
	Pin(LogicLevel initialLevel = LogicLevel::UNDEFINED): value(initialLevel)
	{
	}

	LogicLevel value;
};

struct PinRef
{
	int ComponentID;
	int PinIndex;
};