#pragma once
#include "Pin.h"

struct Wire 
{
	PinRef Source;
	PinRef Destination;
	LogicLevel Value;
	int ID;
};
