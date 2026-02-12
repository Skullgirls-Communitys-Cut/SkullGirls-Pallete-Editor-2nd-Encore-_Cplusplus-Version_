#pragma once

#include "steam/steam_api.h"
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

struct LobbyMember {
    CSteamID SteamID;
    std::string Name;
    bool InGame = false;
    bool Ready = false;
    char IsSpectator;
    CSteamID Opp;
    int Loc = 0;
    int Wins = 0;
    int Plays = 0;
};

struct ChatMessage {
    std::string senderName;
    CSteamID senderID;
    std::string message;
    int messageSize = 0;
    EChatEntryType type;
    bool isLocal = false;
    std::chrono::system_clock::time_point timestamp;

    ChatMessage(const std::string& name, CSteamID id, const char* msg, int size, EChatEntryType t, bool local)
        : senderName(name), senderID(id), message(msg, size), messageSize(size), type(t),
        isLocal(local), timestamp(std::chrono::system_clock::now()) {
    }

    std::string GetTimeString() const {
        auto time = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm;
        localtime_s(&tm, &time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }

    std::string GetMessageString() const {
        return message;
    }
};

struct LobbyInfo {
    CSteamID LobbyID;
    int numMembers = 0;
    int LobbySize = 0;
    int LobbyPrivateSize = 0;
    int RoomType = 0;
    CSteamID OwnerID;
    std::string OwnerName;
    std::string Protocol;
    std::string SearchRegion;
    int SGLobbyName = 0;
    int SGLobbyType = 0;
    std::string SGRegion;
    std::string Skill;
    std::string errorMessage;
    std::vector<LobbyMember> LobbyMembers;

    void Clear() {
        LobbyID = k_steamIDNil;
        numMembers = 0;
        LobbySize = 0;
        LobbyPrivateSize = 0;
        RoomType = 0;
        OwnerID = k_steamIDNil;
        OwnerName.clear();
        Protocol.clear();
        SearchRegion.clear();
        SGLobbyName = 0;
        SGLobbyType = 0;
        SGRegion.clear();
        Skill.clear();
        errorMessage.clear();
        LobbyMembers.clear();
    }

    LobbyMember* FindMember(CSteamID memberID) {
        for (auto& member : LobbyMembers) {
            if (member.SteamID == memberID) {
                return &member;
            }
        }
        return nullptr;
    }
};

class SteamLogger {
private:
    static const int MAX_CHAT_HISTORY = 1000;

    bool m_bInLobby = false;
    LobbyInfo m_LobbyInfo;
    std::vector<ChatMessage> m_chatHistory;

    STEAM_CALLBACK(SteamLogger, OnLobbyEnter, LobbyEnter_t);
    STEAM_CALLBACK(SteamLogger, OnLobbyDataUpdate, LobbyDataUpdate_t);
    STEAM_CALLBACK(SteamLogger, OnLobbyChatMessage, LobbyChatMsg_t);
    STEAM_CALLBACK(SteamLogger, OnLobbyChatUpdate, LobbyChatUpdate_t);

    SteamLogger() = default;

public:
    static SteamLogger& GetInstance() {
        static SteamLogger instance;
        return instance;
    }

    // Ìועמהû הכÿ נאבמעû ס כמבבט
    const LobbyInfo& GetCurrentLobbyInfo() const { return m_LobbyInfo; }

    CSteamID GetLocalSteamID() const {
        return SteamUser() ? SteamUser()->GetSteamID() : k_steamIDNil;
    }

    std::string GetPlayerName(CSteamID steamID) {
        if (!SteamFriends()) return "Unknown";
        const char* name = SteamFriends()->GetFriendPersonaName(steamID);
        return name ? name : "Unknown";
    }

    // Ìועמהû הכÿ נאבמעû ס קאעמל
    const std::vector<ChatMessage>& GetChatHistory() const { return m_chatHistory; }
    void ClearChatHistory() { m_chatHistory.clear(); }
    bool SendChatMessage(const std::string& message);
private:
    void AddChatMessage(const std::string& sender, CSteamID senderID,
        const char* message, int messageSize, EChatEntryType type, bool isLocal);
    void TrimChatHistory();
    void UpdateLobbyMetadata();
    void RefreshMemberData(LobbyMember& member);
    void RefreshLobbyMetadata();
    void UpdateMemberData(CSteamID memberID);
    void UpdateAllMembers();

};

inline auto& temp = SteamLogger::GetInstance();