#include "AutoLoadPalette.h"
#include "pch.h"
#include "Files/Config.h"
#include <json.hpp>
#include "PlayableCharactersManager.h"
#include "Files/PaletteFiles.h"
#include "NetworkingPalette.h"


using json = nlohmann::json;

auto AutoLoadLogger = LOGGER::createLocal("Auto Load Palettes", LogLevel::GENERAL_LOG);

void AutoPalette::init() {
    LOG_LOCAL_INFO(AutoLoadLogger, "Init AutoPalette");
    // Получаем доступ к именам персонажей через геттер
    const std::optional<std::string>* characterNames = PlayableCharactersManager::instance().GetCharacterNames();

    // Для каждого слота персонажей

    int slot;
    int max_slot;
    if (b_LoadOnlyMySide && (NetworkingPalette::GetInstance().GetRNG0() != 0)) {
        slot = (NetworkingPalette::GetInstance().IsPlayer1()) ? 0 : 3;
        max_slot = (NetworkingPalette::GetInstance().IsPlayer1()) ? 3 : 6;
        LOG_LOCAL_DEBUG(AutoLoadLogger, "Load Auto Load Palette Online");
    }
    else {
        slot = 0;
        max_slot = 6;
        LOG_LOCAL_DEBUG(AutoLoadLogger, "Load Auto Load Palette Offline ");
    }
    for (; slot < max_slot; ++slot) {
        try {
            // Проверяем, есть ли персонаж в этом слоте
            if (!characterNames[slot].has_value()) {
                continue; // Слот пуст
            }

            if (!PlayableCharactersManager::LoadCharacter(slot)) {
                continue; // Пропускаем слот при ошибке загрузки персонажа
            }

            auto& currentChar = PlayableCharactersManager::GetCurrentCharacter();

            // Ищем подходящую авто-палитру для этого персонажа
            for (auto& autoPal : Auto_Pals) {
                try {
                    if (autoPal.PalPath.empty()) {
                        continue;
                    }
                    if (autoPal.CharName != currentChar.Char_Name) continue;

                    // Проверяем совпадение номера палитры
                    if (currentChar.Current_Palette_Num == autoPal.PalNum) {
                        PalleteFile::LoadFromFile(autoPal.PalPath);
                        NetworkingPalette::GetInstance().AddNewPaletteToCache(autoPal, slot);
                        LOG_LOCAL_INFO(AutoLoadLogger, "Load Auto Load Palette for ", currentChar.Char_Name, ": ", autoPal.PalNum);
                        break; // Прерываем поиск для этого персонажа
                    }
                }
                catch (const std::exception& e) {
                    // Логируем ошибку загрузки палитры и продолжаем
                    LOG_LOCAL_ERROR(AutoLoadLogger, "Error loading palette for character ",
                        currentChar.Char_Name, ": ", e.what());
                    continue;
                }
            }
        }
        catch (const std::exception& e) {
            // Логируем ошибку обработки слота и продолжаем
            LOG_LOCAL_ERROR(AutoLoadLogger, "Error processing character slot ", slot, ": ", e.what());
            continue;
        }
    }
    PlayableCharactersManager::instance().SetCurrentCharacterIndex(-1);
}

void AutoPalette::load() {
    Auto_Pals.clear();

    nlohmann::json autoPalsJson = config::get_json("AutoLoadPals");

    if (autoPalsJson.is_array()) {
        for (const auto& item : autoPalsJson) {
            Auto_Pal pal;


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

void AutoPalette::save() {
    nlohmann::json autoPalsJson = nlohmann::json::array();


    for (const auto& pal : Auto_Pals) {
        nlohmann::json item;
        item["CharName"] = pal.CharName;
        item["PalNumber"] = pal.PalNum;
        item["PathToPal"] = pal.PalPath;

        autoPalsJson.push_back(item);
    }

    config::set_json("AutoLoadPals", autoPalsJson);
}