#ifndef DIALOGS_H
#define DIALOGS_H

#include <windows.h>
#include <string>
#include <vector>

// 前向声明
class Template;
class Node;

// Input dialog class for text input
class InputDialog {
public:
    // Show an input dialog with the given title, prompt, and initial value
    static bool show(HWND parent, const char* title, const char* prompt, char* buffer, int bufferSize, const char* initialValue = "");
    
private:
    // Dialog procedure
    static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Dialog data
    struct DialogData {
        const char* title;
        const char* prompt;
        char* buffer;
        int bufferSize;
        const char* initialValue;
    };
};

// 前缀类型枚举
enum PrefixType {
    PREFIX_NONE = 0,
    PREFIX_DATE = 1,
    PREFIX_PATH = 2,
    PREFIX_CUSTOM = 3
};

// 日期前缀格式枚举
enum DateFormat {
    DATE_YYYYMMDD = 0,     // 20230520
    DATE_YYYY_MM_DD = 1,   // 2023-05-20
    DATE_YYYY_MM_DD_ = 2   // 2023_05_20
};

// Generation options structure
struct GenerationOptions {
    bool createRootFolder;
    std::string rootFolderName;
    
    // 前缀选项
    bool usePrefix;
    PrefixType prefixType;
    
    // 日期前缀选项
    DateFormat dateFormat;
    
    // 路径前缀选项
    bool includeFolderSeparator;
    
    // 自定义前缀选项
    std::string customPrefix;
    
    // 构造函数，设置默认值
    GenerationOptions() 
        : createRootFolder(true), 
          rootFolderName("root"),
          usePrefix(false), 
          prefixType(PREFIX_DATE), 
          dateFormat(DATE_YYYY_MM_DD),
          includeFolderSeparator(false),
          customPrefix("Prefix_") 
    {}
};

// Generation options dialog class
class GenerationOptionsDialog {
public:
    // Show the generation options dialog
    static bool show(HWND parent, GenerationOptions& options);
    
private:
    // Dialog procedure
    static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Dialog data
    struct DialogData {
        GenerationOptions* options;
    };
    
    // 辅助方法，生成前缀预览
    static std::string generatePrefixPreview(const GenerationOptions& options);
    
    // 辅助方法，生成日期前缀
    static std::string generateDatePrefix(DateFormat format);
    
    // 辅助方法，根据路径生成前缀
    static std::string generatePathPrefix(const std::string& path, bool includeSeparator);
    
    // 辅助方法，验证并格式化自定义前缀
    static std::string validateCustomPrefix(const std::string& prefix);
};

// Dialog class for template and folder operations
class Dialogs {
public:
    // Template dialogs
    static std::string getTemplateName(HWND parent, const std::string& initialName = "");
    static bool confirmDeleteTemplate(HWND parent, const std::string& templateName);
    
    // Folder dialogs
    static std::string getFolderName(HWND parent, const std::string& initialName = "New Folder");
    static bool confirmDeleteFolder(HWND parent, const std::string& folderName);
    
    // File system dialogs
    static std::string selectDirectory(HWND parent, const std::string& title = "Select Directory");
    static bool showFileSelectDialog(HWND parent, std::string& filePath, const char* filter = "All Files\0*.*\0Text Files\0*.txt\0");
    static bool showFolderSelectDialog(HWND parent, std::string& folderPath);
    
    // Generation options dialog
    static bool getGenerationOptions(HWND parent, GenerationOptions& options);
    
    // 验证函数
    static bool isValidFolderName(const std::string& name);
    static bool isValidPrefix(const std::string& prefix);
    
    // 前缀帮助方法
    static std::string generatePrefix(const GenerationOptions& options, const std::string& targetDir);
    static std::string generateDatePrefix(const GenerationOptions& options);
    static std::string generatePathPrefix(const std::string& path, bool includeSeparator);
    static std::string generateTemplatePathPrefix(const Template& templ, const std::string& nodeId, bool createRootFolder = false, const std::string& rootFolderName = "");
};

// 树视图选择对话框类
class TreeSelectDialog {
public:
    static bool show(HWND parent, std::vector<std::string>& selectedItems);
    
private:
    static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif // DIALOGS_H 