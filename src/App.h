#pragma once
#include <math.h>
#include <string>

#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "LogicNode.h"
#include "Circuit.h"
#include "Action.h"

struct PanningContext {
	Vector2 initial_pos;
	Vector2 initial_camera_target;
};

struct DraggingContext {
	Vector2 initial_mouse_pos;
	Vector2 delta;
};

struct ConnectingContext {
	int sourceComponentID;
	PinRef targetPin;
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
	std::vector<NodeInfo> getNodeInfo(std::vector<int>& ids);
	std::vector<NodeInfo> getNodeInfoDeletion(std::vector<int>& ids);

	
	int cell_size = 20;
	int window_width = 800;
	int window_height = 450;

	typedef enum { Idle, Panning, Dragging, Connecting, Selecting } MouseState;

	void(App::* mouse_state_update_functions[5])(const Vector2& mouse_pos) = {
		&App::UpdateIdleState,
		&App::UpdatePanningState,
		&App::UpdateDraggingState,
		&App::UpdateConnectingState,
		&App::UpdateSelectingState
	};

	const std::string mouse_state_names[5] = { "Idle", "Panning", "Dragging", "Connecting", "Selecting" };

	Camera2D camera;
	const float zoom_levels[6] = { 0.33f, 0.45f, 0.60f, 0.75f, 0.90f, 1.00f };
	int current_zoom_index = 5; 

	MouseState current_mouse_state = Idle;
	Component::Type selected_component_type = Component::Type::AND;

	PanningContext panning_context;
	DraggingContext dragging_context;
	ConnectingContext connecting_context;
	SelectionContext selecting_context;

	Circuit circuit;

	bool gate_placed = false;
	bool space_was_pressed = false;

	int major_step = 5;
	float grid_line_minor_thinkness = 0.8f;
	float grid_line_major_thinkness = 1.5f;

	std::vector <int> selected_component_ids;
	int hovered_component_id = -1;
	int selected_wire_id = -1;

	bool is_simulation_running = false;
	float ticks_per_second = 0.5f;
	float time_since_last_tick = 0.0f;
	int number_of_ticks = 0;

	std::vector<NodeInfo> copy_of_components;
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
