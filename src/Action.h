#pragma once

#include <vector>
#include <raylib.h>
#include <memory>
#include "Pin.h"
#include "Circuit.h"

class Action {
	public:
	virtual	void undo(Circuit& circuit) = 0;
	virtual void redo(Circuit& circuit) = 0;
};

class ComponentPlacedAction : public Action {
	public:
	ComponentPlacedAction(int componentID, NodeInfo nodeInfo) : componentID(componentID), nodeInfo(nodeInfo) {}
	void undo(Circuit& circuit) override {
		circuit.removeComponent(componentID);
	}

	void redo(Circuit& circuit) override {
		circuit.restoreComponent(componentID, nodeInfo);
	}

	int componentID;
	NodeInfo nodeInfo;

	virtual ~ComponentPlacedAction() = default;
};

class WirePlacedAction : public Action {
	public:
	WirePlacedAction(int wireID, PinRef input, PinRef output) : wireID(wireID), input(input), output(output) {}
	void undo(Circuit& circuit) override {
		circuit.removeWire(wireID);
	}
	void redo(Circuit& circuit) override {
		wireID = circuit.addWire(input, output);
	}

	int wireID;
	PinRef input;
	PinRef output;
};

class ComponentsMovedAction : public Action {
	public:
	ComponentsMovedAction(std::vector<int> componentIDs, Vector2 translation) : componentIDs(componentIDs), translation(translation) {}
	void undo(Circuit& circuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = circuit.getComponent(componentID);
			component.rect.x -= translation.x;
			component.rect.y -= translation.y;
		}
	}
	void redo(Circuit& circuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = circuit.getComponent(componentID);
			component.rect.x += translation.x;
			component.rect.y += translation.y;
		}
	}
	std::vector<int> componentIDs;
	Vector2 translation;
};

class ComponentsDeletedAction : public Action {
	public:
		ComponentsDeletedAction(std::vector<NodeInfo> nodesInfo, std::vector<int> IDs) : nodeInfo(nodesInfo), IDs(IDs) {}
	void undo(Circuit& circuit) override {
		for (size_t i = 0; i < nodeInfo.size(); ++i) {
			circuit.restoreComponent(IDs[i], nodeInfo[i]);
		}
	}

	void redo(Circuit& circuit) override {
		for (const auto& id : IDs) {
			circuit.removeComponent(id);
		}
	}

	std::vector<NodeInfo> nodeInfo;
	std::vector<int> IDs;
};

class WireDeletedAction : public Action {
	public:
	WireDeletedAction(PinRef input, PinRef output, int wireID) : input(input), output(output), wireID(wireID) {}
	void undo(Circuit& circuit) override {
		wireID = circuit.addWire(input, output);
	}

	void redo(Circuit& circuit) override {
		circuit.removeWire(wireID);
	}

	PinRef input;
	PinRef output;
	int wireID;
};

class PasteAction : public Action {
	public:
	PasteAction(std::vector<int> ids, std::vector<NodeInfo>& nodesInfo, Vector2 translation) : ids(ids), nodesInfo(nodesInfo), translation(translation) {
		for (size_t i = 0; i < nodesInfo.size(); ++i) {
			nodesInfo[i].position.x += translation.x;
			nodesInfo[i].position.y += translation.y;
		}
	}
	void undo(Circuit& circuit) override {
		for (const auto& id : ids) {
			circuit.removeComponent(id);
		}
	}

	void redo(Circuit& circuit) override {
		for (size_t i = 0; i < nodesInfo.size(); ++i) {
			circuit.restoreComponent(ids[i], nodesInfo[i]);
		}
	}

	std::vector<NodeInfo> nodesInfo;
	Vector2 translation;
	std::vector<int> ids;
};

class ActionManager {
	public:
	template <typename T, typename... Args>
	void addAction(Args&&... args) {
		actions.push_back(std::make_unique<T>(std::forward<Args>(args)...));
		++currentIndex;
		if (currentIndex < static_cast<int>(actions.size()) - 1) {
			actions.erase(actions.begin() + currentIndex + 1, actions.end());
		}
	}

	void undo(Circuit& circuit) {
		if (currentIndex < 0) return;
		Action* lastAction = actions[currentIndex].get();
		lastAction->undo(circuit);
		--currentIndex;
	}
	
	void redo(Circuit& circuit) {
		if (currentIndex + 1 >= static_cast<int>(actions.size())) return;
		Action* nextAction = actions[currentIndex + 1].get();
		nextAction->redo(circuit);
		++currentIndex;
	}

	private:
	std::vector<std::unique_ptr<Action>> actions;
	int currentIndex = -1;
};