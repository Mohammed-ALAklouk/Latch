#pragma once

#include <vector>
#include <raylib.h>
#include <memory>
#include "Pin.h"
#include "NewCircuit.h"

class Action {
	public:
	virtual	void undo(NewCircuit& NewCircuit) = 0;
	virtual void redo(NewCircuit& NewCircuit) = 0;

	virtual ~Action() = default;
};

class ComponentPlacedAction : public Action {
	public:
	ComponentPlacedAction(int componentID, NodeInfo nodeInfo) : componentID(componentID), nodeInfo(nodeInfo) {}
	void undo(NewCircuit& NewCircuit) override {
		NewCircuit.removeComponent(componentID);
	}

	void redo(NewCircuit& NewCircuit) override {
		NewCircuit.restoreComponent(componentID, nodeInfo);
	}

	int componentID;
	NodeInfo nodeInfo;

	virtual ~ComponentPlacedAction() = default;
};

class WirePlacedAction : public Action {
	public:
	WirePlacedAction(int wireID, PinRef input, PinRef output) : wireID(wireID), input(input), output(output) {}
	void undo(NewCircuit& NewCircuit) override {
		NewCircuit.removeWire(wireID);
	}
	void redo(NewCircuit& NewCircuit) override {
		wireID = NewCircuit.addWire(input, output);
	}

	int wireID;
	PinRef input;
	PinRef output;
};

class ComponentsMovedAction : public Action {
	public:
	ComponentsMovedAction(std::vector<int> componentIDs, Vector2 translation) : componentIDs(componentIDs), translation(translation) {}
	void undo(NewCircuit& NewCircuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = NewCircuit.getComponent(componentID);
			component->m_rect.x -= translation.x;
			component->m_rect.y -= translation.y;
		}
	}
	void redo(NewCircuit& NewCircuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = NewCircuit.getComponent(componentID);
			component->m_rect.x += translation.x;
			component->m_rect.y += translation.y;
		}
	}
	std::vector<int> componentIDs;
	Vector2 translation;
};

class ComponentsDeletedAction : public Action {
public:
	ComponentsDeletedAction(std::vector<NodeInfo> nodesInfo, std::vector<int> IDs) : nodeInfo(nodesInfo), IDs(IDs) {}
	void undo(NewCircuit& NewCircuit) override {
		for (size_t i = 0; i < nodeInfo.size(); ++i) {
			NewCircuit.restoreComponent(IDs[i], nodeInfo[i]);
		}
	}

	void redo(NewCircuit& NewCircuit) override {
		for (const auto& id : IDs) {
			NewCircuit.removeComponent(id);
		}
	}

	std::vector<NodeInfo> nodeInfo;
	std::vector<int> IDs;
};

class WireDeletedAction : public Action {
	public:
	WireDeletedAction(PinRef input, PinRef output, int wireID) : input(input), output(output), wireID(wireID) {}
	void undo(NewCircuit& NewCircuit) override {
		wireID = NewCircuit.addWire(input, output);
	}

	void redo(NewCircuit& NewCircuit) override {
		NewCircuit.removeWire(wireID);
	}

	PinRef input;
	PinRef output;
	int wireID;
};

class PasteAction : public Action {
	public:
	PasteAction(std::vector<int> ids, std::vector<NodeInfo>& nodesInfo) : ids(ids), nodesInfo(nodesInfo) {
		
	}
	void undo(NewCircuit& NewCircuit) override {
		for (const auto& id : ids) {
			NewCircuit.removeComponent(id);
		}
	}

	void redo(NewCircuit& NewCircuit) override {
		for (size_t i = 0; i < nodesInfo.size(); ++i) {
			NewCircuit.restoreComponent(ids[i], nodesInfo[i]);
		}
	}

	std::vector<NodeInfo> nodesInfo;
	std::vector<int> ids;
};

class ActionManager {
	public:
	template <typename T, typename... Args>
	void addAction(Args&&... args) {
		actions.erase(actions.begin() + currentIndex + 1, actions.end());
		actions.push_back(std::make_unique<T>(std::forward<Args>(args)...));
		++currentIndex;
	}

	void undo(NewCircuit& NewCircuit) {
		if (currentIndex < 0) return;
		Action* lastAction = actions[currentIndex].get();
		lastAction->undo(NewCircuit);
		--currentIndex;
	}
	
	void redo(NewCircuit& NewCircuit) {
		if (currentIndex + 1 >= static_cast<int>(actions.size())) return;
		Action* nextAction = actions[currentIndex + 1].get();
		nextAction->redo(NewCircuit);
		++currentIndex;
	}

	private:
	std::vector<std::unique_ptr<Action>> actions;
	int currentIndex = -1;
};