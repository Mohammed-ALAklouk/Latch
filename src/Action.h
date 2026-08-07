#pragma once

#include <vector>
#include <raylib.h>
#include <memory>
#include "Pin.h"
#include "Circuit.h"

class Action {
	public:
	virtual	void undo(Circuit& Circuit) = 0;
	virtual void redo(Circuit& Circuit) = 0;

	virtual ~Action() = default;
};

class ComponentPlacedAction : public Action {
	public:
	ComponentPlacedAction(int componentID, componentInfo nodeInfo) : componentID(componentID), nodeInfo(nodeInfo) {}
	void undo(Circuit& Circuit) override {
		Circuit.removeComponent(componentID);
	}

	void redo(Circuit& Circuit) override {
		Circuit.restoreComponent(componentID, nodeInfo);
	}

	int componentID;
	componentInfo nodeInfo;

	virtual ~ComponentPlacedAction() = default;
};

class WirePlacedAction : public Action {
	public:
	WirePlacedAction(int wireID, PinRef input, PinRef output) : wireID(wireID), input(input), output(output) {}
	void undo(Circuit& Circuit) override {
		Circuit.removeWire(wireID);
	}
	void redo(Circuit& Circuit) override {
		// TODO: Redo to support the new wire representation
		//wireID = Circuit.addWire(input, output);
	}

	int wireID;
	PinRef input;
	PinRef output;
};

class ComponentsMovedAction : public Action {
	public:
	ComponentsMovedAction(std::vector<int> componentIDs, Vector2 translation) : componentIDs(componentIDs), translation(translation) {}
	void undo(Circuit& Circuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = Circuit.getComponent(componentID);
			component->m_rect.x -= translation.x;
			component->m_rect.y -= translation.y;
		}
	}
	void redo(Circuit& Circuit) override {
		for (const auto& componentID : componentIDs) {
			auto& component = Circuit.getComponent(componentID);
			component->m_rect.x += translation.x;
			component->m_rect.y += translation.y;
		}
	}
	std::vector<int> componentIDs;
	Vector2 translation;
};

class ComponentsDeletedAction : public Action {
public:
	ComponentsDeletedAction(std::vector<componentInfo> nodesInfo, std::vector<int> IDs) : nodeInfo(nodesInfo), IDs(IDs) {}
	void undo(Circuit& Circuit) override {
		for (size_t i = 0; i < nodeInfo.size(); ++i) {
			Circuit.restoreComponent(IDs[i], nodeInfo[i]);
		}
	}

	void redo(Circuit& Circuit) override {
		for (const auto& id : IDs) {
			Circuit.removeComponent(id);
		}
	}

	std::vector<componentInfo> nodeInfo;
	std::vector<int> IDs;
};

class WireDeletedAction : public Action {
	public:
	WireDeletedAction(PinRef input, PinRef output, int wireID) : input(input), output(output), wireID(wireID) {}
	void undo(Circuit& Circuit) override {
		
		// TODO: Redo to support the new wire representation

		//wireID = Circuit.addWire(input, output);
	}

	void redo(Circuit& Circuit) override {
		Circuit.removeWire(wireID);
	}

	PinRef input;
	PinRef output;
	int wireID;
};

class PasteAction : public Action {
	public:
	PasteAction(std::vector<int> ids, std::vector<componentInfo>& nodesInfo) : ids(ids), nodesInfo(nodesInfo) {
		
	}
	void undo(Circuit& Circuit) override {
		for (const auto& id : ids) {
			Circuit.removeComponent(id);
		}
	}

	void redo(Circuit& Circuit) override {
		for (size_t i = 0; i < nodesInfo.size(); ++i) {
			Circuit.restoreComponent(ids[i], nodesInfo[i]);
		}
	}

	std::vector<componentInfo> nodesInfo;
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

	void undo(Circuit& Circuit) {
		if (currentIndex < 0) return;
		Action* lastAction = actions[currentIndex].get();
		lastAction->undo(Circuit);
		--currentIndex;
	}
	
	void redo(Circuit& Circuit) {
		if (currentIndex + 1 >= static_cast<int>(actions.size())) return;
		Action* nextAction = actions[currentIndex + 1].get();
		nextAction->redo(Circuit);
		++currentIndex;
	}

	private:
	std::vector<std::unique_ptr<Action>> actions;
	int currentIndex = -1;
};