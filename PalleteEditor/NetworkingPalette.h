#pragma once
#include "pch.h"
#include "lrucache.hpp"
#include "steam/steam_api.h"
#include "AutoLoadPalette.h"


using NetworkPaletteData = std::pair<uint8_t, std::vector<uint8_t>>;

class NetworkingPalette {
private: 
    //  онструкторы и операторы присваивани€ - приватные
    NetworkingPalette() {
        SendMyPalettes = true;
        LoadTheirPalettes = true;
    
    };  // ѕриватный конструктор по умолчанию
    ~NetworkingPalette() = default; // ѕриватный деструктор

    // «апрещаем копирование и присваивание
    NetworkingPalette(const NetworkingPalette&) = delete;
    NetworkingPalette& operator=(const NetworkingPalette&) = delete;
    NetworkingPalette(NetworkingPalette&&) = delete;
    NetworkingPalette& operator=(NetworkingPalette&&) = delete;

    //Callbacks
    STEAM_CALLBACK(NetworkingPalette, OnLobbyEnter, LobbyEnter_t);
    //STEAM_CALLBACK(NetworkingPalette, OnLobbyDataUpdate, LobbyDataUpdate_t);
    STEAM_CALLBACK(NetworkingPalette, OnLobbyChatMessage, LobbyChatMsg_t);
    STEAM_CALLBACK(NetworkingPalette, OnLobbyChatUpdate, LobbyChatUpdate_t);
    STEAM_CALLBACK(NetworkingPalette, OnP2PSessionRequest, P2PSessionRequest_t);
    STEAM_CALLBACK(NetworkingPalette, OnP2PSessionConnectFail, P2PSessionConnectFail_t);

    // Vars
    CSteamID LobbyID = k_steamIDNil; 
    bool InLobby() { return LobbyID != k_steamIDNil; }
    static cache::lru_cache<uint64_t, NetworkPaletteData>& getCache() {
        static cache::lru_cache<uint64_t, NetworkPaletteData> instance(12);
        return instance;
    }
    struct MatchInfo {
        uint32_t rng0;
        CSteamID player1SteamID;
        CSteamID player2SteamID;
        bool isSpectator;

        bool IsPlayer(CSteamID steamID) const {
            return steamID == player1SteamID or steamID == player2SteamID;
        }

        void Clear() {
            *this = MatchInfo();
        }

    } m_MatchInfo;
    //Functions
public:

    bool SendMyPalettes = true;
    bool LoadTheirPalettes = true;

    static NetworkingPalette& GetInstance() {
        static NetworkingPalette instance;
        return instance;
    }

    void AddNewPaletteToCache(Auto_Pal Palette, uint8_t Slot);
    void ReadIncomingPackages();
    void SendPalettesHash(uint64_t Hash, uint8_t Slot);
    void SendPaletteRequest(CSteamID Responsible, uint64_t Hash);
    void SendPaletteData(CSteamID Responsible, uint64_t Hash);
    uint32_t GetRNG0() { return m_MatchInfo.rng0; }
    bool IsPlayer1(){ return SteamUser()->GetSteamID() == m_MatchInfo.player1SteamID; }
    bool IsSpectator() { return m_MatchInfo.isSpectator; }

    void ApplyNetworkingPalette(NetworkPaletteData PaletteData);
};
