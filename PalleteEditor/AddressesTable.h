#pragma once
class AddressTable {
private:
    // Приватные статические члены с префиксом s_
    inline static int s_Base_Adress_For_Delete = 0x0;
    inline static int s_Base_Adress = 0x852178;
    inline static int s_Offset_GameStatus = 0x168;
    inline static int s_Offset_Character = 0x5E8;
    inline static int s_Offset_Name = 0x310;
    inline static int s_Offset_PaletteData = 0x328;
    inline static int s_Offset_CurrentPalette = 0x330;
    inline static int s_Offset_PaletteTotalOffset = 0x0;
    inline static int s_Offset_NumberOfColor = 0x4;
    inline static int s_Offset_ColorCodeOffset = 0x8;
    inline static int s_Offset_HueShiftOffset = 0x2C;
    inline static int s_NEW_Base_Adress_DonotdisplayCHAR = 0x185534;
    inline static int s_NEW_Base_Adress_DonotdisplaySHADOWS = 0x186060;
    inline static int s_NEW_Base_Adress_Display_SuperShadowforever = 0xA479B;
    inline static int s_NEW_Offset_LineColor = 0x38;
    inline static int s_NEW_Offset_SuperShadow = 0xC;

public:
    // Геттеры (возвращают константную ссылку для чтения)
    static const int& Base_Adress_For_Delete() { return s_Base_Adress_For_Delete; }
    static const int& Base_Adress() { return s_Base_Adress; }
    static const int& Offset_GameStatus() { return s_Offset_GameStatus; }
    static const int& Offset_Character() { return s_Offset_Character; }
    static const int& Offset_Name() { return s_Offset_Name; }
    static const int& Offset_PaletteData() { return s_Offset_PaletteData; }
    static const int& Offset_CurrentPalette() { return s_Offset_CurrentPalette; }
    static const int& Offset_PaletteTotalOffset() { return s_Offset_PaletteTotalOffset; }
    static const int& Offset_NumberOfColor() { return s_Offset_NumberOfColor; }
    static const int& Offset_ColorCodeOffset() { return s_Offset_ColorCodeOffset; }
    static const int& Offset_HueShiftOffset() { return s_Offset_HueShiftOffset; }
    static const int& NEW_Base_Adress_DonotdisplayCHAR() { return s_NEW_Base_Adress_DonotdisplayCHAR; }
    static const int& NEW_Base_Adress_DonotdisplaySHADOWS() { return s_NEW_Base_Adress_DonotdisplaySHADOWS; }
    static const int& NEW_Base_Adress_Display_SuperShadowforever() { return s_NEW_Base_Adress_Display_SuperShadowforever; }
    static const int& NEW_Offset_LineColor() { return s_NEW_Offset_LineColor; }
    static const int& NEW_Offset_SuperShadow() { return s_NEW_Offset_SuperShadow; }

    //For now, we have only static adresses. btw, do we really need load table from file?
    //static bool LoadFromFile();
    //static void ResetToDefaults();
};