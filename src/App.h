#pragma once
#include <memory>
#include <vector>

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
	Sheet& GetCurrentSheet() { return *sheets[current_sheet_index]; }

	int window_width = 800;
	int window_height = 450;

	std::vector<std::unique_ptr<Sheet>> sheets;
	int current_sheet_index = 0;
	Clipboard clipboard;

	char new_sheet_title[64] = ""; 

	componentInfo::Type selected_gate_type = componentInfo::Type::AND;

	UITheme darkTheme = {
	{ 18, 18, 18, 255 },
	{ 30, 30, 30, 255 },
	{ 50, 50, 50, 255 },
	{ 40, 60, 40, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 165, 0, 255 }
	};
};
