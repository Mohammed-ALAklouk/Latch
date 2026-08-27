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

void App::HandleInput()
{
    if (IsWindowResized())
		sheet.SetViewport({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });

    MouseInputs mouse_inputs = {
        IsMouseButtonDown(MOUSE_BUTTON_LEFT),
        IsMouseButtonDown(MOUSE_BUTTON_RIGHT),
		GetMousePosition(),
        GetScreenToWorld2D(GetMousePosition(), sheet.GetCamera()),
        GetMouseWheelMove()
	};
	sheet.SetMouseInputs(mouse_inputs);
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
