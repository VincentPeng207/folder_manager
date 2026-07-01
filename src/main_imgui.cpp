// 确保在任何其他代码之前定义UNICODE和_UNICODE
#define UNICODE
#define _UNICODE

#include <windows.h>
#include "app_imgui.h"
#include <string>
#include <vector>
#include "ui/imgui_main_window.h"
#include <fstream>

static const wchar_t* trSystem(const wchar_t* zh, const wchar_t* en) {
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE ? zh : en;
}

// 错误处理辅助函数
void ShowLastError(const wchar_t* message) {
    DWORD error = GetLastError();
    wchar_t buffer[256] = {0};
    
    if (FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        0,
        buffer,
        256,
        NULL
    )) {
        std::wstring fullMessage = message;
        fullMessage += trSystem(L"\n错误代码: ", L"\nError code: ");
        fullMessage += std::to_wstring(error);
        fullMessage += trSystem(L"\n错误信息: ", L"\nError message: ");
        fullMessage += buffer;
        
        MessageBoxW(NULL, fullMessage.c_str(), trSystem(L"错误", L"Error"), MB_OK | MB_ICONERROR);
    } else {
        std::wstring fallbackMessage = message;
        fallbackMessage += trSystem(L"\n错误代码: ", L"\nError code: ");
        fallbackMessage += std::to_wstring(error);
        
        MessageBoxW(NULL, fallbackMessage.c_str(), trSystem(L"错误", L"Error"), MB_OK | MB_ICONERROR);
    }
}

// 命令行参数解析辅助函数
std::vector<std::string> parseCommandLineArguments(LPSTR lpCmdLine) {
    std::vector<std::string> args;
    
    // 获取命令行参数数量
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (wargv) {
        // 跳过第一个参数（程序路径）
        for (int i = 1; i < argc; i++) {
            // 将宽字符串转换为多字节字符串
            char buffer[MAX_PATH] = {0};
            WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, buffer, MAX_PATH, NULL, NULL);
            args.push_back(buffer);
        }
        
        // 释放内存
        LocalFree(wargv);
    }
    
    return args;
}

int main(int argc, char* argv[]) {
    // 创建日志文件
    // std::ofstream logFile("app_debug.log");
    
    // 写入启动信息
    // logFile << "应用程序开始启动" << std::endl;
    
    // 命令行参数表示当前目录
    std::string currentDirectory = "";
    
    if (argc > 1) {
        currentDirectory = argv[1];
        // logFile << "当前目录: " << currentDirectory << std::endl;
    }
    
    try {
        // 创建应用程序实例
        // logFile << "正在创建应用程序实例..." << std::endl;
        AppImGui app(currentDirectory);
        
        // 初始化应用程序
        // logFile << "正在初始化应用程序..." << std::endl;
        bool result = app.initialize();
        
        if (!result) {
            // logFile << "应用程序初始化失败" << std::endl;
            return 1;
        }
        
        // 显示窗口创建成功消息
        // logFile << "应用程序初始化成功，即将开始主循环" << std::endl;
        
        // 运行应用程序
        // logFile << "开始执行主循环..." << std::endl;
        app.run();
        
        // logFile << "应用程序正常退出" << std::endl;
    }
    catch (const std::exception& e) {
        // logFile << "捕获到异常: " << e.what() << std::endl;
    }
    catch (...) {
        // logFile << "捕获到未知异常" << std::endl;
    }
    
    // logFile.close();
    return 0;
} 
