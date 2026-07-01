#include "app_imgui.h"
#include <windows.h>

static const wchar_t* trSystem(const wchar_t* zh, const wchar_t* en) {
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE ? zh : en;
}

AppImGui::AppImGui(const std::string& currentDirectory) : mainWindow(nullptr), currentDirectory(currentDirectory) {
}

AppImGui::~AppImGui() {
}

bool AppImGui::initialize() {
    // 初始化 COM 库
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(NULL, trSystem(L"COM库初始化失败", L"Failed to initialize COM"),
                    trSystem(L"初始化错误", L"Initialization Error"), MB_OK | MB_ICONERROR);
        return false;
    }
    
    // 创建ImGui主窗口
    try {
        mainWindow = std::make_unique<ImGuiMainWindow>(currentDirectory);
        if (!mainWindow) {
            MessageBoxW(NULL, trSystem(L"无法为主窗口分配内存", L"Could not allocate memory for the main window"),
                        trSystem(L"初始化错误", L"Initialization Error"), MB_OK | MB_ICONERROR);
            CoUninitialize();
            return false;
        }
        
        if (!mainWindow->create()) {
            MessageBoxW(NULL, trSystem(L"无法创建主窗口", L"Could not create the main window"),
                        trSystem(L"初始化错误", L"Initialization Error"), MB_OK | MB_ICONERROR);
            CoUninitialize();
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        // 捕获标准C++异常
        char buffer[1024] = {0};
        sprintf(buffer, PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE ?
                "窗口创建过程中出现异常: %s" : "Exception while creating the window: %s", e.what());
        
        // 转换为宽字符
        wchar_t wbuffer[1024] = {0};
        MultiByteToWideChar(CP_ACP, 0, buffer, -1, wbuffer, 1024);
        
        MessageBoxW(NULL, wbuffer, trSystem(L"初始化错误", L"Initialization Error"), MB_OK | MB_ICONERROR);
        CoUninitialize();
        return false;
    }
    catch (...) {
        // 捕获所有其他异常
        MessageBoxW(NULL, trSystem(L"窗口创建过程中出现未知异常", L"Unknown exception while creating the window"),
                    trSystem(L"初始化错误", L"Initialization Error"), MB_OK | MB_ICONERROR);
        CoUninitialize();
        return false;
    }
}

int AppImGui::run() {
    try {
        // 显示主窗口
        mainWindow->show();
        
        // 运行主循环
        mainWindow->run();
        
        // 清理COM初始化
        CoUninitialize();
        
        return 0;
    }
    catch (const std::exception& e) {
        // 捕获标准C++异常
        char buffer[1024] = {0};
        sprintf(buffer, PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE ?
                "应用程序运行过程中出现异常: %s" : "Exception while running the application: %s", e.what());
        
        // 转换为宽字符
        wchar_t wbuffer[1024] = {0};
        MultiByteToWideChar(CP_ACP, 0, buffer, -1, wbuffer, 1024);
        
        MessageBoxW(NULL, wbuffer, trSystem(L"应用程序错误", L"Application Error"), MB_OK | MB_ICONERROR);
        
        // 清理COM初始化
        CoUninitialize();
        
        return 1;
    }
    catch (...) {
        // 捕获所有其他异常
        MessageBoxW(NULL, trSystem(L"应用程序运行过程中出现未知异常", L"Unknown exception while running the application"),
                    trSystem(L"应用程序错误", L"Application Error"), MB_OK | MB_ICONERROR);
        
        // 清理COM初始化
        CoUninitialize();
        
        return 1;
    }
} 
