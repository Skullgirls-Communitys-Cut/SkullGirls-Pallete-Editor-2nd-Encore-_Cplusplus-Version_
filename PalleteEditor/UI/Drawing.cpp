#include "pch.h"
#include "Drawing.h"
#include "MainThread.h"

#include "PlayableCharactersManager.h"
#include "AutoLoadPalette.h"

#include "Tools/Colors.hpp"

#include "Files/PaletteFiles.h"
#include "Files/GroupJSONFiles.h"
#include "Files/Config.h"

#include "ImGuiCustom.h"
#include "UI/Eyedropper/Eyedropper.h"
#include "UI/ColorWheel.h"
#include "UI/LoggerWidget.hpp"

#include "NetworkingPalette.h"

#include "SteamLogger.h"


auto DrawingLogger = LOGGER::createLocal("Drawing", LogLevel::GENERAL_LOG);
static std::unordered_map<std::string, bool> wheelOpenMap;



void Drawing::Draw()
{
	if ((GetAsyncKeyState(VK_INSERT) & 1) || (GetAsyncKeyState(VK_F1) & 1)) {
		bDrawAll = !bDrawAll;
	}

	if (bDrawAll)
	{
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);

		ImGui::Begin(lpWindowName, &bDrawAll, WindowFlags);
		{

			Developer::DrawDev();		
			DrawColorPickerWindow();
			DrawMenuBar();
			DrawFileDialog();
			DrawAboutWindow();

			EyeDropper::getInstance().EyeDropperToolTip();

			if (ImGui::BeginTabBar("##TabBar")) {

				if (ImGui::BeginTabItem("Palette")) {
					DrawPaletteTabItem();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Auto Load Palette")) {
					DrawAutoLoadPaletteTabItem();				
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Options")) {
					DrawOptionsTabItem();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			
		}
		ImGui::End();

	}
}



void Drawing::DrawMenuBar() {
	int& Curent_Index = PlayableCharactersManager::GetCurrentCharacterIndex();
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (Curent_Index == -1) { //If we haven't Loaded Character;
				ImGui::TextDisabled("Save Palette");
				if (ImGui::IsItemHovered()) {
					if (ImGui::BeginItemTooltip())
					{
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted("You need to load/select a character first!");
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}

				ImGui::TextDisabled("Load Palette");
				if (ImGui::IsItemHovered()) {
					if (ImGui::BeginItemTooltip())
					{
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted("You need to load/select a character first!");
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}

			}
			else {
				if (ImGui::MenuItem("Save Palette"))
				{
					IGFD::FileDialogConfig config;
					config.path = ".";
					config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
					ImGuiFileDialog::Instance()->OpenDialog("SavePaletteFile", "Save Palette File", ".pal", config);
				}

				if (ImGui::MenuItem("Load Palette"))
				{
					IGFD::FileDialogConfig config;
					config.path = ".";
					ImGuiFileDialog::Instance()->OpenDialog("LoadPaletteFile", "Load Palette File", ".pal", config);

				}


			}
			ImGui::Separator();
			if (ImGui::MenuItem("Load JSON (Characters parts)"))
			{
				IGFD::FileDialogConfig config;
				config.path = ".";
				ImGuiFileDialog::Instance()->OpenDialog("LoadJSON", "Load JSON (Characters parts)", ".json", config);
			}
			ImGui::EndMenu();
		}

		// Кнопка About прямо в меню баре
		if (ImGui::MenuItem("About"))
		{
			// Открываем окно About при нажатии
			bDrawAboutWindow = true;

		}
		ImGui::EndMenuBar();

	}
}

void Drawing::DrawAboutWindow() {
	if (!bDrawAboutWindow) return;
	ImGui::Begin("About", &bDrawAboutWindow);
	{
		ImGui::Text("Skullgirls Pallete Editor 2nd Enocore (DLL Version)");
		ImGui::TextDisabled("Version: v0.6");
		ImGui::TextDisabled("Author: ImpDi");
		ImGui::Text("Also, check our Discord: ");
		ImGui::TextLinkOpenURL("Discord", "https://discord.gg/4ufGJQjkpc");
		ImGui::Separator();
		if (ImGui::Button("OK"))
		{
			bDrawAboutWindow = false;
		}
		ImGui::SameLine();
		ImGui::Checkbox("Super secret Dev Window", &bDrawDevWindow);
	}
	ImGui::End();
}

void Drawing::DrawPaletteTabItem() {
	PlayableCharactersManager& charMgr = PlayableCharactersManager::instance();
	if (MainThread::Match_Readed != true) {
		ImGui::Text("Start a match to use the editor.");
		return;
	}
	DrawPlayableCharactersComboBox();
	if (charMgr.GetCurrentCharacterIndex() == -1) return;

	ImGui::Spacing();

	DrawCharacterPaletteNumSlider();

	ImGui::Separator();

	DrawCharacterOptions();

	ImGui::Separator();

	DrawCharacterColors();

}

void Drawing::DrawPlayableCharactersComboBox() {
	PlayableCharactersManager& charMgr = PlayableCharactersManager::instance();
	auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
	int& Curent_Index = PlayableCharactersManager::GetCurrentCharacterIndex();
	const auto& allCharsNames = charMgr.GetCharacterNames();

	std::string preview_text = "Select";
	if (Curent_Index != -1) {
		preview_text = allCharsNames[Curent_Index].value();
	}
	ImGui::Text("Select character:");

	if (ImGui::BeginCombo("##CharSelect", preview_text.c_str())) {

		for (int i = 0; i < MAX_PLAYABLE_CHARACTERS; i++) {
			if (!allCharsNames[i].has_value()) continue; //If we haven't Playable Character in Array

			bool is_selected = (Curent_Index == i);

			std::string display_name = allCharsNames[i].value();

			if (i < 3) {
				display_name += " (Player 1)";
			}
			else {
				display_name += " (Player 2)";
			}

			if (ImGui::Selectable(display_name.c_str(), is_selected)) {
				if (Curent_Index == i) {
					LOG_LOCAL_WARN(DrawingLogger, "We are trying to select an already selected character.");
					continue;
				}
				// Сохраняем ID выбранного персонажа
				LOG_LOCAL_DEBUG(DrawingLogger, "Choose new Playable Character ", allCharsNames[i].value(), " at ", i);
				m_ActiveColorIndex = 1;
				charMgr.SetCurrentCharacterIndex(i);
				charMgr.LoadCharacter();
				LOG_DEBUG("Loading character from slot: ", i);
				LOG_DEBUG("Character name: ", Curent_Char.Char_Name);
				LOG_DEBUG("Character count of palette: ", Curent_Char.Max_Palette_Num);
				LOG_DEBUG("Character current num of Palette: ", Curent_Char.Current_Palette_Num);
				LOG_DEBUG("Character count of colors: ", Curent_Char.Num_Of_Color);

			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}

		}
		ImGui::EndCombo();
	}

}

void Drawing::DrawCharacterOptions() {
	ImGui::Text("Options: ");
	if (GroupColorManager::GetInstance().HasData()) {
		ImGui::Checkbox("Group Character Parts", &bDrawColorGroup);
	}
	else {
		bDrawColorGroup = false;
	}
	ImGui::Checkbox("Single Color Picker", &bUseColorPickerMode);

	if (bUseColorPickerMode) {
		ImGui::SameLine();
		ImGui::Text("Current Color ID %d", m_ActiveColorIndex);
	}
}

void Drawing::DrawCharacterPaletteNumSlider() {
	auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
	int displayValue = Curent_Char.Current_Palette_Num + 1;

	if (ImGui::SliderInt("Palette Number##", &displayValue, 1, Curent_Char.Max_Palette_Num)) {
		// Вычитаем 1 для получения реального индекса (0-based)
		int newPaletteIndex = displayValue - 1;

		// Проверяем, действительно ли значение изменилось
		if (newPaletteIndex != Curent_Char.Current_Palette_Num) {
			PlayableCharactersManager::ChangePaletteNumber(newPaletteIndex);
		}
	}

	// Можно добавить отображение текущего значения
	ImGui::SameLine();
	ImGui::Text("(%d/%d)", displayValue, Curent_Char.Max_Palette_Num);
}

void Drawing::DrawCharacterColors() {

	ImGui::Text("Additional colors: ");
	ImGui::Spacing();
	auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
	//First of all, we change BGRA to RGBA

	ImVec4 colorLineColorVec = ColorsTools::ImU32BGRAtoImVec4RGBA(Curent_Char.LineColor);
	//Then, we do the same for ShadowColor1
	ImVec4 colorShadowColor1Vec = ColorsTools::ImU32BGRAtoImVec4RGBA(Curent_Char.SuperShadowColor1);
	//And ShadowColor2
	ImVec4 colorShadowColor2Vec = ColorsTools::ImU32BGRAtoImVec4RGBA(Curent_Char.SuperShadowColor2);

	//Be careful - We will ColorEdit3, instead of ColorEdit4, becouse Alpha Channel don't change anything
	if (ImGui::ColorEdit3("Line Color", &colorLineColorVec.x, ImGuiColorEditFlags_NoInputs)) {
		LOG_LOCAL_DEBUG(DrawingLogger, "Change Line color: ");
		LOG_LOCAL_VARIABLE(DrawingLogger, colorLineColorVec.x);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorLineColorVec.y);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorLineColorVec.z);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorLineColorVec.w);
		LOG_LOCAL_DEBUG(DrawingLogger, "Red, Green, Blue, Alpha");
		//First, we change from float ImVec4 to ImU32 (__int32)

		//Then, we change BGRA to RGBA
		ImU32 ColorToWrite = ColorsTools::ImVec4RGBAtoImU32BGRA(colorLineColorVec);

		PlayableCharactersManager::ChangeOptionPaletteColor(ColorToWrite, ColorOptionFlag::FLAG_LINE_COLOR);
	}

	ImGui::SameLine();

	if (ImGui::ColorEdit3("Shadow Color 1", &colorShadowColor1Vec.x, ImGuiColorEditFlags_NoInputs)) {
		LOG_LOCAL_DEBUG(DrawingLogger, "Change Shadow Color: ");
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor1Vec.x);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor1Vec.y);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor1Vec.z);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor1Vec.w);
		LOG_LOCAL_DEBUG(DrawingLogger, "Red, Green, Blue, Alpha");
		//First, we change from float ImVec4 to ImU32 (__int32)
		ImU32 ColorToWrite = ColorsTools::ImVec4RGBAtoImU32BGRA(colorShadowColor1Vec);

		PlayableCharactersManager::ChangeOptionPaletteColor(ColorToWrite, ColorOptionFlag::FLAG_SUPER_SHADOW_1);
	}

	ImGui::SameLine();

	if (ImGui::ColorEdit3("Shadow Color 2", &colorShadowColor2Vec.x, ImGuiColorEditFlags_NoInputs)) {
		LOG_LOCAL_DEBUG(DrawingLogger, "Change Line color: ");
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor2Vec.x);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor2Vec.y);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor2Vec.z);
		LOG_LOCAL_VARIABLE(DrawingLogger, colorShadowColor2Vec.w);
		LOG_LOCAL_DEBUG(DrawingLogger, "Red, Green, Blue, Alpha");
		//First, we change from float ImVec4 to ImU32 (__int32)
		ImU32 ColorToWrite = ColorsTools::ImVec4RGBAtoImU32BGRA(colorShadowColor2Vec);

		PlayableCharactersManager::ChangeOptionPaletteColor(ColorToWrite, ColorOptionFlag::FLAG_SUPER_SHADOW_2);
	}

	ImGui::Spacing();

	ImGui::Text("Colors Palette: %d", Curent_Char.Num_Of_Color);
	auto& colors = Curent_Char.Character_Colors;
	bool hasGroups = false;
	const std::vector<ColorGroup>* groupsPtr = nullptr;

	if (bDrawColorGroup) {
		groupsPtr = GroupColorManager::GetInstance().GetGroupsForCharacter(Curent_Char.Char_Name);
		hasGroups = (groupsPtr != nullptr);

		if (!hasGroups) {
			LOG_LOCAL_ERROR(DrawingLogger, "Can't find JSON for Character");
		}
	}




	// Обработка цветов в зависимости от режима
	if (hasGroups && bDrawColorGroup) {
		// Группированный режим
		const auto& groups = *groupsPtr;
		static char searchBuffer[256] = "";

		ImGui::Text("Search in groups:");
		ImGui::InputText("##GroupSearch", searchBuffer, sizeof(searchBuffer));

		auto groupMatchesSearch = [&](const ColorGroup& group) -> bool {
			if (searchBuffer[0] == '\0') return true;

			std::string groupNameLower = group.groupName;
			std::string searchLower = searchBuffer;

			// Приводим к нижнему регистру для регистронезависимого поиска
			std::transform(groupNameLower.begin(), groupNameLower.end(), groupNameLower.begin(), ::tolower);
			std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

			//return groupNameLower.find(searchLower) != std::string::npos; //Mathes
			return groupNameLower.rfind(searchLower, 0) == 0; //First latter
			};

		static bool collapseAll = false;
		static bool expandAll = false;

		if (ImGui::Button("Collapse All")) {
			collapseAll = true;
			expandAll = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Expand All")) {
			expandAll = true;
			collapseAll = false;
		}
		for (const auto& group : groups) {
			if (!groupMatchesSearch(group)) continue;

			if (collapseAll) {
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);
			}
			else if (expandAll) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			}

			if (ImGui::CollapsingHeader(group.groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

				for (int i = group.startIndex;
					i < group.startIndex + group.count && i < colors.size();
					i++) {
					DrawColorButton(i, colors[i]);

					// Используем SameLine() с проверкой
					bool isLastInGroup = (i == group.startIndex + group.count - 1);
					bool isLastValidColor = (i == colors.size() - 1);

					if (!isLastInGroup && !isLastValidColor) {
						ImGui::SameLine();
					}
				}
				ImGui::PopStyleVar();
				ImGui::Separator();
				std::string wheelKey = Curent_Char.Char_Name + std::string("|") + group.groupName;
				std::string btn_id = std::string("wheelBtn_") + std::to_string(group.startIndex);
				ImGui::PushID(btn_id.c_str());
				if (ImGui::Button(("Open Wheel##" + btn_id).c_str())) {
					wheelOpenMap[wheelKey] = !wheelOpenMap[wheelKey];
				}
				ImGui::PopID();

				auto itOpen = wheelOpenMap.find(wheelKey);
				if (itOpen != wheelOpenMap.end() && itOpen->second) {
					bool& openRef = itOpen->second;
					ColorWheel::Draw(group, openRef);
				}
				float available_width = ImGui::GetContentRegionAvail().x;
				float button2_width = ImGui::CalcTextSize("Copy").x + ImGui::GetStyle().FramePadding.x * 2.0f;
				float button3_width = ImGui::CalcTextSize("Paste").x + ImGui::GetStyle().FramePadding.x * 2.0f;
				float spacing = ImGui::GetStyle().ItemSpacing.x;

				ImGui::SameLine(available_width - (button2_width + button3_width + spacing));


				// Кнопка Copy
				if (ImGui::Button(("Copy##" + group.groupName).c_str())) {
					CopyGroupColors(group);
				}

				// Подсказка для кнопки Copy
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Copy all colors in this group to clipboard");
				}

				ImGui::SameLine();

				// Кнопка Paste
				if (ImGui::Button(("Paste##" + group.groupName).c_str())) {
					PasteGroupColors(group);
				}

				// Подсказка для кнопки Paste
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Paste colors from clipboard to this group");
				}
				ImGui::Separator();
				ImGui::Spacing();
			}
			ImGui::Spacing();
		}
		collapseAll = false;
		expandAll = false;
	}
	else {
		// Автоматический расчет столбцов для негруппированного режима
		float availableWidth = ImGui::GetContentRegionAvail().x;
		float colorButtonSize = ImGui::GetFrameHeight();
		float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
		int colorsPerRow = (int)(availableWidth / (colorButtonSize + itemSpacing));
		if (colorsPerRow < 1) colorsPerRow = 1;
		// Негруппированный режим
		int currentColumn = 0;

		for (int i = 1; i < colors.size(); i++) { // Начинаем с второго цвета
			DrawColorButton(i, colors[i]);

			// Решаем, добавлять SameLine или переходить на новую строку
			currentColumn++;
			if (currentColumn < colorsPerRow && i < colors.size() - 1) {
				ImGui::SameLine(0, itemSpacing);
			}
			else {
				currentColumn = 0;
			}
		}
	}
	
}

void Drawing::DrawColorButton(int index, ImU32 colorU32) {
	ImGui::PushID(index);



	// Выбираем флаги в зависимости от режима
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs |
		ImGuiColorEditFlags_NoLabel |
		ImGuiColorEditFlags_AlphaPreview |
		ImGuiColorEditFlags_AlphaBar;

	if (bUseColorPickerMode) {
		flags |= ImGuiColorEditFlags_NoPicker;

		// Делаем активную кнопку более заметной
		//if (m_ActiveColorIndex == index) {
		//	flags |= ImGuiColorEditFlags_Border;
		//}
	}

	// Конвертируем цвет
	ImVec4 colorVec = ColorsTools::ImU32BGRAtoImVec4RGBA(colorU32);
	if (bUseColorPickerMode) {
		ImGui::ColorButton(
			("##color_" + std::to_string(index)).c_str(),
			colorVec
		);

		if (m_ActiveColorIndex == index) {
			ImVec2 p_min = ImGui::GetItemRectMin();
			ImVec2 p_max = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), 0.0f, 0, 2.0f);
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			m_ActiveColorIndex = index;
		}
	}
	else {
		if (bDrawSimpleColorEdit) {
			if (ImGui::ColorEdit4(
				("##color_" + std::to_string(index)).c_str(),
				&colorVec.x,
				flags
			)
				) {
				ProcessColorChange(index, colorVec);
			}
		}
		else {
			// Кнопка цвета
			if (ImGuiCustom::ColorEdit4(
				("##color_" + std::to_string(index)).c_str(),
				&colorVec.x,
				flags
			)
				) {
				ProcessColorChange(index, colorVec);
			}
		}
	}
	// В режиме Color Picker обрабатываем клик для выбора активного цвета

	ImGui::PopID();
}

void Drawing::ProcessColorChange(int index, const ImVec4& colorVec) {
	LOG_LOCAL_DEBUG(DrawingLogger, "Change color at index: ");
	LOG_LOCAL_VARIABLE(DrawingLogger, index);
	LOG_LOCAL_DEBUG(DrawingLogger, "Change color to: ");
	LOG_LOCAL_VARIABLE(DrawingLogger, colorVec.x);
	LOG_LOCAL_VARIABLE(DrawingLogger, colorVec.y);
	LOG_LOCAL_VARIABLE(DrawingLogger, colorVec.z);
	LOG_LOCAL_VARIABLE(DrawingLogger, colorVec.w);
	LOG_LOCAL_DEBUG(DrawingLogger, "Red, Green, Blue, Alpha");

	// Конвертируем обратно
	ImU32 ColorToWrite = ColorsTools::ImVec4RGBAtoImU32BGRA(colorVec);

	PlayableCharactersManager::ChangePaletteColor(index, ColorToWrite);
}

void Drawing::DrawAutoLoadPaletteTabItem() {

	if (ImGui::Button("Add new Auto Load Pallete")) {
		AutoPalette::Auto_Pals.push_back(Auto_Pal{ "Filia", 0, "" });
		AutoPalette::save(); //We save new AutoPal
	}
	if (ImGui::Button("Reset Auto Palettes")) {
		AutoPalette::init();
	}
	ImGui::Separator();

	for (int i = 0; i < AutoPalette::Auto_Pals.size(); i++) {
		Auto_Pal& pal = AutoPalette::Auto_Pals[i];

		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.6f, 0.9f));

		// Группируем каждый элемент в Child с адаптивной высотой
		std::string childId = "AutoPal_group_" + std::to_string(i);

		// Используем флаг для авто-размера вместо фиксированной высоты
		ImGui::BeginChild(childId.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, 0);

		// === Первая строка: Выбор персонажа и номер палитры ===
		
		ImGui::BeginGroup();
		{
			// Левая часть: Название + Combo
			ImGui::BeginGroup();
			ImGui::Text("Character Name");
			ImGui::SameLine();

			const char* preview_value = pal.CharName.c_str();
			std::string comboId = "##CharacterName_" + std::to_string(i);

			// Автоматическая ширина для Combo (50% от доступного пространства)
			float comboWidth = ImGui::GetContentRegionAvail().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 2;
			comboWidth = std::max(comboWidth, 100.0f); // Минимальная ширина
			ImGui::SetNextItemWidth(comboWidth);

			if (ImGui::BeginCombo(comboId.c_str(), preview_value)) {
				for (int j = 0; j < IM_ARRAYSIZE(PlayableCharacterNames); j++) {
					bool isSelected = (pal.CharName == PlayableCharacterNames[j]);
					if (ImGui::Selectable(PlayableCharacterNames[j], isSelected)) {
						pal.CharName = PlayableCharacterNames[j];
						AutoPalette::save();
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::EndGroup();

			// Правая часть: Номер палитры
			ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2); // Больший отступ

			ImGui::BeginGroup();
			ImGui::Text("Palette #");
			ImGui::SameLine();

			std::string palNumId = "##PalNum_" + std::to_string(i);
			int displayValue = pal.PalNum + 1;

			// Фиксированная ширина для InputInt (ширина для 3-4 цифр)
			ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000000").x + ImGui::GetStyle().FramePadding.x * 2);
			if (ImGui::InputInt(palNumId.c_str(), &displayValue)) {
				if (displayValue < 1) displayValue = 1;
				pal.PalNum = displayValue - 1;
				AutoPalette::save();
			}
			ImGui::EndGroup();
		}
		ImGui::EndGroup();

		ImGui::Spacing(); // Отступ между строками

		// === Вторая строка: Путь к палитре ===
		ImGui::BeginGroup();
		{
			// Текст слева
			ImGui::Text("Path to the Palette");
			ImGui::SameLine();

			// Группа для InputText и кнопки
			ImGui::BeginGroup();
			{
				char pathBuffer[512];
				strncpy_s(pathBuffer, pal.PalPath.c_str(), sizeof(pathBuffer));
				pathBuffer[sizeof(pathBuffer) - 1] = '\0';
				std::string pathId = "##Path_" + std::to_string(i);

				// InputText занимает всё доступное пространство минус кнопка
				float availableWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = ImGui::CalcTextSize("Open").x + ImGui::GetStyle().FramePadding.x * 2;
				float inputWidth = availableWidth - buttonWidth - ImGui::GetStyle().ItemSpacing.x;

				ImGui::PushItemWidth(inputWidth);
				if (ImGui::InputText(pathId.c_str(), pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_ElideLeft)) {
					pal.PalPath = pathBuffer;
					AutoPalette::save();
				}
				ImGui::PopItemWidth();

				// Кнопка "Open" на той же строке
				ImGui::SameLine();
				if (ImGui::Button("Open")) {
					IGFD::FileDialogConfig config;
					config.path = ".";
					config.userDatas = reinterpret_cast<void*>(&pal);
					ImGuiFileDialog::Instance()->OpenDialog("LoadPaletteFile_AutoLoad", "Load Palette File for Auto Loading", ".pal", config);
				}
			}
			ImGui::EndGroup();
		}
		ImGui::EndGroup();

		ImGui::Spacing(); // Отступ перед разделителем

		ImGui::Separator();

		ImGui::Spacing(); // Отступ после разделителя

		// === Третья строка: Кнопка Delete ===
		// Центрируем кнопку Delete

		
		if (ImGui::Button("Delete")) {
			AutoPalette::Auto_Pals.erase(AutoPalette::Auto_Pals.begin() + i);
			AutoPalette::save();
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopID();

		//// Отступ между элементами списка
		//if (i < AutoPallete::Auto_Pals.size() - 1) {
		//	ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
		//}
	}

}

void Drawing::DrawOptionsTabItem()
{
	ImGui::Text("Options: ");
	ImGui::SeparatorText("Drawing: ");
	//We need to create "intermediate" bool
	bool Drawing_NODisplayChar = MainThread::b_NODisplayChar.load();
	if (ImGui::Checkbox("Don't display characters", &Drawing_NODisplayChar)) {
		MainThread::b_NODisplayChar.store(Drawing_NODisplayChar); //And now, we save this in our atomic bool
	}
	bool Drawing_NODisplayShadows = MainThread::b_NODisplayShadows.load();
	if (ImGui::Checkbox("Don't display shadows", &Drawing_NODisplayShadows)) {
		MainThread::b_NODisplayShadows.store(Drawing_NODisplayShadows); //And now, we save this in our atomic bool
	}
	bool Drawing_DisplaySuperShadows = MainThread::b_DisplaySuperShadows.load();
	if (ImGui::Checkbox("Display super shadow", &Drawing_DisplaySuperShadows)) {
		MainThread::b_DisplaySuperShadows.store(Drawing_DisplaySuperShadows); //And now, we save this in our atomic bool
	}
	ImGui::Checkbox("Simple Color Edit", &bDrawSimpleColorEdit);
	ImGui::SeparatorText("Auto Load Palettes: ");
	ImGui::Checkbox("Load Palettes only on my side", &AutoPalette::b_LoadOnlyMySide);
	ImGui::SeparatorText("Steam: ");
	ImGui::Checkbox("Send my palettes to other players", &NetworkingPalette::GetInstance().SendMyPalettes);
	ImGui::Checkbox("Download other players' palettes", &NetworkingPalette::GetInstance().LoadTheirPalettes);

}


void Drawing::DrawFileDialog() {

	ImGui::SetNextWindowSize(vFileDialogSize, ImGuiCond_Once);

	if (ImGuiFileDialog::Instance()->Display("SavePaletteFile")) {

		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK

			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

			PalleteFile::SaveToFile(filePathName);
		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGuiFileDialog::Instance()->Display("LoadPaletteFile")) {

		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK

			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

			PalleteFile::LoadFromFile(filePathName);
		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGuiFileDialog::Instance()->Display("LoadJSON")) {

		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK

			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

			GroupColorManager::GetInstance().LoadFromFile(filePathName);
			config::set_string("CharPart", filePathName);
		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGuiFileDialog::Instance()->Display("LoadPaletteFile_AutoLoad")) {

		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK

			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			Auto_Pal* palPtr = reinterpret_cast<Auto_Pal*>(ImGuiFileDialog::Instance()->GetUserDatas());
			palPtr->PalPath = filePathName;

			AutoPalette::save();
		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void Drawing::DrawColorPickerWindow() {
	if (!bUseColorPickerMode) return;
	auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
	auto& colors = Curent_Char.Character_Colors;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_Once);
	if (ImGui::Begin("Color Picker", &bUseColorPickerMode, ImGuiWindowFlags_NoCollapse)) {

		// Отображаем информацию о текущем цвете и группе
		ImGui::Text("Color Index: #%d", m_ActiveColorIndex);

		// Определяем, в какой группе находится активный цвет
		std::string currentGroup = "Ungrouped";
		if (auto groupsPtr = GroupColorManager::GetInstance().GetGroupsForCharacter(Curent_Char.Char_Name)) {
			const auto& groups = *groupsPtr;
			for (const auto& group : groups) {
				if (m_ActiveColorIndex >= group.startIndex &&
					m_ActiveColorIndex < group.startIndex + group.count) {
					currentGroup = group.groupName;
					break;
				}
			}
		}
		ImGui::Text("Group: %s", currentGroup.c_str());

		ImGui::Separator();

		// Получаем текущий цвет
		ImU32 colorRGBA_U32 = ColorsTools::SwapRBChannels(colors[m_ActiveColorIndex]);
		ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(colorRGBA_U32);

		// Используем временный буфер для ref цвета
		static int lastActiveIndex = -1;
		static ImVec4 refColorVec;

		// Если индекс изменился или окно только открылось, обновляем ref цвет
		if (lastActiveIndex != m_ActiveColorIndex) {
			refColorVec = colorVec;
			lastActiveIndex = m_ActiveColorIndex;
		}

		// Color Picker с отображением оригинального цвета
		ImGui::BeginGroup();
		bool colorChanged = ImGuiCustom::ColorPicker4("##color_picker", &colorVec.x,
			ImGuiColorEditFlags_NoLabel |
			ImGuiColorEditFlags_AlphaPreview |
			ImGuiColorEditFlags_AlphaBar,
			&refColorVec.x);
		ImGui::EndGroup();
		//if (ImGui::Button("Eyedropper")) {
		//	EyeDropper::getInstance().StartEyedropper(&colorVec.x);
		//}
		ImGui::Separator();
		ImGui::BeginGroup();
		ImGui::Text("Palette");
		for (int n = 0; n < IM_ARRAYSIZE(ImGuiCustom::saved_palette); n++)
		{
			ImGui::PushID(n);
			if ((n % 8) != 0)
				ImGui::SameLine(0.0f, style.ItemSpacing.y);

			ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
			if (ImGui::ColorButton("##palette", ImGuiCustom::saved_palette[n], palette_button_flags, ImVec2(20, 20)))
			{
				colorVec.x = ImGuiCustom::saved_palette[n].x;
				colorVec.y = ImGuiCustom::saved_palette[n].y;
				colorVec.z = ImGuiCustom::saved_palette[n].z;
				// Preserve alpha
				colorChanged = true;
			}

			// Allow user to drop colors into each palette entry
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
					memcpy((float*)&ImGuiCustom::saved_palette[n], payload->Data, sizeof(float) * 3);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
					memcpy((float*)&ImGuiCustom::saved_palette[n], payload->Data, sizeof(float) * 4);
				ImGui::EndDragDropTarget();
			}
			ImGui::PopID();
		}
		ImGui::EndGroup();

		// Проверяем, завершилась ли пипетка (и не была отменена)
		if (EyeDropper::getInstance().IsThreadFinished()) {
			if (EyeDropper::getInstance().WasCancelled()) {
				// Пипетка была отменена - возвращаем предыдущий цвет
				colorVec.x = refColorVec.x;
				colorVec.y = refColorVec.y;
				colorVec.z = refColorVec.z;
				colorVec.w = refColorVec.w;
				colorChanged = true;
			}
		}

		// Во время работы пипетки показываем предпросмотр
		if (!EyeDropper::getInstance().IsThreadFinished()) {
			ImVec4 EyeDropperColor = EyeDropper::getInstance().GetColorUnderCursorImVec4();
			colorVec.x = EyeDropperColor.x;
			colorVec.y = EyeDropperColor.y;
			colorVec.z = EyeDropperColor.z;
			colorChanged = true;
		}
		if (colorChanged) {
			ProcessColorChange(m_ActiveColorIndex, colorVec);
		}

		// Отображаем информацию о цветах
		ImGui::Separator();
		ImGui::Text("Navigation:");

		if (ImGui::ArrowButton("##prev", ImGuiDir_Left) && m_ActiveColorIndex > 1) {
			m_ActiveColorIndex--;
		}
		ImGui::SameLine();
		ImGui::Text("Index: #%d", m_ActiveColorIndex);
		ImGui::SameLine();
		if (ImGui::ArrowButton("##next", ImGuiDir_Right) && m_ActiveColorIndex < colors.size() - 1) {
			m_ActiveColorIndex++;
		}	
	}
	ImGui::End();
}

//===== Utils ======
std::string ColorsToText(const std::vector<ImU32>& colors) {
	std::string result;

	for (size_t i = 0; i < colors.size(); i++) {
		// Преобразуем ImU32 в RGBA компоненты
		ImU32 colorRGBA = ColorsTools::SwapRBChannels(colors[i]);
		ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(colorRGBA);

		// Формируем строку в формате: R G B A
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%d %d %d %d",
			(int)((colorRGBA >> IM_COL32_R_SHIFT) & 0xFF),  // Красный
			(int)((colorRGBA >> IM_COL32_G_SHIFT) & 0xFF),  // Зеленый
			(int)((colorRGBA >> IM_COL32_B_SHIFT) & 0xFF),  // Синий
			(int)((colorRGBA >> IM_COL32_A_SHIFT) & 0xFF)); // Альфа

		result += buffer;
		if (i < colors.size() - 1) {
			result += "\n";
		}
	}

	return result;
}

void Drawing::CopyGroupColors(const ColorGroup& group) {
	auto& colors = PlayableCharactersManager::GetCurrentCharacter().Character_Colors;

	// Очищаем буфер обмена
	s_ClipboardColors.clear();

	// Копируем цвета группы в буфер обмена
	for (int i = group.startIndex;
		i < group.startIndex + group.count && i < colors.size();
		i++) {
		s_ClipboardColors.push_back(colors[i]);
	}

	// Также копируем в системный буфер обмена в текстовом формате
	std::string clipboardText = ColorsToText(s_ClipboardColors);
	ImGui::SetClipboardText(clipboardText.c_str());

	LOG_LOCAL_DEBUG(DrawingLogger, "Copied group '");
	LOG_LOCAL_VARIABLE(DrawingLogger, group.groupName);
	LOG_LOCAL_DEBUG(DrawingLogger, "' to clipboard. Colors count: ");
	LOG_LOCAL_VARIABLE(DrawingLogger, s_ClipboardColors.size());
}

void Drawing::PasteGroupColors(const ColorGroup& group) {
	auto& colors = PlayableCharactersManager::GetCurrentCharacter().Character_Colors;

	// Проверяем, есть ли что-то в буфере обмена
	if (s_ClipboardColors.empty()) {
		LOG_LOCAL_WARN(DrawingLogger, "Clipboard is empty. Cannot paste.");
		return;
	}

	// Проверяем, что количество цветов совпадает
	if (s_ClipboardColors.size() != group.count) {
		LOG_LOCAL_WARN(DrawingLogger, "Clipboard colors count (");
		LOG_LOCAL_VARIABLE(DrawingLogger, s_ClipboardColors.size());
		LOG_LOCAL_DEBUG(DrawingLogger, ") doesn't match group count (");
		LOG_LOCAL_VARIABLE(DrawingLogger, group.count);
		LOG_LOCAL_DEBUG(DrawingLogger, "). Will paste as many as possible.");
	}

	// Вставляем цвета из буфера обмена
	int pasteCount = std::min((int)s_ClipboardColors.size(), group.count);

	for (int i = 0; i < pasteCount; i++) {
		int colorIndex = group.startIndex + i;
		if (colorIndex >= colors.size()) break;

		PlayableCharactersManager::ChangePaletteColor(colorIndex, s_ClipboardColors[i]);
	}

	LOG_LOCAL_DEBUG(DrawingLogger, "Pasted ");
	LOG_LOCAL_VARIABLE(DrawingLogger, pasteCount);
	LOG_LOCAL_DEBUG(DrawingLogger, " colors to group '");
	LOG_LOCAL_VARIABLE(DrawingLogger, group.groupName);
	LOG_LOCAL_DEBUG(DrawingLogger, "'");
}



//===== DEV WINDOW =======

void Drawing::Developer::DrawDevWindow() {
	if (!bDrawDevWindow) return;
	ImGui::Begin("Developer Window", &bDrawDevWindow);
	ImGui::Text("You want find something here?");
	static ImVec4 Test_Color;
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs |
		ImGuiColorEditFlags_NoLabel |
		ImGuiColorEditFlags_AlphaPreview |
		ImGuiColorEditFlags_AlphaBar 
		// | ImGuiColorEditFlags_Float
		;
	ImGuiCustom::ColorEdit4("Test!", &Test_Color.x, flags);
	if(ImGui::Button("Eyedropper!")) {
		EyeDropper::getInstance().StartEyedropper(&Test_Color.x);
	}
	if(ImGui::Checkbox("Show console", &bDrawConsole)) {
		SetConsoleMode(bDrawConsole);
	}

	ImGui::Checkbox("Show Demo Window", &ShowDemoWindow);

	ImGui::Checkbox("Show Logs", &ShowLogs);

	ImGui::Checkbox("Show Steam Info Window", &ShowSteamInfo);

	ImGui::Checkbox("Show Steam Lobby's", &ShowTestWindow);

	ImGui::End();
}



void Drawing::Developer::DrawSteamInfo() {
	if (!ShowSteamInfo) return;
	
		// Получаем информацию о лобби
		auto lobbyInfo = SteamLogger::GetInstance().GetCurrentLobbyInfo();

		ImGui::Begin("Steam Lobby Info", &ShowSteamInfo);

		// Показать ошибку если есть
		if (!lobbyInfo.errorMessage.empty())
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s",
				lobbyInfo.errorMessage.c_str());
			ImGui::Separator();
		}

		// Основная информация о лобби
		if (ImGui::CollapsingHeader("Lobby Information", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Lobby ID: %llu", lobbyInfo.LobbyID.ConvertToUint64());
			ImGui::Text("Members: %d/%d", lobbyInfo.numMembers, lobbyInfo.LobbySize);
			ImGui::Text("Private Size: %d", lobbyInfo.LobbyPrivateSize);
			ImGui::Text("Room Type: %d", lobbyInfo.RoomType);
			ImGui::Separator();

			// Информация о владельце
			ImGui::Text("Owner: %s", lobbyInfo.OwnerName.c_str());
			ImGui::Text("Owner ID: %llu", lobbyInfo.OwnerID.ConvertToUint64());
			ImGui::Separator();

			// Метаданные
			ImGui::Text("Protocol: %s", lobbyInfo.Protocol.c_str());
			ImGui::Text("Search Region: %s", lobbyInfo.SearchRegion.c_str());
			ImGui::Text("SG Lobby Name: %d", lobbyInfo.SGLobbyName);
			ImGui::Text("SG Lobby Type: %d", lobbyInfo.SGLobbyType);
			ImGui::Text("SG Region: %s", lobbyInfo.SGRegion.c_str());
			ImGui::Text("Skill: %s", lobbyInfo.Skill.c_str());
		}

		if (ImGui::CollapsingHeader("Lobby Members", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (lobbyInfo.LobbyMembers.empty())
			{
				ImGui::Text("No members in lobby");
			}
			else
			{
				// Создаем таблицу для отображения участников
				if (ImGui::BeginTable("MembersTable", 9,
					ImGuiTableFlags_Borders |
					ImGuiTableFlags_RowBg |
					ImGuiTableFlags_SizingFixedFit))
				{
					// Заголовки таблицы
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Steam ID");
					ImGui::TableSetupColumn("In Game");
					ImGui::TableSetupColumn("IsSpectator");
					ImGui::TableSetupColumn("Ready");
					ImGui::TableSetupColumn("Opponent");
					ImGui::TableSetupColumn("Location");
					ImGui::TableSetupColumn("Wins");
					ImGui::TableSetupColumn("Plays");
					ImGui::TableHeadersRow();

					// Данные участников
					for (const auto& member : lobbyInfo.LobbyMembers)
					{
						ImGui::TableNextRow();

						// Имя
						ImGui::TableNextColumn();
						ImGui::Text("%s", member.Name.c_str());

						// Steam ID
						ImGui::TableNextColumn();
						ImGui::Text("%llu", member.SteamID.ConvertToUint64());

						// In Game (с цветовой индикацией)
						ImGui::TableNextColumn();
						if (member.InGame)
						{
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "Yes");
						}
						else
						{
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "No");
						}

						// IsSpectator (с цветовой индикацией)
						ImGui::TableNextColumn();
						ImGui::Text("%s", member.IsSpectator);

						// Ready (с цветовой индикацией)
						ImGui::TableNextColumn();
						if (member.Ready)
						{
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "Ready");
						}
						else
						{
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Ready");
						}

						// Opponent
						ImGui::TableNextColumn();
						ImGui::Text("%llu", member.Opp.ConvertToUint64());

						// Location
						ImGui::TableNextColumn();
						ImGui::Text("%d", member.Loc);

						// Wins
						ImGui::TableNextColumn();
						ImGui::Text("%d", member.Wins);

						// Plays
						ImGui::TableNextColumn();
						ImGui::Text("%d", member.Plays);
					}

					ImGui::EndTable();
				}
			}
		}

		if (ImGui::CollapsingHeader("Lobby Chat", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Прокручиваемая область для сообщений
			ImGui::BeginChild("ChatMessages", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 40),
				false, ImGuiWindowFlags_HorizontalScrollbar);

			const auto& chatHistory = SteamLogger::GetInstance().GetChatHistory();
			CSteamID localID = SteamLogger::GetInstance().GetLocalSteamID();

			for (const auto& msg : chatHistory) {
				// Разный цвет для разных типов сообщений и отправителей
				if (msg.senderID == localID) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
				}
				else if (msg.senderID.IsValid()) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
				}

				// Форматируем строку
				std::string messageLine;

				std::string hexDump;
				hexDump = "[Size:" + std::to_string(msg.messageSize) + "] HEX: ";
				for (int i = 0; i < msg.messageSize; i++) {
					char hex[4];
					sprintf_s(hex, "%02X ", (unsigned char)msg.message[i]); // ПРИВЕДЕНИЕ к unsigned char!
					hexDump += hex;
				}

				// Проверяем, начинается ли сообщение с SPEC или MINF
				if (msg.messageSize >= 24 && (strncmp(msg.message.c_str(), "MINF", 4) == 0 || strncmp(msg.message.c_str(), "SPEC", 4) == 0)) {
					std::string header(msg.message, 4);

					if (header == "MINF" && msg.messageSize >= 24) {
						// MINF: 4 байта заголовок, 8 байт SteamID1, 8 байт SteamID2, 4 байта MatchID/RNG0
						uint64_t steamID1 = 0;
						uint64_t steamID2 = 0;
						uint32_t matchIdRng0 = 0;

						const char* rawData = msg.message.c_str();

						// Извлекаем SteamID1 (little-endian)
						memcpy(&steamID1, rawData + 4, 8);
						// Извлекаем SteamID2 (little-endian)
						memcpy(&steamID2, rawData + 12, 8);
						// Извлекаем MatchID/RNG0 (little-endian)
						memcpy(&matchIdRng0, rawData + 20, 4);

						CSteamID steamID1Obj(steamID1);
						CSteamID steamID2Obj(steamID2);

						std::string player1Name = SteamLogger::GetInstance().GetPlayerName(steamID1Obj);
						std::string player2Name = SteamLogger::GetInstance().GetPlayerName(steamID2Obj);

						messageLine = "[" + msg.GetTimeString() + "] " + msg.senderName + ": MINF\n" +
							"  Player 1: " + player1Name + " (" + std::to_string(steamID1) + ")\n" +
							"  Player 2: " + player2Name + " (" + std::to_string(steamID2) + ")\n" +
							"  MatchID/RNG0: " + std::to_string(matchIdRng0)
							+ "\n" + hexDump;
					}
					else if (header == "SPEC" && msg.messageSize >= 36) {
						// SPEC: 4 байта заголовок, 8 байт Spectator SteamID, 8 байт Player1 SteamID, 8 байт Player2 SteamID, 4 байта RNG0
						uint64_t spectatorID = 0;
						uint64_t player1ID = 0;
						uint64_t player2ID = 0;
						uint32_t specID = 0;
						uint32_t rng0 = 0;

						const char* rawData = msg.message.c_str();

						// Извлекаем данные (little-endian)
						memcpy(&spectatorID, rawData + 4, 8);
						memcpy(&player1ID, rawData + 12, 8);
						memcpy(&player2ID, rawData + 20, 8);
						memcpy(&specID, rawData + 28, 4);
						memcpy(&rng0, rawData + 32, 4);

						CSteamID spectatorIDObj(spectatorID);
						CSteamID player1IDObj(player1ID);
						CSteamID player2IDObj(player2ID);

						std::string spectatorName = SteamLogger::GetInstance().GetPlayerName(spectatorIDObj);
						std::string player1Name = SteamLogger::GetInstance().GetPlayerName(player1IDObj);
						std::string player2Name = SteamLogger::GetInstance().GetPlayerName(player2IDObj);

						messageLine = "[" + msg.GetTimeString() + "] " + msg.senderName + ": SPEC\n" +
							"  Spectator: " + spectatorName + " (" + std::to_string(spectatorID) + ")\n" +
							"  Player 1: " + player1Name + " (" + std::to_string(player1ID) + ")\n" +
							"  Player 2: " + player2Name + " (" + std::to_string(player2ID) + ")\n" +
							"  RNG0: " + std::to_string(rng0) + "\n"
							"  specID: " + std::to_string(specID)
							+ "\n" + hexDump;
					}
					else {
						// Неполное сообщение
						messageLine = "[" + msg.GetTimeString() + "] " + msg.senderName + ": " +
							header + " (incomplete message, size: " + std::to_string(msg.messageSize) + " bytes)";
					}
				}
				else {
					// Обычное сообщение
					if (msg.type == k_EChatEntryTypeEmote) {
						messageLine = "[" + msg.GetTimeString() + "] * " + msg.senderName + " " + msg.message;
					}
					else {
						messageLine = "[" + msg.GetTimeString() + "] " + msg.senderName + ": " + msg.message;
					};
				}

				// Отображаем сообщение
				ImGui::TextWrapped("%s", messageLine.c_str());
				ImGui::PopStyleColor();
				ImGui::Separator();
			}

			// Автоматическая прокрутка вниз при новых сообщениях
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10) {
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::EndChild();


			// Поле для ввода нового сообщения
			static char inputBuffer[256] = "";
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70);
			if (ImGui::InputText("##ChatInput", inputBuffer, sizeof(inputBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (strlen(inputBuffer) > 0) {
					SteamLogger::GetInstance().SendChatMessage(inputBuffer);
					inputBuffer[0] = '\0'; // Очищаем поле ввода
				}
				ImGui::SetKeyboardFocusHere(-1); // Фокус обратно на поле ввода
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::Button("Send", ImVec2(60, 0))) {
				if (strlen(inputBuffer) > 0) {
					SteamLogger::GetInstance().SendChatMessage(inputBuffer);
					inputBuffer[0] = '\0';
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Clear History"))
			{
				SteamLogger::GetInstance().ClearChatHistory();
			}
		}

		ImGui::End();
	
}

