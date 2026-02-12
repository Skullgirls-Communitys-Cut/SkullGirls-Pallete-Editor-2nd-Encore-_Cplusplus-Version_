#include "pch.h"
#include "SteamLogger.h"

bool SteamLogger::SendChatMessage(const std::string& message) {
    if (!m_bInLobby || message.empty() || !SteamMatchmaking()) return false;

    bool success = SteamMatchmaking()->SendLobbyChatMsg(m_LobbyInfo.LobbyID,
        message.c_str(),
        message.length() + 1);
    return success;
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

void SteamLogger::UpdateLobbyMetadata() {
    if (!m_LobbyInfo.LobbyID.IsValid() || !SteamMatchmaking()) return;

    const char* privateSizeStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "LobbyPrivateSize");
    m_LobbyInfo.LobbyPrivateSize = privateSizeStr && privateSizeStr[0] ? atoi(privateSizeStr) : 0;

    const char* sizeStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "LobbySize");
    m_LobbyInfo.LobbySize = sizeStr && sizeStr[0] ? atoi(sizeStr) : 0;

    const char* ownerIDStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "OwnerID");
    m_LobbyInfo.OwnerID = (ownerIDStr && ownerIDStr[0]) ? CSteamID(strtoull(ownerIDStr, nullptr, 10)) : k_steamIDNil;

    const char* ownerNameStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "OwnerName");
    m_LobbyInfo.OwnerName = (ownerNameStr && ownerNameStr[0]) ? ownerNameStr : "";

    const char* protocolStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "Protocol");
    m_LobbyInfo.Protocol = (protocolStr && protocolStr[0]) ? protocolStr : "";

    const char* roomTypeStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "RoomType");
    m_LobbyInfo.RoomType = (roomTypeStr && roomTypeStr[0]) ? atoi(roomTypeStr) : 0;

    const char* searchRegionStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "SearchRegion");
    m_LobbyInfo.SearchRegion = (searchRegionStr && searchRegionStr[0]) ? searchRegionStr : "";

    const char* sgLobbyNameStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "SGLobbyName");
    m_LobbyInfo.SGLobbyName = (sgLobbyNameStr && sgLobbyNameStr[0]) ? atoi(sgLobbyNameStr) : 0;

    const char* sgLobbyTypeStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "SGLobbyType");
    m_LobbyInfo.SGLobbyType = (sgLobbyTypeStr && sgLobbyTypeStr[0]) ? atoi(sgLobbyTypeStr) : 0;

    const char* sgRegionStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "SGRegion");
    m_LobbyInfo.SGRegion = (sgRegionStr && sgRegionStr[0]) ? sgRegionStr : "";

    const char* skillStr = SteamMatchmaking()->GetLobbyData(m_LobbyInfo.LobbyID, "Skill");
    m_LobbyInfo.Skill = (skillStr && skillStr[0]) ? skillStr : "";
}

void SteamLogger::RefreshMemberData(LobbyMember& member) {
    if (!m_LobbyInfo.LobbyID.IsValid() || !SteamMatchmaking() || !SteamFriends()) return;

    const char* name = SteamFriends()->GetFriendPersonaName(member.SteamID);
    member.Name = name ? name : "Unknown";

    const char* inGameStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "InGame");
    member.InGame = (inGameStr && inGameStr[0] && (inGameStr[0] == 'T' || inGameStr[0] == 't'));

    const char* readyStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "Ready");
    member.Ready = (readyStr && readyStr[0] && (readyStr[0] == 'T' || readyStr[0] == 't'));

    const char* IsSpectatorStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "IsSpectator");
    member.IsSpectator = *IsSpectatorStr;

    const char* oppStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "Opp");
    member.Opp = (oppStr && oppStr[0]) ? CSteamID(strtoull(oppStr, nullptr, 10)) : k_steamIDNil;

    const char* locStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "Loc");
    member.Loc = (locStr && locStr[0]) ? atoi(locStr) : 0;

    const char* winsStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "Wins");
    member.Wins = (winsStr && winsStr[0]) ? atoi(winsStr) : 0;

    const char* PlaysStr = SteamMatchmaking()->GetLobbyMemberData(m_LobbyInfo.LobbyID, member.SteamID, "Plays");
    member.Plays = (PlaysStr && PlaysStr[0]) ? atoi(PlaysStr) : 0;
}

void SteamLogger::RefreshLobbyMetadata() {
    if (!m_LobbyInfo.LobbyID.IsValid() || !SteamMatchmaking()) return;
    m_LobbyInfo.numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_LobbyInfo.LobbyID);
    UpdateLobbyMetadata();
}

void SteamLogger::UpdateMemberData(CSteamID memberID) {
    LobbyMember* member = m_LobbyInfo.FindMember(memberID);
    if (member) RefreshMemberData(*member);
}

void SteamLogger::UpdateAllMembers() {
    if (!m_LobbyInfo.LobbyID.IsValid() || !SteamMatchmaking()) return;

    m_LobbyInfo.LobbyMembers.clear();
    m_LobbyInfo.numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_LobbyInfo.LobbyID);

    for (int i = 0; i < m_LobbyInfo.numMembers; i++) {
        CSteamID memberID = SteamMatchmaking()->GetLobbyMemberByIndex(m_LobbyInfo.LobbyID, i);
        if (memberID.IsValid()) {
            LobbyMember newMember;
            newMember.SteamID = memberID;
            RefreshMemberData(newMember);
            m_LobbyInfo.LobbyMembers.push_back(newMember);
        }
    }
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
    }
}

void SteamLogger::OnLobbyChatMessage(LobbyChatMsg_t* pCallback) {
    CSteamID senderID;
    EChatEntryType entryType;
    char messageData[4096];

    if (!SteamMatchmaking() || !SteamFriends()) return;

    int messageSize = SteamMatchmaking()->GetLobbyChatEntry(
        pCallback->m_ulSteamIDLobby,
        pCallback->m_iChatID,
        &senderID,
        messageData,
        sizeof(messageData),
        &entryType
    );

    if (messageSize > 0) {
        const char* senderName = SteamFriends()->GetFriendPersonaName(senderID);
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