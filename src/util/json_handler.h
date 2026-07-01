#ifndef JSON_HANDLER_H
#define JSON_HANDLER_H

#include <string>
#include <vector>
#include <fstream>
#include "../model/template.h"

// JSON Handler class for serializing and deserializing template objects
class JsonHandler {
public:
    JsonHandler();
    ~JsonHandler();

    // Serialization/Deserialization
    std::string serializeTemplate(const Template& templ);
    std::unique_ptr<Template> deserializeTemplate(const std::string& json);
    
    // File operations
    bool saveTemplateToFile(const Template& templ, const std::string& filePath);
    std::unique_ptr<Template> loadTemplateFromFile(const std::string& filePath);
    
    // Template storage operations
    bool saveTemplateToStorage(const Template& templ);
    std::unique_ptr<Template> loadTemplateFromStorage(const std::string& templateId);
    bool deleteTemplateFromStorage(const std::string& templateId);
    std::vector<std::pair<std::string, std::string>> getAvailableTemplates(); // Returns pairs of (id, name)
    
    // Storage directory management
    static std::string getTemplateStorageDirectory();

private:
    // Helper methods
    bool createStorageDirectory();
    std::string getTemplateFilePath(const std::string& templateId);
};

#endif // JSON_HANDLER_H 