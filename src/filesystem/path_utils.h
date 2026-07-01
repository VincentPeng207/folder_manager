#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <string>
#include <vector>

// Path utilities class
class PathUtils {
public:
    // Path validation and manipulation
    static bool isValidPath(const std::string& path);
    static std::string getParentPath(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string combinePath(const std::string& basePath, const std::string& relativePath);
    
    // Special directory retrieval
    static std::string getAppDataPath();
    static std::string getDesktopPath();
    static std::string getDocumentsPath();
    
    // Path formatting
    static std::string formatPath(const std::string& path);
    static std::string normalizePath(const std::string& path);
    static bool isAbsolutePath(const std::string& path);
    
    // Path validation and checking
    static bool pathExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool isFile(const std::string& path);
};

#endif // PATH_UTILS_H 