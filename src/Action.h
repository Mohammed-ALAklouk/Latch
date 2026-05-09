#pragma once

#include <vector>
#include <raylib.h>
#include <memory>
#include "Pin.h"
#include "Circuit.h"

class Action {
	public:
	virtual	void reverse(Circuit& circuit) = 0;
};

class ComponentPlacedAction : public Action {
	public:
	ComponentPlacedAction(int componentID) : componentID(componentID) {}
	void reverse(Circuit& circuit) override {
		circuit.removeComponent(componentID);
	}
	int componentID;

	virtual ~ComponentPlacedAction() = default;
};

class WirePlacedAction : public Action {
	public:
	WirePlacedAction(int wireID) : wireID(wireID) {}
	void reverse(Circuit& circuit) override {
		circuit.removeWire(wireID);
	}
	int wireID;
};

class ComponentsMovedAction : public Action {
	public:
	ComponentsMovedAction(std::vector<int> componentIDs, Vector2 translation) : componentIDs(componentIDs), translation(translation) {}
	void reverse(Circuit& circuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = circuit.getComponent(componentID);
			component.rect.x -= translation.x;
			component.rect.y -= translation.y;
		}
	}
	std::vector<int> componentIDs;
	Vector2 translation;
};

class ComponentsDeletedAction : public Action {
	public:
		ComponentsDeletedAction(std::vector<NodeInfo> nodesInfo) : nodeInfo(nodesInfo) {}
	void reverse(Circuit& circuit) override {
		std::vector<int> newIDs;
		for (size_t i = 0; i < nodeInfo.size(); ++i) {
			int newID = circuit.addComponent(nodeInfo[i].type, nodeInfo[i].position);
			newIDs.push_back(newID);
		}

		for (size_t i = 0; i < nodeInfo.size(); ++i) {
			for (const auto& input_wire : nodeInfo[i].input_wires) {
				
				if (input_wire.ComponentID != -1) {
					circuit.addWire({ newIDs[input_wire.ComponentID], input_wire.PinIndex }, { newIDs[i], 0 });
				}
			}
		}
	}
	std::vector<NodeInfo> nodeInfo;
};

class WireDeletedAction : public Action {
	public:
	WireDeletedAction(PinRef input, PinRef output, int wireID) : input(input), output(output), wireID(wireID) {}
	void reverse(Circuit& circuit) override {
		circuit.addWire(input, output);
	}
	PinRef input;
	PinRef output;
	int wireID;
};

class PasteAction : public Action {
	public:
	PasteAction(std::vector<int> ids) : ids(ids) {}
	void reverse(Circuit& circuit) override {
		for (const auto& id : ids) {
			circuit.removeComponent(id);
		}
	}

	std::vector<int> ids;
};

class ActionManager {
	public:
	template <typename T, typename... Args>
	void addAction(Args&&... args) {
		actions.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

	void undo(Circuit& circuit) {
		if (actions.empty()) return;
		Action* lastAction = actions.back().get();
		lastAction->reverse(circuit);
		actions.pop_back();
	}
	private:
	std::vector<std::unique_ptr<Action>> actions;
};