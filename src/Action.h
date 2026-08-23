#pragma once
#include <vector>
#include "Circuit.h"

class ActionManager {
	public:
	void AddSnapshot(const Circuit::CircuitSnapshot& snapshot) {
		if (currentIndex + 1 < static_cast<int>(snapshots.size())) {
			snapshots.erase(snapshots.begin() + currentIndex + 1, snapshots.end());
		}
		snapshots.push_back(snapshot);
		currentIndex = static_cast<int>(snapshots.size()) - 1;
	}

	void undo(Circuit& circuit) {
		if (currentIndex <= 0) return;
		--currentIndex;
		circuit.RestoreSnapshot(snapshots[currentIndex]);
	}
	
	void redo(Circuit& circuit) {
		if (currentIndex + 1 >= static_cast<int>(snapshots.size())) return;
		++currentIndex;
		circuit.RestoreSnapshot(snapshots[currentIndex]);
	}

	const std::vector<Circuit::CircuitSnapshot>& getSnapshots() const {
		return snapshots;
	}
	const int getCurrentIndex() const {
		return currentIndex;
	}

	private:
	std::vector<Circuit::CircuitSnapshot> snapshots;
	int currentIndex = -1;
};