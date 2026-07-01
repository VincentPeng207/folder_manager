#include "directory_ops.h"
#include <windows.h>
#include <direct.h>
#include <iostream>
#include <filesystem>
#include "../util/uuid_generator.h"

namespace fs = std::filesystem;

DirectoryOps::DirectoryOps() {
}

DirectoryOps::~DirectoryOps() {
}

// 带有前缀选项的目录创建
bool DirectoryOps::createDirectoryStructure(const Template& templ, const std::string& basePath, const GenerationOptions& options) {
    std::string rootPath = basePath;
    
    // 生成基本前缀（对于日期和自定义前缀）
    std::string basePrefix = "";
    if (options.usePrefix && options.prefixType != PREFIX_PATH) {
        basePrefix = Dialogs::generatePrefix(options, basePath);
    }
    
    // If requested, create a root folder
    if (options.createRootFolder && !options.rootFolderName.empty()) {
        // 应用前缀到顶层目录名称
        std::string rootFolderName = basePrefix + options.rootFolderName;
        rootPath = basePath + "\\" + rootFolderName;
        if (!createDirectory(rootPath)) {
            return false;
        }
    }
    
    // Map to keep track of node paths
    std::map<std::string, std::string> nodePaths;
    
    // Get all root level nodes (nodes with no parent)
    auto rootNodes = templ.getRootNodes();
    
    // First pass: create all root level directories
    for (const auto& node : rootNodes) {
        // 对于根节点，如果使用路径前缀，需要考虑顶层目录
        std::string folderName;
        if (options.usePrefix && options.prefixType == PREFIX_PATH) {
            // 为根节点生成路径前缀，考虑顶层目录
            std::string pathPrefix = Dialogs::generateTemplatePathPrefix(templ, node->getId(), options.createRootFolder, options.rootFolderName);
            folderName = pathPrefix + node->getName();
        } else {
            // 否则使用基本前缀
            folderName = basePrefix + node->getName();
        }
        
        std::string dirPath = rootPath + "\\" + folderName;
        
        if (!createDirectory(dirPath)) {
            return false;
        }
        nodePaths[node->getId()] = dirPath;
    }
    
    // Second pass: create all child directories in level order
    bool allCreated = true;
    bool createdAny = !rootNodes.empty();
    
    while (createdAny && allCreated) {
        createdAny = false;
        
        // Get all nodes
        auto allNodes = templ.getAllNodes();
        
        for (const auto& node : allNodes) {
            // Skip nodes we've already processed
            if (nodePaths.find(node->getId()) != nodePaths.end()) {
                continue;
            }
            
            // If parent path is known, create this directory
            auto parentId = node->getParentId();
            if (!parentId.empty() && nodePaths.find(parentId) != nodePaths.end()) {
                // 生成前缀
                std::string prefix = basePrefix;
                
                // 如果使用路径前缀，则根据模板内的路径生成前缀
                if (options.usePrefix && options.prefixType == PREFIX_PATH) {
                    prefix = Dialogs::generateTemplatePathPrefix(templ, node->getId(), options.createRootFolder, options.rootFolderName);
                }
                
                // 应用前缀到文件夹名称
                std::string folderName = prefix + node->getName();
                std::string dirPath = nodePaths[parentId] + "\\" + folderName;
                
                if (!createDirectory(dirPath)) {
                    allCreated = false;
                    break;
                }
                nodePaths[node->getId()] = dirPath;
                createdAny = true;
            }
        }
    }
    
    return allCreated;
}

bool DirectoryOps::createDirectoryStructure(const Template& templ, const std::string& basePath, bool createRoot, const std::string& rootName) {
    std::string rootPath = basePath;
    
    // If requested, create a root folder
    if (createRoot && !rootName.empty()) {
        rootPath = basePath + "\\" + rootName;
        if (!createDirectory(rootPath)) {
            return false;
        }
    }
    
    // Map to keep track of node paths
    std::map<std::string, std::string> nodePaths;
    
    // Get all root level nodes (nodes with no parent)
    auto rootNodes = templ.getRootNodes();
    
    // First pass: create all root level directories
    for (const auto& node : rootNodes) {
        std::string dirPath = rootPath + "\\" + node->getName();
        if (!createDirectory(dirPath)) {
            return false;
        }
        nodePaths[node->getId()] = dirPath;
    }
    
    // Second pass: create all child directories in level order
    bool allCreated = true;
    bool createdAny = !rootNodes.empty();
    
    while (createdAny && allCreated) {
        createdAny = false;
        
        // Get all nodes
        auto allNodes = templ.getAllNodes();
        
        for (const auto& node : allNodes) {
            // Skip nodes we've already processed
            if (nodePaths.find(node->getId()) != nodePaths.end()) {
                continue;
            }
            
            // If parent path is known, create this directory
            auto parentId = node->getParentId();
            if (!parentId.empty() && nodePaths.find(parentId) != nodePaths.end()) {
                std::string dirPath = nodePaths[parentId] + "\\" + node->getName();
                if (!createDirectory(dirPath)) {
                    allCreated = false;
                    break;
                }
                nodePaths[node->getId()] = dirPath;
                createdAny = true;
            }
        }
    }
    
    return allCreated;
}

std::unique_ptr<Template> DirectoryOps::createTemplateFromDirectory(const std::string& directoryPath, int maxDepth, int maxChildrenPerLevel, int maxTotalFolders) {
    // Create a new template
    auto templ = std::make_unique<Template>(UuidGenerator::generateUuid(), fs::path(directoryPath).filename().string());
    
    // Initialize total folders counter
    int totalFolders = 0;
    
    // Start recursive directory scanning
    createNodesFromDirectory(templ, directoryPath, "", 0, maxDepth, maxChildrenPerLevel, totalFolders, maxTotalFolders);
    
    return templ;
}

bool DirectoryOps::isValidDirectoryName(const std::string& name) {
    // Check if the name is empty
    if (name.empty()) {
        return false;
    }
    
    // Check if the name contains invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    if (name.find_first_of(invalidChars) != std::string::npos) {
        return false;
    }
    
    // Check if the name ends with space or period
    if (name.back() == ' ' || name.back() == '.') {
        return false;
    }
    
    // Check for reserved names
    const std::string reservedNames[] = {
        "CON", "PRN", "AUX", "NUL", 
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    for (const auto& reserved : reservedNames) {
        if (_stricmp(name.c_str(), reserved.c_str()) == 0) {
            return false;
        }
    }
    
    return true;
}

bool DirectoryOps::hasWriteAccess(const std::string& path) {
    // Check if path exists
    if (!fs::exists(path)) {
        return false;
    }
    
    // Try to create a temporary file to check write access
    std::string testPath = path + "\\~writetest.tmp";
    HANDLE hFile = CreateFileA(
        testPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    CloseHandle(hFile);
    return true;
}

bool DirectoryOps::createDirectory(const std::string& path) {
    return _mkdir(path.c_str()) == 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool DirectoryOps::createNodesFromDirectory(std::unique_ptr<Template>& templ, const std::string& dirPath, const std::string& parentId, int currentDepth, int maxDepth, int maxChildrenPerLevel, int& totalFolders, int maxTotalFolders) {
    // Stop if we've reached max depth
    if (currentDepth >= maxDepth) {
        return true;
    }
    
    // Stop if we've reached max total folders
    if (totalFolders >= maxTotalFolders) {
        return true;
    }
    
    try {
        // Count how many children we've processed
        int childCount = 0;
        
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            // Only process directories
            if (!entry.is_directory()) {
                continue;
            }
            
            // Stop if we've reached max children per level
            if (childCount >= maxChildrenPerLevel) {
                break;
            }
            
            // Stop if we've reached max total folders
            if (totalFolders >= maxTotalFolders) {
                break;
            }
            
            // Create a node for this directory
            std::string nodeId = UuidGenerator::generateUuid();
            std::string nodeName = entry.path().filename().string();
            
            auto node = std::make_shared<Node>(nodeId, nodeName, parentId);
            templ->addNode(node);
            
            // Increment total folders counter
            totalFolders++;
            
            // Recursively process subdirectories
            createNodesFromDirectory(templ, entry.path().string(), nodeId, currentDepth + 1, maxDepth, maxChildrenPerLevel, totalFolders, maxTotalFolders);
            
            childCount++;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
        return false;
    }
}

// 检查目录冲突
std::vector<std::string> DirectoryOps::checkDirectoryConflicts(const Template& templ, const std::string& basePath, bool createRootFolder, const std::string& rootFolderName, const std::string& basePrefix) {
    std::vector<std::string> conflicts;
    
    // 如果创建顶层目录，只需要检查顶层目录是否存在
    if (createRootFolder) {
        std::string rootPath = basePath + "\\" + basePrefix + rootFolderName;
        if (fs::exists(rootPath) && fs::is_directory(rootPath)) {
            conflicts.push_back(rootPath);
        }
        return conflicts;
    }
    
    // 否则，需要检查所有根节点
    auto rootNodes = templ.getRootNodes();
    for (const auto& node : rootNodes) {
        std::string dirPath = basePath + "\\" + basePrefix + node->getName();
        if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
            conflicts.push_back(dirPath);
        }
    }
    
    return conflicts;
} 