#pragma once
#include <math.h>
#include <string>
#include <unordered_map>

#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "Action.h"
#include "Circuit.h"

struct PanningContext {
	Vector2 initial_pos;
	Vector2 initial_camera_target;
};

struct DraggingContext {
	Vector2 initial_mouse_pos;
	Vector2 snapped_delta;
};

struct ConnectingContext {
	enum ConnectionType {PIN, JUNCTION} type;
	int sourceNodeID;
	int wireID;
	Vector2 sourcePos;
	WireSegment sourceSegment;

	int sourceComponentID;
	PinRef targetPin;
	Wire::Elbow elbow = Wire::Elbow::HorizontalFirst; // which way the L-route bends
	bool elbowLocked = false;                         // held once set; re-armed by returning to the source
};

struct SelectionContext {
	Vector2 selectionStart;
	Vector2 selectionEnd;
	Rectangle selectionRect;
};

struct UITheme {
	Color background;
	Color gridMinor;
	Color gridMajor;
	Color logicLow;    // Logic 0
	Color logicHigh;   // Logic 1
	Color logicX;      // Undefined
};

class App
{
public:
	App();
	~App();
	void Run();
	
private:
	void HandleInput();
	void Update(float deltaTime);
	void UI();
	void Draw() ;

	void DrawGrid() const;
	
	void UpdateIdleState(const Vector2& mouse_pos);
	void UpdatePanningState(const Vector2& mouse_pos);
	void UpdateDraggingState(const Vector2& mouse_pos);
	void UpdateConnectingState(const Vector2& mouse_pos);
	void UpdateSelectingState(const Vector2& mouse_pos);
	std::vector<NodeInfo > getNodeInfoCopy(std::vector<int>& ids);
	std::vector<NodeInfo > getNodeInfoDeletion(std::vector<int>& ids);

	void clearSelection();
	
	int cell_size = CELL_SIZE;
	int window_width = 800;
	int window_height = 450;

	enum class MouseState { Idle, Panning, Dragging, Connecting, Selecting } ;

	void(App::* mouse_state_update_functions[5])(const Vector2& mouse_pos) = {
		&App::UpdateIdleState,
		&App::UpdatePanningState,
		&App::UpdateDraggingState,
		&App::UpdateConnectingState,
		&App::UpdateSelectingState
	};

	const std::string mouse_state_names[5] = { "Idle", "Panning", "Dragging", "Connecting", "Selecting" };

	Camera2D camera;
	
	const float min_zoom = 0.3f;
	const float max_zoom = 3.0f;
	float zoom_sensitivity = 0.1f;
	float current_zoom = 1.0f;

	
	MouseState current_mouse_state = MouseState::Idle;
	NodeInfo::Type selected_component_type = NodeInfo::Type::AND;

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

	std::vector<NodeInfo > copy_of_components;
	ActionManager action_manager;



	UITheme darkTheme = {
	{ 18, 18, 18, 255 },
	{ 30, 30, 30, 255 },
	{ 50, 50, 50, 255 },
	{ 40, 60, 40, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 165, 0, 255 }
	};
};
