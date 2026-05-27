#include "App.h"

App::App()
{
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "GateSimulator");
    MaximizeWindow();
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    
    SetTargetFPS(60);
    rlImGuiSetup(true);

    panning_context = { 0, 0 };
    dragging_context = { {0, 0} };
    connecting_context = { -1, { -1, -1 } };
    
    camera = { 0 };
    camera.offset = Vector2{ window_width / 2.0f, window_height / 2.0f };
    camera.zoom = 1;
}

App::~App()
{
    rlImGuiShutdown();
    CloseWindow();
}

void App::HandleInput()
{
    if (IsWindowResized())
    {
		window_height = GetScreenHeight();
		window_width = GetScreenWidth();
        camera.offset = Vector2{ window_width / 2.0f, window_height / 2.0f };
    }

    auto zoom_change = GetMouseWheelMove();
    if (zoom_change)
    {
        int index = current_zoom_index + zoom_change;
        int max_zoom_index = sizeof(zoom_levels) / sizeof(zoom_levels[0]);
        if (index >= 0 && index < max_zoom_index) {
            current_zoom_index = index;
            camera.zoom = zoom_levels[current_zoom_index];
        }
    }

    if (IsMouseButtonDown(MouseButton::MOUSE_BUTTON_RIGHT) && !gate_placed)
    {
        auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
		int new_id = circuit.addComponent(selected_component_type, world_mouse_pos);
        action_manager.addAction<ComponentPlacedAction>(new_id, NodeInfo{selected_component_type, world_mouse_pos, {}});
        gate_placed = true;
    }

    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_RIGHT))
        gate_placed = false;

    if(IsKeyPressed(KEY_SPACE))
		circuit.evaluate();

    if (IsKeyPressed(KEY_DELETE)) {
        action_manager.addAction<ComponentsDeletedAction>(getNodeInfoDeletion(selected_component_ids), selected_component_ids);
		
        for (int id : selected_component_ids) {
            circuit.removeComponent(id);
        }

        if (selected_wire_id != -1) {
            circuit.removeWire(selected_wire_id);
        }
        

        selected_component_ids.clear();
        selected_wire_id = -1;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_C)) {
            copy_of_components = getNodeInfoCopy(selected_component_ids);
            
			Vector2 min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			Vector2 max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

            for (const auto& component : copy_of_components) {
                if (component.position.x < min.x) min.x = component.position.x;
                if (component.position.y < min.y) min.y = component.position.y;
                if (component.position.x > max.x) max.x = component.position.x;
                if (component.position.y > max.y) max.y = component.position.y;
			}

			auto mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);

			// Center the copied components around the origin
            for (auto& component : copy_of_components) {
                component.position.x -= (min.x + max.x) / 2.0f;
                component.position.y -= (min.y + max.y) / 2.0f;
            }
        }
        if (IsKeyPressed(KEY_V)) {
            std::vector<int> new_ids;
			std::vector<NodeInfo> pasted_components = copy_of_components;
			auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
            for (auto& component : pasted_components) {
				component.position.x += world_mouse_pos.x;
				component.position.y += world_mouse_pos.y;

                int new_id = circuit.addComponent(component.type, component.position);
				new_ids.push_back(new_id);
            }
            

            for (int i = 0; i < pasted_components.size(); ++i) {
                auto& component = pasted_components[i];
                int new_id = new_ids[i];
				int index = 0;
                for (int& input_component : component.input_components) {
                    if (input_component != -1) {
                        input_component = new_ids[input_component];
                        circuit.addWire({ input_component, 0 }, { new_id, index });
                    }
					++index;
                }
			}

            action_manager.addAction<PasteAction>(new_ids, pasted_components);
            selected_component_ids = new_ids;
		}

        if (IsKeyPressed(KEY_Z)) {
			action_manager.undo(circuit);
        }

        if (IsKeyPressed(KEY_Y)) {
			action_manager.redo(circuit);
        }
    }
}

void App::Update(float deltaTime)
{
	auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
	auto mouse_state_update = mouse_state_update_functions[static_cast<int>(current_mouse_state)];
	(this->*mouse_state_update)(world_mouse_pos);

    if (is_simulation_running)
	{
		time_since_last_tick += deltaTime;
		if (time_since_last_tick >= 1.0f / ticks_per_second)
		{
			circuit.evaluate();
			time_since_last_tick -= 1.0f / ticks_per_second;
			number_of_ticks++;
		}
	}
}

void App::UI() 
{
    rlImGuiBegin();
    ImGui::Begin("Components");

    if (ImGui::Button("AND"))
        selected_component_type = NodeInfo::Type::AND;
    if (ImGui::Button("OR"))
        selected_component_type = NodeInfo::Type::OR;
    if (ImGui::Button("NOT"))
        selected_component_type = NodeInfo::Type::NOT;
    if (ImGui::Button("HIGH"))
        selected_component_type = NodeInfo::Type::HIGH;
    if (ImGui::Button("LOW"))
        selected_component_type = NodeInfo::Type::LOW;
    ImGui::End();

	ImGui::Begin("Simulation");
    if (ImGui::Button("Step"))
    {
		circuit.evaluate();
		number_of_ticks++;
    }

	if (ImGui::Button(is_simulation_running ? "Stop" : "Run"))
		is_simulation_running = !is_simulation_running;
    
    if (ImGui::SliderFloat("Ticks per second", &ticks_per_second, 0.5f, 60.0f, "%.1f"))
    {
        time_since_last_tick = 0.0f; 
	}

	ImGui::Text("Ticks: %d", number_of_ticks);

	ImGui::End();

	ImGui::Begin("Selected Component Info");
	
	if (selected_component_ids.size() == 1) {
		int componentID = selected_component_ids[0];
		auto& component = circuit.getComponent(componentID);
		ImGui::Text("Component ID: %d", componentID);
		ImGui::Text("Type: %d", static_cast<int>(component->getNodeInfoType()));
		ImGui::Text("Position: (%.2f, %.2f)", component->m_rect.x, component->m_rect.y);
		ImGui::Text("Input Wires:");
		for (int i = 0; i < component->m_inputWireIds.size(); ++i) {
			ImGui::Text("  Input %d: %d", i, component->m_inputWireIds[i]);
		}
	}
    
    ImGui::End();

    rlImGuiEnd();
}

void App::Draw() 
{ 
    BeginDrawing();

    BeginMode2D(camera);
    ClearBackground(darkTheme.background);

    DrawGrid();

	circuit.draw(selected_component_ids, hovered_component_id);

    if (current_mouse_state == MouseState::Connecting)
    {
		auto& inputComponent = circuit.getComponent(connecting_context.sourceComponentID);
        Vector2 start = inputComponent->getOutputPosition();
        Vector2 end = GetScreenToWorld2D(GetMousePosition(), camera);

        DrawLineEx(start, end, 3, LogicLevelColors[inputComponent->m_outputValues[0]]);
    }
    else if (current_mouse_state == MouseState::Selecting)
    {
		DrawRectangleRec(selecting_context.selectionRect, { 255, 0, 0, 50 });
        DrawRectangleLinesEx(selecting_context.selectionRect, 2, RED);
    }

	auto top_left = GetScreenToWorld2D({ 0, 0 }, camera);
    std::string state_text = mouse_state_names[static_cast<int>(current_mouse_state)];
    DrawText(("State: " + state_text).c_str(), top_left.x, top_left.y, 20, RED);

    EndMode2D();

    UI();
    EndDrawing();
}

void App::DrawGrid() const     
{
    Vector2 min = GetScreenToWorld2D({ 0, 0 }, camera);
    Vector2 max = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera);

    float startX = floor(min.x / cell_size) * cell_size;
    float startY = floor(min.y / cell_size) * cell_size;

    for (float x = startX; x <= max.x; x += cell_size) {
        int lineIndex = (int)round(x / cell_size);
        bool isMajor = (lineIndex % major_step == 0);

        Color color = isMajor ? darkTheme.gridMajor : darkTheme.gridMinor;

        float baseThickness = isMajor ? grid_line_major_thickness : grid_line_minor_thickness;
        float adjustedThickness = (baseThickness / camera.zoom);
        if (adjustedThickness < 1.0f / camera.zoom) adjustedThickness = 1.0f / camera.zoom;

        DrawLineEx({ x, min.y }, { x, max.y }, adjustedThickness, color);
    }
    
    for (float y = startY; y <= max.y; y += cell_size) {
        int lineIndex = (int)round(y / cell_size);
        bool isMajor = (lineIndex % major_step == 0);

        Color color = isMajor ? darkTheme.gridMajor : darkTheme.gridMinor;

        float baseThickness = isMajor ? grid_line_major_thickness : grid_line_minor_thickness;
        float adjustedThickness = (baseThickness / camera.zoom);
        if (adjustedThickness < 1.0f / camera.zoom) adjustedThickness = 1.0f / camera.zoom;

        DrawLineEx({ min.x, y }, { max.x, y }, adjustedThickness, color);
    }
}

void App::UpdateIdleState(const Vector2& world_mouse_pos)
{
    bool is_button_down = IsMouseButtonDown(MouseButton::MOUSE_BUTTON_LEFT);
    if (is_button_down)
    {
        if (IsKeyDown(KeyboardKey::KEY_LEFT_SHIFT))
        {
            selecting_context.selectionStart = world_mouse_pos;
            selecting_context.selectionEnd = world_mouse_pos;
            current_mouse_state = MouseState::Selecting;
            return;
        }


        bool clicked_on_input_pin = false;
        bool clicked_on_component = false;

        for (auto& component : circuit.m_components) {
            if (component->outputPinContainsPoint(world_mouse_pos) != -1)
            {
				connecting_context.sourceComponentID = component->m_id;
                connecting_context.targetPin = { 0, 0 };
                current_mouse_state = MouseState::Connecting;
                clicked_on_input_pin = true;
                break;
            }

        }

        if (clicked_on_input_pin) return;

        for (auto& component : circuit.m_components) {
            if (component->containsPoint(world_mouse_pos)) {
                current_mouse_state = MouseState::Dragging;
                dragging_context.initial_mouse_pos = world_mouse_pos;
                clicked_on_component = true;

                if (std::find(selected_component_ids.begin(), selected_component_ids.end(), component->m_id) == selected_component_ids.end())
                {
                    selected_component_ids.clear();
                    selected_component_ids.push_back(component->m_id);
                }

                break;
            }
        }

        if (clicked_on_component) return;

        selected_component_ids.clear();
        selected_wire_id = -1;
        current_mouse_state = MouseState::Panning;
        panning_context.initial_pos = GetMousePosition();
        panning_context.initial_camera_target = camera.target;
    }
    else 
    {
        hovered_component_id = -1;
        for (auto& component : circuit.m_components) {
            if (component->containsPoint(world_mouse_pos)) {
                hovered_component_id = component->m_id;
                break;
            }
        }
	}

}

void App::UpdatePanningState(const Vector2& world_mouse_pos)
{
    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_LEFT)) {
        current_mouse_state = MouseState::Idle;
        panning_context = { {0, 0}, {0, 0} };
        return;
    }

	auto screen_mouse_pos = GetMousePosition();

    Vector2 delta = {
        panning_context.initial_pos.x - screen_mouse_pos.x,
        panning_context.initial_pos.y - screen_mouse_pos.y
    };

    camera.target = Vector2{
        panning_context.initial_camera_target.x + (delta.x / zoom_levels[current_zoom_index]),
        panning_context.initial_camera_target.y + (delta.y / zoom_levels[current_zoom_index])
    };
}

void App::UpdateDraggingState(const Vector2& world_mouse_pos)
{
    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_LEFT)) {
        action_manager.addAction<ComponentsMovedAction>(selected_component_ids, dragging_context.delta);
		current_mouse_state = MouseState::Idle;
        dragging_context = { {0, 0}, {0, 0} };

        return;
    }
    
    Vector2 delta = {
        world_mouse_pos.x - dragging_context.initial_mouse_pos.x,
        world_mouse_pos.y - dragging_context.initial_mouse_pos.y
    };

	dragging_context.delta.x += delta.x;
	dragging_context.delta.y += delta.y;

    dragging_context.initial_mouse_pos = world_mouse_pos;

    for (int id : selected_component_ids) {
        auto& component = circuit.getComponent(id);
        auto position = component->m_rect;
        component->m_rect.x = position.x + delta.x;
        component->m_rect.y = position.y + delta.y;
    }
}

void App::UpdateConnectingState(const Vector2& world_mouse_pos)
{
    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_LEFT)) {
        if (connecting_context.targetPin.ComponentID != -1) {
            
            int id = circuit.addWire({ connecting_context.sourceComponentID, 0 }, connecting_context.targetPin);		    
            action_manager.addAction<WirePlacedAction>(id, PinRef{ connecting_context.sourceComponentID, 0 }, connecting_context.targetPin);
        }

        connecting_context = { -1, { -1, -1 } };
        current_mouse_state = MouseState::Idle;
        return;
    }

    bool found_target = false;
    for (auto& Component : circuit.m_components) {
        auto input_pin_index = Component->inputPinsContainPoint(world_mouse_pos);

        if (input_pin_index != -1 && Component->getInputWireId(input_pin_index) == -1) {
            connecting_context.targetPin = { Component->m_id, input_pin_index };
            found_target = true;
            return;
        }
    }

    if (!found_target)
        connecting_context.targetPin = { -1, -1 };
}

void App::UpdateSelectingState(const Vector2& world_mouse_pos)
{
    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_LEFT)) {
        current_mouse_state = MouseState::Idle;
        selecting_context = { {0, 0}, {0, 0}, {0, 0, 0, 0} };
        return;
    }

    selecting_context.selectionEnd = world_mouse_pos;

    Vector2 topLeft = {
        std::min(selecting_context.selectionStart.x, selecting_context.selectionEnd.x),
        std::min(selecting_context.selectionStart.y, selecting_context.selectionEnd.y)
    };

    Vector2 size = {
        std::abs(selecting_context.selectionEnd.x - selecting_context.selectionStart.x),
        std::abs(selecting_context.selectionEnd.y - selecting_context.selectionStart.y)
    };
    selecting_context.selectionRect = { topLeft.x, topLeft.y, size.x, size.y };

    selected_component_ids.clear();
    circuit.selectComponentsInArea(selecting_context.selectionRect, selected_component_ids);
}

std::vector<NodeInfo> App::getNodeInfoCopy(std::vector<int>& ids)
{
	std::vector<NodeInfo> nodes;
    for (int id : ids) {
        auto& component = circuit.getComponent(id);
        NodeInfo node_info = { component->getNodeInfoType(), {component->m_rect.x, component->m_rect.y}};
        nodes.push_back(node_info);
    }

    int index = 0;
    for (int id : ids) {

        auto& component = circuit.getComponent(id);
        for (int input_wire_id : component->m_inputWireIds) {
            if (input_wire_id == -1) {
                nodes[index].input_components.push_back(-1);
                continue;
            }

            Wire& wire = circuit.getWire(input_wire_id);
            int source_component_id = wire.Source.ComponentID;
            if (std::find(selected_component_ids.begin(), selected_component_ids.end(), source_component_id) != selected_component_ids.end()) {
                int source_index = std::distance(selected_component_ids.begin(), std::find(selected_component_ids.begin(), selected_component_ids.end(), source_component_id));
                nodes[index].input_components.push_back(source_index);
            }
            else {
                nodes[index].input_components.push_back(-1);
            }

        }

        index++;
    }

	return nodes;
}

std::vector<NodeInfo> App::getNodeInfoDeletion(std::vector<int>& ids)
{
    std::vector<NodeInfo> nodes;
    for (int id : ids) {
        auto& component = circuit.getComponent(id);
		std::vector<int> input_component_ids;
        for (int input_wire_id : component->m_inputWireIds) {
            if (input_wire_id == -1) {
                input_component_ids.push_back(-1);
                continue;
            }

            Wire& wire = circuit.getWire(input_wire_id);
            int source_component_id = wire.Source.ComponentID;
            input_component_ids.push_back(source_component_id);
		}

        NodeInfo node_info = { component->getNodeInfoType(), {component->m_rect.x, component->m_rect.y}, input_component_ids};
        nodes.push_back(node_info);
    }
    return nodes;
}

void App::Run()
{
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        HandleInput();
        Update(deltaTime);
        Draw();
    }
}
