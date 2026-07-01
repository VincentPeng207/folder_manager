#include "path_utils.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

bool PathUtils::isValidPath(const std::string& path) {
    // Check for empty path
    if (path.empty()) {
        return false;
    }
    
    // Check for invalid characters
    for (const char c : path) {
        if (c < 32) { // Control characters
            return false;
        }
    }
    
    // Check for reserved characters in the path components (not in the path separators)
    std::string pathCopy = path;
    std::replace(pathCopy.begin(), pathCopy.end(), '\\', '/');
    std::vector<std::string> components;
    
    size_t start = 0;
    size_t end = pathCopy.find('/');
    
    while (end != std::string::npos) {
        if (end > start) {
            components.push_back(pathCopy.substr(start, end - start));
        }
        start = end + 1;
        end = pathCopy.find('/', start);
    }
    
    if (start < pathCopy.length()) {
        components.push_back(pathCopy.substr(start));
    }
    
    for (const auto& component : components) {
        if (component.empty()) {
            continue;
        }
        
        const std::string invalidChars = "<>:\"|?*";
        if (component.find_first_of(invalidChars) != std::string::npos) {
            return false;
        }
        
        // Check for names ending with spaces or periods
        if (component.back() == ' ' || component.back() == '.') {
            return false;
        }
        
        // Check for reserved names
        const std::string reservedNames[] = {
            "CON", "PRN", "AUX", "NUL", 
            "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
            "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
        };
        
        for (const auto& reserved : reservedNames) {
            if (_stricmp(component.c_str(), reserved.c_str()) == 0) {
                return false;
            }
        }
    }
    
    return true;
}

std::string PathUtils::getParentPath(const std::string& path) {
    fs::path fsPath(path);
    return fsPath.parent_path().string();
}

std::string PathUtils::getFileName(const std::string& path) {
    fs::path fsPath(path);
    return fsPath.filename().string();
}

std::string PathUtils::combinePath(const std::string& basePath, const std::string& relativePath) {
    fs::path result = fs::path(basePath) / relativePath;
    return result.string();
}

std::string PathUtils::getAppDataPath() {
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return std::string(appDataPath);
    }
    return "";
}

std::string PathUtils::getDesktopPath() {
    char desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        return std::string(desktopPath);
    }
    return "";
}

std::string PathUtils::getDocumentsPath() {
    char documentsPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath))) {
        return std::string(documentsPath);
    }
    return "";
}

std::string PathUtils::formatPath(const std::string& path) {
    std::string formattedPath = path;
    
    // Replace forward slashes with backslashes
    std::replace(formattedPath.begin(), formattedPath.end(), '/', '\\');
    
    // Remove duplicate backslashes
    size_t pos = 0;
    while ((pos = formattedPath.find("\\\\", pos)) != std::string::npos) {
        formattedPath.replace(pos, 2, "\\");
    }
    
    // Remove trailing backslash if not a root path
    if (formattedPath.length() > 3 && formattedPath.back() == '\\') {
        formattedPath.pop_back();
    }
    
    return formattedPath;
}

std::string PathUtils::normalizePath(const std::string& path) {
    try {
        fs::path normalized = fs::absolute(path).lexically_normal();
        return normalized.string();
    }
    catch (const std::exception&) {
        return path; // Return original if normalization fails
    }
}

bool PathUtils::isAbsolutePath(const std::string& path) {
    if (path.length() < 2) {
        return false;
    }
    
    // Check for drive letter format (X:)
    if (isalpha(path[0]) && path[1] == ':') {
        return true;
    }
    
    // Check for UNC path (\\server\share)
    if (path.length() >= 2 && path[0] == '\\' && path[1] == '\\') {
        return true;
    }
    
    return false;
}

bool PathUtils::pathExists(const std::string& path) {
    return fs::exists(path);
}

bool PathUtils::isDirectory(const std::string& path) {
    return fs::is_directory(path);
}

bool PathUtils::isFile(const std::string& path) {
    return fs::is_regular_file(path);
} 