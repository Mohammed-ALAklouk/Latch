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
	connecting_context = { ConnectingContext::PIN, -1, -1, {0, 0}, {-1, -1}, -1, {-1, -1} };
    
    camera = { 0 };
    camera.offset = Vector2{ window_width / 2.0f, window_height / 2.0f };
    camera.zoom = 1;

	action_manager.AddSnapshot(circuit.GetSnapshot("Initial state"));
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
		current_zoom += zoom_change * zoom_sensitivity;
		if (current_zoom < min_zoom) current_zoom = min_zoom;
		else if (current_zoom > max_zoom) current_zoom = max_zoom;
        camera.zoom = current_zoom;
    }

    if (IsMouseButtonDown(MouseButton::MOUSE_BUTTON_RIGHT) && !gate_placed)
    {
        auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
		world_mouse_pos.x = int(world_mouse_pos.x / cell_size) * cell_size;
		world_mouse_pos.y = int(world_mouse_pos.y / cell_size) * cell_size;
		int new_id = circuit.addComponent(selected_component_type, world_mouse_pos);
		auto& component = circuit.getComponent(new_id);
        action_manager.AddSnapshot(circuit.GetSnapshot("Added component"));
        gate_placed = true;
    }

    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_RIGHT))
        gate_placed = false;

    if(IsKeyPressed(KEY_SPACE))
		circuit.evaluate();

    if (IsKeyPressed(KEY_DELETE)) {
        for (int id : selected_component_ids) {
            circuit.removeComponent(id);
        }

        for (auto& pair : selected_wire_nodes) {
            int wireID = pair.first;
            if (circuit.wireExists(wireID))
                circuit.removeWireNodes(wireID, pair.second);
        }

        for (auto& pair : selected_wire_segments) {
            int wireID = pair.first;
            if (circuit.wireExists(wireID))
                circuit.removeWireSegments(wireID, pair.second);
		}

		clearSelection();
        action_manager.AddSnapshot(circuit.GetSnapshot("Deleted components and wires"));
    }

    if (IsKeyDown(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_C)) {
            Vector2 min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
            Vector2 max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

            copy_of_components = getNodeInfoCopy(selected_component_ids);
            copy_of_wires.clear();
            for (const auto& pair : selected_wire_nodes) {
                auto& wire = circuit.getWire(pair.first);
                copy_of_wires.push_back(wire);

                for (const auto& nodeID : pair.second) {
                    Vector2 nodePos = wire.getNodePosition(nodeID);
                    if (nodePos.x < min.x) min.x = nodePos.x;
                    if (nodePos.y < min.y) min.y = nodePos.y;
                    if (nodePos.x > max.x) max.x = nodePos.x;
                    if (nodePos.y > max.y) max.y = nodePos.y;
				}
            }
			
            for (const auto& component : copy_of_components) {
                if (component.position.x < min.x) min.x = component.position.x;
                if (component.position.y < min.y) min.y = component.position.y;
                if (component.position.x > max.x) max.x = component.position.x;
                if (component.position.y > max.y) max.y = component.position.y;
			}

			copy_center = { (min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f };
        }
        if (IsKeyPressed(KEY_V)) {
			Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
			Vector2 displacement = { mouse_pos.x - copy_center.x, mouse_pos.y - copy_center.y };

			circuit.paste(copy_of_wires, copy_of_components, displacement);
			action_manager.AddSnapshot(circuit.GetSnapshot("Pasted components and wires"));
		}

        if (IsKeyPressed(KEY_Z)) {
			action_manager.undo(circuit);
            clearSelection();
        }

        if (IsKeyPressed(KEY_Y)) {
			action_manager.redo(circuit);
            clearSelection();
        }
    }
}

void App::Update(float deltaTime)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
	    auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
	    auto mouse_state_update = mouse_state_update_functions[static_cast<int>(current_mouse_state)];
	    (this->*mouse_state_update)(world_mouse_pos);
    }

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
        selected_component_type = componentInfo::Type::AND;
    if (ImGui::Button("NAND"))
		selected_component_type = componentInfo::Type::NAND;
    if (ImGui::Button("OR"))
        selected_component_type = componentInfo::Type::OR;
	if (ImGui::Button("NOR"))
		selected_component_type = componentInfo::Type::NOR;
    if (ImGui::Button("XOR"))
		selected_component_type = componentInfo::Type::XOR;
	if (ImGui::Button("XNOR"))
		selected_component_type = componentInfo::Type::XNOR;
    if (ImGui::Button("NOT"))
        selected_component_type = componentInfo::Type::NOT;
    if (ImGui::Button("LED"))
		selected_component_type = componentInfo::Type::LED;
    if (ImGui::Button("TOGGLE")) 
		selected_component_type = componentInfo::Type::TOGGLE;
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
        ImGui::Text("Output Wires:");
        for (int i = 0; i < component->m_outputWireIds.size(); ++i) {
            ImGui::Text("  Output %d: %d", i, component->m_outputWireIds[i]);
        }
	}
    else if (selected_wire_nodes.size() == 1) {
		auto& pair = *selected_wire_nodes.begin();
        if (pair.second.size() == 1)
        {
		    int wireID = pair.first;
		    auto& wire = circuit.getWire(wireID);
		    ImGui::Text("Wire ID: %d", wireID);
            ImGui::Text("Node ID: %d", pair.second[0]);
		    ImGui::Text("Node Counter: %d", wire.Nodes.size());
		    ImGui::Text("Segment Counter: %d", wire.Segments.size());
        }
    }

    
    ImGui::End();

	ImGui::Begin("Snapshots");
	int index = 0;
    for (const auto& snapshot : action_manager.getSnapshots()) {
        if (index == action_manager.getCurrentIndex())
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Snapshot %d: %s", index, snapshot.note.c_str());
		else
            ImGui::Text("%s", snapshot.note.c_str());
		index++;
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

    // While dragging, hide the nodes the preview is about to redraw at a new
    // spot (moved source pins + dragged wire nodes) so the real geometry and the
    // preview don't render on top of each other. Input pin nodes stay -- the
    // reroute keeps them (as bends) and the preview only adds a leg.
    std::unordered_map<int, std::vector<int>> hidden_wire_nodes;
    if (current_mouse_state == MouseState::Dragging) {
        for (int id : selected_component_ids) {
            auto& component = circuit.getComponent(id);
            for (int j = 0; j < component->m_outputWireIds.size(); ++j) {
                int wireID = component->m_outputWireIds[j];
                if (wireID == -1) continue;
                int nodeID = circuit.getWire(wireID).findNodebyPin({ component->m_id, j });
                if (nodeID != -1) hidden_wire_nodes[wireID].push_back(nodeID);
            }
            for (int i = 0; i < component->m_inputWireIds.size(); ++i) {
                int wireID = component->m_inputWireIds[i];
                if (wireID == -1) continue;
                int nodeID = circuit.getWire(wireID).findNodebyPin({ component->m_id, i });
                if (nodeID != -1) hidden_wire_nodes[wireID].push_back(nodeID);
            }
        }
        for (auto& t : selected_wire_nodes)
            hidden_wire_nodes[t.first].insert(hidden_wire_nodes[t.first].end(), t.second.begin(), t.second.end());
    }

	circuit.draw(selected_component_ids, hovered_component_id, selected_wire_nodes, selected_wire_segments, hidden_wire_nodes);

    if (current_mouse_state == MouseState::Connecting)
    {
        Vector2 start;
		Color line_color;
		auto world_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
        Vector2 destination_pos = { round(world_mouse_pos.x / cell_size) * cell_size, round(world_mouse_pos.y / cell_size) * cell_size };

        if (connecting_context.type == ConnectingContext::ConnectionType::PIN)
        {
		    auto& sourceComponent = circuit.getComponent(connecting_context.sourceComponentID);
            start = sourceComponent->getOutputPosition();
			line_color = LogicLevelColors[sourceComponent->m_outputValues[0]];
        }
        else if (connecting_context.type == ConnectingContext::ConnectionType::INPUT)
        {
            auto& sourceComponent = circuit.getComponent(connecting_context.sourceComponentID);
            start = sourceComponent->getInputPosition(connecting_context.sourceInputIndex);
            line_color = LogicLevelColors[LogicLevel::UNDEFINED]; // nothing drives it until it lands on an output
        }
        else if (connecting_context.sourceNodeID != -1){
            auto& sourceWire = circuit.getWire(connecting_context.wireID);
			start = sourceWire.getNodePosition(connecting_context.sourceNodeID);
            line_color = LogicLevelColors[sourceWire.Value];
        }
        else {
            auto& sourceWire = circuit.getWire(connecting_context.wireID);
            start = connecting_context.sourcePos;
            line_color = LogicLevelColors[sourceWire.Value];
        }

        auto route = Wire::routePoints(start, destination_pos, connecting_context.elbow);
        for (int i = 0; i < route.size() - 1; i++) 
            DrawLineEx(route[i], route[i+1], 3, line_color);

        if (route.size() == 3) 
			DrawCircleV(route[1], 5, line_color);
    }
    else if (current_mouse_state == MouseState::Dragging)
    {
        auto drawLeg = [](const std::vector<Vector2>& pts, Color c) {
            for (size_t k = 0; k + 1 < pts.size(); ++k)
                DrawLineEx(pts[k], pts[k + 1], 3, c);
        };

        // Preview the reroute for every wire attached to a moved gate. Mirrors
        // UpdateDraggingState's release path, but reads geometry only: same
        // alreadyMoved skip, and neighbors that are dragged along anchor at
        // their post-move positions (the commit translates them first).
        static const std::vector<int> no_nodes;
        auto selectedNodesOf = [&](int wireID) -> const std::vector<int>& {
            auto it = selected_wire_nodes.find(wireID);
            return it != selected_wire_nodes.end() ? it->second : no_nodes;
        };
        auto contains = [](const std::vector<int>& v, int x) {
            return std::find(v.begin(), v.end(), x) != v.end();
        };

        for (int id : selected_component_ids) {
            auto& component = circuit.getComponent(id);

            for (int j = 0; j < component->m_outputWireIds.size(); ++j) {
                int wireID = component->m_outputWireIds[j];
                if (wireID == -1) continue;
                auto& wire = circuit.getWire(wireID);
                int nodeID = wire.findNodebyPin({ component->m_id, j });
                if (nodeID == -1) continue;
                const auto& moving = selectedNodesOf(wireID);
                if (contains(moving, nodeID)) continue; // group preview covers it
                for (auto& leg : wire.previewMoveLegs(nodeID, component->getOutputPosition(j), dragging_context.elbow, moving, dragging_context.snapped_delta))
                    drawLeg(leg, LogicLevelColors[wire.Value]);
            }

            for (int i = 0; i < component->m_inputWireIds.size(); ++i) {
                int wireID = component->m_inputWireIds[i];
                if (wireID == -1) continue;
                auto& wire = circuit.getWire(wireID);
                int nodeID = wire.findNodebyPin({ component->m_id, i });
                if (nodeID == -1) continue;
                const auto& moving = selectedNodesOf(wireID);
                if (contains(moving, nodeID)) continue; // group preview covers it
                // input pin node moves to the new pin position (mirrors moveNodeWithElbow)
                for (auto& leg : wire.previewMoveLegs(nodeID, component->getInputPosition(i), dragging_context.elbow, moving, dragging_context.snapped_delta))
                    drawLeg(leg, LogicLevelColors[wire.Value]);
            }
        }

        // Selected wire nodes translate by the accumulated delta.
        for (auto& t : selected_wire_nodes) {
            if (!circuit.wireExists(t.first)) continue;
            auto& wire = circuit.getWire(t.first);
            for (auto& leg : wire.previewMoveGroupLegs(t.second, dragging_context.snapped_delta, dragging_context.elbow))
                drawLeg(leg, LogicLevelColors[wire.Value]);
        }
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


		bool clicked_on_wire_node = false;
		bool clicked_on_wire_segment = false;
        bool clicked_on_output_pin = false;
        bool clicked_on_component = false;

        for (auto& wire : circuit.m_wires) {
            int nodeID = wire.findNodeAt(world_mouse_pos);
            if (nodeID != -1) {
                connecting_context.type = ConnectingContext::JUNCTION;
                connecting_context.wireID = wire.ID;
                connecting_context.sourceNodeID = nodeID;
                current_mouse_state = MouseState::Connecting;

				clearSelection();

                if (selected_wire_nodes.find(wire.ID) == selected_wire_nodes.end())
                    selected_wire_nodes[wire.ID] = std::vector<int>();

                selected_wire_nodes[wire.ID].push_back(nodeID);

                clicked_on_wire_node = true;
                break;
            }
        }

        if (clicked_on_wire_node) return;

        for (auto& wire : circuit.m_wires) {
            WireSegment segment = wire.findSegmentAt(world_mouse_pos);
            if (segment.first != -1) {
				connecting_context.sourceSegment = segment;
				connecting_context.type = ConnectingContext::JUNCTION;
				connecting_context.wireID = wire.ID;
                connecting_context.sourcePos = { std::round(world_mouse_pos.x / cell_size) * cell_size, std::round(world_mouse_pos.y / cell_size) * cell_size };
				current_mouse_state = MouseState::Connecting;
                clearSelection();

                if (selected_wire_nodes.find(wire.ID) == selected_wire_nodes.end())
                    selected_wire_nodes[wire.ID] = std::vector<int>();
                
				selected_wire_segments[wire.ID].push_back(segment);
                clicked_on_wire_segment = true;
                break;
            }
		}

		if (clicked_on_wire_segment) return;

        for (auto& component : circuit.m_components) {
            if (component->outputPinContainsPoint(world_mouse_pos) != -1)
            {
				connecting_context.sourceComponentID = component->m_id;
                connecting_context.targetPin = { 0, 0 };
                current_mouse_state = MouseState::Connecting;
                clicked_on_output_pin = true;
                break;
            }

        }

        if (clicked_on_output_pin) return;

        // a free input pin can start a connection too; the drag runs input -> output
        bool clicked_on_input_pin = false;
        for (auto& component : circuit.m_components) {
            int input_index = component->inputPinsContainPoint(world_mouse_pos);
            if (input_index != -1 && component->getInputWireId(input_index) == -1) {
                connecting_context.type = ConnectingContext::INPUT;
                connecting_context.sourceComponentID = component->m_id;
                connecting_context.sourceInputIndex = input_index;
                connecting_context.targetPin = { -1, -1 };
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
				component->onClick();
                clicked_on_component = true;

                if (std::find(selected_component_ids.begin(), selected_component_ids.end(), component->m_id) == selected_component_ids.end())
                {
					clearSelection();
                    selected_component_ids.push_back(component->m_id);

                }

                break;
            }
        }

        if (clicked_on_component) return;

		clearSelection();
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
        panning_context.initial_camera_target.x + (delta.x / current_zoom),
        panning_context.initial_camera_target.y + (delta.y / current_zoom)
    };
}

void App::UpdateDraggingState(const Vector2& world_mouse_pos)
{
    if (IsMouseButtonUp(MouseButton::MOUSE_BUTTON_LEFT)) {
        for (auto t: selected_wire_nodes) {
            if (!circuit.wireExists(t.first)) continue;
            circuit.getWire(t.first).moveNodesWithElbow(t.second, dragging_context.snapped_delta, dragging_context.elbow);
		}

        for (int id: selected_component_ids) {
            auto& component = circuit.getComponent(id);
            if (dragging_context.snapped_delta.x == 0 && dragging_context.snapped_delta.y == 0) continue; // gate didn't move; nothing to reroute

            // output side: each output pin drives one wire; move its source node
            // (never demote -- it must stay the wire's source) and re-route with the same elbow.
            for (int j = 0; j < component->m_outputWireIds.size(); ++j) {
                int wireID = component->m_outputWireIds[j];
                if (wireID == -1) continue;
                auto& wire = circuit.getWire(wireID);
                int nodeID = wire.findNodebyPin({ component->m_id, j });
                if (nodeID == -1) continue;

                auto it = selected_wire_nodes.find(wireID);
                bool alreadyMoved = it != selected_wire_nodes.end() &&
                    std::find(it->second.begin(), it->second.end(), nodeID) != it->second.end();
                if (alreadyMoved) continue;

                wire.moveNodeWithElbow(nodeID, component->getOutputPosition(j), dragging_context.elbow);
            }

            for (int i = 0; i < component->m_inputWireIds.size(); ++i) {
                int wireID = component->m_inputWireIds[i];
                if (wireID != -1) {
                    auto& wire = circuit.getWire(wireID);
					int nodeID = wire.findNodebyPin({ component->m_id, i });
                    if (nodeID == -1) continue;

                    auto it = selected_wire_nodes.find(wireID);
                    bool alreadyMoved = it != selected_wire_nodes.end() &&
                        std::find(it->second.begin(), it->second.end(), nodeID) != it->second.end();
                    
                    if (alreadyMoved) continue;

					// if the component lands on a node we turn it into a junction, otherwise we just move the node to the new position
					int destinationNodeID = wire.findNodeAt(component->getInputPosition(i));
                    if (destinationNodeID != -1) {
						wire.promoteNodeToPin(destinationNodeID, { component->m_id, i });
                        wire.removeNodes({ nodeID });
                    }
                    else {
                        wire.moveNodeWithElbow(nodeID, component->getInputPosition(i), dragging_context.elbow);
                    }
                }
                else {
                    for (auto& wire : circuit.m_wires) {
                        for (auto segment: wire.Segments) {
                            if (component->m_inputWireIds[i] == -1 && wire.segmentContainsPoint(segment, component->getInputPosition(i))) {
								wire.splicePinIntoSegment({component->m_id, i}, component->getInputPosition(i));
                                component->m_inputWireIds[i] = wire.ID; // register the connection so evaluate() reads it
                                break;
                            }
						}
                        
                    }
                }
			}
		}

        action_manager.AddSnapshot(circuit.GetSnapshot("Moved components"));
		current_mouse_state = MouseState::Idle;
        dragging_context = { {0, 0}, {0, 0} };
        


        return;
    }
    
    Vector2 delta = {
        int(world_mouse_pos.x - dragging_context.initial_mouse_pos.x) / cell_size * cell_size - dragging_context.snapped_delta.x,
        int(world_mouse_pos.y - dragging_context.initial_mouse_pos.y) / cell_size * cell_size - dragging_context.snapped_delta.y
    };

	dragging_context.snapped_delta.x += delta.x;
	dragging_context.snapped_delta.y += delta.y;

    // Pick the reroute elbow from how you dragged, mirroring the connecting logic:
    // lock it once you commit to a direction, re-arm when you return near the start.
    Vector2 fromStart = { world_mouse_pos.x - dragging_context.initial_mouse_pos.x,
                          world_mouse_pos.y - dragging_context.initial_mouse_pos.y };
    float distSq = fromStart.x * fromStart.x + fromStart.y * fromStart.y;
    const float resetDist = (float)cell_size;      // back near the start -> re-arm
    const float armDist = (float)cell_size * 1.5f; // dragged this far -> lock the bend in

    if (distSq < resetDist * resetDist)
        dragging_context.elbowLocked = false;
    else if (!dragging_context.elbowLocked && distSq > armDist * armDist) {
        dragging_context.elbow = (fabsf(fromStart.x) > fabsf(fromStart.y))
            ? Wire::Elbow::HorizontalFirst // dragged mostly horizontally -> first leg horizontal
            : Wire::Elbow::VerticalFirst;  // dragged mostly vertically   -> first leg vertical
        dragging_context.elbowLocked = true;
    }

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
        Vector2 destination_pos = { round(world_mouse_pos.x / cell_size) * cell_size, round(world_mouse_pos.y / cell_size) * cell_size };
        
        if (connecting_context.type == ConnectingContext::JUNCTION) {
			auto& sourceWire = circuit.getWire(connecting_context.wireID);
            if (sourceWire.findNodeAt(destination_pos) == -1) {
                if (connecting_context.sourceNodeID != -1) {
                    circuit.extendWireTo(connecting_context.wireID, connecting_context.sourceNodeID, connecting_context.targetPin, destination_pos, connecting_context.elbow);
                }
                else {
                    circuit.extendWireTo(connecting_context.wireID, connecting_context.sourceSegment, connecting_context.sourcePos, connecting_context.targetPin, destination_pos, connecting_context.elbow);
                }

            }
        }
        else if (connecting_context.type == ConnectingContext::INPUT) {
            // Dragged from a free input pin. A wire is always driven by an output,
            // so the real route runs the opposite way to the drag; flipping the
            // elbow keeps the committed L identical to the previewed one.
            Wire::Elbow flipped = connecting_context.elbow == Wire::Elbow::HorizontalFirst
                ? Wire::Elbow::VerticalFirst : Wire::Elbow::HorizontalFirst;
            PinRef input_pin = { connecting_context.sourceComponentID, connecting_context.sourceInputIndex };
            Vector2 input_pos = circuit.getComponent(connecting_context.sourceComponentID)->getInputPosition(connecting_context.sourceInputIndex);

            if (connecting_context.targetPin.ComponentID != -1) {
                auto& driver = circuit.getComponent(connecting_context.targetPin.ComponentID);
                int existing_wire = driver->m_outputWireIds[connecting_context.targetPin.PinIndex];
                if (existing_wire != -1 && circuit.wireExists(existing_wire)) {
                    // output already drives a wire; branch it to this input instead of replacing it
                    auto& wire = circuit.getWire(existing_wire);
                    circuit.extendWireTo(existing_wire, wire.SourceNodeID, input_pin, input_pos, flipped);
                }
                else {
                    circuit.addWire(
                        connecting_context.targetPin,
                        driver->getOutputPosition(connecting_context.targetPin.PinIndex),
                        input_pin,
                        input_pos,
                        flipped);
                }
            }
            else {
                // released over an existing wire? tap it into this input
                for (auto& wire : circuit.m_wires) {
                    int nodeID = wire.findNodeAt(world_mouse_pos);
                    if (nodeID != -1) {
                        circuit.extendWireTo(wire.ID, nodeID, input_pin, input_pos, flipped);
                        action_manager.AddSnapshot(circuit.GetSnapshot("Extended wire to node"));
                        break;
                    }

                    WireSegment segment = wire.findSegmentAt(world_mouse_pos);
                    if (segment.first != -1) {
                        circuit.extendWireTo(wire.ID, segment, destination_pos, input_pin, input_pos, flipped);
						action_manager.AddSnapshot(circuit.GetSnapshot("Extended wire to segment"));
                        break;
                    }
                }
            }
        }
        else {
		    auto source_pos = circuit.getComponent(connecting_context.sourceComponentID)->getOutputPosition();
            if (source_pos.x != destination_pos.x || source_pos.y != destination_pos.y)
            {
                int id = circuit.addWire(
                    { connecting_context.sourceComponentID, 0 },
                    source_pos,
                    connecting_context.targetPin,
                    destination_pos,
                    connecting_context.elbow);
				action_manager.AddSnapshot(circuit.GetSnapshot("Added wire"));
            }
        }

        connecting_context = { ConnectingContext::PIN, -1, -1, {0, 0}, {-1, -1}, -1, {-1, -1} };
        current_mouse_state = MouseState::Idle;
        return;
    }

    // The elbow is set by the initial direction you drag away from the source,
    // then held. Returning near the source re-arms it, so the only way to change
    // the bend is to go back to the start and draw it again.
    Vector2 dragSource;
    if (connecting_context.type == ConnectingContext::PIN)
        dragSource = circuit.getComponent(connecting_context.sourceComponentID)->getOutputPosition();
    else if (connecting_context.type == ConnectingContext::INPUT)
        dragSource = circuit.getComponent(connecting_context.sourceComponentID)->getInputPosition(connecting_context.sourceInputIndex);
    else if (connecting_context.sourceNodeID != -1)
        dragSource = circuit.getWire(connecting_context.wireID).getNodePosition(connecting_context.sourceNodeID);
    else
        dragSource = connecting_context.sourcePos;

    Vector2 fromSource = { world_mouse_pos.x - dragSource.x, world_mouse_pos.y - dragSource.y };
    float distSq = fromSource.x * fromSource.x + fromSource.y * fromSource.y;
    const float resetDist = (float)cell_size;      // back near the source -> re-arm
    const float armDist = (float)cell_size * 1.5f; // dragged this far away -> lock the bend in

    if (distSq < resetDist * resetDist) {
        connecting_context.elbowLocked = false;
    }
    else if (!connecting_context.elbowLocked && distSq > armDist * armDist) {
        connecting_context.elbow = (fabsf(fromSource.x) > fabsf(fromSource.y))
            ? Wire::Elbow::HorizontalFirst // dragged away horizontally -> first segment horizontal
            : Wire::Elbow::VerticalFirst;  // dragged away vertically   -> first segment vertical
        connecting_context.elbowLocked = true;
    }

    bool found_target = false;
    for (auto& Component : circuit.m_components) {
        if (connecting_context.type == ConnectingContext::INPUT) {
            // dragging from an input: the thing we can land on is an output pin
            auto output_pin_index = Component->outputPinContainsPoint(world_mouse_pos);
            if (output_pin_index != -1) {
                connecting_context.targetPin = { Component->m_id, output_pin_index };
                found_target = true;
                return;
            }
        }
        else {
            auto input_pin_index = Component->inputPinsContainPoint(world_mouse_pos);
            if (input_pin_index != -1 && Component->getInputWireId(input_pin_index) == -1) {
                connecting_context.targetPin = { Component->m_id, input_pin_index };
                found_target = true;
                return;
            }
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

	clearSelection();
    circuit.selectInArea(selecting_context.selectionRect, selected_component_ids, selected_wire_nodes, selected_wire_segments);
}

std::vector<componentInfo> App::getNodeInfoCopy(std::vector<int>& ids)
{
	std::vector<componentInfo> nodes;
    for (int id : ids) {
        auto& component = circuit.getComponent(id);
        nodes.push_back(component->getComponentInfo());
    }
	return nodes;
}

std::vector<componentInfo> App::getNodeInfoDeletion(std::vector<int>& ids)
{
    std::vector<componentInfo> nodes;
    for (int id : ids) {
        auto& component = circuit.getComponent(id);
        nodes.push_back(component->getComponentInfo());
	}
    return nodes;
}

void App::clearSelection()
{
    selected_component_ids.clear();
    selected_wire_nodes.clear();
	selected_wire_segments.clear();
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
