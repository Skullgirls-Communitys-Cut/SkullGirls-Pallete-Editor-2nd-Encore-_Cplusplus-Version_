// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once

// add headers that you want to pre-compile here
#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <thread>
#include <vector>
#include <string>
#include <optional>

//Files
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

//ImGui:
#include <d3d9.h>
#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx9.h>
#include <ImGui/imgui_impl_win32.h>

//ImGuiFileDialog
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <ImGuiFileDialog/ImGuiFileDialogConfig.h>
#include <ImGuiFileDialog/dirent/dirent.h>


#include "Include/json.hpp" //For saves and grouping color

//Myself:
#include "Tools/Logger.hpp"

//DirectXWrapper:
#include "DirectXWrapper/common.h"
#include "DirectXWrapper/DirectXWrapper.h"
#include "DirectXWrapper/IDirect3D9Proxy.h"
#include "DirectXWrapper/IDirect3DDevice9Proxy.h"