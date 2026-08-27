#pragma once
#include "raylib.h"
#include "imgui.h"

#include "Action.h"
#include "Circuit.h"
#include "ComponentInfo.h"

struct UITheme {
	Color background;
	Color gridMinor;
	Color gridMajor;
	Color logicLow;    // Logic 0
	Color logicHigh;   // Logic 1
	Color logicX;      // Undefined
};

struct MouseInputs {
	bool leftButtonDown			= false;
	bool rightButtonDown		= false;
	Vector2 mousePositionScreen = { 0, 0 };
	Vector2 mousePositionWorld	= { 0, 0 };
	float mouseWheelDelta		= 0.0f;
};

struct PanningContext {
	Vector2 initial_pos;
	Vector2 initial_camera_target;
};

struct DraggingContext {
	Vector2 initial_mouse_pos;
	Vector2 snapped_delta;
	Wire::Elbow elbow = Wire::Elbow::HorizontalFirst; // reroute bend, chosen from the drag direction
	bool elbowLocked = false;                         // held once committed; re-armed near the start
};

struct ConnectingContext {
	enum ConnectionType { PIN, JUNCTION, INPUT } type; // PIN = dragging from an output pin, INPUT = from a free input pin
	int sourceNodeID;
	int wireID;
	Vector2 sourcePos;
	WireSegment sourceSegment;

	int sourceComponentID;
	PinRef targetPin;
	int sourceInputIndex = -1;                        // which input pin the drag started from (type == INPUT)
	Wire::Elbow elbow = Wire::Elbow::HorizontalFirst; // which way the L-route bends
	bool elbowLocked = false;                         // held once set; re-armed by returning to the source
};

struct SelectionContext {
	Vector2 selectionStart;
	Vector2 selectionEnd;
	Rectangle selectionRect;
};


class Sheet

{
public:
	Sheet();
	
	void HandleInput();
	void Update(float deltaTime);
	void UI();
	void Draw();

	void SetViewport(Rectangle viewport);
	void SetMouseInputs(MouseInputs input) { mouse_inputs = input; }
	Rectangle GetViewport() const { return viewport; }
	const Camera2D& GetCamera() const { return camera; }
	Circuit& GetCircuit() { return circuit; }

private:
	void DrawGrid() const;

	void UpdateIdleState();
	void UpdatePanningState();
	void UpdateDraggingState();
	void UpdateConnectingState();
	void UpdateSelectingState();
	std::vector<componentInfo > getNodeInfoCopy(std::vector<int>& ids);
	std::vector<componentInfo > getNodeInfoDeletion(std::vector<int>& ids);

	void clearSelection();


	Rectangle viewport;

	int cell_size = CELL_SIZE;

	enum class MouseState { Idle, Panning, Dragging, Connecting, Selecting };

	void(Sheet::* mouse_state_update_functions[5])() = {
		&Sheet::UpdateIdleState,
		&Sheet::UpdatePanningState,
		&Sheet::UpdateDraggingState,
		&Sheet::UpdateConnectingState,
		&Sheet::UpdateSelectingState
	};

	const std::string mouse_state_names[5] = { "Idle", "Panning", "Dragging", "Connecting", "Selecting" };

	Camera2D camera;

	const float min_zoom = 0.3f;
	const float max_zoom = 3.0f;
	float zoom_sensitivity = 0.1f;
	float current_zoom = 1.0f;


	MouseState current_mouse_state = MouseState::Idle;
	componentInfo::Type selected_component_type = componentInfo::Type::AND;

	PanningContext panning_context;
	DraggingContext dragging_context;
	ConnectingContext connecting_context;
	SelectionContext selecting_context;

	Circuit circuit;

	bool gate_placed = false;
	bool space_was_pressed = false;

	int major_step = 100 / CELL_SIZE;
	float grid_line_minor_thickness = 0.8f;
	float grid_line_major_thickness = 1.5f;

	std::vector <int> selected_component_ids;
	int hovered_component_id = -1;
	std::unordered_map<int, std::vector<int>> selected_wire_nodes;
	std::unordered_map<int, std::vector<WireSegment>> selected_wire_segments;

	bool is_simulation_running = false;
	float ticks_per_second = 0.5f;
	float time_since_last_tick = 0.0f;
	int number_of_ticks = 0;

	std::vector<componentInfo> copy_of_components;
	std::vector<Wire> copy_of_wires;
	Vector2 copy_center = { 0, 0 };

	ActionManager action_manager;

	UITheme darkTheme = {
	{ 18, 18, 18, 255 },
	{ 30, 30, 30, 255 },
	{ 50, 50, 50, 255 },
	{ 40, 60, 40, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 165, 0, 255 }
	};

	MouseInputs mouse_inputs;
};