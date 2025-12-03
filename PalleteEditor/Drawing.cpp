#include "Drawing.h"
#include "PalleteEditor.h"
#include "FileLoad.h"

LPCSTR Drawing::lpWindowName = "Skullgirls Pallete Editor 2nd Encore";
ImVec2 Drawing::vWindowSize = { 512, 512 };
ImGuiWindowFlags Drawing::WindowFlags = ImGuiWindowFlags_MenuBar;
bool Drawing::bDraw = true;
bool Drawing::show_about_window = false;

void Drawing::Active()
{
	bDraw = true;
}

bool Drawing::isActive()
{
	return bDraw == true;
}

void Drawing::Draw()
{
	if (isActive())
	{
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
		{
			//Меню-Бар
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Load Table"))
					{
						AddressTable::LoadFromFile();
					}
					if (PalEdit::bGameOpenned and PalEdit::bMatchStarted and (PalEdit::current_character_idx != -1)) {
						ImGui::Separator();
						if (ImGui::MenuItem("Save Pallete"))
						{
							PalleteFile::SaveToFile(
								PalEdit::Character_Vector[PalEdit::FindVectorIndexByID(PalEdit::current_character_idx)]

							);
						}
						if (ImGui::MenuItem("Load Pallete"))
						{
							PalleteFile::LoadFromFile(
								PalEdit::Character_Vector[PalEdit::FindVectorIndexByID(PalEdit::current_character_idx)]
							);
							PalEdit::ChangeAllColors();
							PalEdit::Read_Character();
						}
					}
					ImGui::EndMenu();
	
				}

				// Кнопка About прямо в меню баре
				if (ImGui::MenuItem("About"))
				{
					// Открываем окно About при нажатии
					show_about_window = true;
				}

				ImGui::EndMenuBar();
			}
				//Окно "About"
				if (show_about_window)
				{
					ImGui::Begin("About", &show_about_window);
					ImGui::Text("Skullgirls Pallete Editor 2nd Enocre");
					if (ImGui::Button("OK"))
					{
						show_about_window = false;
					}
					ImGui::End();
				}

				PalEdit::Init();

				if (PalEdit::bGameOpenned != true) {
					ImGui::Text("Open the game to use the editor.");

				}
				else {
					if (PalEdit::bMatchStarted != true) {
						ImGui::Text("Start a match to use the editor.\nIf this message persists, the table may be invalid.");
					}
					else {
						//Комбо-бокс выбора персонажа
						ImGui::Text("Select character:");
						const char* preview_text = "Select";
						for (const auto& character : PalEdit::Character_Vector) {
							if (character.ID == PalEdit::current_character_idx) {
								preview_text = character.Char_Name.c_str();
								break;
							}
						}

						if (ImGui::BeginCombo("##CharSelect", preview_text)) {
							for (int i = 0; i < PalEdit::Character_Vector.size(); i++) {
								// Сравниваем ID, а не индекс в векторе
								bool is_selected = (PalEdit::current_character_idx == PalEdit::Character_Vector[i].ID);

								std::string Display_Name = PalEdit::Character_Vector[i].Char_Name;
								if (PalEdit::Character_Vector[i].ID < 3) {
									Display_Name += " (Player 1)";
								}
								else {
									Display_Name += " (Player 2)";
								}

								if (ImGui::Selectable(Display_Name.c_str(), is_selected)) {
									// Сохраняем ID выбранного персонажа
									PalEdit::current_character_idx = PalEdit::Character_Vector[i].ID;
									PalEdit::Read_Character();
								}
								if (is_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
					
						
						if (PalEdit::current_character_idx != -1) {
							Character& currentChar = PalEdit::Character_Vector[PalEdit::FindVectorIndexByID(PalEdit::current_character_idx)];
							int displayValue = currentChar.Current_Pallete_Num + 1;
							if (ImGui::SliderInt("PalleteNum##", &displayValue, 1, currentChar.Max_Pallete_Num)) { //Слайдер выбора палетки
								{
									
									currentChar.Current_Pallete_Num = displayValue - 1;
									PalEdit::ChangePallete();
									PalEdit::Read_Character();
								};
							};

							ImGui::Text("Color Palettes: %d", currentChar.Num_Of_Color);
							ImGui::Separator();
							for (int i = 1; i < currentChar.Character_Colors.size(); i++) {
								// Получаем ссылку на 32-битный цвет
								__int32& colorValue = currentChar.Character_Colors[i];

								// Распаковываем компоненты (формат ARGB)
								float colorFloat[4] = {
									((colorValue >> 16) & 0xFF) / 255.0f,  // Red
									((colorValue >> 8) & 0xFF) / 255.0f,   // Green
									(colorValue & 0xFF) / 255.0f,          // Blue
									((colorValue >> 24) & 0xFF) / 255.0f   // Alpha
								};

								ImGui::PushID(i);
								// Отображаем ColorEdit
								if (ImGui::ColorEdit4(("Color##" + std::to_string(i)).c_str(), colorFloat, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {

									// Упаковываем обратно в ARGB формат
									colorValue =
										(static_cast<__int32>(colorFloat[3] * 255) << 24) |  // Alpha
										(static_cast<__int32>(colorFloat[0] * 255) << 16) |  // Red
										(static_cast<__int32>(colorFloat[1] * 255) << 8) |   // Green
										(static_cast<__int32>(colorFloat[2] * 255));         // Blue

									PalEdit::ChangeColor(i, colorValue);
									PalEdit::Read_Character();
								}

								ImGui::PopID();

								if (i > 0 and i % 16 != 0) {
									ImGui::SameLine();
								}
							}
						}






				}
			}
			ImGui::End();
		}
	}
}
