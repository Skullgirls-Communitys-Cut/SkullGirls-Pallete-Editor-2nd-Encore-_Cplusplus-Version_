// SteamWorksWrapper.h
#pragma once

#include "steam/steam_api.h"
#include <vector>
#include <string>
#include <functional>

struct LobbyMember {
    CSteamID SteamID;
    std::string Name;
    bool InGame = false;
    bool Ready = false;
    bool IsSpectator = false;
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

    ChatMessage(const std::string& name, CSteamID id, const char* msg, int size, EChatEntryType t, bool local)
        : senderName(name), senderID(id), message(msg, size), messageSize(size), type(t), isLocal(local) {
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

    bool m_bInitialized = false;
    bool m_bInLobby = false;
    LobbyInfo m_LobbyInfo;
    CSteamID m_opponentSteamID;
    std::vector<CSteamID> m_spectators;
    std::vector<ChatMessage> m_chatHistory;

    std::function<void(bool, CSteamID, const std::string&)> m_lobbyJoinCallback;
    std::function<void(CSteamID, CSteamID, const char*, const char*)> m_lobbyDataChangedCallback;
    std::function<void(CSteamID, CSteamID, EChatEntryType, const std::string&)> m_lobbyChatMessageCallback;

    SteamLogger() = default;

public:
    static SteamLogger& GetInstance() {
        static SteamLogger instance;
        return instance;
    }

    bool Init();

    // Методы для работы с лобби
    const LobbyInfo& GetCurrentLobbyInfo() const { return m_LobbyInfo; }
    CSteamID GetLocalSteamID() const { return g_pSteamUser ? g_pSteamUser->GetSteamID() : k_steamIDNil; }
    std::string GetPlayerName(CSteamID steamID) {
        if (!g_pSteamFriends) return "Unknown";
        return g_pSteamFriends->GetFriendPersonaName(steamID);
    }

    // Методы для работы с чатом
    const std::vector<ChatMessage>& GetChatHistory() const { return m_chatHistory; }
    void ClearChatHistory() { m_chatHistory.clear(); }
    bool SendChatMessage(const std::string& message);

    // Колбэки
    void OnLobbyEnter(LobbyEnter_t* pCallback);
    void OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback);
    void OnLobbyChatMessage(LobbyChatMsg_t* pCallback);
    void OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback);

private:
    void AddChatMessage(const std::string& sender, CSteamID senderID,
        const char* message, int messageSize, EChatEntryType type, bool isLocal);
    void TrimChatHistory();
    void UpdateLobbyMetadata();
    void RefreshMemberData(LobbyMember& member);
    void RefreshLobbyMetadata();
    void UpdateMemberData(CSteamID memberID);
    void UpdateAllMembers();
    void UpdateOpponentAndSpectators();
    bool IsSpectator(CSteamID steamID) const;
};