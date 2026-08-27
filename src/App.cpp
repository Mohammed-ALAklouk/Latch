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

void App::GetInputs(MouseInputs& mouse_inputs, KeyboardInputs& keyboard_inputs)
{
    mouse_inputs.leftButtonDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    mouse_inputs.rightButtonDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    mouse_inputs.mousePositionScreen = GetMousePosition();
    mouse_inputs.mousePositionWorld = GetScreenToWorld2D(GetMousePosition(), sheet.GetCamera());
    mouse_inputs.mouseWheelDelta = GetMouseWheelMove();

    keyboard_inputs.shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    keyboard_inputs.ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    keyboard_inputs.CPressed = IsKeyPressed(KEY_C);
    keyboard_inputs.VPressed = IsKeyPressed(KEY_V);
    keyboard_inputs.ZPressed = IsKeyPressed(KEY_Z);
    keyboard_inputs.YPressed = IsKeyPressed(KEY_Y);
    keyboard_inputs.deletePressed = IsKeyPressed(KEY_DELETE);
    keyboard_inputs.spacePressed = IsKeyPressed(KEY_SPACE);
}

void App::HandleInput()
{
    if (IsWindowResized())
		sheet.SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });

    
	MouseInputs mouse_inputs;
	KeyboardInputs keyboard_inputs;
	GetInputs(mouse_inputs, keyboard_inputs);
	sheet.SetInputs(mouse_inputs, keyboard_inputs);
	sheet.HandleInput();
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
