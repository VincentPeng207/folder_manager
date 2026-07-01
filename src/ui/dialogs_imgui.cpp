#include "dialogs.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include "../model/template.h"

// 生成前缀
std::string Dialogs::generatePrefix(const GenerationOptions& options, const std::string& targetDir) {
    if (!options.usePrefix) {
        return "";
    }
    
    std::string prefix;
    
    switch (options.prefixType) {
        case PREFIX_DATE:
            prefix = Dialogs::generateDatePrefix(options);
            break;
        case PREFIX_PATH:
            // 路径前缀现在应该由模板内的路径生成，而不是目标目录
            // 这个方法需要在调用点传入模板和节点信息
            // 这里保留为空，实际生成将在DirectoryOps::createDirectoryStructure中处理
            prefix = "";
            break;
        case PREFIX_CUSTOM:
            prefix = options.customPrefix;
            // 确保自定义前缀末尾有下划线
            if (!prefix.empty() && prefix.back() != '_') {
                prefix += '_';
            }
            break;
        default:
            return "";
    }
    
    return prefix;
}

// 生成日期前缀
std::string Dialogs::generateDatePrefix(const GenerationOptions& options) {
    // 获取当前时间
    std::time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    
    std::ostringstream oss;
    
    switch (options.dateFormat) {
        case DATE_YYYYMMDD:
            oss << std::put_time(timeInfo, "%Y%m%d");
            break;
        case DATE_YYYY_MM_DD:
            oss << std::put_time(timeInfo, "%Y-%m-%d");
            break;
        case DATE_YYYY_MM_DD_:
            oss << std::put_time(timeInfo, "%Y_%m_%d");
            break;
        default:
            oss << std::put_time(timeInfo, "%Y-%m-%d");
            break;
    }
    
    return oss.str() + "_";
}

// 根据目标路径生成前缀（已不再使用，保留为兼容）
std::string Dialogs::generatePathPrefix(const std::string& path, bool includeSeparator) {
    if (path.empty()) {
        return "";
    }
    
    // 处理Windows路径
    std::string processedPath = path;
    
    // 替换所有反斜杠为正斜杠，以便std::filesystem正确处理
    std::replace(processedPath.begin(), processedPath.end(), '\\', '/');
    
    // 解析路径
    std::filesystem::path fsPath(processedPath);
    
    // 获取路径的所有组件
    std::vector<std::string> pathComponents;
    for (const auto& component : fsPath) {
        // 过滤掉空组件和驱动器盘符（如"C:"）
        std::string comp = component.string();
        if (!comp.empty() && comp.back() != ':') {
            pathComponents.push_back(comp);
        }
    }
    
    // 如果路径组件少于1个，返回空前缀
    if (pathComponents.empty()) {
        return "";
    }
    
    // 构建前缀，使用下划线连接所有父文件夹名称
    // 不包括最后一个组件（当前目录）
    std::string prefix;
    size_t lastIndex = pathComponents.size() > 1 ? pathComponents.size() - 1 : pathComponents.size();
    
    for (size_t i = 0; i < lastIndex; ++i) {
        if (!pathComponents[i].empty()) {
            if (!prefix.empty()) {
                prefix += "_";
            }
            prefix += pathComponents[i];
        }
    }
    
    // 添加末尾的下划线
    if (!prefix.empty()) {
        prefix += "_";
    }
    
    return prefix;
}

// 根据模板内的路径生成前缀
std::string Dialogs::generateTemplatePathPrefix(const Template& templ, const std::string& nodeId, bool createRootFolder, const std::string& rootFolderName) {
    if (nodeId.empty()) {
        return "";
    }
    
    // 获取节点
    auto node = templ.getNode(nodeId);
    if (!node) {
        return "";
    }
    
    // 从当前节点向上构建路径
    std::vector<std::string> pathComponents;
    std::shared_ptr<Node> currentNode = node;
    
    // 如果是当前节点，不添加到路径组件中
    while (currentNode) {
        if (currentNode->getId() != nodeId) {
            pathComponents.push_back(currentNode->getName());
        }
        
        // 如果是根节点，则停止
        if (currentNode->getParentId().empty()) {
            break;
        }
        
        // 获取父节点
        currentNode = templ.getNode(currentNode->getParentId());
        if (!currentNode) {
            break;
        }
    }
    
    // 反转路径组件（从根到叶）
    std::reverse(pathComponents.begin(), pathComponents.end());
    
    // 如果创建了顶层目录，将其添加为路径的第一个组件
    if (createRootFolder && !rootFolderName.empty()) {
        pathComponents.insert(pathComponents.begin(), rootFolderName);
    }
    
    // 构建前缀，使用下划线连接所有父文件夹名称
    std::string prefix;
    for (size_t i = 0; i < pathComponents.size(); ++i) {
        if (!pathComponents[i].empty()) {
            if (!prefix.empty()) {
                prefix += "_";
            }
            prefix += pathComponents[i];
        }
    }
    
    // 添加末尾的下划线
    if (!prefix.empty()) {
        prefix += "_";
    }
    
    return prefix;
}

// 选择目录
std::string Dialogs::selectDirectory(HWND parent, const std::string& title) {
    // 在ImGui版本中，我们使用了selectDirectoryDialog函数
    // 这里只是为了保持API兼容性
    return "";
} 