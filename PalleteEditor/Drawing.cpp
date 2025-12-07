#include "Drawing.h"
#include "PalleteEditor.h"
#include "FileLoad.h"
#include "Auto-Load-Pallete.h"
#include "tinyfiledialogs.h"
#include "ColorWheel.h"

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
		static std::unordered_map<std::string, bool> wheelOpenMap;

		PalEdit::Init();
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
		{
			//����-���
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Load Table"))
					{
						AddressTable::LoadFromFile();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Load JSON (Characters parts)"))
					{
						GroupColorGroup::LoadFromFile();
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
							if (PalleteFile::LoadFromFile(
								PalEdit::Character_Vector[PalEdit::FindVectorIndexByID(PalEdit::current_character_idx)]
							)) {
								PalEdit::ChangeAllColors();
								PalEdit::ChangeLineColor();
								PalEdit::ChangeSuperShadow1();
								PalEdit::ChangeSuperShadow2();
								PalEdit::Read_Character();
							}
						}
					}
					ImGui::EndMenu();
	
				}

				// ������ About ����� � ���� ����
				if (ImGui::MenuItem("About"))
				{
					// ��������� ���� About ��� �������
					bShow_about_window = true;
				}

				ImGui::EndMenuBar();
			}
				//���� "About"
				if (bShow_about_window)
				{
					ImGui::Begin("About", &bShow_about_window);
					ImGui::Text("Skullgirls Pallete Editor 2nd Enocore");
					ImGui::TextDisabled("Version: v0.5");
					ImGui::Text("Also, check our Discord: ");
					ImGui::TextLinkOpenURL("Discord", "https://discord.gg/4ufGJQjkpc");
					ImGui::Separator();
					if (ImGui::Button("OK"))
					{
						bShow_about_window = false;
					}
					ImGui::End();
				}
				if (ImGui::BeginTabBar("##TabBar")) {
				if (ImGui::BeginTabItem("Pallete")) {
			
				if (PalEdit::bGameOpenned != true) {
					ImGui::Text("Open the game to use the editor.");

				}
				else {
					if (PalEdit::bMatchStarted != true) {
						ImGui::Text("Start a match to use the editor.\nIf this message persists, the table may be invalid.");
					}
					else {
						//�����-���� ������ ���������
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
								// ���������� ID, � �� ������ � �������
								bool is_selected = (PalEdit::current_character_idx == PalEdit::Character_Vector[i].ID);

								std::string Display_Name = PalEdit::Character_Vector[i].Char_Name;
								if (PalEdit::Character_Vector[i].ID < 3) {
									Display_Name += " (Player 1)";
								}
								else {
									Display_Name += " (Player 2)";
								}

								if (ImGui::Selectable(Display_Name.c_str(), is_selected)) {
									// ��������� ID ���������� ���������
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
							if (ImGui::SliderInt("PalleteNum##", &displayValue, 1, currentChar.Max_Pallete_Num)) { //������� ������ �������
								{

									currentChar.Current_Pallete_Num = displayValue - 1;
									PalEdit::ChangePallete();
									PalEdit::Read_Character();
								};
							};
							ImGui::Separator();

							// ��������������� ���������� ������ (�������� ��� ���������)
							__int32& LineColor = currentChar.LineColor;
							float LinecolorFloat[4] = {
								((LineColor >> 16) & 0xFF) / 255.0f,
								((LineColor >> 8) & 0xFF) / 255.0f,
								(LineColor & 0xFF) / 255.0f,
								((LineColor >> 24) & 0xFF) / 255.0f
							};

							__int32& i32SuperShadow1 = currentChar.SuperShadowColor1;
							float fSuperShadow1[4] = {
								((i32SuperShadow1 >> 16) & 0xFF) / 255.0f,
								((i32SuperShadow1 >> 8) & 0xFF) / 255.0f,
								(i32SuperShadow1 & 0xFF) / 255.0f,
								((i32SuperShadow1 >> 24) & 0xFF) / 255.0f
							};

							__int32& i32SuperShadow2 = currentChar.SuperShadowColor2;
							float fSuperShadow2[4] = {
								((i32SuperShadow2 >> 16) & 0xFF) / 255.0f,
								((i32SuperShadow2 >> 8) & 0xFF) / 255.0f,
								(i32SuperShadow2 & 0xFF) / 255.0f,
								((i32SuperShadow2 >> 24) & 0xFF) / 255.0f
							};

							// ������� ������� � 2 ���������
							if (ImGui::BeginTable("ColorSettings", 2, ImGuiTableFlags_SizingFixedFit))
							{
								ImGui::TableSetupColumn("Options", ImGuiTableColumnFlags_WidthFixed, 200.0f);
								ImGui::TableSetupColumn("Colors", ImGuiTableColumnFlags_WidthFixed, 150.0f);

								// ������ ������: Don't display character
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								if (ImGui::Checkbox("Don't display character", &PalEdit::bNODisplayChar)) {
									PalEdit::NODisplayChar();
								}

								ImGui::TableSetColumnIndex(1);
								if (ImGui::ColorEdit3("Line Color", LinecolorFloat, ImGuiColorEditFlags_NoInputs)) {
									__int32 LineColorValue =
										(static_cast<__int32>(LinecolorFloat[3] * 255) << 24) |
										(static_cast<__int32>(LinecolorFloat[0] * 255) << 16) |
										(static_cast<__int32>(LinecolorFloat[1] * 255) << 8) |
										(static_cast<__int32>(LinecolorFloat[2] * 255));

									currentChar.LineColor = LineColorValue;
									PalEdit::ChangeLineColor();
									PalEdit::Read_Character();
								}

								// ������ ������: Don't display shadows
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								if (ImGui::Checkbox("Don't display shadows", &PalEdit::bNODisplayShadows)) {
									PalEdit::NODisplayShadow();
								}

								ImGui::TableSetColumnIndex(1);
								if (ImGui::ColorEdit3("Super Shadow 1", fSuperShadow1, ImGuiColorEditFlags_NoInputs)) {
									__int32 ColorEdit =
										(static_cast<__int32>(fSuperShadow1[3] * 255) << 24) |
										(static_cast<__int32>(fSuperShadow1[0] * 255) << 16) |
										(static_cast<__int32>(fSuperShadow1[1] * 255) << 8) |
										(static_cast<__int32>(fSuperShadow1[2] * 255));

									currentChar.SuperShadowColor1 = ColorEdit;
									PalEdit::ChangeSuperShadow1();
									PalEdit::Read_Character();
								}

								// ������ ������: Display super shadow
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								if (ImGui::Checkbox("Display super shadow", &PalEdit::bDisplaySuperShadows)) {
									PalEdit::DisplaySuperShadow();
								}

								ImGui::TableSetColumnIndex(1);
								if (ImGui::ColorEdit3("Super Shadow 2", fSuperShadow2, ImGuiColorEditFlags_NoInputs)) {
									__int32 ColorEdit =
										(static_cast<__int32>(fSuperShadow2[3] * 255) << 24) |
										(static_cast<__int32>(fSuperShadow2[0] * 255) << 16) |
										(static_cast<__int32>(fSuperShadow2[1] * 255) << 8) |
										(static_cast<__int32>(fSuperShadow2[2] * 255));

									currentChar.SuperShadowColor2 = ColorEdit;
									PalEdit::ChangeSuperShadow2();
									PalEdit::Read_Character();
								}
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								auto it = GroupColorGroup::characterGroups.find(currentChar.Char_Name); //���� ���� JSON � ������� ���������
								if (it != GroupColorGroup::characterGroups.end() && !it->second.empty()) {
									bJSONEnable = true;
									ImGui::Checkbox("Group Character Parts", &bGrouping);
								}
								ImGui::EndTable();
							}


							ImGui::Text("Color Palettes: %d", currentChar.Num_Of_Color);
							ImGui::Separator();
							if (bJSONEnable and bGrouping) {
								auto it = GroupColorGroup::characterGroups.find(currentChar.Char_Name);
								// ���������� � ������������
								for (const auto& group : it->second) {
									// ��������� ImGuiTreeNodeFlags_DefaultOpen ��� ��������� ��������� �� ���������
									if (ImGui::CollapsingHeader(group.groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
										// small button next to header to open the color-wheel window for this group
										std::string wheelKey = currentChar.Char_Name + std::string("|") + group.groupName;
										std::string btn_id = std::string("wheelBtn_") + std::to_string(group.startIndex);
										ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

										// ���������� ����� � ������
										for (int i = group.startIndex;
											i < group.startIndex + group.count && i < currentChar.Character_Colors.size();
											i++) {

											__int32& colorValue = currentChar.Character_Colors[i];

											float colorFloat[4] = {
												((colorValue >> 16) & 0xFF) / 255.0f,
												((colorValue >> 8) & 0xFF) / 255.0f,
												(colorValue & 0xFF) / 255.0f,
												((colorValue >> 24) & 0xFF) / 255.0f
											};

											ImGui::PushID(i);
											if (ImGui::ColorEdit4(("##Color_" + std::to_string(i)).c_str(),
												colorFloat,
												ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar)) {
												colorValue =
													(static_cast<__int32>(colorFloat[3] * 255) << 24) |
													(static_cast<__int32>(colorFloat[0] * 255) << 16) |
													(static_cast<__int32>(colorFloat[1] * 255) << 8) |
													(static_cast<__int32>(colorFloat[2] * 255));

												PalEdit::ChangeColor(i, colorValue);
												PalEdit::Read_Character();
											}

											if (ImGui::IsItemHovered()) {
												ImGui::SetTooltip("Index: %d", i);
											}

											ImGui::PopID();

											// ���������� SameLine() � ���������, ���������� �� ��������� �������
											bool isLastInGroup = (i == group.startIndex + group.count - 1);
											bool isLastValidColor = (i == currentChar.Character_Colors.size() - 1);

											if (!isLastInGroup && !isLastValidColor) {
												ImGui::SameLine();
											}
										}
										ImGui::Separator();
										ImGui::PushID(btn_id.c_str());
										if (ImGui::Button(("Open Wheel##" + btn_id).c_str())) {
											wheelOpenMap[wheelKey] = !wheelOpenMap[wheelKey];
										}
										ImGui::PopID();

										// If the wheel is open for this group, delegate rendering to ColorWheel implementation
										auto itOpen = wheelOpenMap.find(wheelKey);
										if (itOpen != wheelOpenMap.end() && itOpen->second) {
											bool& openRef = itOpen->second;
											ColorWheel::Draw(currentChar, group, openRef);
										}

										ImGui::PopStyleVar();
									}
								}
							} //���� JSON ���.
							else {
								for (int i = 1; i < currentChar.Character_Colors.size(); i++) {
									// �������� ������ �� 32-������ ����
									__int32& colorValue = currentChar.Character_Colors[i];

									// ������������� ���������� (������ ARGB)
									float colorFloat[4] = {
										((colorValue >> 16) & 0xFF) / 255.0f,  // Red
										((colorValue >> 8) & 0xFF) / 255.0f,   // Green
										(colorValue & 0xFF) / 255.0f,          // Blue
										((colorValue >> 24) & 0xFF) / 255.0f   // Alpha
									};

									ImGui::PushID(i);
									// ���������� ColorEdit
									if (ImGui::ColorEdit4(("Color##" + std::to_string(i)).c_str(), colorFloat, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar)) {

										// ����������� ������� � ARGB ������
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
			}
			ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Auto Load Pallete")) {
						if (ImGui::Button("Add new Auto Load Pallete")) {
							AutoPallete::Auto_Pals.push_back(Auto_Pal{ "Filia", 0, "" });
							AutoPallete::save();
						}
						if (ImGui::Button("Reset Auto Palettes")) {
							AutoPallete::init();
						}
						ImGui::Separator();

						for (int i = 0; i < AutoPallete::Auto_Pals.size(); i++) {
							Auto_Pal& pal = AutoPallete::Auto_Pals[i];

							// ���������� PushID ��� �������� ����������� ������������ ����
							ImGui::PushID(i);
							ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.6f, 0.9f));

							// ���������� ID ��� ������� child
							std::string childId = "pal_group_" + std::to_string(i);
							ImGui::BeginChild(childId.c_str(), ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 4), true, ImGuiWindowFlags_NoDecoration);

							// ���������� ID ��� ������ �������
							std::string tableId = "ColorSettings_" + std::to_string(i);
							if (!ImGui::BeginTable(tableId.c_str(), 3, ImGuiTableFlags_SizingFixedFit)) {
								// ���� ������� �� �������, ���������� �������
								ImGui::EndChild();
								ImGui::PopStyleColor();
								ImGui::PopID();
								continue;
							}
							ImVec2 display_size = ImGui::GetIO().DisplaySize;

							// �������� �� ������ ������ (�� � �������������)
							float labels_width = display_size.x * 1.1f;  // 11% �� ������ ������
							float actions_width = display_size.x * 1.4f; // 14% �� ������ ������

							ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, labels_width);
							ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);
							ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, actions_width);

							// ������ ������: Character Name
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Character Name");

							ImGui::TableSetColumnIndex(1);
							const char* preview_value = pal.CharName.c_str();
							ImGui::SetNextItemWidth(-FLT_MIN);

							// ���������� ID ��� combo
							std::string comboId = "##CharacterName_" + std::to_string(i);
							if (ImGui::BeginCombo(comboId.c_str(), preview_value)) {
								for (int j = 0; j < IM_ARRAYSIZE(characterNames); j++) {
									bool isSelected = (pal.CharName == characterNames[j]);
									if (ImGui::Selectable(characterNames[j], isSelected)) {
										pal.CharName = characterNames[j];
										AutoPallete::save();
									}
									if (isSelected) {
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}

							ImGui::TableSetColumnIndex(2);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Palette Number");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(-FLT_MIN);

							// ���������� ID ��� InputInt
							std::string palNumId = "##PalNum_" + std::to_string(i);
							int displayValue = pal.PalNum + 1;
							if (ImGui::InputInt(palNumId.c_str(), &displayValue)) {
								if (displayValue < 1) displayValue = 1;
								pal.PalNum = displayValue - 1;
								AutoPallete::save();
							}

							// ������ ������: Path
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Path to the Palette");

							ImGui::TableSetColumnIndex(1);
							// �� ����������� ����� - ������� ��������� ��� ������� ��������
							char pathBuffer[512];
							strncpy_s(pathBuffer, pal.PalPath.c_str(), sizeof(pathBuffer));
							pathBuffer[sizeof(pathBuffer) - 1] = '\0';
							ImGui::SetNextItemWidth(-FLT_MIN);

							// ���������� ID ��� InputText
							std::string pathId = "##Path_" + std::to_string(i);
							if (ImGui::InputText(pathId.c_str(), pathBuffer, sizeof(pathBuffer))) {
								pal.PalPath = pathBuffer;
								AutoPallete::save();
							}

							ImGui::TableSetColumnIndex(2);
							// ���������� ID ��� ������ Open

							if (ImGui::Button("Open", ImVec2(-FLT_MIN, 0))) {
								const char* filterPatterns[1] = { "*.pal" };
								const char* filePath = tinyfd_openFileDialog(
									"Load Pallete",
									"",
									1,
									filterPatterns,
									NULL,
									0
								);
								if (filePath != NULL) {
									pal.PalPath = filePath;
									AutoPallete::save();
								}
							}

							ImGui::EndTable();
							ImGui::Separator();

							// ���������� ID ��� ������ Delete
							if (ImGui::Button("Delete")) {
								AutoPallete::Auto_Pals.erase(AutoPallete::Auto_Pals.begin() + i);
								AutoPallete::save();
								// ����� �������� ����� ����� �� �����
								ImGui::EndChild();
								ImGui::PopStyleColor();
								ImGui::PopID();
								break;
							}

							ImGui::EndChild();
							ImGui::PopStyleColor();
							ImGui::PopID();
						}

						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
				ImGui::End();
		}
	}
}
