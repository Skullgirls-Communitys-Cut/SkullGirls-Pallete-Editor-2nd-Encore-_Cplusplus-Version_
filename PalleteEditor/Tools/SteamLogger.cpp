// SteamWorksWrapper.cpp
#include "pch.h"
#include "SteamLogger.h"

bool SteamLogger::Init() {
    if (m_bInitialized) return true;

    if (!SteamAPI_Init()) return false;

    g_pSteamUser = SteamUser();
    g_pSteamFriends = SteamFriends();
    g_pSteamMatchmaking = SteamMatchmaking();

    m_bInitialized = true;
    return true;
}

void SteamLogger::AddChatMessage(const std::string& sender, CSteamID senderID,
    const char* message, int messageSize, EChatEntryType type, bool isLocal) {
    if (m_chatHistory.size() >= MAX_CHAT_HISTORY) {
        TrimChatHistory();
    }
    m_chatHistory.emplace_back(sender, senderID, message, messageSize, type, isLocal);
}

void SteamLogger::TrimChatHistory() {
    if (m_chatHistory.size() > MAX_CHAT_HISTORY) {
        size_t toRemove = m_chatHistory.size() - MAX_CHAT_HISTORY;
        m_chatHistory.erase(m_chatHistory.begin(), m_chatHistory.begin() + toRemove);
    }
}

bool SteamLogger::SendChatMessage(const std::string& message) {
    if (!m_bInitialized || !m_bInLobby || message.empty()) return false;

    bool success = g_pSteamMatchmaking->SendLobbyChatMsg(m_LobbyInfo.LobbyID,
        message.c_str(),
        message.length() + 1);
    if (success) {
        CSteamID localID = GetLocalSteamID();
        std::string localName = GetPlayerName(localID);
        AddChatMessage(localName, localID, message.c_str(),
            static_cast<int>(message.length()),
            k_EChatEntryTypeChatMsg, true);
    }
    return success;
}

void SteamLogger::UpdateLobbyMetadata() {
    if (!m_LobbyInfo.LobbyID.IsValid()) return;

    m_LobbyInfo.LobbyPrivateSize = atoi(g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "LobbyPrivateSize"));
    m_LobbyInfo.LobbySize = atoi(g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "LobbySize"));

    const char* ownerIDStr = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "OwnerID");
    m_LobbyInfo.OwnerID = ownerIDStr[0] ? CSteamID(strtoull(ownerIDStr, nullptr, 10)) : k_steamIDNil;

    const char* ownerNameStr = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "OwnerName");
    m_LobbyInfo.OwnerName = ownerNameStr[0] ? ownerNameStr : "";

    m_LobbyInfo.Protocol = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "Protocol");
    m_LobbyInfo.RoomType = atoi(g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "RoomType"));
    m_LobbyInfo.SearchRegion = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "SearchRegion");
    m_LobbyInfo.SGLobbyName = atoi(g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "SGLobbyName"));
    m_LobbyInfo.SGLobbyType = atoi(g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "SGLobbyType"));
    m_LobbyInfo.SGRegion = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "SGRegion");
    m_LobbyInfo.Skill = g_pSteamMatchmaking->GetLobbyData(m_LobbyInfo.LobbyID, "Skill");
}

void SteamLogger::RefreshMemberData(LobbyMember& member) {
    if (!m_LobbyInfo.LobbyID.IsValid()) return;

    member.Name = g_pSteamFriends->GetFriendPersonaName(member.SteamID);

    const char* inGameStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
        member.SteamID,
        "InGame");
    member.InGame = (inGameStr[0] == 'T' || inGameStr[0] == 't');

    const char* readyStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
        member.SteamID,
        "Ready");
    member.Ready = (readyStr[0] == 'T' || readyStr[0] == 't');

    const char* oppStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
        member.SteamID,
        "Opp");
    member.Opp = oppStr[0] ? CSteamID(strtoull(oppStr, nullptr, 10)) : k_steamIDNil;

    const char* locStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
        member.SteamID,
        "Loc");
    member.Loc = atoi(locStr);

    const char* winsStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
        member.SteamID,
        "Wins");
    member.Wins = atoi(winsStr);
}

void SteamLogger::RefreshLobbyMetadata() {
    m_LobbyInfo.numMembers = g_pSteamMatchmaking->GetNumLobbyMembers(m_LobbyInfo.LobbyID);
    UpdateLobbyMetadata();
}

void SteamLogger::UpdateMemberData(CSteamID memberID) {
    LobbyMember* member = m_LobbyInfo.FindMember(memberID);
    if (member) RefreshMemberData(*member);
}

void SteamLogger::UpdateAllMembers() {
    m_LobbyInfo.LobbyMembers.clear();
    m_LobbyInfo.numMembers = g_pSteamMatchmaking->GetNumLobbyMembers(m_LobbyInfo.LobbyID);

    for (int i = 0; i < m_LobbyInfo.numMembers; i++) {
        CSteamID memberID = g_pSteamMatchmaking->GetLobbyMemberByIndex(m_LobbyInfo.LobbyID, i);
        if (memberID.IsValid()) {
            LobbyMember newMember;
            newMember.SteamID = memberID;
            RefreshMemberData(newMember);
            m_LobbyInfo.LobbyMembers.push_back(newMember);
        }
    }
}

void SteamLogger::UpdateOpponentAndSpectators() {
    if (!m_bInLobby || !m_LobbyInfo.LobbyID.IsValid()) {
        m_opponentSteamID = k_steamIDNil;
        m_spectators.clear();
        return;
    }

    CSteamID localID = GetLocalSteamID();
    CSteamID currentOpponent = k_steamIDNil;
    std::vector<CSteamID> currentSpectators;

    for (const auto& member : m_LobbyInfo.LobbyMembers) {
        if (member.SteamID == localID) {
            if (member.Opp.IsValid() && member.Opp != k_steamIDNil) {
                currentOpponent = member.Opp;
            }
        }
        else {
            const char* isSpecStr = g_pSteamMatchmaking->GetLobbyMemberData(m_LobbyInfo.LobbyID,
                member.SteamID,
                "IsSpectator");
            bool isSpectator = false;

            if (isSpecStr && isSpecStr[0] != '\0') {
                isSpectator = (isSpecStr[0] == 'T' || isSpecStr[0] == 't');
            }
            else if (!member.Ready && !member.InGame) {
                isSpectator = true;
            }

            if (isSpectator) {
                currentSpectators.push_back(member.SteamID);
            }
        }
    }

    m_opponentSteamID = currentOpponent;
    m_spectators = currentSpectators;
}

bool SteamLogger::IsSpectator(CSteamID steamID) const {
    return std::find(m_spectators.begin(), m_spectators.end(), steamID) != m_spectators.end();
}

void SteamLogger::OnLobbyEnter(LobbyEnter_t* pCallback) {
    m_LobbyInfo.Clear();
    ClearChatHistory();

    if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        m_LobbyInfo.errorMessage = std::to_string(pCallback->m_EChatRoomEnterResponse);
        m_bInLobby = false;
        return;
    }

    m_LobbyInfo.LobbyID = CSteamID(pCallback->m_ulSteamIDLobby);
    RefreshLobbyMetadata();
    UpdateAllMembers();
    UpdateOpponentAndSpectators();
    m_bInLobby = true;
}

void SteamLogger::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback) {
    CSteamID lobbyID(pCallback->m_ulSteamIDLobby);
    CSteamID memberID(pCallback->m_ulSteamIDMember);

    if (lobbyID != m_LobbyInfo.LobbyID) return;

    if (memberID == lobbyID) {
        RefreshLobbyMetadata();
    }
    else {
        UpdateMemberData(memberID);
        UpdateOpponentAndSpectators();
    }
}

void SteamLogger::OnLobbyChatMessage(LobbyChatMsg_t* pCallback) {
    CSteamID senderID;
    EChatEntryType entryType;
    char messageData[4096];

    int messageSize = g_pSteamMatchmaking->GetLobbyChatEntry(
        pCallback->m_ulSteamIDLobby,
        pCallback->m_iChatID,
        &senderID,
        messageData,
        sizeof(messageData),
        &entryType
    );

    if (messageSize > 0) {
        const char* senderName = g_pSteamFriends->GetFriendPersonaName(senderID);
        bool isLocalPlayer = (senderID == GetLocalSteamID());
        AddChatMessage(senderName ? senderName : "Unknown",
            senderID,
            messageData,
            messageSize,
            entryType,
            isLocalPlayer);
    }
}

void SteamLogger::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {
    CSteamID lobbyID(pCallback->m_ulSteamIDLobby);
    if (lobbyID != m_LobbyInfo.LobbyID) return;

    UpdateAllMembers();
}