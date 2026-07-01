#ifndef DIRECTORY_OPS_H
#define DIRECTORY_OPS_H

#include <string>
#include <vector>
#include <memory>
#include "../model/template.h"
#include "../ui/dialogs.h"

// Directory operations class
class DirectoryOps {
public:
    DirectoryOps();
    ~DirectoryOps();

    // Directory creation from template
    bool createDirectoryStructure(const Template& templ, const std::string& basePath, bool createRoot = false, const std::string& rootName = "");
    
    // 带有前缀选项的目录创建
    bool createDirectoryStructure(const Template& templ, const std::string& basePath, const GenerationOptions& options);

    // Template creation from directory
    std::unique_ptr<Template> createTemplateFromDirectory(const std::string& directoryPath, int maxDepth = 10, int maxChildrenPerLevel = 10, int maxTotalFolders = 100);

    // Validation functions
    static bool isValidDirectoryName(const std::string& name);
    static bool hasWriteAccess(const std::string& path);
    
    // Directory creation
    bool createDirectory(const std::string& path);
    
    // 检查目录冲突
    std::vector<std::string> checkDirectoryConflicts(const Template& templ, const std::string& basePath, bool createRootFolder, const std::string& rootFolderName, const std::string& prefix = "");

private:
    // Helper methods
    bool createNodesFromDirectory(std::unique_ptr<Template>& templ, const std::string& dirPath, 
                                 const std::string& parentId, int currentDepth, int maxDepth, 
                                 int maxChildrenPerLevel, int& totalFolders, int maxTotalFolders);
};

#endif // DIRECTORY_OPS_H 