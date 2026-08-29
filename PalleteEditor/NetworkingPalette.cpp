#include "NetworkingPalette.h"

#include "xxhash.h"
#include "tsl/ordered_map.h"
#include "steam/steam_api.h"

#include "PlayableCharactersManager.h"
#include "AutoLoadPalette.h"

bool SendMyPalettes;
bool LoadTheirPalettes;

constexpr int NETWORK_CHANNEL_PAL = 80; //Letter I in ASCII!
constexpr int NETWORK_CHANNEL_INIT = 73; //Letter I in ASCII!

auto NetworkingPaletteLogger = LOGGER::createLocal("Networking Palette", LogLevel::DEBUG_LOG);

#pragma region Callbacks

void NetworkingPalette::OnLobbyEnter(LobbyEnter_t* pCallback) {
    if (pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Joined a Lobby!");
        LobbyID = CSteamID(pCallback->m_ulSteamIDLobby);
        LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "LobbyID: ", std::to_string(LobbyID.ConvertToUint64()));
    }
    else {
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Can't join a Lobby! Error Code: ", pCallback->m_EChatRoomEnterResponse);
        LobbyID = k_steamIDNil;
    }
}


void NetworkingPalette::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {
    if (pCallback->m_ulSteamIDUserChanged == SteamUser()->GetSteamID().ConvertToUint64()) {

        uint32_t state = pCallback->m_rgfChatMemberStateChange;

        bool isLeaving = (state & (k_EChatMemberStateChangeLeft |
            k_EChatMemberStateChangeKicked |
            k_EChatMemberStateChangeDisconnected |
            k_EChatMemberStateChangeBanned)) != 0;

        if (isLeaving) {
            m_MatchInfo.Clear();


            // Логируем конкретную причину, если нужно
            if (state & k_EChatMemberStateChangeLeft) {
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Left lobby voluntarily");
            }
            if (state & k_EChatMemberStateChangeKicked) {
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Kicked from lobby");
            }
            if (state & k_EChatMemberStateChangeDisconnected) {
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Disconnected from lobby");
            }
            if (state & k_EChatMemberStateChangeBanned) {
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Banned from lobby");
            }
        }
        else if (state & k_EChatMemberStateChangeEntered) {
            LobbyID = CSteamID(pCallback->m_ulSteamIDLobby);
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "Entered lobby");
        }
    }
}

#define MAX_MESSAGE_SIZE 256
void NetworkingPalette::OnLobbyChatMessage(LobbyChatMsg_t* pCallback) {

    char Message[MAX_MESSAGE_SIZE];
    int messageSize = SteamMatchmaking()->GetLobbyChatEntry(
        pCallback->m_ulSteamIDLobby,
        pCallback->m_iChatID,
        NULL,
        Message,
        sizeof(Message),
        NULL
    );

    if (memcmp(Message, "MINF", 4) == 0) {
        uint64_t steamID64P1 = 0;
        uint64_t steamID64P2 = 0;
        uint32_t matchIdRng0 = 0;

        // Извлекаем SteamID1 (little-endian)
        memcpy(&steamID64P1, Message + 4, 8);
        // Извлекаем SteamID2 (little-endian)
        memcpy(&steamID64P2, Message + 12, 8);
        // Извлекаем MatchID/RNG0 (little-endian)
        memcpy(&matchIdRng0, Message + 20, 4);

        CSteamID steamIDP1(steamID64P1);
        CSteamID steamIDP2(steamID64P2);

        if (steamIDP1 == SteamUser()->GetSteamID() or
            steamIDP2 == SteamUser()->GetSteamID()) {

            m_MatchInfo.player1SteamID = steamIDP1;
            m_MatchInfo.player2SteamID = steamIDP2;
            m_MatchInfo.rng0 = matchIdRng0;
            m_MatchInfo.isSpectator = false;

            LOG_LOCAL_INFO(NetworkingPaletteLogger, "We are playing this match! ", matchIdRng0);

            CSteamID opponentSteamID = (SteamUser()->GetSteamID() == steamIDP1) ? steamIDP2 : steamIDP1;

            if (SteamUser()->GetSteamID() > opponentSteamID){ //If our steamID is bigger, init session
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Session Request to: ", SteamFriends()->GetFriendPersonaName(opponentSteamID));
                SteamNetworking()->SendP2PPacket(
                    opponentSteamID,
                    NULL,
                    NULL,
                    EP2PSend::k_EP2PSendUnreliable,
                    NETWORK_CHANNEL_INIT
                );
            }
        }
    }
    else if (memcmp(Message, "SPEC", 4) == 0) {
        uint64_t steamID64Spec = 0;
        uint64_t steamID64P1 = 0;
        uint64_t steamID64P2 = 0;
        uint32_t matchIdRng0 = 0;

        // Извлекаем steamIDSpec (little-endian)
        memcpy(&steamID64Spec, Message + 4, 8);
        // Извлекаем steamID1 (little-endian)
        memcpy(&steamID64P1, Message + 12, 8);
        // Извлекаем steamID2 (little-endian)
        memcpy(&steamID64P2, Message + 20, 8);

        //Be careful! We skip 4 bytes from message, becouse it's SpecID or something - we don't need that
        memcpy(&matchIdRng0, Message + 32, 4); 

        CSteamID steamIDP1(steamID64P1);
        CSteamID steamIDP2(steamID64P2);
        CSteamID steamIDSpec(steamID64Spec);
        //We don't need check player we or not - we already done this in MINF
        if (steamIDSpec == SteamUser()->GetSteamID()) {
            m_MatchInfo.player1SteamID = steamIDP1;
            m_MatchInfo.player2SteamID = steamIDP2;
            m_MatchInfo.rng0 = matchIdRng0;
            m_MatchInfo.isSpectator = true;
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "We are watching this match! ", matchIdRng0);
            //We spectator -> We are init session
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Session Request to: ", SteamFriends()->GetFriendPersonaName(steamIDP1));
            SteamNetworking()->SendP2PPacket(
                steamIDP1,
                NULL,
                NULL,
                EP2PSend::k_EP2PSendUnreliable,
                NETWORK_CHANNEL_INIT
            );
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Session Request to: ", SteamFriends()->GetFriendPersonaName(steamIDP2));
            SteamNetworking()->SendP2PPacket(
                steamIDP2,
                NULL,
                NULL,
                EP2PSend::k_EP2PSendUnreliable,
                NETWORK_CHANNEL_INIT
            );
        }
    }
    else if (memcmp(Message, "PAL_HASH", 8) == 0) {
        if (pCallback->m_ulSteamIDUser == SteamUser()->GetSteamID().ConvertToUint64()) return;

        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Recive Palette Hash from ", SteamFriends()->GetFriendPersonaName(pCallback->m_ulSteamIDUser) );
        if (LoadTheirPalettes == false) return;
        uint64_t Hash;
        uint8_t Slot;

        memcpy(&Slot, Message + 8, 1);
        memcpy(&Hash, Message + 9, 8);

        LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "Recive Palette Hash with slot", std::to_string(Slot));
        LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "Recive Palette Hash with Hash ", std::to_string(Hash));
        uint64_t steamID = pCallback->m_ulSteamIDUser;

        bool isPlayer1 = (steamID == m_MatchInfo.player1SteamID.ConvertToUint64() && Slot <= 2);
        bool isPlayer2 = (steamID == m_MatchInfo.player2SteamID.ConvertToUint64() && Slot > 2);

        if (isPlayer1 || isPlayer2) {
            CSteamID targetID = isPlayer1 ? m_MatchInfo.player1SteamID : m_MatchInfo.player2SteamID;
            const char* playerName = isPlayer1 ? "Player 1" : "Player 2";

            LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "%s is correct", playerName);

            if (getCache().exists(Hash)) {
                getCache().put(Hash, NetworkPaletteData(Slot, getCache().get(Hash).second));
                ApplyNetworkingPalette(getCache().get(Hash));
                LOG_LOCAL_INFO(NetworkingPaletteLogger, "Apply Palette from cache!");
            }
            else {
                getCache().put(Hash, NetworkPaletteData(Slot, NULL));
                SendPaletteRequest(targetID, Hash);
            }
        }
    }
    else if (memcmp(Message, "PAL_REQ", 7) == 0) {
        if (pCallback->m_ulSteamIDUser == SteamUser()->GetSteamID().ConvertToUint64()) return;
        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Recive Palette Request from ", SteamFriends()->GetFriendPersonaName(pCallback->m_ulSteamIDUser));
        if (SendMyPalettes == false) return;
        uint64_t Hash;
        uint64_t SteamID;

        memcpy(&SteamID, Message + 7, 8);

        if (SteamID != SteamUser()->GetSteamID().ConvertToUint64()) return; //If that request not for us -> Skip

        memcpy(&Hash, Message + 15, 18);

        SendPaletteData(pCallback->m_ulSteamIDUser, Hash);

    }
}

#undef MAX_MESSAGE_SIZE

void NetworkingPalette::OnP2PSessionRequest(P2PSessionRequest_t* pCallback) {
    //We check, if a RequestSteamID in our Lobby -> AcceptP2PSession
    int memberCount = SteamMatchmaking()->GetNumLobbyMembers(LobbyID);

    for (int i = 0; i < memberCount; i++)
    {
        CSteamID memberID = SteamMatchmaking()->GetLobbyMemberByIndex(LobbyID, i);
        if (memberID == pCallback->m_steamIDRemote)
        {
            SteamNetworking()->AcceptP2PSessionWithUser(pCallback->m_steamIDRemote);
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "Accept Session Request From: ", SteamFriends()->GetFriendPersonaName(pCallback->m_steamIDRemote) );
            break;
        }
    }
}

void NetworkingPalette::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback) {

    LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Recive OnP2PSessionConnectFail");
    switch (pCallback->m_eP2PSessionError)
    {
    case k_EP2PSessionErrorNone:
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "There was no error.");
        break;
    case k_EP2PSessionErrorNoRightsToApp:
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "local user doesn't own the app that is running");
        break;
    case k_EP2PSessionErrorTimeout:
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "The connection timed out");
        break;
    default:
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "We don't know what's going on! m_eP2PSessionError = ",std::to_string(pCallback->m_eP2PSessionError));
        break;
    }  
}

#pragma endregion

void NetworkingPalette::AddNewPaletteToCache(Auto_Pal AutoPalette, uint8_t Slot)
{
    if (!InLobby()) return;

    //First, we read Palette in buffer
    std::ifstream file(AutoPalette.PalPath, std::ios::binary);

    if (!file) {
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Can't open file to add: ", AutoPalette.PalPath.c_str());
        return;
    }

    // Получаем размер файла
    auto size = std::filesystem::file_size(AutoPalette.PalPath);
    std::vector<uint8_t> buffer(size);

    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();
    //Then, generate hash
    uint64_t hash = XXH3_64bits(buffer.data(), size);

    

    if (!getCache().exists(hash)){

        NetworkPaletteData PaletteData(Slot, buffer);
        //Then, add this in cache
        getCache().put(hash, PaletteData);
    }
    LOG_LOCAL_INFO(NetworkingPaletteLogger, "Added New Palette into a cache! ",AutoPalette.CharName, " Pal Num: ", AutoPalette.PalNum, " Hash:", hash);

    SendPalettesHash(hash, Slot);
}

void NetworkingPalette::ReadIncomingPackages() {
    
    uint32 msgSize = 0;
    if(SteamNetworking()->IsP2PPacketAvailable(&msgSize, NETWORK_CHANNEL_PAL))
    {
        
        void* packet = malloc(msgSize);
        CSteamID steamIDRemote;
        uint32 bytesRead = 0;
        
        if (SteamNetworking()->ReadP2PPacket(packet, msgSize, &bytesRead, &steamIDRemote, NETWORK_CHANNEL_PAL))
        {
            //bool Close = SteamNetworking()->CloseP2PSessionWithUser(steamIDRemote); //Close instanly after reading!
            //LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "CloseP2PSessionWithUser", SteamFriends()->GetFriendPersonaName(steamIDRemote),std::to_string(Close) );
            LOG_LOCAL_INFO(NetworkingPaletteLogger,"Read Packet From:", SteamFriends()->GetFriendPersonaName(steamIDRemote));
            LOG_LOCAL_INFO(NetworkingPaletteLogger, "Packet Lenght: ", msgSize);
            std::string hexString;
            uint8_t* byteData = reinterpret_cast<uint8_t*>(packet);

            for (uint32 i = 0; i < 16; ++i)
            {
                char hexByte[8]; // "0x12, "
                snprintf(hexByte, sizeof(hexByte), "0x%02X", byteData[i]);
                hexString += hexByte;

                if (i < bytesRead - 1)
                    hexString += ", ";
            }

            LOG_LOCAL_DEBUG(NetworkingPaletteLogger, "Packet Data (Hex): ", hexString);

            uint64_t hash = XXH3_64bits(packet, msgSize);

            if (getCache().exists(hash)) {
                //LOG_LOCAL_WARN(NetworkingPaletteLogger, "Recive packet with hash, that already been in cache ", std::to_string(hash));
            
            NetworkPaletteData CurrentPalette = getCache().get(hash);

            CurrentPalette.second.assign(
                static_cast<uint8_t*>(packet),
                static_cast<uint8_t*>(packet) + bytesRead
            );
            getCache().put(hash, CurrentPalette);
            ApplyNetworkingPalette(CurrentPalette);
            }
        }
         
        free(packet);
        
    }
   
}
void NetworkingPalette::SendPalettesHash(uint64_t Hash, uint8_t Slot)
{
    if (!InLobby()) return;

    std::string Header = "PAL_HASH";

    char Message[17] = "Some bad";
    memcpy(Message, Header.c_str(), Header.size());  // Copy Header without /0
    memcpy(Message + Header.size(), &Slot, sizeof(Slot));
    memcpy(Message + Header.size() + sizeof(Slot), &Hash, sizeof(Hash));  // Copy Hash

    bool success = SteamMatchmaking()->SendLobbyChatMsg(
        LobbyID,
        Message,
        sizeof(Message)
    );

    if (success) {
        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Palette Hash ", "with slot", std::to_string(Slot), "with hash ", std::to_string(Hash));
    }
    else {
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Something bad happend, when send Palette Hash ", std::to_string(Hash));
    }
}

void NetworkingPalette::SendPaletteRequest(CSteamID Responsible, uint64_t Hash) {

    if (!InLobby()) return;

    std::string Header = "PAL_REQ";
    uint64_t Responsible64 = Responsible.ConvertToUint64();
    char Message[23] = "Some bad";
    memcpy(Message, Header.c_str(), Header.size());  // Copy Header without /0
    memcpy(Message + Header.size(), &Responsible64, sizeof(Responsible64)); //SteamID
    memcpy(Message + Header.size() + sizeof(Responsible64), &Hash, sizeof(Hash));  // Copy Hash

    bool success = SteamMatchmaking()->SendLobbyChatMsg(
        LobbyID,
        Message,
        sizeof(Message)
    );

    if (success) {
        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Palette Request to", SteamFriends()->GetFriendPersonaName(Responsible), "with hash ", std::to_string(Hash));
    }
    else {
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Something bad happend, when send Palette Request ", std::to_string(Hash));
    }
}

void NetworkingPalette::SendPaletteData(CSteamID Responsible, uint64_t Hash) {
    NetworkPaletteData PaletteData = getCache().get(Hash);

    bool success = SteamNetworking()->SendP2PPacket(
        Responsible,
        PaletteData.second.data(),
        PaletteData.second.size(),
        EP2PSend::k_EP2PSendReliable,
        NETWORK_CHANNEL_PAL
    );

    if (success) {
        LOG_LOCAL_INFO(NetworkingPaletteLogger, "Send Palette Data to", SteamFriends()->GetFriendPersonaName(Responsible), "with hash ", std::to_string(Hash));
    }
    else {
        LOG_LOCAL_ERROR(NetworkingPaletteLogger, "Something bad happend, when send Palette Data ", std::to_string(Hash));
    }
}

void NetworkingPalette::ApplyNetworkingPalette(NetworkPaletteData PaletteData)
{
    PlayableCharactersManager::LoadCharacter(PaletteData.first);
    auto& currentChar = PlayableCharactersManager::GetCurrentCharacter();
    PlayableCharacter temp_Character = currentChar; //We will make a copy so as not to affect the original. 

    auto& Palette = PaletteData.second;

    size_t offset = 0;

    // Чтение имени персонажа (16 байт)
    if (offset + 16 > Palette.size()) {
        LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete palette data!");
        return;
    }
    memcpy(temp_Character.Char_Name, Palette.data() + offset, 16);
    offset += 16;

    // Проверка имени персонажа
    if (strcmp(temp_Character.Char_Name, currentChar.Char_Name)) {
        LOG_LOCAL_WARN(NetworkingPaletteLogger, "This palette is of another character!");
        return;
    }

    // Чтение количества цветов
    if (offset + sizeof(temp_Character.Num_Of_Color) > Palette.size()) {
        LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete palette data!");
        return;
    }
    memcpy(&temp_Character.Num_Of_Color, Palette.data() + offset, sizeof(temp_Character.Num_Of_Color));
    offset += sizeof(temp_Character.Num_Of_Color);

    // Чтение HueShift_Cos (1 байт)
    if (offset + 1 > Palette.size()) {
        LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete palette data!");
        return;
    }
    memcpy(&temp_Character.HueShift_Cos, Palette.data() + offset, 1);
    offset += 1;

    // Чтение HueShift_Sin (1 байт)
    if (offset + 1 > Palette.size()) {
        LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete palette data!");
        return;
    }
    memcpy(&temp_Character.HueShift_Sin, Palette.data() + offset, 1);
    offset += 1;

    // Чтение цветов (начиная со второго)
    for (int i = 1; i < temp_Character.Num_Of_Color; i++) {
        if (offset + sizeof(temp_Character.Character_Colors[i]) > Palette.size()) {
            LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete palette data!");
            return;
        }
        memcpy(&temp_Character.Character_Colors[i], Palette.data() + offset, sizeof(temp_Character.Character_Colors[i]));
        offset += sizeof(temp_Character.Character_Colors[i]);
    }

    // Если есть дополнительные данные (опциональные цвета)
    if (offset < Palette.size()) {
        // Чтение LineColor
        if (offset + sizeof(temp_Character.LineColor) > Palette.size()) {
            LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete optional color data!");
            return;
        }
        memcpy(&temp_Character.LineColor, Palette.data() + offset, sizeof(temp_Character.LineColor));
        offset += sizeof(temp_Character.LineColor);

        // Чтение SuperShadowColor1
        if (offset + sizeof(temp_Character.SuperShadowColor1) > Palette.size()) {
            LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete optional color data!");
            return;
        }
        memcpy(&temp_Character.SuperShadowColor1, Palette.data() + offset, sizeof(temp_Character.SuperShadowColor1));
        offset += sizeof(temp_Character.SuperShadowColor1);

        // Чтение SuperShadowColor2
        if (offset + sizeof(temp_Character.SuperShadowColor2) > Palette.size()) {
            LOG_LOCAL_WARN(NetworkingPaletteLogger, "Incomplete optional color data!");
            return;
        }
        memcpy(&temp_Character.SuperShadowColor2, Palette.data() + offset, sizeof(temp_Character.SuperShadowColor2));
        offset += sizeof(temp_Character.SuperShadowColor2);
    }

    currentChar = temp_Character; //Updata local copy, ONLY if reading of files is correct.

    for (int i = 0; i < currentChar.Character_Colors.size(); i++) { //Now, write our colors
        PlayableCharactersManager::ChangePaletteColor(i, currentChar.Character_Colors[i]);
    }

    PlayableCharactersManager::ChangeOptionPaletteColor(currentChar.LineColor, ColorOptionFlag::FLAG_LINE_COLOR);
    PlayableCharactersManager::ChangeOptionPaletteColor(currentChar.SuperShadowColor1, ColorOptionFlag::FLAG_SUPER_SHADOW_1);
    PlayableCharactersManager::ChangeOptionPaletteColor(currentChar.SuperShadowColor2, ColorOptionFlag::FLAG_SUPER_SHADOW_2);

    PlayableCharactersManager::instance().SetCurrentCharacterIndex(-1);

    LOG_LOCAL_INFO(NetworkingPaletteLogger, "Applied Network Palette data!");
}

