#include "App.h"

App::App()
{
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Latch");
    MaximizeWindow();
    ClearWindowState(FLAG_WINDOW_HIDDEN);

	sheet.SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
    
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
    mouse_inputs.mousePositionWorld = GetScreenToWorld2D(GetMousePosition(), sheet.GetCamera());
    mouse_inputs.mouseWheelDelta = GetMouseWheelMove();
}

void App::HandleInput()
{
    if (IsWindowResized())
		sheet.SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });

    
	MouseInputs mouse_inputs;
	GetMouseInputs(mouse_inputs);
	sheet.SetInputs(mouse_inputs, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
	sheet.HandleInput();

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (IsKeyPressed(KEY_SPACE))
        sheet.StepSimulation();

    if (IsKeyPressed(KEY_DELETE))
        sheet.DeleteSelected();

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
        if (IsKeyPressed(KEY_C))        sheet.CopySelected();
        else if (IsKeyPressed(KEY_V))   sheet.Paste();

        if (IsKeyPressed(KEY_Z))        sheet.Undo();
        else if (IsKeyPressed(KEY_Y))   sheet.Redo();
    }
}

void App::Update(float deltaTime)
{
	sheet.Update(deltaTime);
}

void App::UI() 
{
    rlImGuiBegin();
	sheet.UI();
    rlImGuiEnd();
}

void App::Draw() 
{ 
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
