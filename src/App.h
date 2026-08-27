#pragma once
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "Sheet.h"

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
	void Draw();

	void GetMouseInputs(MouseInputs& mouse_inputs);

	int window_width = 800;
	int window_height = 450;

	Sheet sheet;
	Clipboard clipboard;

	UITheme darkTheme = {
	{ 18, 18, 18, 255 },
	{ 30, 30, 30, 255 },
	{ 50, 50, 50, 255 },
	{ 40, 60, 40, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 165, 0, 255 }
	};
};
