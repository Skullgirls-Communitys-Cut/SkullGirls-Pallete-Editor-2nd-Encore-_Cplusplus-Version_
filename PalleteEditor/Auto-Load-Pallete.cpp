#include "Auto-Load-Pallete.h"
#include "PalleteEditor.h"
#include <iostream>
#include <Character.h>
#include <json.hpp>
#include "Config.h"

using json = nlohmann::json;

void AutoPallete::init() {

	std::cout << "Test";
	for (auto& Auto_Pal : Auto_Pals) {
		if (Auto_Pal.PalPath == "") continue;
		for (auto& current_char : PalEdit::Character_Vector) {
			if (current_char.Char_Name == Auto_Pal.CharName and current_char.Current_Pallete_Num == Auto_Pal.PalNum) {

                std::ifstream file(Auto_Pal.PalPath, std::ios::binary);
				if (!file.is_open()) {
					continue;
				}


                file.seekg(16, std::ios::cur);


                uint32_t numOfColors = 0;
                file.read(reinterpret_cast<char*>(&numOfColors), sizeof(numOfColors));

                uint8_t HueShift_inc = 0;
                uint8_t HueShift_int = 0;
                file.read(reinterpret_cast<char*>(&HueShift_inc), 1);
                file.read(reinterpret_cast<char*>(&HueShift_int), 1);
                std::vector<__int32>  temp(1);
                for (int i = 0; i < current_char.Num_Of_Color - 1; i++) {
                    __int32 color;
                    file.read(reinterpret_cast<char*>(&color), sizeof(color));
                    temp.push_back(color);
                }

                current_char.Character_Colors = std::move(temp);
                file.read(reinterpret_cast<char*>(&current_char.LineColor), sizeof(current_char.LineColor));
                file.read(reinterpret_cast<char*>(&current_char.SuperShadowColor1), sizeof(current_char.SuperShadowColor1));
                file.read(reinterpret_cast<char*>(&current_char.SuperShadowColor2), sizeof(current_char.SuperShadowColor2));
			}
		}
	}
	PalEdit::UpdateAllCharacters();
}

void AutoPallete::load() {
    Auto_Pals.clear();

    // Получаем JSON массив из конфига
    nlohmann::json autoPalsJson = config::get_json("AutoLoadPals");

    // Проверяем, что это массив
    if (autoPalsJson.is_array()) {
        for (const auto& item : autoPalsJson) {
            Auto_Pal pal;

            // Проверяем наличие полей
            if (item.contains("CharName") && item["CharName"].is_string()) {
                pal.CharName = item["CharName"];
            }

            if (item.contains("PalNumber") && item["PalNumber"].is_number()) {
                pal.PalNum = item["PalNumber"];
            }

            if (item.contains("PathToPal") && item["PathToPal"].is_string()) {
                pal.PalPath = item["PathToPal"];
            }

            Auto_Pals.push_back(pal);
        }
    }
}

void AutoPallete::save() {
    nlohmann::json autoPalsJson = nlohmann::json::array();

    // Преобразуем вектор в JSON массив
    for (const auto& pal : Auto_Pals) {
        nlohmann::json item;
        item["CharName"] = pal.CharName;
        item["PalNumber"] = pal.PalNum;
        item["PathToPal"] = pal.PalPath;

        autoPalsJson.push_back(item);
    }

    // Сохраняем в конфиг
    config::set_json("AutoLoadPals", autoPalsJson);
}