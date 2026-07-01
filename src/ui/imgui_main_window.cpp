#include "imgui_main_window.h"
#include "../util/uuid_generator.h"
#include "../util/json_handler.h"
#include "../filesystem/directory_ops.h"

// STB Image for icon loading
#include "../../include/stb/stb_image.h"

// Dear ImGui相关头文件
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// 实用工具函数
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

// C++标准库
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>

// ImGui风格常量 - 现代化配色
const ImVec4 COLOR_BG = ImVec4(0.08f, 0.12f, 0.16f, 1.00f);           // 深蓝色背景
const ImVec4 COLOR_BG_PANEL = ImVec4(0.15f, 0.20f, 0.25f, 1.00f);     // 面板背景色
const ImVec4 COLOR_TEXT = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);         // 文本色
const ImVec4 COLOR_TEXT_DIM = ImVec4(0.60f, 0.65f, 0.70f, 1.00f);     // 次要文本色
const ImVec4 COLOR_BORDER = ImVec4(0.25f, 0.30f, 0.35f, 0.50f);       // 边框色
const ImVec4 COLOR_ACCENT = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);       // 主题色(蓝色)
const ImVec4 COLOR_ACCENT_DARK = ImVec4(0.19f, 0.43f, 0.72f, 1.00f);  // 深主题色
const ImVec4 COLOR_BUTTON = ImVec4(0.20f, 0.41f, 0.68f, 0.40f);       // 按钮色
const ImVec4 COLOR_BUTTON_HOVER = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // 按钮悬停色
const ImVec4 COLOR_BUTTON_ACTIVE = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);// 按钮激活色 
const ImVec4 COLOR_HEADER = ImVec4(0.20f, 0.40f, 0.60f, 0.45f);       // 标题色
const ImVec4 COLOR_HEADER_HOVERED = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
const ImVec4 COLOR_HEADER_ACTIVE = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
const ImVec4 COLOR_SUCCESS = ImVec4(0.26f, 0.85f, 0.60f, 1.00f);      // 成功色(绿色)
const ImVec4 COLOR_WARNING = ImVec4(0.95f, 0.66f, 0.06f, 1.00f);      // 警告色(黄色)
const ImVec4 COLOR_ERROR = ImVec4(0.95f, 0.35f, 0.35f, 1.00f);        // 错误色(红色)

// 选择目录对话框辅助函数
std::string selectDirectoryDialog(UiLanguage uiLanguage = UiLanguage::Chinese) {
    auto trDialog = [uiLanguage](const char* zh, const char* en) {
        return uiLanguage == UiLanguage::Chinese ? zh : en;
    };
    std::string selectedPath;
    
    // 初始化COM库
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileDialog* pFileDialog;
        
        // 创建文件对话框对象
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                             IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));
        
        if (SUCCEEDED(hr)) {
            // 设置选项为选择文件夹
            DWORD dwOptions;
            hr = pFileDialog->GetOptions(&dwOptions);
            if (SUCCEEDED(hr)) {
                hr = pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
                
                if (SUCCEEDED(hr)) {
                    // 显示对话框
                    hr = pFileDialog->Show(NULL);
                    
                    if (SUCCEEDED(hr)) {
                        // 获取所选项目
                        IShellItem* pItem;
                        hr = pFileDialog->GetResult(&pItem);
                        
                        if (SUCCEEDED(hr)) {
                            PWSTR pszFilePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                            
                            if (SUCCEEDED(hr)) {
                                // 转换宽字符路径为多字节字符串
                                int pathLen = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                                if (pathLen > 0) {
                                    char* buffer = new char[pathLen];
                                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer, pathLen, NULL, NULL);
                                    selectedPath = buffer;
                                    delete[] buffer;
                                }
                                
                                CoTaskMemFree(pszFilePath);
                            }
                            pItem->Release();
                        }
                    }
                    else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
                        // 用户取消了对话框，这是正常的
                        // 不需要显示错误消息
                    }
                    else {
                        MessageBoxA(NULL, trDialog("显示文件对话框失败", "Failed to show file dialog"),
                                    trDialog("错误", "Error"), MB_OK | MB_ICONERROR);
                    }
                }
                else {
                    MessageBoxA(NULL, trDialog("设置文件对话框选项失败", "Failed to set file dialog options"),
                                trDialog("错误", "Error"), MB_OK | MB_ICONERROR);
                }
            }
            else {
                MessageBoxA(NULL, trDialog("获取文件对话框选项失败", "Failed to get file dialog options"),
                            trDialog("错误", "Error"), MB_OK | MB_ICONERROR);
            }
            pFileDialog->Release();
        }
        else {
            MessageBoxA(NULL, trDialog("创建文件对话框失败", "Failed to create file dialog"),
                        trDialog("错误", "Error"), MB_OK | MB_ICONERROR);
        }
        CoUninitialize();
    }
    else {
        MessageBoxA(NULL, trDialog("初始化COM库失败", "Failed to initialize COM"),
                    trDialog("错误", "Error"), MB_OK | MB_ICONERROR);
    }
    
    return selectedPath;
}

// GLFW错误回调
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

const char* ImGuiMainWindow::tr(const char* zh, const char* en) const {
    return uiLanguage == UiLanguage::Chinese ? zh : en;
}

std::string ImGuiMainWindow::trStr(const char* zh, const char* en) const {
    return std::string(tr(zh, en));
}

UiLanguage ImGuiMainWindow::detectSystemLanguage() const {
    LANGID langId = GetUserDefaultUILanguage();
    return PRIMARYLANGID(langId) == LANG_CHINESE ? UiLanguage::Chinese : UiLanguage::English;
}

std::string ImGuiMainWindow::getSettingsFilePath() const {
    char appDataPath[MAX_PATH] = {0};
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) != S_OK) {
        return "folder_manager_settings.ini";
    }
    
    std::string settingsDir = std::string(appDataPath) + "\\FolderManager";
    std::filesystem::create_directories(settingsDir);
    return settingsDir + "\\settings.ini";
}

void ImGuiMainWindow::loadLanguagePreference() {
    std::ifstream inFile(getSettingsFilePath());
    if (!inFile.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(inFile, line)) {
        if (line == "language=zh-CN") {
            uiLanguage = UiLanguage::Chinese;
            return;
        }
        if (line == "language=en-US") {
            uiLanguage = UiLanguage::English;
            return;
        }
    }
}

void ImGuiMainWindow::saveLanguagePreference() const {
    std::ofstream outFile(getSettingsFilePath(), std::ios::trunc);
    if (!outFile.is_open()) {
        return;
    }
    
    outFile << "language=" << (uiLanguage == UiLanguage::Chinese ? "zh-CN" : "en-US") << "\n";
}

void ImGuiMainWindow::updateWindowTitle() {
    if (window) {
        glfwSetWindowTitle(window, tr("文件目录生成器", "Folder Manager"));
    }
}

void ImGuiMainWindow::setLanguage(UiLanguage language) {
    if (uiLanguage == language) {
        return;
    }
    
    uiLanguage = language;
    saveLanguagePreference();
    updateWindowTitle();
    updateTemplateInfo();
    
    updateStatusText(uiLanguage == UiLanguage::Chinese ? "已切换到中文" : "Language changed to English");
}

// 构造函数
ImGuiMainWindow::ImGuiMainWindow(const std::string& currentDirectory) 
    : window(nullptr), 
      currentTemplate(nullptr),
      uiLanguage(UiLanguage::Chinese),
      statusText(""), 
      templateInfo(""),
      templateNameInput(""),
      selectedTemplateIndex(0),
      selectedNodeId(""),
      searchQuery(""),
      isTemplateModified(false), 
      usePrefix(false), 
      prefixType(PREFIX_DATE), 
      dateFormat(DATE_YYYY_MM_DD), 
      includeFolderSeparator(false),
      customPrefix("Prefix"), 
      createRootFolder(true),
      targetDirectory(""),
      currentDirectory(currentDirectory),  // 初始化当前目录
      rootFolderName(""),
      showAboutDialog(false),
      showHelpDialog(false),
      showOpenFolderDialog(false),
      showConfirmGenerateDialog(false),    // 初始化确认对话框标志
      showImportFolderDialog(false),
      showConfirmDeleteTemplateDialog(false), // 初始化删除模板确认对话框标志
      openFolderPopupOpened(false),
      generatedFolderPath(""),
      lastEditingNodeId(""),
      isGeneratingFolders(false),
      requestClose(false)
{
    uiLanguage = detectSystemLanguage();
    loadLanguagePreference();
    
    statusText = trStr("就绪", "Ready");
    templateNameInput = trStr("新模板", "New Template");
    
    // 创建一个新的空模板
    currentTemplate = std::make_unique<Template>(UuidGenerator::generate(), templateNameInput);
    updateTemplateInfo();
    
    // 如果当前目录不为空，则自动设置为目标目录
    if (!currentDirectory.empty()) {
        targetDirectory = currentDirectory;
        updateStatusText(trStr("当前目录已设置为目标目录: ", "Current directory set as target: ") + targetDirectory);
    }
}

// 析构函数
ImGuiMainWindow::~ImGuiMainWindow() {
    shutdownImGui();
}

// 创建窗口和初始化ImGui
bool ImGuiMainWindow::create() {
    // 设置GLFW错误回调
    glfwSetErrorCallback(glfw_error_callback);
    
    // 初始化GLFW
    if (!glfwInit())
        return false;

    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // 创建窗口时设置为隐藏状态
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // 创建窗口 - 使用更紧凑的尺寸
    const char* windowTitle = tr("文件目录生成器", "Folder Manager");
    window = glfwCreateWindow(500, 500, windowTitle, NULL, NULL);
    if (window == NULL)
        return false;
    
    // 立即设置窗口位置 - 在窗口显示前设置位置
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int xpos = (mode->width - 500) / 2;
    int ypos = (mode->height - 500) / 2;
    glfwSetWindowPos(window, xpos, ypos);
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 启用垂直同步
    
    // 设置用户指针指向当前对象，以便在回调中访问
    glfwSetWindowUserPointer(window, this);
    
    // 注册窗口关闭回调
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    
    // 设置窗口图标
    setWindowIcon();
    
    // 设置ImGui
    setupImGui();
    
    // 首次空状态显示一个中性示例模板，不写入用户模板存储。
    JsonHandler jsonHandler;
    if (jsonHandler.getAvailableTemplates().empty()) {
        initializeNeutralExampleTemplate();
    }
    
    // 初始化模板列表
    refreshTemplateList();
    
    return true;
}

// 设置窗口图标
void ImGuiMainWindow::setWindowIcon() {
    // 创建日志文件
    // std::ofstream logFile("icon_debug.log");
    // if (!logFile.is_open()) {
    //     std::cerr << "无法创建日志文件" << std::endl;
    //     return;
    // }
    
    // 获取当前工作目录
    char currentPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentPath);
    std::string workingDir = currentPath;
    
    // logFile << "当前工作目录: " << workingDir << std::endl;
    
    // 尝试加载PNG/ICO文件
    GLFWimage images[1];
    images[0].pixels = nullptr;
    
    // 尝试的不同路径 - 优先使用folder_icon.png
    std::vector<std::string> iconPaths = {
        "resources/folder_icon.png",                   // 优先使用SVG转换的PNG
        workingDir + "\\resources\\folder_icon.png",   // 绝对路径
        "folder_icon.png",                             // 当前目录
        "resources/icon.ico",                          // 尝试ICO
        "resources/test_icon.png",                     // 备用测试图标
        workingDir + "\\resources\\test_icon.png"      // 备用测试图标绝对路径
    };
    
    bool iconLoaded = false;
    for (const auto& iconPath : iconPaths) {
        int width, height, channels;
        unsigned char* image_data = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);
        if (image_data) {
            images[0].width = width;
            images[0].height = height;
            images[0].pixels = image_data;
            glfwSetWindowIcon(window, 1, images);
            stbi_image_free(image_data);
            iconLoaded = true;
            // logFile << "成功从路径加载图标: " << iconPath << std::endl;
            // logFile << "图标尺寸: " << width << "x" << height << " 通道数: " << channels << std::endl;
            break;
        } else {
            // logFile << "无法从路径加载图标: " << iconPath << std::endl;
            // logFile << "错误原因: " << stbi_failure_reason() << std::endl;
        }
    }
    
    if (iconLoaded) {
        // logFile << "成功设置窗口图标" << std::endl;
    } else {
        // logFile << "无法从任何路径加载图标，可能是格式问题。" << std::endl;
        // logFile << "最后一次错误: " << stbi_failure_reason() << std::endl;
    }
    
    // logFile.close();
}

// 设置ImGui
void ImGuiMainWindow::setupImGui() {
    // 设置ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // 禁用imgui.ini文件生成
    io.IniFilename = NULL;
    
    // 启用键盘导航功能
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 启用键盘导航
    // 注意：由于当前ImGui版本不支持，注释掉Docking功能
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // 启用Docking功能
    
    // 禁用ID冲突高亮显示
    io.ConfigDebugHighlightIdConflicts = false;
    
    // 应用轻量化单色主题
    setLightweightTheme();
    
    // 设置中文字体支持
    ImFontConfig font_config;
    font_config.OversampleH = 1;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = true;
    
    // 优先使用微软雅黑，如果找不到则尝试其他字体
    const char* font_paths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",     // 微软雅黑
        "C:\\Windows\\Fonts\\simsun.ttc",   // 宋体
        "C:\\Windows\\Fonts\\simhei.ttf"    // 黑体
    };
    
    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        if (std::filesystem::exists(font_path)) {
            // 加载正常大小的字体
            io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            
            // 加载较大的字体用于图标
            ImFontConfig icon_font_config = font_config;
            icon_font_config.MergeMode = false;
            io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &icon_font_config, io.Fonts->GetGlyphRangesChineseFull());
            
            font_loaded = true;
            break;
        }
    }
    
    // 如果找不到中文字体，使用默认字体并添加中文字符范围
    if (!font_loaded) {
        // 加载默认字体
        io.Fonts->AddFontDefault(&font_config);
        
        // 加载较大的默认字体用于图标
        ImFontConfig icon_font_config = font_config;
        icon_font_config.MergeMode = false;
        io.Fonts->AddFontDefault(&icon_font_config);
    }
    
    // 初始化ImGui平台/渲染器后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

// 关闭ImGui
void ImGuiMainWindow::shutdownImGui() {
    if (window) {
        // 设置一个关闭开始时间点，用于超时处理
        auto shutdownStart = std::chrono::steady_clock::now();
        bool shutdownTimeout = false;
        
        // 标记为关闭状态，防止后续渲染循环继续处理
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
        
        try {
            // 先清理OpenGL渲染器资源
            ImGui_ImplOpenGL3_Shutdown();
            
            // 然后清理GLFW资源
            ImGui_ImplGlfw_Shutdown();
            
            // 最后清理ImGui核心资源
            ImGui::DestroyContext();
            
            // 检查是否超时
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - shutdownStart).count();
            shutdownTimeout = (elapsedMs > 2000); // 如果超过2秒就认为超时
            
            // 关闭GLFW窗口和终止GLFW
            if (!shutdownTimeout) {
                // 设置窗口用户指针为nullptr，防止回调函数访问已释放的资源
                glfwSetWindowUserPointer(window, nullptr);
                
                // 关闭所有回调
                glfwSetKeyCallback(window, nullptr);
                glfwSetCharCallback(window, nullptr);
                glfwSetMouseButtonCallback(window, nullptr);
                glfwSetScrollCallback(window, nullptr);
                glfwSetCursorPosCallback(window, nullptr);
                glfwSetWindowCloseCallback(window, nullptr);
                glfwSetWindowFocusCallback(window, nullptr);
                glfwSetWindowIconifyCallback(window, nullptr);
                
                // 销毁窗口
                glfwDestroyWindow(window);
                
                // 终止GLFW
                glfwTerminate();
            }
        }
        catch (...) {
            // 捕获所有异常，防止析构函数抛出异常
            shutdownTimeout = true;
        }
        
        // 如果超时或发生异常，不做更多清理，直接返回
        window = nullptr;
    }
}

// 显示窗口
void ImGuiMainWindow::show() {
    // 窗口设置完成后显示窗口
    glfwShowWindow(window);
}

// 主循环
void ImGuiMainWindow::run() {
    // 主循环
    while (!glfwWindowShouldClose(window) && !requestClose) {
        // 轮询和处理事件
        glfwPollEvents();
        
        // 如果收到关闭请求，立即跳出循环
        if (requestClose) {
            break;
        }
        
        // 处理导入目录的请求，确保在目录生成后不会被错误触发
        if (showImportFolderDialog && !isGeneratingFolders) {
            showImportFolderDialog = false;
            std::string dir = selectDirectoryDialog(uiLanguage);
            if (!dir.empty()) {
                // 确认这确实是一个导入操作
                if (MessageBoxA(NULL, tr("确认要导入此目录结构吗？", "Import this folder structure?"),
                    tr("导入确认", "Confirm Import"), 
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    importDirectoryStructure(dir);
                    updateStatusText(trStr("已成功导入目录结构: ", "Imported folder structure: ") + dir);
                    isTemplateModified = true;
                } else {
                    updateStatusText(trStr("已取消导入操作", "Import canceled"));
                }
            } else {
                updateStatusText(trStr("未选择目录，导入已取消", "No directory selected. Import canceled."));
            }
            showImportFolderDialog = false;
        }
        
        // 开始新的ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 处理快捷键
        if (ImGui::IsKeyPressed(ImGuiKey_F2) && !selectedNodeId.empty()) {
            // F2修改选定的目录
            auto node = currentTemplate->getNode(selectedNodeId);
            if (node) {
                auto& state = treeNodeStates[selectedNodeId];
                state.isEditing = true;
                state.editLabel = node->getName();
                lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
                updateStatusText(trStr("开始编辑 '", "Editing '") + node->getName() + "'");
            }
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Insert)) {
            // Insert添加目录
            addFolder();
        }
        
        // 渲染UI
        render();
        
        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        // 更新为灰色背景，与轻量化主题匹配
        glClearColor(0.26f, 0.26f, 0.26f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    
    // 当收到关闭请求时，立即开始关闭过程
    if (window) {
        // 禁用所有回调，防止回调函数在关闭过程中被触发
        glfwSetWindowCloseCallback(window, nullptr);
        glfwSetWindowFocusCallback(window, nullptr);
        glfwSetWindowIconifyCallback(window, nullptr);
        
        // 标记为关闭状态，防止后续渲染循环继续处理
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
    }
}

// 渲染ImGui UI
void ImGuiMainWindow::render() {
    // 设置窗口填满整个视口
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    // 创建无边框、无标题栏的主窗口
    ImGui::Begin("MainWindow", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    
    // 获取整个窗口的尺寸
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // 定义整个应用程序的左右边距 - 设置更小的边距值
    const float globalLeftMargin = 1.0f;  // 全局左边距，设置为极小值
    const float globalRightMargin = 1.0f; // 全局右边距，设置为极小值
    
    // 计算有效内容区域宽度（减去左右边距）
    float contentWidth = windowSize.x - globalLeftMargin - globalRightMargin;
    
    // 渲染菜单栏
    renderMenuBar();
    
    // 明确获取主菜单栏高度 - 这是关键
    float menuBarHeight = ImGui::GetFrameHeightWithSpacing();
    
    // 明确设置光标位置到菜单栏下方
    ImGui::SetCursorPos(ImVec2(globalLeftMargin, menuBarHeight));
    
    // 计算各区域高度 - 使用固定数值
    float templatePanelHeight = 45.0f;      // 模板面板高度
    float statusBarHeight = 22.0f;          // 状态栏高度
    float buttonHeight = 30.0f;             // 按钮高度
    float buttonMargin = 5.0f;              // 按钮到状态栏的间距
    float treeViewBottomMargin = 10.0f;     // 树视图底部边距
    
    // 定义缺失的变量
    const float buttonWidth = 100.0f;       // 按钮宽度
    const float contentHeight = ImGui::GetFrameHeightWithSpacing(); // 内容高度，使用ImGui默认帧高度
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGuiWindowFlags buttonPanelFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    
    // 计算按钮区域到窗口顶部的距离
    float buttonsFromTop = windowSize.y - statusBarHeight - buttonHeight - buttonMargin;
    
    // 计算树视图区域高度 - 让它填充剩余空间，但与按钮保持间距
    float mainAreaHeight = buttonsFromTop - menuBarHeight - templatePanelHeight - treeViewBottomMargin;
    
    // Template面板（顶部）
    ImGui::SetCursorPos(ImVec2(globalLeftMargin, menuBarHeight));
    ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置

    if (ImGui::BeginChild("TemplatePanel", ImVec2(contentWidth, templatePanelHeight), true, windowFlags)) {
        // 垂直居中
        ImGui::SetCursorPosY((templatePanelHeight - contentHeight) * 0.5f);
        ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置
        renderTemplatePanel(contentWidth);
        ImGui::EndChild();
    }
    else {
        ImGui::EndChild(); // 即使BeginChild返回false，也必须调用EndChild
    }
    
    // 主要内容区（中间）
    ImGui::SetCursorPos(ImVec2(globalLeftMargin, menuBarHeight + templatePanelHeight));
    ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置
    
    // 目录树视图区域
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::BeginChild("TreeViewPanel", ImVec2(contentWidth, mainAreaHeight), true);
    
    // 删除原来的搜索框，直接从树视图开始
        ImGui::Separator();
    
    // 树视图区域
    renderTreeView();
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    // 将按钮放在窗口底部靠近状态栏的位置
    // 留出5像素的状态栏上边距
    ImGui::SetCursorPos(ImVec2(globalLeftMargin, windowSize.y - statusBarHeight - buttonHeight - buttonMargin));

    // 使用两侧对齐的按钮布局
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    
    // 保存按钮 - 左对齐，向右偏移5像素
    ImGui::SetCursorPosX(globalLeftMargin + 50.0f);
    
    // 记录推入的样式数量
    int buttonStyleCount = 0;
    
    // 根据模板修改状态设置保存按钮颜色
    if (isTemplateModified) {
        // 模板已修改，将按钮设为绿色
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
        buttonStyleCount = 3;
    }
    
    if (ImGui::Button(tr("保存模板", "Save Template"), ImVec2(buttonWidth, buttonHeight))) {
        saveTemplate();
    }
    
    // 如果之前改变了按钮颜色，恢复默认颜色
    if (buttonStyleCount > 0) {
        ImGui::PopStyleColor(buttonStyleCount);
        buttonStyleCount = 0;
    }
    
    // 删除模板按钮 - 居中对齐
    ImGui::SameLine();
    ImGui::SetCursorPosX((contentWidth - buttonWidth) / 2.0f + globalLeftMargin);
    if (ImGui::Button(tr("删除模板", "Delete Template"), ImVec2(buttonWidth, buttonHeight))) {
        // 检查是否有有效的模板
        if (!currentTemplate) {
            updateStatusText(trStr("当前没有模板可删除", "No template to delete"));
        } else if (currentTemplate->isEmpty()) {
            updateStatusText(trStr("当前模板没有保存且没有目录，无需删除", "The current template is unsaved and empty. Nothing to delete."));
        } else {
            // 显示确认对话框
            showConfirmDeleteTemplateDialog = true;
        }
    }
    
    // 生成目录按钮 - 右对齐
    ImGui::SameLine();
    ImGui::SetCursorPosX(contentWidth - buttonWidth + globalLeftMargin - 50.0f);
    if (ImGui::Button(tr("生成目录", "Generate"), ImVec2(buttonWidth, buttonHeight))) {
        // 检查当前模板是否为空
        if (currentTemplate->isEmpty()) {
            // 设置对话框位置在窗口中央
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            
            // 显示一个对话框提醒用户
            ImGui::OpenPopup("EmptyTemplateWarning");
            updateStatusText(trStr("当前模板为空，请先添加文件夹", "The current template is empty. Add a folder first."));
        } else {
            // 如果模板不为空，打开生成设置弹窗
            renderGenerationPanel();
        }
    }
    ImGui::PopStyleVar();
    
    // 计算状态栏位置
    ImGui::SetCursorPos(ImVec2(globalLeftMargin, windowSize.y - statusBarHeight));
    ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置
    
    // 底部状态栏 - 使用内容宽度
    renderStatusBar(contentWidth);
    
    // 处理生成设置弹窗
    // 设置弹窗位置在窗口中央偏左50px
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImVec2 adjustedCenter = ImVec2(center.x, center.y);
    ImGui::SetNextWindowPos(adjustedCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal(tr("生成设置", "Generation Settings"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // 弹窗标题
        const char* appTitle = tr("文件目录生成器", "Folder Manager");
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(appTitle).x) * 0.5f);
        ImGui::Text("%s", appTitle);
        ImGui::PopFont();
        
        ImGui::Separator();
        
        // 增加上方空白
        ImGui::Dummy(ImVec2(0, 10));
        
        // 生成设置标题 - 改为"设置选项"
        ImGui::Text("%s", tr("设置选项", "Options"));
        ImGui::Separator();
        
        // 生成选项内容 - 主要选项
        const float optionsWidth = 400.0f; // 弹窗宽度
        
        // 直接生成选项
        bool isCreateRoot = createRootFolder;
        if (ImGui::RadioButton(tr("直接生成", "Generate directly"), !isCreateRoot)) {
            createRootFolder = false;
        }
        
        // 创建顶层目录选项
        if (ImGui::RadioButton(tr("创建顶层目录", "Create top-level folder"), isCreateRoot)) {
            createRootFolder = true;
        }
        
        // 如果选择了创建顶层目录，显示输入框
        if (createRootFolder) {
    ImGui::SameLine();
            ImGui::SetNextItemWidth(optionsWidth - 150.0f);
            // 为InputText准备字符缓冲区
            static char rootFolderBuffer[256] = "";
            // 当缓冲区为空时，复制字符串到缓冲区
            if (rootFolderBuffer[0] == '\0' && !rootFolderName.empty()) {
                strncpy(rootFolderBuffer, rootFolderName.c_str(), sizeof(rootFolderBuffer) - 1);
                rootFolderBuffer[sizeof(rootFolderBuffer) - 1] = '\0';  // 确保字符串终止
            }
            // 显示输入框并更新字符串
            if (ImGui::InputText("##RootFolderName", rootFolderBuffer, sizeof(rootFolderBuffer))) {
                rootFolderName = rootFolderBuffer;
            }
        }
        
        // 目录名添加前缀选项
        ImGui::Checkbox(tr("目录名添加前缀", "Add folder name prefix"), &usePrefix);
        
        // 前缀选项下拉菜单
        if (usePrefix) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(optionsWidth - 150.0f);
            const char* prefixItems[] = {
                tr("日期前缀", "Date prefix"),
                tr("路径前缀", "Path prefix"),
                tr("自定义前缀", "Custom prefix")
            };
            int prefixIndex = prefixType - 1; // 将枚举值转换为索引: PREFIX_DATE(1)->0, PREFIX_PATH(2)->1, PREFIX_CUSTOM(3)->2
            
            if (ImGui::Combo("##PrefixType", &prefixIndex, prefixItems, IM_ARRAYSIZE(prefixItems))) {
                prefixType = static_cast<PrefixType>(prefixIndex + 1); // 将索引转换回枚举值
            }
            
            // 自定义前缀输入框
            if (prefixType == PREFIX_CUSTOM) {
                // 设置与"目录名添加前缀"复选框相同的对齐方式
                ImGui::AlignTextToFramePadding();
                
                // 不使用缩进，保持与顶层元素相同的缩进级别
                float startPosX = ImGui::GetCursorPosX();
                const char* customPrefixLabel = tr("自定义前缀", "Custom prefix");
                float textWidth = ImGui::CalcTextSize(customPrefixLabel).x;
                float spacingX = 10.0f; // 文本与输入框之间的间距
                
                // 显示标签
                ImGui::Text("%s", customPrefixLabel);
                
                // 将输入框放在后面，确保与下拉菜单位置一致
                ImGui::SameLine(startPosX + textWidth + spacingX);
                
                // 设置输入框宽度，与前面的组合框保持一致
                ImGui::SetNextItemWidth(optionsWidth - textWidth - spacingX - 20.0f);
                
                // 为InputText准备字符缓冲区
                static char customPrefixBuffer[256] = "";
                // 当缓冲区为空时，复制字符串到缓冲区
                if (customPrefixBuffer[0] == '\0' && !customPrefix.empty()) {
                    strncpy(customPrefixBuffer, customPrefix.c_str(), sizeof(customPrefixBuffer) - 1);
                    customPrefixBuffer[sizeof(customPrefixBuffer) - 1] = '\0';  // 确保字符串终止
                }
                // 使用没有标签的输入框
                if (ImGui::InputText("##CustomPrefix", customPrefixBuffer, sizeof(customPrefixBuffer))) {
                    customPrefix = customPrefixBuffer;
                }
            }
        }
        
        ImGui::Separator();
        
        // 增加下方空白
        ImGui::Dummy(ImVec2(0, 10));
        
        // 将按钮居中但相互有间距
        float totalButtonWidth = 280.0f; // 两个按钮(120px * 2)加上间距(40px)
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2.0f);
        
        // 取消按钮 - 高度增加约1/5
        if (ImGui::Button(tr("取消", "Cancel"), ImVec2(120.0f, 30.0f))) {
            ImGui::CloseCurrentPopup();
        }
        
        // 在按钮之间添加间距
        ImGui::SameLine();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2.0f + 160.0f);
        
        // 确认按钮 - 高度增加约1/5
        if (ImGui::Button(tr("确认", "OK"), ImVec2(120.0f, 30.0f))) {
            selectTargetAndGenerateFolders();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // 处理打开文件夹的弹窗
    if (showOpenFolderDialog) {
        // 只在第一次显示时打开弹窗
        if (!openFolderPopupOpened) {
            ImGui::OpenPopup(tr("打开目标文件夹", "Open Target Folder"));
            openFolderPopupOpened = true;
        }
        
        // 设置弹窗位置在窗口中央
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal(tr("打开目标文件夹", "Open Target Folder"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", tr("目录结构已成功生成。是否打开目标文件夹？",
                                  "Folder structure generated. Open the target folder?"));
            ImGui::Separator();
            
            // 计算按钮布局
            float buttonWidth = 100.0f;
            float spacing = 20.0f;
            float windowWidth = ImGui::GetWindowWidth();
            float totalButtonWidth = 2 * buttonWidth + spacing;
            float startX = (windowWidth - totalButtonWidth) * 0.5f;
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10); // 增加一些垂直间距
            
            // 设置第一个按钮的位置
            ImGui::SetCursorPosX(startX);
            if (ImGui::Button(tr("是", "Yes"), ImVec2(buttonWidth, 0))) {
                openFolderInExplorer(generatedFolderPath);
                showOpenFolderDialog = false;
                openFolderPopupOpened = false;
                
                // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
                if (currentDirectory.empty()) {
                    targetDirectory = "";
                }
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine(0, spacing);
            
            // 第二个按钮会自动定位在第一个按钮+间距之后
            if (ImGui::Button(tr("否", "No"), ImVec2(buttonWidth, 0))) {
                showOpenFolderDialog = false;
                openFolderPopupOpened = false;
                
                // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
                if (currentDirectory.empty()) {
                    targetDirectory = "";
                }
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        // 如果弹窗被关闭但标志仍为true，重置状态
        else if (openFolderPopupOpened) {
            showOpenFolderDialog = false;
            openFolderPopupOpened = false;
        }
    }
    
    // 处理空模板警告对话框
    if (ImGui::BeginPopupModal("EmptyTemplateWarning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // 设置最小内容宽度(默认宽度的120%)
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f); // 增加约20%的宽度
        
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", tr("模板为空", "Empty Template"));
        ImGui::PopFont();
        ImGui::Separator();
        
        ImGui::TextWrapped("%s", tr("当前模板没有任何目录结构，请先添加至少一个文件夹再生成目录结构。",
                                     "The current template has no folders. Add at least one folder before generating."));
        ImGui::Separator();
        
        ImGui::PopTextWrapPos();
        
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120.0f) / 2.0f);
        if (ImGui::Button(tr("确定", "OK"), ImVec2(120.0f, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // 结束主窗口
    ImGui::End();
    
    // 渲染关于对话框
    if (showAboutDialog) {
        ImGui::SetNextWindowSize(ImVec2(500, 340), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.45f), 
                               ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        
        std::string aboutTitle = trStr("关于 - 文件目录生成器", "About - Folder Manager");
        if (ImGui::Begin(aboutTitle.c_str(), &showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            
            // 标题区域
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.67f, 0.78f, 1.0f));
            ImGui::Text("%s", tr("文件目录生成器 v1.0", "Folder Manager v1.0"));
            ImGui::PopStyleColor();
            ImGui::Separator();
            
            // 在文本之间添加更多空间
            ImGui::Spacing();
            
            // 软件说明
            ImGui::TextWrapped("%s",
                tr("本软件用于快速创建和管理文件目录结构模板，方便用户进行项目目录规划和批量创建。"
                   "通过可视化界面，用户可以直观地设计、保存和应用目录结构，提高工作效率。",
                   "This tool helps you quickly create and manage folder structure templates for project planning and repeated folder creation. "
                   "The visual interface makes it easy to design, save, and apply reusable folder structures."));
            
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TextWrapped("%s",
                tr("如果在安装时没有勾选添加到右键菜单，将无法在文件资源管理器中通过右键进入程序。"
                   "如需使用，请卸载后，勾选添加到右键菜单，重新安装。",
                   "If the context menu option was not selected during installation, the app will not appear in File Explorer's right-click menu. "
                   "To enable it, reinstall the app and select the context menu option."));
            
            ImGui::Spacing();

            // 特性列表 - 调整了顺序
            ImGui::Text("%s", tr("主要功能:", "Main features:"));
            ImGui::BulletText("%s", tr("创建和编辑目录结构模板", "Create and edit folder structure templates"));
            ImGui::BulletText("%s", tr("生成自定义目录结构", "Generate custom folder structures"));
            ImGui::BulletText("%s", tr("导入现有目录结构", "Import existing folder structures"));
            ImGui::BulletText("%s", tr("支持前缀设置和自定义选项", "Support prefix settings and custom options"));
            ImGui::BulletText("%s", tr("保存和加载模板", "Save and load templates"));
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // 项目信息
            ImGui::Text("%s", tr("项目信息:", "Project:"));
            ImGui::BulletText("%s", tr("个人自用开源工具", "Personal open-source utility"));
            ImGui::BulletText("%s", tr("按个人需求维护", "Maintained according to personal needs"));
            // ImGui::BulletText("基于 Dear ImGui 构建");
            // ImGui::BulletText("使用 GLFW 和 OpenGL 作为后端");
            // ImGui::BulletText("C++17 标准开发");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // 开源协议信息
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("%s", tr("以 GNU GPL-3.0 协议开源发布", "Released under GNU GPL-3.0"));
            ImGui::PopStyleColor();
            
            ImGui::PopFont();
            
            // 移除了关闭按钮
            
            ImGui::End();
        }
    }
    
    // 渲染帮助对话框
    if (showHelpDialog) {
        // 设置合适的窗口尺寸，更小一些以适应不同分辨率
        float screenWidth = ImGui::GetIO().DisplaySize.x;
        float screenHeight = ImGui::GetIO().DisplaySize.y;
        
        // 窗口比例调整为屏幕的60%，避免占用过多空间
        float dialogWidth = screenWidth * 0.6f;
        float dialogHeight = screenHeight * 0.7f;
        
        // 减小最小尺寸限制，增加在小屏幕上的适应性
        dialogWidth = dialogWidth < 500.0f ? 500.0f : dialogWidth;
        dialogHeight = dialogHeight < 400.0f ? 400.0f : dialogHeight;
        
        ImVec2 windowSize(dialogWidth, dialogHeight);
        ImVec2 windowPos(
            (screenWidth - dialogWidth) * 0.5f,
            (screenHeight - dialogHeight) * 0.45f  // 稍微靠上以便于阅读
        );
        
        // 只在首次显示时设置窗口位置，之后允许用户移动
        static bool firstShowHelp = true;
        if (firstShowHelp) {
            ImGui::SetNextWindowPos(windowPos, ImGuiCond_Once);
            firstShowHelp = false;
        }
        
        // 设置窗口尺寸，但允许用户调整大小
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
        
        // 移除NoResize标志，允许窗口调整大小
        ImGuiWindowFlags helpWindowFlags = ImGuiWindowFlags_NoCollapse;
        
        // 记录样式变量的推入数量，确保数量匹配
        int styleVarCount = 0;
        int styleColorCount = 0;
        
        // 增加全局项目间距和边距
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4)); styleVarCount++;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15)); styleVarCount++;
        
        // 添加窗口边框样式，使对话框更加美观
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); styleVarCount++;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f); styleVarCount++;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); styleVarCount++;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f); styleVarCount++;
        
        // 全局启用文本自动换行
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f)); styleVarCount++;
        
        // 应用主题颜色 - 确保在深色背景上文本可见
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.0f)); styleColorCount++; // 亮白色文本
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.18f, 0.20f, 0.27f, 1.0f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.25f, 0.30f, 0.45f, 1.0f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.30f, 0.40f, 0.60f, 1.0f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.22f, 1.0f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.40f, 0.50f)); styleColorCount++;
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.30f, 0.30f, 0.40f, 0.70f)); styleColorCount++;
        
        // 额外样式
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14); styleVarCount++;
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4); styleVarCount++;
        
        bool helpOpen = true;
        std::string helpTitle = trStr("帮助", "Help") + "##Dialog";
        if (ImGui::Begin(helpTitle.c_str(), &helpOpen, helpWindowFlags)) {
            if (!helpOpen) {
                showHelpDialog = false;
                firstShowHelp = true;  // 下次打开时重置位置
            }
            
            // 计算内容区域大小
            float contentAreaHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 20;
            
            // 设置更紧凑的显示样式
            float textSize = ImGui::GetFontSize() * 0.90f;  // 稍小的文本尺寸
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 3));  // 更紧凑的项目间距
            int innerStyleVarCount = 1; // 记录内部样式变量数量
            
            // 使用固定高度的标签页栏
            ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
            if (ImGui::BeginTabBar("HelpTabBar", tabBarFlags)) {
                
                // 基本操作标签页
                if (ImGui::BeginTabItem(tr("基本操作", "Basics"))) {
                    ImGui::BeginChild("BasicOpsContent", ImVec2(0, contentAreaHeight), true, 
                                     ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    
                    // 启用文本自动换行
                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                    
                    // 使用更亮的颜色突出显示标题
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("模板管理", "Template Management"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
                    ImGui::Separator();
        ImGui::Spacing();
        
                    // 强制文本宽度限制，确保在窗口小的情况下也能完整显示
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 创建新模板: 点击「+ 模板」按钮，输入名称",
                                                 "- Create a template: click the + Template button and enter a name"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 编辑模板: 点击下拉箭头，选择一个模板，输入框修改名称",
                                                 "- Edit a template: use the dropdown to select one, then edit its name in the input box"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 删除模板: 点击「- 模板」按钮，删除当前模板",
                                                 "- Delete a template: click the Delete Template button"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 导入目录: 点击「导入」按钮，从系统中导入文件夹结构",
                                                 "- Import folders: click Import to import a folder structure from your computer"));
                    ImGui::Spacing();
        ImGui::Spacing();
        
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("目录操作", "Folder Operations"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
        ImGui::Separator();
                    ImGui::Spacing();
                    
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 添加目录: 选择目录，点击「+ 目录」，添加子目录",
                                                 "- Add folders: select a folder and click + Folder to add a child folder"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 编辑名称: 右键点击/双击目录，进行名称编辑",
                                                 "- Rename: right-click or double-click a folder name"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 删除目录: delete键或右键点击目录，选择「删除」",
                                                 "- Delete folders: press Delete or right-click and choose Delete"));
                    ImGui::Spacing();
        ImGui::Spacing();
        
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("生成目录", "Generate Folders"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
                    ImGui::Separator();
        ImGui::Spacing();
        
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 选择模板: 在模板列表中选择要使用的模板",
                                                 "- Select a template from the template list"));
        ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 生成目录: 点击「生成目录」按钮",
                                                 "- Generate folders: click the Generate button"));
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
                    ImGui::TextWrapped("%s", tr("• 查看结果: 操作完成后会显示生成结果",
                                                 "- Review the result after generation finishes"));
                    
                    // 结束文本自动换行
                    ImGui::PopTextWrapPos();
                    
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                
                // 快捷键标签页
                if (ImGui::BeginTabItem(tr("快捷键", "Shortcuts"))) {
                    ImGui::BeginChild("ShortcutsContent", ImVec2(0, contentAreaHeight), true,
                                     ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    
                    // 启用文本自动换行
                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("文件操作", "Actions"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
        ImGui::Separator();
                    ImGui::Spacing();
                    
                    // 更紧凑的表格布局
                    if (ImGui::BeginTable("FileShortcutsTable", 2, ImGuiTableFlags_BordersInnerH)) {
                        ImGui::TableSetupColumn(tr("命令", "Command"), ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn(tr("快捷键", "Shortcut"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", tr("添加目录", "Add Folder"));
                        ImGui::TableNextColumn(); ImGui::Text("Insert");                        
                        ImGui::TableNextRow();

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", tr("删除目录", "Delete Folder"));
                        ImGui::TableNextColumn(); ImGui::Text("Delete"); 

                        ImGui::TableNextColumn(); ImGui::Text("%s", tr("重命名", "Rename"));
                        ImGui::TableNextColumn(); ImGui::Text("F2");                        
                        
                        ImGui::EndTable();
                    }
                    
                    // 结束文本自动换行
                    ImGui::PopTextWrapPos();
                    
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                
                // 使用技巧标签页
                if (ImGui::BeginTabItem(tr("使用技巧", "Tips"))) {
                    ImGui::BeginChild("TipsContent", ImVec2(0, contentAreaHeight), true,
                                     ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    
                    // 启用文本自动换行
                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("在文件管理器中创建文件夹", "Create folders from File Explorer"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
        ImGui::Separator();
                    ImGui::Spacing();
        
                    // 使用TextWrapped替代Text，确保文本能够自动换行
                    ImGui::TextWrapped("%s", tr("• 打开文件管理器，右键点击空白处或文件夹。",
                                                 "- Open File Explorer and right-click an empty area or a folder."));
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 点击'使用文件夹管理器打开'，即可打开应用。",
                                                 "- Click 'Open with Folder Manager' to start the app."));
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 可以更方便地在当前位置创建文件夹",
                                                 "- This makes it easier to generate folders in the current location."));
                    ImGui::Spacing();
                    ImGui::Spacing();
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);

                    ImGui::Text("%s", tr("模板设计技巧", "Template Design Tips"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();
        ImGui::Separator();
                    ImGui::Spacing();
        
                    // 使用TextWrapped替代Text，确保文本能够自动换行
                    ImGui::TextWrapped("%s", tr("• 组织结构: 先规划顶层目录，再向下细化子目录和文件",
                                                 "- Structure: plan top-level folders first, then refine subfolders."));
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 命名规范: 为模板元素使用清晰一致的命名方式，便于理解和维护",
                                                 "- Naming: use clear and consistent names for easier maintenance."));
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 模板分类: 根据项目类型创建不同的模板，如Web开发、移动应用等",
                                                 "- Template types: create different templates for different project types."));
                    ImGui::Spacing();
                    ImGui::Spacing();
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("导入技巧", "Import Tips"));
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor(); // 修复：添加缺少的 PopStyleColor
        ImGui::Separator();
                    ImGui::Spacing();
                    
                    // 使用TextWrapped替代Text
                    ImGui::TextWrapped("%s", tr("• 可以从系统文件夹导入目录结构，生成模板",
                                                 "- You can import a folder structure from an existing system folder."));
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 支持一次最多导入100个文件夹，深度不超过10层。",
                                                 "- Import is limited to 100 folders and 10 levels deep."));
                    ImGui::Spacing();
                    ImGui::Spacing();
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::Text("%s", tr("生成技巧", "Generation Tips"));
                    ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
                    ImGui::Separator();
                    ImGui::Spacing();
        
                    // 使用TextWrapped替代Text
                    ImGui::TextWrapped("%s", tr("• 增量生成: 可以在现有目录中增量生成新的文件和文件夹",
                                                 "- Incremental generation: generate new folders inside an existing directory."));
        ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 使用前缀: 使用文件路径、日期或自定义前缀",
                                                 "- Prefixes: use path, date, or custom prefixes."));
        ImGui::Spacing();
                    ImGui::TextWrapped("%s", tr("• 预览功能: 生成前先预览结构，确认无误后再执行",
                                                 "- Preview: review the structure before generating."));
                    ImGui::Spacing();                    
                    
                    ImGui::PopTextWrapPos();
                    
            ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
            
            // 底部按钮 - 更灵活的定位，允许窗口缩放时按钮位置仍然合理
            ImGui::Separator();
            ImGui::Spacing();
            
            float buttonWidth = 120.0f;
            float windowWidth = ImGui::GetWindowWidth();
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
            
            if (ImGui::Button(tr("关闭", "Close"), ImVec2(buttonWidth, 0))) {
                showHelpDialog = false;
                firstShowHelp = true;  // 下次打开时重置位置
            }
            
            // 先弹出内部样式变量
            ImGui::PopStyleVar(innerStyleVarCount);
        }
        ImGui::End();
        
        // 恢复样式设置 - 确保与Push匹配
        ImGui::PopStyleVar(styleVarCount);  // 弹出所有推入的样式变量
        ImGui::PopStyleColor(styleColorCount); // 弹出所有推入的颜色样式
    }
    
    // 渲染确认生成位置对话框
    confirmGenerateInLocation();
    
    // 处理打开目标文件夹对话框
    renderOpenFolderDialog();
    
    // 处理删除模板确认对话框
    renderDeleteTemplateConfirmDialog();
    
    // 处理导入文件夹对话框
    if (showImportFolderDialog) {
        std::string dir = selectDirectoryDialog(uiLanguage);
        if (!dir.empty()) {
            // 确认这确实是一个导入操作
            if (MessageBoxA(NULL, tr("确认要导入此目录结构吗？", "Import this folder structure?"),
                          tr("导入确认", "Confirm Import"), 
                          MB_YESNO | MB_ICONQUESTION) == IDYES) {
                importDirectoryStructure(dir);
                updateStatusText(trStr("已成功导入目录结构: ", "Imported folder structure: ") + dir);
            } else {
                updateStatusText(trStr("已取消导入操作", "Import canceled"));
            }
        } else {
            updateStatusText(trStr("未选择目录，导入已取消", "No directory selected. Import canceled."));
        }
        showImportFolderDialog = false;
    }
}

// 渲染菜单栏
void ImGuiMainWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(tr("文件", "File"))) {
            if (ImGui::MenuItem(tr("新建模板", "New Template"), "Ctrl+N")) {
                newTemplate();
            }
            
            if (ImGui::MenuItem(tr("保存模板", "Save Template"), "Ctrl+S")) {
                saveTemplate();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem(tr("导入目录结构", "Import Folder Structure"), "Ctrl+I")) {
                updateStatusText(trStr("请选择要导入的目录结构", "Choose a folder structure to import"));
                showImportFolderDialog = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem(tr("退出", "Exit"), "Alt+F4")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(tr("编辑", "Edit"))) {
            if (ImGui::MenuItem(tr("添加目录", "Add Folder"), "Ins")) {
                addFolder();
            }
            
            bool hasSelection = !selectedNodeId.empty();
            
            if (ImGui::MenuItem(tr("删除目录", "Delete Folder"), "Del", false, hasSelection)) {
                deleteFolder();
            }
            
            if (ImGui::MenuItem(tr("重命名", "Rename"), "F2", false, hasSelection)) {
                auto nodeId = selectedNodeId;
                auto node = currentTemplate->getNode(nodeId);
                if (node) {
                    auto& state = treeNodeStates[nodeId];
                    state.isEditing = true;
                    state.editLabel = node->getName();
                    lastEditingNodeId = "";
                    updateStatusText(trStr("开始编辑 '", "Editing '") + state.editLabel + "'");
                }
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(tr("语言", "Language"))) {
            if (ImGui::MenuItem("中文", nullptr, uiLanguage == UiLanguage::Chinese)) {
                setLanguage(UiLanguage::Chinese);
            }
            
            if (ImGui::MenuItem("English", nullptr, uiLanguage == UiLanguage::English)) {
                setLanguage(UiLanguage::English);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(tr("帮助", "Help"))) {
            if (ImGui::MenuItem(tr("关于", "About"))) {
                showAboutDialog = true;
            }
            
            if (ImGui::MenuItem(tr("帮助", "Help"))) {
                showHelpDialog = true;
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

// 实现可编辑的下拉框控件
std::pair<bool, bool> ImGuiMainWindow::EditableComboBox(const char* label, char* buffer, size_t bufferSize,
                                       const std::vector<std::string>& items, int* currentItem) {
    bool modified = false;
    bool selectedFromDropdown = false; // 新增: 标识是否通过下拉列表选择
    
    // 创建一个组合控件的布局
    ImGui::PushID(label);
    ImGui::BeginGroup();
    
    // 获取控件完整宽度
    float totalWidth = ImGui::CalcItemWidth();
    float buttonWidth = ImGui::GetFrameHeight(); // 按钮宽度与高度相同
    float inputWidth = totalWidth - buttonWidth - ImGui::GetStyle().ItemSpacing.x;
    
    // 设置输入框宽度并渲染输入框
    ImGui::PushItemWidth(inputWidth);
    
    // 确保输入框文本可见
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f)); // 完全不透明的深色背景
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 确保文本为纯白色
    
    // 保存原始的 currentItem 值，以防我们需要恢复它
    int originalItemIndex = *currentItem;
    
    if (ImGui::InputText("##EditableInput", buffer, bufferSize)) {
        modified = true;
        // 输入文本已修改，但不搜索匹配项也不更新currentItem
        // 让外部调用方知道文本已更改，但不触发模板切换操作
    }
    
    ImGui::PopStyleColor(2); // 恢复原始样式
    ImGui::PopItemWidth();
    
    // 紧靠输入框放置下拉按钮
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::ArrowButton("##ComboArrow", ImGuiDir_Down)) {
        ImGui::OpenPopup("##ComboPopup");
    }
    
    // 显示标签（如果有）
    if (label != nullptr && label[0] != '\0') {
        ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
        ImGui::Text("%s", label);
    }
    
    // 下拉列表部分
    if (ImGui::BeginPopup("##ComboPopup")) {
        for (size_t i = 0; i < items.size(); i++) {
            if (ImGui::Selectable(items[i].c_str(), static_cast<int>(i) == *currentItem)) {
                strncpy(buffer, items[i].c_str(), bufferSize - 1);
                buffer[bufferSize - 1] = '\0'; // 确保以null结尾
                *currentItem = static_cast<int>(i);
                modified = true;
                selectedFromDropdown = true; // 标记为通过下拉列表选择
            }
        }
        ImGui::EndPopup();
    }
    
    ImGui::EndGroup();
    ImGui::PopID();
    
    // 返回一个pair，指示是否修改了值以及是否是通过下拉列表选择的
    return std::make_pair(modified, selectedFromDropdown);
}

// 渲染模板面板
void ImGuiMainWindow::renderTemplatePanel(float contentWidth) {
    float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
    float indent = 10.0f;
    float comboWidth = 180.0f;
    
    ImGui::SetCursorPosX(indent);
    ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置
    
    // 简化样式设置 - 减小内边距
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    
    // 我们需要记录推入的样式颜色和变量数量
    int styleColorCount = 0;
    int styleVarCount = 1; // 我们已经推入了一个样式变量
    
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.17f, 0.22f, 0.8f));
    styleColorCount++;
    
    // 固定值定义 - 使用极小边距，与全局边距保持一致
    const float leftMargin = 0.0f;          // 内部左边距设为0
    const float rightMargin = 0.0f;         // 内部右边距设为0
    const float buttonWidth = 60.0f;         // 固定按钮宽度
    const float buttonSpacing = 10.0f;       // 按钮之间的间隔
    
    // 获取当前窗口尺寸
    float windowWidth = contentWidth;
    float initialY = ImGui::GetCursorPosY(); // 记录初始Y位置
    
    // 准备模板名称列表
    std::vector<std::string> templateNames;
    for (const auto& templ : availableTemplates) {
        templateNames.push_back(templ.name);
    }
    
    // 如果没有可用模板，显示"无可用模板"
    if (templateNames.empty()) {
        templateNames.push_back(trStr("无可用模板", "No templates"));
    }
    
    // 准备输入缓冲区
    static char templateNameBuffer[256];
    strcpy(templateNameBuffer, templateNameInput.c_str());
    
    // 计算ComboBox宽度，减小宽度以便为新按钮腾出空间
    float comboBoxWidth = windowWidth * 0.45f - 30.0f; // 从0.45f再减小30px
    
    // 设置ComboBox的位置，左对齐并留出边距
    ImGui::SetCursorPosX(leftMargin);
    
    // 使用自定义的可编辑下拉框控件
    ImGui::SetNextItemWidth(comboBoxWidth);
    auto [modified, selectedFromDropdown] = EditableComboBox("", templateNameBuffer, sizeof(templateNameBuffer), 
                        templateNames, &selectedTemplateIndex);
    
    if (modified) {
        // 如果用户修改了名称或者选择了新模板
        templateNameInput = templateNameBuffer;
        
        if (selectedFromDropdown) {
            // 用户通过下拉列表选择了一个模板
            if (selectedTemplateIndex >= 0 && selectedTemplateIndex < static_cast<int>(availableTemplates.size())) {
                if (currentTemplate) {
                    // 如果只是改了名称，则更新当前模板名称
                    if (currentTemplate->getId() == availableTemplates[selectedTemplateIndex].id) {
                        currentTemplate->setName(templateNameInput);
                        // 设置模板已修改标志
                        isTemplateModified = true;
                        // 更新模板信息显示
                        updateTemplateInfo();
                    } else {
                        // 如果选择了不同的模板，则加载该模板
                        loadTemplate(availableTemplates[selectedTemplateIndex].id);
                        // loadTemplate内部会调用updateStatusText，但为确保模板信息更新，再次调用
                        updateTemplateInfo();
                    }
                }
            }
        } else {
            // 用户仅通过键盘输入修改了文本，仅更新当前模板名称
            if (currentTemplate) {
                currentTemplate->setName(templateNameInput);
                // 设置模板已修改标志
                isTemplateModified = true;
                // 更新模板信息显示
                updateTemplateInfo();
            }
        }
    }
    
    // 存储ComboBox下方的Y位置（如果需要恢复光标位置）
    float afterComboBoxY = ImGui::GetCursorPosY();
    
    // 添加目录操作按钮 - 始终显示按钮组，无论selectedTemplateIndex是否有效
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    styleColorCount++;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    styleColorCount++;
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    styleColorCount++;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    styleVarCount++;
    
    // 计算按钮区域和起始位置
    // 按钮居中垂直对齐
    float buttonY = initialY + (ImGui::GetFrameHeight() - ImGui::GetFrameHeightWithSpacing()) / 2.0f + 1.0f;
    
    // 计算四个按钮的总宽度，包括间隔
    float totalButtonsWidth = 4 * buttonWidth + 3 * buttonSpacing; // 增加到4个按钮
    float buttonsStartX = windowWidth - rightMargin - totalButtonsWidth - 5.0f; // 向左偏移5像素
    
    // 放置第一个按钮："+ 模板"
    ImGui::SetCursorPos(ImVec2(buttonsStartX, buttonY));
    if (ImGui::Button(tr("+ 模板", "+ Template"), ImVec2(buttonWidth, 0))) {
        newTemplate(); // 创建新模板
    }
    
    // 放置第二个按钮："+ 目录"
    ImGui::SetCursorPos(ImVec2(buttonsStartX + buttonWidth + buttonSpacing, buttonY));
    if (ImGui::Button(tr("+ 目录", "+ Folder"), ImVec2(buttonWidth, 0))) {
        addFolder();
    }
    
    // 放置第三个按钮："- 目录"
    ImGui::SetCursorPos(ImVec2(buttonsStartX + 2 * buttonWidth + 2 * buttonSpacing, buttonY));
    if (ImGui::Button(tr("- 目录", "- Folder"), ImVec2(buttonWidth, 0))) {
        deleteFolder();
    }
    
    // 放置第四个按钮："导入"
    ImGui::SetCursorPos(ImVec2(buttonsStartX + 3 * buttonWidth + 3 * buttonSpacing, buttonY));
    if (ImGui::Button(tr("导入", "Import"), ImVec2(buttonWidth, 0))) {
        // 调用导入功能
        updateStatusText(trStr("请选择要导入的目录结构", "Choose a folder structure to import"));
        showImportFolderDialog = true;
    }
    
    // 恢复光标位置到ComboBox之后
    ImGui::SetCursorPosY(afterComboBoxY);
    
    // 确保弹出正确数量的样式
    ImGui::PopStyleColor(styleColorCount); // 弹出所有推入的样式颜色
    ImGui::PopStyleVar(styleVarCount);     // 弹出所有推入的样式变量
}

// 渲染树视图
void ImGuiMainWindow::renderTreeView() {
    if (!currentTemplate || currentTemplate->getRootNodes().empty()) {
        // 空状态下提供更友好的提示
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        float centerY = ImGui::GetCursorPosY() + windowSize.y * 0.4f;
        
        // 居中显示空状态提示
        ImGui::SetCursorPosY(centerY - 50);
        const char* emptyTreeText = tr("目录结构为空", "Folder structure is empty");
        ImVec2 textSize = ImGui::CalcTextSize(emptyTreeText);
        ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", emptyTreeText);
        
        ImGui::SetCursorPosY(centerY);
        const char* addHintText = tr("点击 + 按钮或右键菜单添加目录", "Click + or right-click to add a folder");
        textSize = ImGui::CalcTextSize(addHintText);
        ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", addHintText);
        
        ImGui::SetCursorPosY(centerY + 30);
        const char* importHintText = tr("也可以从文件菜单导入已有目录结构", "You can also import an existing folder structure from the File menu");
        textSize = ImGui::CalcTextSize(importHintText);
        ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", importHintText);

        ImGui::SetCursorPosY(centerY + 60);
        const char* contextHintText = tr("还可以从文件管理器右键进入，在该位置创建文件夹", "You can also open it from File Explorer's context menu");
        textSize = ImGui::CalcTextSize(contextHintText);
        ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", contextHintText);
        
        // 允许在空白区域右键添加目录
        if (ImGui::BeginPopupContextWindow("EmptyTreeContextMenu")) {
            if (ImGui::MenuItem(tr("添加根目录", "Add Root Folder"))) {
                addFolder();
            }
            ImGui::EndPopup();
        }
        
        return;
    }
    
    // 树视图样式设置
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.26f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.34f, 0.34f, 0.34f, 1.0f));
    
    // 获取根节点
    auto rootNodes = currentTemplate->getRootNodes();
        
    // 遍历显示树结构
    for (const auto& node : rootNodes) {
        renderTreeNode(node);
    }
        
    // 检测Delete键按下，且有选中节点时，删除当前节点
    if (!selectedNodeId.empty() && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        deleteFolder();
    }
        
    // 检测鼠标点击且不在任何项目上时，清除选择
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
        if (!selectedNodeId.empty()) {
            selectedNodeId = "";
            updateStatusText(trStr("已清除选择", "Selection cleared"));
        }
    }
    
    // 处理右键菜单
    if (ImGui::BeginPopupContextWindow("TreeContextMenu")) {
        if (ImGui::MenuItem(tr("添加根目录", "Add Root Folder"))) {
            addFolder();
        }
        if (!selectedNodeId.empty()) {
            ImGui::Separator();
            
            if (ImGui::MenuItem(tr("删除目录", "Delete Folder"))) {
                deleteFolder();
            }
            
            if (ImGui::MenuItem(tr("重命名", "Rename"))) {
                auto nodeId = selectedNodeId;
                auto node = currentTemplate->getNode(nodeId);
                if (node) {
                    auto& state = treeNodeStates[nodeId];
                    state.isEditing = true;
                    state.editLabel = node->getName();
                    lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
                    updateStatusText(trStr("开始编辑 '", "Editing '") + state.editLabel + "'");
                }
            }
            
            if (ImGui::MenuItem(tr("添加子目录", "Add Subfolder"))) {
                std::string newNodeId = generateUUID();
                std::string newName = trStr("新目录", "New Folder");
                
                // 创建新节点
                auto newNode = std::make_shared<Node>(newNodeId, newName, selectedNodeId);
                currentTemplate->addNode(newNode);
                
                // 初始化新节点状态
                TreeNodeState newState;
                newState.isOpen = true;
                newState.isEditing = true;
                newState.editLabel = newName;
                treeNodeStates[newNodeId] = newState;
                
                // 确保父节点是打开的
                auto parentState = treeNodeStates.find(selectedNodeId);
                if (parentState != treeNodeStates.end()) {
                    parentState->second.isOpen = true;
                }
                
                // 选中新创建的节点
                selectNode(newNodeId);
                lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
                updateStatusText(trStr("创建了新目录 '", "Created new folder '") + newName + "'");
            }
        }
        
        ImGui::EndPopup();
    }
    
    // 在函数结束前弹出所有样式设置
    ImGui::PopStyleColor(3); // 弹出3个颜色样式 (Header, HeaderHovered, HeaderActive)
    ImGui::PopStyleVar(1);   // 弹出1个样式变量 (IndentSpacing)
}

// 渲染单个树节点
bool ImGuiMainWindow::renderTreeNode(const std::shared_ptr<Node>& node) {
    if (!node) return false;
    
    const std::string& nodeId = node->getId();
    std::string nodeName = node->getName();
    
    // 如果找不到节点状态，则初始化一个新的
    auto it = treeNodeStates.find(nodeId);
    if (it == treeNodeStates.end()) {
        TreeNodeState newState;
        newState.isOpen = false;
        newState.isEditing = false;
        newState.editLabel = "";
        treeNodeStates[nodeId] = newState;
    }
    
    TreeNodeState& state = treeNodeStates[nodeId];
    
    // 获取子节点列表
    auto children = currentTemplate->getChildNodes(nodeId);
    bool hasChildren = !children.empty();
    
    // 为当前层级生成唯一ID
    std::string uniqueNodeIdStr = nodeId;
    
    // 编辑模式 - 纯粹实现
    if (state.isEditing) {
        // 保存当前缩进，用于后续绘制
        float indentSpacing = ImGui::GetStyle().IndentSpacing;
        float currentIndent = ImGui::GetCursorPosX();
        
        // 记录开始位置用于后续绘制选中状态
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        
        // 确保每个节点推入唯一ID堆栈，避免ID冲突
        ImGui::PushID(nodeId.c_str());
        
        // 绘制树状图标（手动方式）
        if (hasChildren) {
            // 绘制折叠/展开箭头
            if (ImGui::ArrowButton("##arrow", state.isOpen ? ImGuiDir_Down : ImGuiDir_Right)) {
                state.isOpen = !state.isOpen;
            }
        } else {
            // 为叶节点保留相同缩进
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
        }
        
        ImGui::SameLine(0, 4);
        
        // 准备编辑缓冲区
        char buffer[256] = {0};
        strncpy(buffer, state.editLabel.c_str(), sizeof(buffer) - 1);
        
        // 设置输入框样式
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1)); // 较小的内边距
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f)); // 完全不透明的深色背景
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 确保文本为纯白色
        
        // 计算输入框宽度
        float inputWidth = ImGui::GetContentRegionAvail().x * 0.8f;
        inputWidth = std::min(inputWidth, 300.0f);
        
        // 设置焦点到此输入框
        if (lastEditingNodeId != nodeId) {
            ImGui::SetKeyboardFocusHere();
            lastEditingNodeId = nodeId;
            // 调试信息
            updateStatusText(trStr("正在编辑: ", "Editing: ") + nodeName + " (ID: " + nodeId + ")");
        }
        
        // 显示输入框 - 使用简单ID确保唯一性
        ImGui::PushItemWidth(inputWidth);
        bool enterPressed = ImGui::InputText("##edit", buffer, sizeof(buffer), 
                                        ImGuiInputTextFlags_EnterReturnsTrue | 
                                        ImGuiInputTextFlags_AutoSelectAll);
        
        // 获取项目高度
        float itemHeight = ImGui::GetItemRectSize().y;
        
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2); // 弹出两个颜色样式
        ImGui::PopStyleVar();
        
        // 移除选中高亮矩形绘制，这是导致文本被覆盖的根本原因
        // 如果需要选中状态指示，可以使用边框或其他不会覆盖文本的方式
        
        // 如果按下回车或失去焦点，应用更改并退出编辑模式
        if (enterPressed || (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())) {
            state.isEditing = false;
            lastEditingNodeId = "";
            
            // 应用新名称（确保不是空名称）
            if (strlen(buffer) > 0) {
                renameNode(nodeId, buffer);
                updateStatusText(trStr("已重命名为 '", "Renamed to '") + std::string(buffer) + "'");
            }
        }
        
        // 处理子节点（仅当节点是展开状态）
        if (state.isOpen && hasChildren) {
            ImGui::Indent(indentSpacing);
            for (const auto& child : children) {
                renderTreeNode(child);
            }
            ImGui::Unindent(indentSpacing);
        }
        
        ImGui::PopID(); // 弹出节点ID
        
        return state.isOpen;
    } 
    else {
        // 确保每个节点推入唯一ID堆栈
        ImGui::PushID(nodeId.c_str());
        
        // 正常显示模式 - 使用标准TreeNode
        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                      ImGuiTreeNodeFlags_SpanAvailWidth;
        
        // 如果是选中的节点，设置选中样式
        if (nodeId == selectedNodeId) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // 如果没有子节点，设置为叶子节点
        if (!hasChildren) {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        // 设置展开状态
        ImGui::SetNextItemOpen(state.isOpen);
        
        // 显示节点并处理交互，使用简单文本而非格式化字符串
        bool nodeOpen = ImGui::TreeNodeEx("##node", nodeFlags, "%s", nodeName.c_str());
        
        // 处理点击 - 只在点击非箭头部分时处理
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            // 如果点击的不是展开箭头，处理选择逻辑
            selectNode(nodeId);
            
            // 双击进入编辑模式
            if (ImGui::IsMouseDoubleClicked(0)) {
                state.isEditing = true;
                state.editLabel = nodeName;
                lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
                updateStatusText(trStr("开始编辑 '", "Editing '") + nodeName + "'");
            }
        }
        
        // 移除编辑按钮 - 现在使用双击或右键菜单进行编辑
        
        // 右键菜单
        if (ImGui::BeginPopupContextItem()) {
            showNodeContextMenu(nodeId, nodeName);
            ImGui::EndPopup();
        }
        
        // 节点处于打开状态，渲染子节点
        if (nodeOpen) {
            state.isOpen = true;
            
            for (const auto& child : children) {
                renderTreeNode(child);
            }
            
            ImGui::TreePop();
        } else {
            state.isOpen = false;
        }
        
        ImGui::PopID(); // 弹出节点ID
        
        return nodeOpen;
    }
}

// 删除文件夹
void ImGuiMainWindow::deleteFolder() {
    if (selectedNodeId.empty()) {
        updateStatusText(trStr("请先选择要删除的文件夹", "Select a folder to delete first"));
        return;
    }
    
    // 获取节点名称
    auto node = currentTemplate->getNode(selectedNodeId);
    if (!node) {
        updateStatusText(trStr("无效的节点ID", "Invalid node ID"));
        return;
    }
    
    std::string nodeName = node->getName();
    
    // 真实情况中应该有确认对话框，这里简化处理
    // 从模板中删除节点及其子节点
    currentTemplate->removeNode(selectedNodeId);
    
    // 设置模板已修改标志
    isTemplateModified = true;
    
    // 清除节点状态
    treeNodeStates.erase(selectedNodeId);
    selectedNodeId = "";
    
    // 更新状态
    updateStatusText(trStr("已删除文件夹 '", "Deleted folder '") + nodeName + trStr("' 及其子文件夹", "' and its subfolders"));
}

// 重命名节点
void ImGuiMainWindow::renameNode(const std::string& nodeId, const std::string& newName) {
    if (nodeId.empty() || newName.empty()) {
        return;
    }
    
    // 获取节点
    auto node = currentTemplate->getNode(nodeId);
    if (!node) {
        return;
    }
    
    // 更新节点名称
    std::string oldName = node->getName();
    node->setName(newName);
    
    // 设置模板已修改标志
    isTemplateModified = true;
    
    // 更新状态
    updateStatusText(trStr("重命名文件夹 '", "Renamed folder '") + oldName +
                     trStr("' 为 '", "' to '") + newName + "'");
}

// 生成文件夹
void ImGuiMainWindow::generateFolders() {
    // 检查当前模板是否为空
    if (currentTemplate->isEmpty()) {
        updateStatusText(trStr("当前模板为空，请先添加文件夹", "The current template is empty. Add a folder first."));
        return;
    }
    
    // 设置标志，表示正在生成目录
    isGeneratingFolders = true;
    
    // 如果目标目录未设置，打开目录选择对话框
    if (targetDirectory.empty()) {
        std::string dir = selectDirectoryDialog(uiLanguage);
        if (dir.empty()) {
            updateStatusText(trStr("未选择目标目录，操作已取消", "No target directory selected. Operation canceled."));
            isGeneratingFolders = false; // 重置标志
            return;
        }
        targetDirectory = dir;
        updateStatusText(trStr("已选择目标目录: ", "Selected target directory: ") + targetDirectory);
    }
    
    // 创建目录生成器
    DirectoryOps dirOps;
    
    // 创建生成选项
    GenerationOptions options;
    options.createRootFolder = createRootFolder;
    options.rootFolderName = rootFolderName;
    options.usePrefix = usePrefix;
    options.prefixType = prefixType;
    options.dateFormat = dateFormat;
    options.includeFolderSeparator = includeFolderSeparator;
    options.customPrefix = customPrefix;
    
    // 如果rootFolderName为空，使用模板名称作为默认值
    if (createRootFolder && rootFolderName.empty()) {
        rootFolderName = currentTemplate->getName();
        options.rootFolderName = rootFolderName;
    }
    
    // 生成基本前缀（对于日期和自定义前缀）
    std::string basePrefix = "";
    if (usePrefix && prefixType != PREFIX_PATH) {
        // 确保目标目录不为空
        if (targetDirectory.empty()) {
            updateStatusText(trStr("错误: 目标目录未设置，无法生成前缀", "Error: Target directory is not set. Cannot generate prefix."));
            return;
        }
        
        // 根据前缀类型生成前缀
        basePrefix = Dialogs::generatePrefix(options, targetDirectory);
        
        // 检查前缀是否成功生成
        if (basePrefix.empty() && prefixType != PREFIX_NONE) {
            std::string errorMsg = trStr("警告: 前缀生成失败，将不使用前缀。",
                                         "Warning: Prefix generation failed. No prefix will be used.");
            if (prefixType == PREFIX_CUSTOM && customPrefix.empty()) {
                errorMsg += trStr("\n自定义前缀不能为空。", "\nCustom prefix cannot be empty.");
            }
            MessageBoxA(NULL, errorMsg.c_str(), tr("前缀生成警告", "Prefix Warning"), MB_OK | MB_ICONWARNING);
            updateStatusText(trStr("警告: 前缀生成失败", "Warning: Prefix generation failed"));
        }
    }
    
    // 检查目录冲突
    auto conflicts = dirOps.checkDirectoryConflicts(*currentTemplate, targetDirectory, createRootFolder, rootFolderName, basePrefix);
    
    if (!conflicts.empty()) {
        std::string conflictMsg = trStr("错误: 检测到文件夹冲突，继续操作将导致数据丢失!\n\n以下文件夹已存在:\n",
                                        "Error: Folder conflicts detected. Continuing could cause data loss.\n\nExisting folders:\n");
        for (size_t i = 0; i < conflicts.size() && i < 5; ++i) {
            conflictMsg += "- " + conflicts[i] + "\n";
        }
        
        if (conflicts.size() > 5) {
            conflictMsg += trStr("... 以及其他 ", "... and ") + std::to_string(conflicts.size() - 5) +
                           trStr(" 个文件夹\n", " more folders\n");
        }
        
        conflictMsg += trStr("\n请修改目标路径或前缀设置后重试。",
                             "\nChange the target path or prefix settings and try again.");
        
        // 显示错误消息，并阻止继续操作
        MessageBoxA(NULL, conflictMsg.c_str(), tr("文件夹冲突错误", "Folder Conflict Error"), MB_OK | MB_ICONERROR);
        // 更新状态文本，提示用户操作已取消
        updateStatusText(trStr("错误: 检测到文件夹冲突，操作已取消", "Error: Folder conflicts detected. Operation canceled."));
        
        // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
        if (currentDirectory.empty()) {
            targetDirectory = "";
        }
        
        return;
    }
    
    // 生成目录结构
    bool success = dirOps.createDirectoryStructure(*currentTemplate, targetDirectory, options);
    
    if (success) {
        // 确定生成的根目录路径
        std::string rootPath = targetDirectory;
        if (createRootFolder && !rootFolderName.empty()) {
            rootPath += "\\" + basePrefix + rootFolderName;
        }
        
        // 保存生成的文件夹路径，用于后续打开
        generatedFolderPath = rootPath;
        
        // 显示成功消息并提供打开文件夹的选项
        updateStatusText(trStr("目录结构已成功生成", "Folder structure generated successfully"));
        showOpenFolderDialog = true;
        openFolderPopupOpened = false; // 确保弹窗会显示
    } else {
        updateStatusText(trStr("生成目录结构时出错", "Error generating folder structure"));
    }
    
    // 完成目录生成，重置标志
    isGeneratingFolders = false;
    
    // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
    // 这样下次生成时会打开目录选择对话框
    if (currentDirectory.empty()) {
        targetDirectory = "";
    }
}

// 构建节点路径
std::string ImGuiMainWindow::buildNodePath(const std::shared_ptr<Node>& node, const std::string& basePrefix) {
    if (!node) return "";
    
    std::vector<std::string> pathParts;
    std::shared_ptr<Node> currentNode = node;
    
    // 从当前节点向上构建路径
    while (currentNode) {
        // 获取节点名称
        std::string nodeName = currentNode->getName();
        
        // 如果使用路径前缀，则根据模板内的路径生成前缀
        if (usePrefix && prefixType == PREFIX_PATH) {
            // 为当前节点生成路径前缀，考虑顶层目录
            std::string pathPrefix = Dialogs::generateTemplatePathPrefix(*currentTemplate, currentNode->getId(), createRootFolder, rootFolderName);
            nodeName = pathPrefix + nodeName;
        } else {
            // 否则使用基本前缀
            nodeName = basePrefix + nodeName;
        }
        
        // 将节点名称添加到路径部分
        pathParts.push_back(nodeName);
        
        // 如果是根节点，则停止
        if (currentNode->getParentId().empty()) {
            break;
        }
        
        // 获取父节点
        auto parentNode = currentTemplate->getNode(currentNode->getParentId());
        if (!parentNode) {
            // 如果找不到父节点，停止遍历
            break;
        }
        currentNode = parentNode;
    }
    
    // 反转路径部分（从根到叶）
    std::reverse(pathParts.begin(), pathParts.end());
    
    // 构建路径字符串
    std::string path;
    for (size_t i = 0; i < pathParts.size(); i++) {
        if (i > 0) {
            path += "\\";
        }
        path += pathParts[i];
    }
    
    return path;
}

// 导入目录结构
void ImGuiMainWindow::importDirectoryStructure(const std::string& directoryPath) {
    if (directoryPath.empty()) {
        updateStatusText(trStr("错误: 无效的目录路径", "Error: Invalid directory path"));
        return;
    }
    
    // 创建目录操作对象
    DirectoryOps dirOps;
    
    // 导入目录结构到临时模板
    const int maxDepth = 10;         // 最大扫描深度
    const int maxChildrenPerLevel = 10;  // 每个父文件夹下最多扫描100个子文件夹
    const int maxTotalFolders = 100;  // 总的扫描文件夹数量不超过1000个
    auto importedTemplate = dirOps.createTemplateFromDirectory(directoryPath, maxDepth, maxChildrenPerLevel, maxTotalFolders);
    
    if (!importedTemplate || importedTemplate->isEmpty()) {
        updateStatusText(trStr("警告: 目录结构为空或导入失败", "Warning: Folder structure is empty or import failed"));
        return;
    }
    
    // 如果当前模板为空，直接使用导入的模板
    if (!currentTemplate || currentTemplate->isEmpty()) {
        currentTemplate = std::move(importedTemplate);
        
        // 从导入路径获取默认名称（取最后一级目录）
        size_t lastSlash = directoryPath.find_last_of("/\\");
        std::string defaultName = (lastSlash != std::string::npos) ? 
                                 directoryPath.substr(lastSlash + 1) : 
                                 tr("导入的模板", "Imported Template");
        
        currentTemplate->setName(defaultName);
        templateNameInput = defaultName;
        
        // 初始化树节点状态
        treeNodeStates.clear();
        for (const auto& node : currentTemplate->getAllNodes()) {
            TreeNodeState state;
            state.isOpen = true;
            state.isEditing = false;
            treeNodeStates[node->getId()] = state;
        }
        
        // 设置模板已修改标志
        isTemplateModified = true;
        
        updateStatusText(trStr("成功导入目录结构: ", "Imported folder structure: ") + directoryPath);
        return;
    } else {
        // 如果有选中节点，则合并到该节点下
        // 否则作为根节点添加
        if (!selectedNodeId.empty()) {
            if (mergeImportedTemplate(std::move(importedTemplate), selectedNodeId)) {
                // 合并成功，已在mergeImportedTemplate中设置修改标志
                return;
            } else {
                updateStatusText(trStr("错误: 无法合并导入的目录结构", "Error: Could not merge imported folder structure"));
                return;
            }
        } else {
            // 将所有顶层节点添加为根节点
            int addedCount = 0;
            int totalNodeCount = 0;  // 跟踪添加的总节点数
            
            for (const auto& node : importedTemplate->getRootNodes()) {
                // 检查总节点数限制
                if (totalNodeCount >= 100) {
                    updateStatusText(trStr("警告: 已达到导入节点数量限制(100个)",
                                           "Warning: Import node limit reached (100)"));
                    break;
                }
                
                // 创建新的节点ID
                std::string newId = generateUUID();
                auto newNode = std::make_shared<Node>(newId, node->getName(), "");
                
                // 添加到当前模板
                currentTemplate->addNode(newNode);
                totalNodeCount++; // 计数根节点
                
                // 添加所有子节点(递归)
                addChildrenRecursively(importedTemplate.get(), node->getId(), currentTemplate.get(), newId, 1, &totalNodeCount);
                
                // 初始化状态
                TreeNodeState state;
                state.isOpen = true;
                state.isEditing = false;
                treeNodeStates[newId] = state;
                
                addedCount++;
                
                // 限制根节点数量
                if (addedCount >= 10) {
                    updateStatusText(trStr("警告: 已达到导入根节点数量限制(10个)",
                                           "Warning: Import root node limit reached (10)"));
                    break;
                }
            }
            
            if (totalNodeCount >= 100) {
                updateStatusText(trStr("已添加 ", "Added ") + std::to_string(addedCount) +
                                 trStr(" 个根目录，节点数已达最大限制(100个)", " root folders. Node limit reached (100)."));
            } else {
                updateStatusText(trStr("已添加 ", "Added ") + std::to_string(addedCount) +
                                 trStr(" 个根目录，共 ", " root folders, ") + std::to_string(totalNodeCount) +
                                 trStr(" 个节点", " nodes total"));
            }
        }
    }
    
    // 刷新UI
    refreshTemplateList();
}

// 递归添加子节点（用于导入）
void ImGuiMainWindow::addChildrenRecursively(const Template* sourceTemplate, const std::string& sourceNodeId,
                                              Template* targetTemplate, const std::string& targetParentId,
                                              int currentDepth, int* totalCount) {
    // 如果是第一次调用，初始化计数器
    int localCount = 0;
    int* count = totalCount ? totalCount : &localCount;
    
    // 最大深度和数量限制
    const int maxDepth = 10;  // 最大递归深度
    const int maxChildrenPerLevel = 10;  // 每层最多子节点数
    const int maxTotalNodes = 100;  // 总节点数限制
    
    // 检查深度和总数量限制
    if (currentDepth > maxDepth || *count >= maxTotalNodes) {
        return;
    }
    
    // 获取源节点的所有子节点
    auto childNodes = sourceTemplate->getChildNodes(sourceNodeId);
    
    // 限制每层的子节点数量
    int childrenToProcess = std::min(static_cast<int>(childNodes.size()), maxChildrenPerLevel);
    
    // 对每个子节点递归处理
    for (int i = 0; i < childrenToProcess; ++i) {
        // 检查总数量限制
        if (*count >= maxTotalNodes) {
            break;
        }
        
        const auto& childNode = childNodes[i];
        
        // 创建新节点
        std::string newId = generateUUID();
        auto newNode = std::make_shared<Node>(newId, childNode->getName(), targetParentId);
        
        // 添加到目标模板
        targetTemplate->addNode(newNode);
        (*count)++;
        
        // 初始化状态
        TreeNodeState state;
        state.isOpen = true;
        state.isEditing = false;
        treeNodeStates[newId] = state;
        
        // 递归处理子节点的子节点
        addChildrenRecursively(sourceTemplate, childNode->getId(), targetTemplate, newId, currentDepth + 1, count);
    }
}

// 合并导入的模板
bool ImGuiMainWindow::mergeImportedTemplate(std::unique_ptr<Template> importedTemplate, const std::string& targetNodeId) {
    if (!importedTemplate || importedTemplate->isEmpty() || targetNodeId.empty()) {
        return false;
    }
    
    // 获取目标节点
    auto targetNode = currentTemplate->getNode(targetNodeId);
    if (!targetNode) {
        return false;
    }
    
    // 将导入模板的根节点作为目标节点的子节点
    int addedCount = 0;
    int totalNodeCount = 0;  // 跟踪添加的总节点数
    
    for (const auto& rootNode : importedTemplate->getRootNodes()) {
        // 检查总节点数限制
        if (totalNodeCount >= 100) {
            updateStatusText(trStr("警告: 已达到导入节点数量限制(100个)",
                                   "Warning: Import node limit reached (100)"));
            break;
        }
        
        // 创建新节点ID
        std::string newId = generateUUID();
        auto newNode = std::make_shared<Node>(newId, rootNode->getName(), targetNodeId);
        
        // 添加到当前模板
        currentTemplate->addNode(newNode);
        totalNodeCount++;  // 计数根节点
        
        // 添加所有子节点(递归)
        addChildrenRecursively(importedTemplate.get(), rootNode->getId(), currentTemplate.get(), newId, 1, &totalNodeCount);
        
        // 初始化状态
        TreeNodeState state;
        state.isOpen = true;
        state.isEditing = false;
        treeNodeStates[newId] = state;
        
        addedCount++;
        
        // 限制子目录数量
        if (addedCount >= 10) {
            updateStatusText(trStr("警告: 已达到导入子目录数量限制(10个)",
                                   "Warning: Import subfolder limit reached (10)"));
            break;
        }
    }
    
    // 确保目标节点展开
    auto& targetState = treeNodeStates[targetNodeId];
    targetState.isOpen = true;
    
    // 设置模板已修改标志
    isTemplateModified = true;
    
    if (totalNodeCount >= 100) {
        updateStatusText(trStr("已添加 ", "Added ") + std::to_string(addedCount) +
                         trStr(" 个子目录，节点数已达最大限制(100个)", " subfolders. Node limit reached (100)."));
    } else {
        updateStatusText(trStr("已添加 ", "Added ") + std::to_string(addedCount) +
                         trStr(" 个子目录，共 ", " subfolders, ") + std::to_string(totalNodeCount) +
                         trStr(" 个节点", " nodes total"));
    }
    
    return true;
}

// 设置深蓝主题 - 默认主题
void ImGuiMainWindow::setDarkBlueTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // 重置为默认的深蓝主题
    colors[ImGuiCol_Text] = COLOR_TEXT;
    colors[ImGuiCol_TextDisabled] = COLOR_TEXT_DIM;
    colors[ImGuiCol_WindowBg] = COLOR_BG;
    colors[ImGuiCol_ChildBg] = COLOR_BG_PANEL;
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.15f, 0.20f, 0.95f);
    colors[ImGuiCol_Border] = COLOR_BORDER;
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.20f, 0.26f, 0.60f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.30f, 0.35f, 0.70f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.35f, 0.80f);
    colors[ImGuiCol_TitleBg] = COLOR_BG;
    colors[ImGuiCol_TitleBgActive] = COLOR_ACCENT_DARK;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.42f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = COLOR_ACCENT;
    colors[ImGuiCol_CheckMark] = COLOR_ACCENT;
    colors[ImGuiCol_SliderGrab] = COLOR_ACCENT_DARK;
    colors[ImGuiCol_SliderGrabActive] = COLOR_ACCENT;
    colors[ImGuiCol_Button] = COLOR_BUTTON;
    colors[ImGuiCol_ButtonHovered] = COLOR_BUTTON_HOVER;
    colors[ImGuiCol_ButtonActive] = COLOR_BUTTON_ACTIVE;
    colors[ImGuiCol_Header] = COLOR_HEADER;
    colors[ImGuiCol_HeaderHovered] = COLOR_HEADER_HOVERED;
    colors[ImGuiCol_HeaderActive] = COLOR_HEADER_ACTIVE;
    colors[ImGuiCol_Separator] = COLOR_BORDER;
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.25f, 0.37f, 0.80f);
    colors[ImGuiCol_TabHovered] = COLOR_HEADER_HOVERED;
    colors[ImGuiCol_TabSelected] = COLOR_ACCENT_DARK;
    colors[ImGuiCol_TabDimmed] = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.20f, 0.26f, 0.32f, 1.00f);
    
    
    // 导航和鼠标指针
    colors[ImGuiCol_NavCursor] = COLOR_ACCENT;
    
    updateStatusText(trStr("已切换到深蓝主题", "Switched to dark blue theme"));
}

// 设置暗色主题
void ImGuiMainWindow::setDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // 标准暗色主题
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 0.50f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 0.70f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.33f, 0.80f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.40f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.80f, 0.80f, 0.70f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.38f, 0.60f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.45f, 0.48f, 0.80f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.55f, 0.58f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.33f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 0.44f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.50f, 0.55f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.28f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.43f, 0.80f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    
    updateStatusText(trStr("已切换到暗色主题", "Switched to dark theme"));
}

// 设置亮色主题
void ImGuiMainWindow::setLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // 亮色主题
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.95f, 0.95f, 0.95f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.82f, 0.88f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.65f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.82f, 0.82f, 0.82f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.46f, 0.98f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Button] = ImVec4(0.75f, 0.75f, 0.75f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 0.95f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.82f, 0.82f, 0.82f, 0.93f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
    
    updateStatusText(trStr("已切换到亮色主题", "Switched to light theme"));
}

// 设置轻量化单色主题 - 替代现有的多主题方案
void ImGuiMainWindow::setLightweightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // 专业现代UI配色方案 - 中性色调为基础，蓝灰为强调
    // 采用60-30-10原则：主色调-辅助色-强调色
    ImVec4 BG_DARK = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);        // 主背景色 - 深灰蓝
    ImVec4 BG_MID = ImVec4(0.20f, 0.21f, 0.23f, 1.00f);         // 中间色 - 中灰蓝
    ImVec4 BG_LIGHT = ImVec4(0.24f, 0.25f, 0.28f, 1.00f);       // 浅背景 - 浅灰蓝
    ImVec4 TEXT_COLOR = ImVec4(0.90f, 0.92f, 0.94f, 1.00f);     // 文本 - 浅灰白
    ImVec4 TEXT_DIM = ImVec4(0.60f, 0.65f, 0.70f, 1.00f);       // 次要文本
    ImVec4 ACCENT = ImVec4(0.40f, 0.67f, 0.78f, 1.00f);         // 强调色 - 柔和蓝
    ImVec4 ACCENT_BRIGHT = ImVec4(0.52f, 0.78f, 0.88f, 1.00f);  // 高亮强调色
    ImVec4 ACCENT_DIM = ImVec4(0.30f, 0.52f, 0.62f, 1.00f);     // 暗强调色
    ImVec4 BORDER = ImVec4(0.26f, 0.28f, 0.30f, 1.00f);         // 边框色
    
    // 输入相关颜色定义 - 确保用户输入时文本可见
    ImVec4 INPUT_BG = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);       // 输入框背景 - 深色不透明
    ImVec4 INPUT_BG_HOVER = ImVec4(0.15f, 0.15f, 0.17f, 1.00f); // 输入框悬停背景
    ImVec4 INPUT_BG_ACTIVE = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);// 输入框激活背景
    ImVec4 INPUT_TEXT = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);     // 输入框文本 - 纯白色
    
    // 现代化轻量风格设置
    style.WindowPadding = ImVec2(3, 3);            // 适当的窗口内边距
    style.FramePadding = ImVec2(5, 3);             // 框架内边距
    style.CellPadding = ImVec2(4, 2);
    style.ItemSpacing = ImVec2(6, 4);              // 项目间距
    style.ItemInnerSpacing = ImVec2(4, 4);         // 内部间距
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;
    
    // 现代圆角设计 - 适度圆角
    style.WindowRounding = 2.0f;                   // 窗口圆角
    style.ChildRounding = 2.0f;                    // 子窗口圆角
    style.FrameRounding = 3.0f;                    // 框架圆角
    style.PopupRounding = 2.0f;                    // 弹窗圆角
    style.ScrollbarRounding = 3.0f;                // 滚动条圆角
    style.GrabRounding = 3.0f;                     // 抓取点圆角
    style.TabRounding = 2.0f;                      // 选项卡圆角
    
    // 边框设置 - 精细边框
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;
    
    // 应用专业协调的配色方案
    colors[ImGuiCol_Text] = TEXT_COLOR;
    colors[ImGuiCol_TextDisabled] = TEXT_DIM;
    colors[ImGuiCol_WindowBg] = BG_MID;
    colors[ImGuiCol_ChildBg] = BG_DARK;
    colors[ImGuiCol_PopupBg] = BG_MID;
    colors[ImGuiCol_Border] = BORDER;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // 输入框相关元素 - 确保高对比度
    colors[ImGuiCol_FrameBg] = INPUT_BG;
    colors[ImGuiCol_FrameBgHovered] = INPUT_BG_HOVER;
    colors[ImGuiCol_FrameBgActive] = INPUT_BG_ACTIVE;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.40f); // 增强文本选中背景
    
    colors[ImGuiCol_TitleBg] = BG_DARK;
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.14f, 0.16f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = BG_DARK;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.34f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.38f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.42f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ACCENT;
    colors[ImGuiCol_SliderGrab] = ACCENT_DIM;
    colors[ImGuiCol_SliderGrabActive] = ACCENT;
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.50f, 0.62f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ACCENT;
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.36f, 0.42f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.44f, 0.54f, 1.00f);
    colors[ImGuiCol_Separator] = BORDER;
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.35f, 0.40f, 0.30f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.40f, 0.45f, 0.40f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.45f, 0.50f, 0.60f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.25f, 0.37f, 0.80f);
    colors[ImGuiCol_TabHovered] = COLOR_HEADER_HOVERED;
    colors[ImGuiCol_TabSelected] = COLOR_ACCENT_DARK;
    colors[ImGuiCol_TabDimmed] = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.20f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.68f, 0.72f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ACCENT_BRIGHT;
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.55f, 0.60f, 0.65f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ACCENT_BRIGHT;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, 0.30f);
    colors[ImGuiCol_DragDropTarget] = ACCENT_BRIGHT;
    colors[ImGuiCol_NavCursor] = ACCENT;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.35f);
}

// 显示节点上下文菜单
void ImGuiMainWindow::showNodeContextMenu(const std::string& nodeId, const std::string& nodeName) {
    if (ImGui::MenuItem(tr("添加子目录", "Add Subfolder"))) {
        std::string newNodeId = generateUUID();
        std::string newName = trStr("新目录", "New Folder");
        
        // 创建新节点
        auto newNode = std::make_shared<Node>(newNodeId, newName, nodeId);
        currentTemplate->addNode(newNode);
        
        // 初始化新节点状态
        TreeNodeState newState;
        newState.isOpen = true;
        newState.isEditing = true;
        newState.editLabel = newName;
        treeNodeStates[newNodeId] = newState;
        
        // 确保父节点是打开的
        auto parentState = treeNodeStates.find(nodeId);
        if (parentState != treeNodeStates.end()) {
            parentState->second.isOpen = true;
        }
        
        // 选中新创建的节点
        selectNode(newNodeId);
        lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
        updateStatusText(trStr("创建了新目录 '", "Created new folder '") + newName + "'");
    }
    
    if (ImGui::MenuItem(tr("添加根目录", "Add Root Folder"))) {
        std::string newNodeId = generateUUID();
        std::string newName = trStr("新根目录", "New Root Folder");
        
        // 创建新根节点（没有父节点）
        auto newNode = std::make_shared<Node>(newNodeId, newName, "");
        currentTemplate->addNode(newNode);
        
        // 初始化新节点状态
        TreeNodeState newState;
        newState.isOpen = true;
        newState.isEditing = true;
        newState.editLabel = newName;
        treeNodeStates[newNodeId] = newState;
        
        // 选中新创建的节点
        selectNode(newNodeId);
        lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
        updateStatusText(trStr("创建了新根目录 '", "Created new root folder '") + newName + "'");
    }
    
    ImGui::Separator();
    
    if (ImGui::MenuItem(tr("删除", "Delete"))) {
        // 检查是否删除了选中节点
        if (nodeId == selectedNodeId) {
            selectedNodeId = "";
        }
        
        // 从模板中删除节点
        currentTemplate->removeNode(nodeId);
        
        // 设置模板已修改标志
        isTemplateModified = true;
        
        // 从状态映射中移除节点
        treeNodeStates.erase(nodeId);
        
        updateStatusText(trStr("已删除目录 '", "Deleted folder '") + nodeName + "'");
    }
    
    if (ImGui::MenuItem(tr("重命名", "Rename"))) {
        selectNode(nodeId);
        auto& state = treeNodeStates[nodeId];
        state.isEditing = true;
        state.editLabel = nodeName;
        lastEditingNodeId = ""; // 重置编辑节点ID以确保设置焦点
        updateStatusText(trStr("开始编辑 '", "Editing '") + nodeName + "'");
    }
}

// 添加文件夹
void ImGuiMainWindow::addFolder() {
    std::string nodeId = generateUUID();
    std::string baseFolderName = trStr("新建文件夹", "New Folder");
    std::string folderName = baseFolderName;
    
    int suffix = 1;
    bool nameExists = false;
    
    // 获取父节点ID
    std::string parentId = selectedNodeId;
    
    // 确保文件夹名称不重复
    do {
        nameExists = false;
        
        if (!parentId.empty()) {
            // 检查选中节点的子节点
            auto children = currentTemplate->getChildNodes(parentId);
            for (const auto& child : children) {
                if (child->getName() == folderName) {
                    nameExists = true;
                    folderName = baseFolderName + " " + std::to_string(++suffix);
                    break;
                }
            }
        } else {
            // 检查根节点
            auto rootNodes = currentTemplate->getRootNodes();
            for (const auto& node : rootNodes) {
                if (node->getName() == folderName) {
                    nameExists = true;
                    folderName = baseFolderName + " " + std::to_string(++suffix);
                    break;
                }
            }
        }
    } while (nameExists);
    
    // 创建新节点
    auto newNode = std::make_shared<Node>(nodeId, folderName, parentId);
    
    // 添加到模板
    currentTemplate->addNode(newNode);
    
    // 设置模板已修改标志
    isTemplateModified = true;
    
    // 选择新节点
    selectedNodeId = nodeId;
    
    // 新建节点时的初始状态初始化
    TreeNodeState& state = treeNodeStates[nodeId];
    state.isOpen = true;
    state.isEditing = true;
    state.editLabel = folderName;
    
    // 确保父节点也是展开的，这样用户可以看到新添加的节点
    if (!parentId.empty()) {
        auto& parentState = treeNodeStates[parentId];
        parentState.isOpen = true;
    }
    
    // 重置lastEditingNodeId以确保下次渲染正确设置焦点
    lastEditingNodeId = "";
    
    // 更新状态
    updateStatusText(trStr("添加了新文件夹 '", "Added new folder '") + folderName + "'");
}

// 更新状态文本
void ImGuiMainWindow::updateStatusText(const std::string& text) {
    statusText = text;
}

// 选择节点
void ImGuiMainWindow::selectNode(const std::string& nodeId) {
    selectedNodeId = nodeId;
}

// 生成UUID的临时方法
std::string ImGuiMainWindow::generateUUID() {
    static int counter = 0;
    // 简单的临时实现，实际项目中应使用适当的UUID库
    std::stringstream ss;
    ss << "template-" << std::time(nullptr) << "-" << counter++;
    return ss.str();
}

// 渲染生成选项面板
void ImGuiMainWindow::renderGenerationPanel() {
    // 这个函数现在只是打开生成设置弹窗，实际内容已移至弹窗
    if (createRootFolder && rootFolderName.empty() && currentTemplate) {
        rootFolderName = currentTemplate->getName();
    }
    ImGui::OpenPopup(tr("生成设置", "Generation Settings"));
}

// 渲染状态栏
void ImGuiMainWindow::renderStatusBar(float contentWidth) {
    // 设置状态栏样式
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.19f, 0.19f, 0.19f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);  // 无圆角
    
    // 创建底部状态栏
    ImGui::BeginChild("StatusBar", ImVec2(contentWidth, 22), false, 
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    ImGui::SetCursorPosY(4);  // 垂直居中对齐
    
    // 状态文本 - 左侧显示
    ImGui::SetCursorPosX(8);
    
    // 根据状态文本内容使用不同颜色
    ImVec4 statusColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // 默认色
    if (statusText.find("成功") != std::string::npos || 
        statusText.find("完成") != std::string::npos ||
        statusText.find("success") != std::string::npos ||
        statusText.find("Success") != std::string::npos ||
        statusText.find("generated") != std::string::npos ||
        statusText.find("Generated") != std::string::npos) {
        statusColor = ImVec4(0.3f, 0.7f, 0.3f, 1.0f); // 成功色 - 绿色
    } else if (statusText.find("错误") != std::string::npos || 
               statusText.find("失败") != std::string::npos ||
               statusText.find("Error") != std::string::npos ||
               statusText.find("error") != std::string::npos ||
               statusText.find("failed") != std::string::npos ||
               statusText.find("Failed") != std::string::npos) {
        statusColor = ImVec4(0.8f, 0.3f, 0.3f, 1.0f); // 错误色 - 红色
    } else if (statusText.find("警告") != std::string::npos ||
               statusText.find("Warning") != std::string::npos ||
               statusText.find("warning") != std::string::npos) {
        statusColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f); // 警告色 - 黄色
    }
    
    ImGui::TextColored(statusColor, "%s", statusText.c_str());
    
    // 右侧显示时间和模板信息
    std::string rightText = getCurrentDateTime() + " | " + templateInfo;
    float rightTextWidth = ImGui::CalcTextSize(rightText.c_str()).x;
    
    ImGui::SameLine();
    ImGui::SetCursorPosX(contentWidth - rightTextWidth - 8);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.00f), "%s", rightText.c_str());
    
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// 新建模板
void ImGuiMainWindow::newTemplate() {
    // 创建具有唯一ID的新模板
    std::string templateId = generateUUID();
    currentTemplate = std::make_unique<Template>(templateId, trStr("新模板", "New Template"));
    
    // 初始化模板名称输入
    templateNameInput = currentTemplate->getName();
    
    // 清除选中节点和树节点状态
    selectedNodeId = "";
    treeNodeStates.clear();
    
    // 自动添加一个默认的根目录
    std::string rootNodeId = generateUUID();
    std::string rootFolderName = trStr("根目录", "Root");
    auto rootNode = std::make_shared<Node>(rootNodeId, rootFolderName, "");
    currentTemplate->addNode(rootNode);
    
    // 设置树节点状态
    TreeNodeState rootState;
    rootState.isOpen = true;
    rootState.isEditing = false;
    rootState.editLabel = rootFolderName;
    treeNodeStates[rootNodeId] = rootState;
    
    // 重置模板修改标志
    isTemplateModified = false;
    
    // 更新状态
    updateStatusText(trStr("已创建新模板", "New template created"));
    updateTemplateInfo(); // 更新模板信息
}

// 保存模板
void ImGuiMainWindow::saveTemplate() {
    if (!currentTemplate) {
        updateStatusText(trStr("错误: 没有活动模板可保存", "Error: No active template to save"));
        return;
    }
    
    // 设置当前输入的名称
    currentTemplate->setName(templateNameInput);
    
    // 使用JsonHandler保存模板
    JsonHandler jsonHandler;
    if (jsonHandler.saveTemplateToStorage(*currentTemplate)) {
        // 重置模板修改标志
        isTemplateModified = false;
        
        updateStatusText(trStr("成功: 模板已保存", "Success: Template saved"));
        refreshTemplateList();
    } else {
        updateStatusText(trStr("错误: 保存模板失败", "Error: Failed to save template"));
    }
}

// 加载模板
void ImGuiMainWindow::loadTemplate(const std::string& templateId) {
    // 使用JsonHandler加载模板
    JsonHandler jsonHandler;
    auto loadedTemplate = jsonHandler.loadTemplateFromStorage(templateId);
    
    if (loadedTemplate) {
        // 清除之前的状态
        selectedNodeId = "";
        treeNodeStates.clear();
        
        // 设置新模板
        currentTemplate = std::move(loadedTemplate);
        templateNameInput = currentTemplate->getName();
        
        // 初始化树节点状态
        for (const auto& node : currentTemplate->getAllNodes()) {
            TreeNodeState state;
            state.isOpen = true;      // 默认展开节点
            state.isEditing = false;
            treeNodeStates[node->getId()] = state;
        }
        
        // 重置模板修改标志
        isTemplateModified = false;
        
        updateStatusText(trStr("已加载模板: ", "Loaded template: ") + currentTemplate->getName());
    } else {
        updateStatusText(trStr("错误: 加载模板失败", "Error: Failed to load template"));
    }
}

// 删除模板
void ImGuiMainWindow::deleteTemplate() {
    if (!currentTemplate) {
        updateStatusText(trStr("错误：没有当前模板", "Error: No current template"));
        return;
    }
    
    // 检查模板是否已保存到存储
    JsonHandler jsonHandler;
    bool isSavedTemplate = false;
    
    // 通过查询存储确认模板是否已保存
    auto templates = jsonHandler.getAvailableTemplates();
    for (const auto& templ : templates) {
        if (templ.first == currentTemplate->getId()) {
            isSavedTemplate = true;
            break;
        }
    }
    
    if (isSavedTemplate) {
        // 删除已保存的模板
        if (jsonHandler.deleteTemplateFromStorage(currentTemplate->getId())) {
            // 显示成功消息
            updateStatusText(trStr("模板删除成功", "Template deleted"));
            
            // 创建新模板
            std::string templateId = generateUUID();
            currentTemplate = std::make_unique<Template>(templateId, trStr("新模板", "New Template"));
            templateNameInput = currentTemplate->getName();
            
            // 清除状态
            selectedNodeId = "";
            treeNodeStates.clear();
            
            // 刷新模板列表
            refreshTemplateList();
        } else {
            updateStatusText(trStr("错误：删除模板失败", "Error: Failed to delete template"));
        }
    } else {
        // 仅清空当前模板的内容，不需要从存储中删除
        currentTemplate->clear();
        
        // 清除状态
        selectedNodeId = "";
        treeNodeStates.clear();
        
        // 更新templateInfo
        updateTemplateInfo();
        
        updateStatusText(trStr("模板内容已清空", "Template content cleared"));
    }
}

// 刷新模板列表
void ImGuiMainWindow::refreshTemplateList() {
    // 清空现有模板列表
    availableTemplates.clear();
    
    // 添加当前模板
    if (currentTemplate) {
        TemplateInfo currentInfo{currentTemplate->getId(), currentTemplate->getName()};
        availableTemplates.push_back(currentInfo);
    }
    
    // 获取所有可用模板
    JsonHandler jsonHandler;
    auto templates = jsonHandler.getAvailableTemplates();
    
    // 添加到列表中（排除当前模板）
    for (const auto& templateInfo : templates) {
        if (currentTemplate && templateInfo.first == currentTemplate->getId()) {
            continue;
        }
        
        TemplateInfo info{templateInfo.first, templateInfo.second};
        availableTemplates.push_back(info);
    }
    
    // 更新选择索引
    selectedTemplateIndex = 0;
}

void ImGuiMainWindow::initializeNeutralExampleTemplate() {
    std::string templateId = generateUUID();
    currentTemplate = std::make_unique<Template>(templateId, trStr("示例模板", "Example Template"));
    templateNameInput = currentTemplate->getName();
    
    selectedNodeId = "";
    treeNodeStates.clear();
    
    auto addExampleNode = [this](const std::string& id, const std::string& name, const std::string& parentId = "") {
        auto node = std::make_shared<Node>(id, name, parentId);
        currentTemplate->addNode(node);
        
        TreeNodeState state;
        state.isOpen = true;
        state.isEditing = false;
        state.editLabel = name;
        treeNodeStates[id] = state;
    };
    
    std::string inputId = generateUUID();
    std::string workingId = generateUUID();
    
    addExampleNode(inputId, trStr("01_输入", "01_Input"));
    addExampleNode(generateUUID(), trStr("参考资料", "References"), inputId);
    addExampleNode(workingId, trStr("02_工作中", "02_Working"));
    addExampleNode(generateUUID(), trStr("草稿", "Drafts"), workingId);
    addExampleNode(generateUUID(), trStr("03_输出", "03_Output"));
    addExampleNode(generateUUID(), trStr("04_归档", "04_Archive"));
    
    rootFolderName = currentTemplate->getName();
    isTemplateModified = false;
    updateTemplateInfo();
}

// 更新模板信息
void ImGuiMainWindow::updateTemplateInfo() {
    if (currentTemplate) {
        templateNameInput = currentTemplate->getName();
        
        // 更新状态栏的模板信息
        int nodeCount = 0;
        std::function<void(const std::string&)> countNodes = [&](const std::string& nodeId) {
            nodeCount++;
            auto children = currentTemplate->getChildNodes(nodeId);
            for (const auto& child : children) {
                countNodes(child->getId());
            }
        };
        
        // 从根节点开始计数
        auto rootNodes = currentTemplate->getRootNodes();
        for (const auto& root : rootNodes) {
            countNodes(root->getId());
        }
        
        // 格式化模板信息
        templateInfo = templateNameInput + " | " + std::to_string(nodeCount) +
                       (uiLanguage == UiLanguage::Chinese ? "个项目" : (nodeCount == 1 ? " item" : " items"));
    } else {
        templateInfo = trStr("无模板", "No template");
    }
}

// 更新UI状态
void ImGuiMainWindow::updateUI(const std::string& statusMessage) {
    // 更新状态文本
    updateStatusText(statusMessage);
    
    // 更新模板信息
    updateTemplateInfo();
    
    // 清除选中节点
    if (statusMessage.find("新建") != std::string::npos ||
        statusMessage.find("New") != std::string::npos) {
        selectedNodeId = "";
    }
    
    // 更新树视图节点状态
    if (currentTemplate) {
        // 为新节点初始化状态
        auto allNodes = currentTemplate->getAllNodes();
        for (const auto& node : allNodes) {
            if (treeNodeStates.find(node->getId()) == treeNodeStates.end()) {
                TreeNodeState state;
                state.isOpen = true;      // 默认展开新节点
                state.isEditing = false;
                treeNodeStates[node->getId()] = state;
            }
        }
    }
}

// 保存模板到存储的临时方法
bool ImGuiMainWindow::saveTemplateToStorage(const Template& templ) {
    // 使用JsonHandler保存模板到存储
    JsonHandler jsonHandler;
    return jsonHandler.saveTemplateToStorage(templ);
}

// 从存储加载模板的临时方法
std::unique_ptr<Template> ImGuiMainWindow::loadTemplateFromStorage(const std::string& templateId) {
    // 使用JsonHandler从文件加载模板
    JsonHandler jsonHandler;
    auto loadedTemplate = jsonHandler.loadTemplateFromStorage(templateId);
    
    if (loadedTemplate) {
        // 清除之前的树节点状态
        treeNodeStates.clear();
        
        // 初始化新节点的状态
        for (const auto& node : loadedTemplate->getAllNodes()) {
            TreeNodeState state;
            state.isOpen = true;      // 默认展开节点
            state.isEditing = false;
            treeNodeStates[node->getId()] = state;
        }
        
        return loadedTemplate;
    }
    
    // 如果加载失败，返回一个新的空模板
    updateStatusText(trStr("警告: 加载模板失败，创建新模板", "Warning: Failed to load template. Creating a new template."));
    return std::make_unique<Template>(templateId, trStr("新模板", "New Template"));
}

// 选择目标目录并生成文件夹
void ImGuiMainWindow::selectTargetAndGenerateFolders() {
    // 检查当前模板是否为空
    if (currentTemplate->isEmpty()) {
        updateStatusText(trStr("当前模板为空，请先添加文件夹", "The current template is empty. Add a folder first."));
        // 显示一个对话框提醒用户
        ImGui::OpenPopup("EmptyTemplateWarning");
        return;
    }
    
    // 如果是从文件管理器右键菜单启动并且已设置当前目录，显示确认对话框
    if (!currentDirectory.empty() && targetDirectory == currentDirectory) {
        showConfirmGenerateDialog = true;
        return;
    }
    
    // 设置标志，表示正在生成目录
    isGeneratingFolders = true;
    
    // 如果目标目录未设置，打开目录选择对话框
    if (targetDirectory.empty()) {
        std::string dir = selectDirectoryDialog(uiLanguage);
        if (dir.empty()) {
            updateStatusText(trStr("未选择目标目录，操作已取消", "No target directory selected. Operation canceled."));
            isGeneratingFolders = false; // 重置标志
            return;
        }
        targetDirectory = dir;
        updateStatusText(trStr("已选择目标目录: ", "Selected target directory: ") + targetDirectory);
    }
    
    // 现在使用共享的代码生成文件夹
    generateFoldersInLocation(targetDirectory);
}

// 打开文件夹
bool ImGuiMainWindow::openFolderInExplorer(const std::string& path) {
    if (path.empty()) {
        updateStatusText(trStr("错误: 无法打开空路径", "Error: Cannot open an empty path"));
        return false;
    }
    
    // 使用ShellExecute替代system调用，避免CMD窗口和阻塞等待
    HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
    
    // ShellExecute返回值如果<=32表示错误
    if ((intptr_t)result <= 32) {
        updateStatusText(trStr("错误: 无法打开文件夹 - ", "Error: Could not open folder - ") + path);
        return false;
    }
    
    return true;
}

// 克隆目录
void ImGuiMainWindow::cloneDirectory() {
    if (selectedNodeId.empty()) {
        updateStatusText(trStr("请先选择要克隆的目录", "Select a folder to clone first"));
        return;
    }
    
    // 获取选中节点
    auto selectedNode = currentTemplate->getNode(selectedNodeId);
    if (!selectedNode) {
        updateStatusText(trStr("无效的节点", "Invalid node"));
        return;
    }
    
    // 创建新节点ID
    std::string newNodeId = generateUUID();
    
    // 创建新节点
    auto parentId = selectedNode->getParentId();
    std::string newName = selectedNode->getName() + trStr(" 副本", " Copy");
    auto newNode = std::make_shared<Node>(newNodeId, newName, parentId);
    
    // 添加到模板
    currentTemplate->addNode(newNode);
    
    // 递归克隆子节点
    cloneChildrenRecursively(currentTemplate.get(), selectedNodeId, newNodeId);
    
    // 初始化新节点状态
    TreeNodeState state;
    state.isOpen = true;
    state.isEditing = false;
    treeNodeStates[newNodeId] = state;
    
    // 选择新节点
    selectNode(newNodeId);
    
    updateStatusText(trStr("已克隆目录: ", "Cloned folder: ") + selectedNode->getName());
}

// 递归克隆子节点
void ImGuiMainWindow::cloneChildrenRecursively(Template* templ, const std::string& sourceNodeId, const std::string& targetParentId) {
    // 获取源节点的所有子节点
    auto childNodes = templ->getChildNodes(sourceNodeId);
    
    // 对每个子节点递归处理
    for (const auto& childNode : childNodes) {
        // 创建新节点ID
        std::string newId = generateUUID();
        
        // 创建新节点
        auto newNode = std::make_shared<Node>(newId, childNode->getName(), targetParentId);
        
        // 添加到模板
        templ->addNode(newNode);
        
        // 初始化状态
        TreeNodeState state;
        state.isOpen = true;
        state.isEditing = false;
        treeNodeStates[newId] = state;
        
        // 递归处理子节点的子节点
        cloneChildrenRecursively(templ, childNode->getId(), newId);
    }
}

// 获取当前日期时间的辅助函数
std::string ImGuiMainWindow::getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    std::tm local_tm;
    localtime_s(&local_tm, &now_time);
    
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local_tm);
    
    return std::string(buffer);
}

// 确认是否在当前位置生成文件夹
void ImGuiMainWindow::confirmGenerateInLocation() {
    if (!showConfirmGenerateDialog) {
        return;
    }
    
    // 创建确认对话框
    ImGui::OpenPopup(tr("确认生成位置", "Confirm Target Location"));
    
    // 计算对话框大小和位置
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 190));
    
    // 设置对话框样式
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, COLOR_BG_PANEL);
    ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    
    // 记录推入的样式数量
    int styleColorCount = 2;  // PopupBg 和 Border
    int styleVarCount = 2;    // WindowPadding 和 WindowBorderSize
    
    if (ImGui::BeginPopupModal(tr("确认生成位置", "Confirm Target Location"), NULL, ImGuiWindowFlags_NoResize)) {
        // 标题
        ImGui::TextColored(COLOR_ACCENT, "%s", tr("确认生成位置", "Confirm Target Location"));
        ImGui::Separator();
        ImGui::Spacing();
        
        // 内容区域
        ImGui::Text("%s", tr("是否在以下位置生成文件夹结构?", "Generate the folder structure in this location?"));
        ImGui::Spacing();
        
        // 显示路径
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f)); // 高亮路径颜色
        styleColorCount++;
        ImGui::Text("%s", currentDirectory.c_str());
        ImGui::PopStyleColor();
        styleColorCount--;
        
        ImGui::Spacing();
        
        // 警告信息
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARNING);
        styleColorCount++;
        ImGui::TextWrapped("%s", tr("注意: 如果目标位置已存在同名文件夹，将会被覆盖!",
                                     "Note: Existing folders with the same name may be overwritten."));
        ImGui::PopStyleColor();
        styleColorCount--;
        
        ImGui::Dummy(ImVec2(0, 15));
        
        // 按钮布局
        float buttonWidth = 120;
        float spacing = 10.0f;
        float windowWidth = ImGui::GetWindowWidth();
        float buttonsWidth = 2 * buttonWidth + spacing;
        ImGui::SetCursorPosX((windowWidth - buttonsWidth) * 0.5f);
        
        // 取消按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        styleColorCount += 3;
        
        if (ImGui::Button(tr("取消", "Cancel"), ImVec2(buttonWidth, 0))) {
            showConfirmGenerateDialog = false;
            ImGui::CloseCurrentPopup();
            updateStatusText(trStr("已取消生成操作", "Generation canceled"));
        }
        
        ImGui::PopStyleColor(3);
        styleColorCount -= 3;
        
        ImGui::SameLine(0, spacing);
        
        // 确认按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.45f, 0.25f, 1.0f));
        styleColorCount += 3;
        
        if (ImGui::Button(tr("确认", "OK"), ImVec2(buttonWidth, 0))) {
            showConfirmGenerateDialog = false;
            ImGui::CloseCurrentPopup();
            
            // 在指定位置生成文件夹
            generateFoldersInLocation(currentDirectory);
        }
        
        ImGui::PopStyleColor(3);
        styleColorCount -= 3;
        
        ImGui::EndPopup();
    }
    
    // 确保弹出所有剩余的样式
    if (styleVarCount > 0) {
        ImGui::PopStyleVar(styleVarCount);
    }
    if (styleColorCount > 0) {
        ImGui::PopStyleColor(styleColorCount);
    }
}

// 在指定位置生成文件夹
void ImGuiMainWindow::generateFoldersInLocation(const std::string& location) {
    // 检查当前模板是否为空
    if (currentTemplate->isEmpty()) {
        updateStatusText(trStr("当前模板为空，请先添加文件夹", "The current template is empty. Add a folder first."));
        // 显示一个对话框提醒用户
        ImGui::OpenPopup("EmptyTemplateWarning");
        return;
    }
    
    // 设置标志，表示正在生成目录
    isGeneratingFolders = true;
    
    // 确保目标目录已设置
    targetDirectory = location;
    
    // 创建目录生成器
    DirectoryOps dirOps;
    
    // 创建生成选项
    GenerationOptions options;
    options.createRootFolder = createRootFolder;
    options.rootFolderName = rootFolderName;
    options.usePrefix = usePrefix;
    options.prefixType = prefixType;
    options.dateFormat = dateFormat;
    options.includeFolderSeparator = includeFolderSeparator;
    options.customPrefix = customPrefix;
    
    // 如果rootFolderName为空，使用模板名称作为默认值
    if (createRootFolder && rootFolderName.empty()) {
        rootFolderName = currentTemplate->getName();
        options.rootFolderName = rootFolderName;
    }
    
    // 生成基本前缀（对于日期和自定义前缀）
    std::string basePrefix = "";
    if (usePrefix && prefixType != PREFIX_PATH) {
        // 确保目标目录不为空
        if (targetDirectory.empty()) {
            updateStatusText(trStr("错误: 目标目录未设置，无法生成前缀", "Error: Target directory is not set. Cannot generate prefix."));
            return;
        }
        
        // 根据前缀类型生成前缀
        basePrefix = Dialogs::generatePrefix(options, targetDirectory);
        
        // 检查前缀是否成功生成
        if (basePrefix.empty() && prefixType != PREFIX_NONE) {
            std::string errorMsg = trStr("警告: 前缀生成失败，将不使用前缀。",
                                         "Warning: Prefix generation failed. No prefix will be used.");
            if (prefixType == PREFIX_CUSTOM && customPrefix.empty()) {
                errorMsg += trStr("\n自定义前缀不能为空。", "\nCustom prefix cannot be empty.");
            }
            MessageBoxA(NULL, errorMsg.c_str(), tr("前缀生成警告", "Prefix Warning"), MB_OK | MB_ICONWARNING);
            updateStatusText(trStr("警告: 前缀生成失败", "Warning: Prefix generation failed"));
        }
    }
    
    // 检查目录冲突
    auto conflicts = dirOps.checkDirectoryConflicts(*currentTemplate, targetDirectory, createRootFolder, rootFolderName, basePrefix);
    
    if (!conflicts.empty()) {
        std::string conflictMsg = trStr("错误: 检测到文件夹冲突，继续操作将导致数据丢失!\n\n以下文件夹已存在:\n",
                                        "Error: Folder conflicts detected. Continuing could cause data loss.\n\nExisting folders:\n");
        for (size_t i = 0; i < conflicts.size() && i < 5; ++i) {
            conflictMsg += "- " + conflicts[i] + "\n";
        }
        
        if (conflicts.size() > 5) {
            conflictMsg += trStr("... 以及其他 ", "... and ") + std::to_string(conflicts.size() - 5) +
                           trStr(" 个文件夹\n", " more folders\n");
        }
        
        conflictMsg += trStr("\n请修改目标路径或前缀设置后重试。",
                             "\nChange the target path or prefix settings and try again.");
        
        // 显示错误消息，并阻止继续操作
        MessageBoxA(NULL, conflictMsg.c_str(), tr("文件夹冲突错误", "Folder Conflict Error"), MB_OK | MB_ICONERROR);
        // 更新状态文本，提示用户操作已取消
        updateStatusText(trStr("错误: 检测到文件夹冲突，操作已取消", "Error: Folder conflicts detected. Operation canceled."));
        
        // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
        if (currentDirectory.empty()) {
            targetDirectory = "";
        }
        
        return;
    }
    
    // 生成目录结构
    bool success = dirOps.createDirectoryStructure(*currentTemplate, targetDirectory, options);
    
    if (success) {
        // 确定生成的根目录路径
        std::string rootPath = targetDirectory;
        if (createRootFolder && !rootFolderName.empty()) {
            rootPath += "\\" + basePrefix + rootFolderName;
        }
        
        // 保存生成的文件夹路径，用于后续打开
        generatedFolderPath = rootPath;
        
        // 显示成功消息
        updateStatusText(trStr("目录结构已成功生成", "Folder structure generated successfully"));
        
        // 只有当不是从文件管理器右键打开应用时，才显示打开文件夹的对话框
        // 因为用户已经在文件管理器中看到了这个目录
        if (currentDirectory.empty() || targetDirectory != currentDirectory) {
            showOpenFolderDialog = true;
            openFolderPopupOpened = false; // 确保弹窗会显示
        }
    } else {
        updateStatusText(trStr("生成目录结构时出错", "Error generating folder structure"));
    }
    
    // 完成目录生成，重置标志
    isGeneratingFolders = false;
    
    // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
    // 这样下次生成时会打开目录选择对话框
    if (currentDirectory.empty()) {
        targetDirectory = "";
    }
}

// 窗口关闭回调函数 - 静态函数作为GLFW回调
void ImGuiMainWindow::windowCloseCallback(GLFWwindow* window) {
    // 获取用户指针，找到对应的ImGuiMainWindow实例
    ImGuiMainWindow* mainWindow = static_cast<ImGuiMainWindow*>(glfwGetWindowUserPointer(window));
    if (mainWindow) {
        // 调用实例方法处理窗口关闭
        mainWindow->handleWindowClose();
    }
}

// 实例方法处理窗口关闭
void ImGuiMainWindow::handleWindowClose() {
    // 设置立即关闭标志
    requestClose = true;
    
    // 设置窗口不可见，立即让用户感知到关闭操作
    glfwHideWindow(window);
    
    // 禁用所有事件处理
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
    
    // 将窗口标记为应该关闭，确保GLFW层面也知道需要关闭
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// 处理删除模板确认对话框
void ImGuiMainWindow::renderDeleteTemplateConfirmDialog() {
    if (!showConfirmDeleteTemplateDialog || !currentTemplate) {
        return;
    }
    
    // 检查模板是否已保存
    JsonHandler jsonHandler;
    bool isSavedTemplate = false;
    
    // 通过查询存储确认模板是否已保存
    auto templates = jsonHandler.getAvailableTemplates();
    for (const auto& templ : templates) {
        if (templ.first == currentTemplate->getId()) {
            isSavedTemplate = true;
            break;
        }
    }
    
    // 创建确认对话框
    ImGui::OpenPopup(tr("确认删除模板", "Confirm Template Deletion"));
    
    // 计算对话框大小和位置（设置更紧凑的尺寸）
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360, 200)); // 增加高度从180到200
    
    // 设置对话框样式
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, COLOR_BG_PANEL);
    ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    
    // 记录推入的样式数量
    int styleColorCount = 2;  // PopupBg 和 Border
    int styleVarCount = 2;    // WindowPadding 和 WindowBorderSize
    
    if (ImGui::BeginPopupModal(tr("确认删除模板", "Confirm Template Deletion"), NULL, ImGuiWindowFlags_NoResize)) {
        // 创建水平对齐的标题和图标
        ImGui::BeginGroup();
        
        // 添加警告图标（用文字表示）
        ImGui::AlignTextToFramePadding(); // 确保文本垂直对齐
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // 使用较大字体
        ImGui::TextColored(COLOR_ERROR, "⚠"); // 警告图标
        ImGui::PopFont();
        
        ImGui::SameLine();
        
        // 标题文本
        ImGui::AlignTextToFramePadding(); // 确保文本垂直对齐
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // 恢复普通字体
        if (isSavedTemplate) {
            ImGui::TextColored(COLOR_ERROR, "%s", tr("删除模板", "Delete Template"));
        } else {
            ImGui::TextColored(COLOR_ERROR, "%s", tr("清空模板", "Clear Template"));
        }
        ImGui::PopFont();
        
        ImGui::EndGroup();
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // 内容区域
        if (isSavedTemplate) {
            ImGui::Text("%s", tr("确定要删除模板 ", "Delete template "));
            ImGui::SameLine(0, 0);
            ImGui::TextColored(COLOR_ACCENT, "\"%s\"", currentTemplate->getName().c_str());
            ImGui::SameLine(0, 0);
            ImGui::Text("%s", tr(" 吗？", "?"));
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARNING);
            ImGui::Text("%s", tr("此操作无法撤销！", "This cannot be undone."));
            ImGui::PopStyleColor();
        } else {
            ImGui::Text("%s", tr("确定要清空当前模板吗？", "Clear the current template?"));
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, COLOR_WARNING);
            ImGui::Text("%s", tr("此操作将删除模板中的所有目录结构！",
                                  "This will remove all folders from the template."));
            ImGui::PopStyleColor();
        }
        
        ImGui::Dummy(ImVec2(0, 10)); // 增加垂直间距
        
        // 按钮布局
        float buttonWidth = 120;
        float spacing = 15.0f;
        float windowWidth = ImGui::GetWindowWidth();
        
        // 右对齐按钮，但稍微向左移动
        ImGui::SetCursorPosX(windowWidth - 2 * buttonWidth - spacing - 30); // 从20改为30，按钮左移
        
        // 设置取消按钮样式
        ImGui::PushStyleColor(ImGuiCol_Button, COLOR_BG_PANEL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.30f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.35f, 0.40f, 1.00f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        styleColorCount += 3;  // 增加取消按钮的样式计数
        styleVarCount += 1;    // 增加取消按钮的变量计数
        
        if (ImGui::Button(tr("取消", "Cancel"), ImVec2(buttonWidth, 30))) {
            showConfirmDeleteTemplateDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::PopStyleColor(3);  // 弹出取消按钮的颜色样式
        ImGui::PopStyleVar();     // 弹出取消按钮的变量样式
        styleColorCount -= 3;     // 减少已弹出的样式计数
        styleVarCount -= 1;       // 减少已弹出的变量计数
        
        ImGui::SameLine(0, spacing);
        
        // 设置确认按钮样式（红色警告按钮）
        ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ERROR);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.45f, 0.45f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.25f, 0.25f, 1.00f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        styleColorCount += 3;  // 增加确认按钮的样式计数
        styleVarCount += 1;    // 增加确认按钮的变量计数
        
        // 根据模板状态显示不同的按钮文本
        if (ImGui::Button(isSavedTemplate ? tr("确定删除", "Delete") : tr("确定清空", "Clear"), ImVec2(buttonWidth, 30))) {
            showConfirmDeleteTemplateDialog = false;
            ImGui::CloseCurrentPopup();
            // 执行删除操作
            deleteTemplate();
        }
        
        ImGui::PopStyleColor(3);  // 弹出确认按钮的颜色样式
        ImGui::PopStyleVar();     // 弹出确认按钮的变量样式
        styleColorCount -= 3;     // 减少已弹出的样式计数
        styleVarCount -= 1;       // 减少已弹出的变量计数
        
        ImGui::EndPopup();
    }
    
    // 确保弹出所有剩余的样式
    if (styleVarCount > 0) {
        ImGui::PopStyleVar(styleVarCount);
    }
    if (styleColorCount > 0) {
        ImGui::PopStyleColor(styleColorCount);
    }
}

// 处理打开目标文件夹对话框
void ImGuiMainWindow::renderOpenFolderDialog() {
    if (showOpenFolderDialog) {
        // 只在第一次显示时打开弹窗
        if (!openFolderPopupOpened) {
            ImGui::OpenPopup(tr("打开目标文件夹", "Open Target Folder"));
            openFolderPopupOpened = true;
        }
        
        // 计算对话框大小和位置
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 230));
        
        // 设置对话框样式
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, COLOR_BG_PANEL);
        ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        
        // 记录推入的样式数量
        int styleColorCount = 2;  // PopupBg 和 Border
        int styleVarCount = 2;    // WindowPadding 和 WindowBorderSize
        
        if (ImGui::BeginPopupModal(tr("打开目标文件夹", "Open Target Folder"), NULL, ImGuiWindowFlags_NoResize)) {
            ImGui::Text("%s", tr("目录结构已成功生成。是否打开目标文件夹？",
                                  "Folder structure generated. Open the target folder?"));
            ImGui::Separator();
            
            ImGui::Dummy(ImVec2(0, 10));
            
            // 底部按钮
            float buttonWidth = 120;
            float spacing = 10.0f;
            float windowWidth = ImGui::GetWindowWidth();
            float buttonsWidth = 2 * buttonWidth + spacing;
            ImGui::SetCursorPosX((windowWidth - buttonsWidth) * 0.5f);
            
            // 取消按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            styleColorCount += 3;
            
            if (ImGui::Button(tr("否", "No"), ImVec2(buttonWidth, 0))) {
                showOpenFolderDialog = false;
                openFolderPopupOpened = false;
                ImGui::CloseCurrentPopup();
                
                // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
                if (currentDirectory.empty()) {
                    targetDirectory = "";
                }
            }
            
            ImGui::PopStyleColor(3);
            styleColorCount -= 3;
            
            ImGui::SameLine(0, spacing);
            
            // 确认按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.2f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.45f, 0.25f, 1.0f));
            styleColorCount += 3;
            
            if (ImGui::Button(tr("是", "Yes"), ImVec2(buttonWidth, 0))) {
                showOpenFolderDialog = false;
                openFolderPopupOpened = false;
                ImGui::CloseCurrentPopup();
                
                // 执行打开文件夹操作
                openFolderInExplorer(generatedFolderPath);
                
                // 如果不是从文件管理器启动的（currentDirectory为空），则重置目标目录
                if (currentDirectory.empty()) {
                    targetDirectory = "";
                }
            }
            
            ImGui::PopStyleColor(3);
            styleColorCount -= 3;
            
            ImGui::EndPopup();
        } else {
            openFolderPopupOpened = false;
        }
        
        // 确保弹出所有剩余的样式
        if (styleVarCount > 0) {
            ImGui::PopStyleVar(styleVarCount);
        }
        if (styleColorCount > 0) {
            ImGui::PopStyleColor(styleColorCount);
        }
    } else {
        openFolderPopupOpened = false;
    }
}
