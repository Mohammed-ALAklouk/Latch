#include "App.h"

App::App()
{
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Latch");
    MaximizeWindow();
    ClearWindowState(FLAG_WINDOW_HIDDEN);

	sheets.push_back(std::make_unique<Sheet>());
	GetCurrentSheet().SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
    
    SetTargetFPS(60);
    rlImGuiSetup(true);
}

App::~App()
{
    rlImGuiShutdown();
    CloseWindow();
}

void App::GetMouseInputs(MouseInputs& mouse_inputs)
{
    mouse_inputs.leftButtonDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    mouse_inputs.rightButtonDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    mouse_inputs.mousePositionScreen = GetMousePosition();
    mouse_inputs.mousePositionWorld = GetScreenToWorld2D(GetMousePosition(), GetCurrentSheet().GetCamera());
    mouse_inputs.mouseWheelDelta = GetMouseWheelMove();
}

void App::HandleInput()
{
    if (IsWindowResized()) {
        for (auto& sheet : sheets) {
		    sheet->SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
        }
    }

    
	Sheet& sheet = GetCurrentSheet();
	MouseInputs mouse_inputs;
	GetMouseInputs(mouse_inputs);
	sheet.SetInputs(mouse_inputs, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), selected_gate_type);
	sheet.HandleInput();

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (IsKeyPressed(KEY_SPACE))
        sheet.StepSimulation();

    if (IsKeyPressed(KEY_DELETE))
        sheet.DeleteSelected();

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
        if (IsKeyPressed(KEY_C))        sheet.CopySelected(clipboard);
        else if (IsKeyPressed(KEY_V))   sheet.Paste(clipboard);

        if (IsKeyPressed(KEY_Z))        sheet.Undo();
        else if (IsKeyPressed(KEY_Y))   sheet.Redo();
    }
}

void App::Update(float deltaTime)
{
	GetCurrentSheet().Update(deltaTime);
}

void App::UI() 
{
    rlImGuiBegin();

    ImGui::Begin("Components");

    if (ImGui::Button("AND"))
        selected_gate_type = componentInfo::Type::AND;
    if (ImGui::Button("NAND"))
        selected_gate_type = componentInfo::Type::NAND;
    if (ImGui::Button("OR"))
        selected_gate_type = componentInfo::Type::OR;
    if (ImGui::Button("NOR"))
        selected_gate_type = componentInfo::Type::NOR;
    if (ImGui::Button("XOR"))
        selected_gate_type = componentInfo::Type::XOR;
    if (ImGui::Button("XNOR"))
        selected_gate_type = componentInfo::Type::XNOR;
    if (ImGui::Button("NOT"))
        selected_gate_type = componentInfo::Type::NOT;
    if (ImGui::Button("LED"))
        selected_gate_type = componentInfo::Type::LED;
    if (ImGui::Button("TOGGLE"))
        selected_gate_type = componentInfo::Type::TOGGLE;
    ImGui::End();

    ImGui::Begin("Sheets");
	int index = 0;
    for (auto sheet = sheets.begin(); sheet != sheets.end();)
    {
		bool should_delete = ImGui::Button(("Delete##" + std::to_string(index)).c_str());
        
		ImGui::SameLine();

        if (ImGui::Button(((*sheet)->GetTitle() + "##" + std::to_string(index)).c_str()))
            current_sheet_index = index;

        if (current_sheet_index == index)
        {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Current");
		}
        
        if (should_delete && sheets.size() > 1)
        {
            sheet = sheets.erase(sheet);   
            if (current_sheet_index > index)          // strictly greater: only shift when above
                current_sheet_index--;
            else if (current_sheet_index == index)    // deleted the active sheet
                current_sheet_index = (index > 0) ? index - 1 : 0;


            continue;                      
        }
        index++;
		sheet++;
    }

	ImGui::InputText("Sheet Title", new_sheet_title, sizeof(new_sheet_title));
	ImGui::SameLine();
    if (ImGui::Button("Add Sheet"))
    {
        sheets.push_back(std::make_unique<Sheet>());
		sheets.back()->SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
        sheets.back()->SetTitle(new_sheet_title);
		if (new_sheet_title[0] == '\0')
            sheets.back()->SetTitle("Untitled Sheet" + std::to_string(sheets.size()));
        
		current_sheet_index = sheets.size() - 1;
		new_sheet_title[0] = '\0';   // clear the input for the next sheet
	}
	ImGui::End();

	GetCurrentSheet().UI();
    rlImGuiEnd();
}

void App::Draw() 
{ 
    Sheet& sheet = GetCurrentSheet();
    BeginDrawing();

    BeginMode2D(sheet.GetCamera());
    ClearBackground(darkTheme.background);
	sheet.Draw();

    EndMode2D();

    UI();
    EndDrawing();
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
