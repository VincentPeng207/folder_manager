#ifndef APP_IMGUI_H
#define APP_IMGUI_H

#include <memory>
#include <string>
#include "ui/imgui_main_window.h"

// ImGui版本的应用程序类
class AppImGui {
public:
    AppImGui(const std::string& currentDirectory = "");
    ~AppImGui();
    
    // 应用程序生命周期
    bool initialize();
    int run();
    
private:
    std::unique_ptr<ImGuiMainWindow> mainWindow;
    std::string currentDirectory; // 存储当前目录
};

#endif // APP_IMGUI_H 