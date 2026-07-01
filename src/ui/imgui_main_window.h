#ifndef IMGUI_MAIN_WINDOW_H
#define IMGUI_MAIN_WINDOW_H

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include <ctime>     // 添加时间相关头文件
#include <sstream>   // 添加stringstream支持
#include <chrono>    // 添加对chrono的支持
#include <iostream>  // 添加iostream支持错误输出
#include "../model/template.h"
#include "../model/node.h"
#include "../ui/dialogs.h"  // 添加dialogs.h头文件以使用前缀相关枚举

// 前向声明
struct GLFWwindow;

enum class UiLanguage {
    Chinese,
    English
};

// ImGuiMainWindow类 - Dear ImGui版本的主窗口
class ImGuiMainWindow {
public:
    ImGuiMainWindow(const std::string& currentDirectory = "");
    ~ImGuiMainWindow();
    
    // 窗口生命周期
    bool create();
    void show();
    void run();  // 主循环
    
private:
    // ImGui UI组件和布局
    void setupImGui();
    void shutdownImGui();
    void render();
    void setWindowIcon();  // 设置窗口图标
    
    // UI组件和布局
    void renderMenuBar();
    void renderTemplatePanel(float contentWidth);
    void renderTreeView();
    bool renderTreeNode(const std::shared_ptr<Node>& node);
    void renderGenerationPanel();
    void renderStatusBar(float contentWidth);
    void renderDeleteTemplateConfirmDialog();
    void renderOpenFolderDialog();
    
    // 自定义UI控件
    std::pair<bool, bool> EditableComboBox(const char* label, char* buffer, size_t bufferSize,
                                        const std::vector<std::string>& items, int* currentItem);
    
    // UI主题设置
    void setLightweightTheme();  // 新增：轻量化单一配色主题
    void setDarkBlueTheme();  // 深蓝主题 - 默认
    void setDarkTheme();      // 暗色主题
    void setLightTheme();     // 亮色主题
    
    // 模板管理
    void newTemplate();
    void saveTemplate();
    void loadTemplate(const std::string& templateId);
    void deleteTemplate();
    void refreshTemplateList();
    void initializeNeutralExampleTemplate();
    
    // 文件夹操作
    void addFolder();
    void deleteFolder();
    void renameNode(const std::string& nodeId, const std::string& newName);
    void generateFolders();
    void selectTargetAndGenerateFolders(); // 选择目标目录并生成文件夹
    void generateFoldersInLocation(const std::string& location); // 在指定位置生成文件夹
    void confirmGenerateInLocation(); // 确认是否在当前位置生成文件夹
    void cloneDirectory();
    void showNodeContextMenu(const std::string& nodeId, const std::string& nodeName);
    
    // 导入功能
    void importDirectoryStructure(const std::string& directoryPath);
    bool mergeImportedTemplate(std::unique_ptr<Template> importedTemplate, const std::string& targetNodeId);
    void addChildrenRecursively(const Template* sourceTemplate, const std::string& sourceNodeId,
                              Template* targetTemplate, const std::string& targetParentId,
                              int currentDepth = 1, int* totalCount = nullptr);
    void cloneChildrenRecursively(Template* templ, const std::string& sourceNodeId, const std::string& targetParentId);
    
    // 路径相关辅助函数
    std::string buildNodePath(const std::shared_ptr<Node>& node, const std::string& basePrefix);
    bool openFolderInExplorer(const std::string& path);
    
    // 辅助函数
    std::string getCurrentDateTime();
    void updateStatusText(const std::string& text);
    void updateTemplateInfo();
    void updateUI(const std::string& statusMessage);
    void selectNode(const std::string& nodeId);
    std::string getSelectedNodeId() const;
    const char* tr(const char* zh, const char* en) const;
    std::string trStr(const char* zh, const char* en) const;
    void setLanguage(UiLanguage language);
    UiLanguage detectSystemLanguage() const;
    void loadLanguagePreference();
    void saveLanguagePreference() const;
    std::string getSettingsFilePath() const;
    void updateWindowTitle();
    
    // 临时实现的辅助方法
    std::string generateUUID();
    bool saveTemplateToStorage(const Template& templ);
    std::unique_ptr<Template> loadTemplateFromStorage(const std::string& templateId);
    
    // GLFW窗口句柄
    GLFWwindow* window;
    
    // 当前模板
    std::unique_ptr<Template> currentTemplate;
    
    // UI状态变量
    UiLanguage uiLanguage;
    std::string statusText;
    std::string templateInfo;  // 模板信息
    std::string templateNameInput;
    int selectedTemplateIndex;
    std::string selectedNodeId;
    std::string searchQuery;   // 搜索查询字符串
    bool isTemplateModified;   // 跟踪模板是否被修改
    bool usePrefix;
    PrefixType prefixType;         // 前缀类型
    DateFormat dateFormat;         // 日期格式
    bool includeFolderSeparator;   // 是否包含路径分隔符
    std::string customPrefix;      // 自定义前缀
    bool createRootFolder;
    std::string targetDirectory;
    std::string currentDirectory;  // 从资源管理器右键启动时的当前目录
    std::string rootFolderName;    // 存储用户自定义的顶层目录名称
    bool showAboutDialog;
    bool showHelpDialog;
    bool showOpenFolderDialog;     // 显示"是否打开生成的文件夹"对话框的标志
    bool showConfirmGenerateDialog; // 显示"是否在位置生成文件夹"对话框的标志
    bool showImportFolderDialog;   // 显示导入目录对话框的标志
    bool showConfirmDeleteTemplateDialog; // 是否显示删除模板确认对话框
    bool openFolderPopupOpened;    // 跟踪弹窗是否已经打开
    std::string generatedFolderPath; // 生成的文件夹路径
    std::string lastEditingNodeId;
    bool isGeneratingFolders;      // 标记是否正在生成文件夹
    
    // 可用模板列表
    struct TemplateInfo {
        std::string id;
        std::string name;
    };
    std::vector<TemplateInfo> availableTemplates;
    
    // 树视图内部状态
    struct TreeNodeState {
        bool isOpen;
        std::string editLabel;
        bool isEditing;
    };
    std::map<std::string, TreeNodeState> treeNodeStates;
    
    // 是否请求立即关闭
    bool requestClose;
    
    // 初始化GLFW窗口
    void initWindow();
    
    // 窗口关闭回调函数
    static void windowCloseCallback(GLFWwindow* window);
    
    // 处理窗口关闭事件
    void handleWindowClose();
};

#endif // IMGUI_MAIN_WINDOW_H 
