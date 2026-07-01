#include "json_handler.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <direct.h>
#include "../model/node.h"

namespace fs = std::filesystem;

JsonHandler::JsonHandler() {
    createStorageDirectory();
}

JsonHandler::~JsonHandler() {
}

std::string JsonHandler::serializeTemplate(const Template& templ) {
    // Start with the basic template information
    std::string json = "{\n";
    json += "    \"id\": \"" + templ.getId() + "\",\n";
    json += "    \"name\": \"" + templ.getName() + "\",\n";
    json += "    \"structure\": [\n";
    
    // Add nodes
    auto nodes = templ.getAllNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        json += "        {\n";
        json += "            \"id\": \"" + nodes[i]->getId() + "\",\n";
        json += "            \"name\": \"" + nodes[i]->getName() + "\",\n";
        json += "            \"parentId\": ";
        
        if (nodes[i]->getParentId().empty()) {
            json += "null";
        } else {
            json += "\"" + nodes[i]->getParentId() + "\"";
        }
        
        json += "\n";
        json += "        }";
        
        if (i < nodes.size() - 1) {
            json += ",";
        }
        json += "\n";
    }
    
    json += "    ]\n";
    json += "}\n";
    
    return json;
}

std::unique_ptr<Template> JsonHandler::deserializeTemplate(const std::string& json) {
    // A very basic JSON parser for the template format
    std::unique_ptr<Template> templ = std::make_unique<Template>();
    
    // Extract the template ID
    size_t idPos = json.find("\"id\":");
    if (idPos != std::string::npos) {
        size_t idStartPos = json.find("\"", idPos + 5) + 1;
        size_t idEndPos = json.find("\"", idStartPos);
        std::string id = json.substr(idStartPos, idEndPos - idStartPos);
        templ->setId(id);
    }
    
    // Extract the template name
    size_t namePos = json.find("\"name\":");
    if (namePos != std::string::npos) {
        size_t nameStartPos = json.find("\"", namePos + 7) + 1;
        size_t nameEndPos = json.find("\"", nameStartPos);
        std::string name = json.substr(nameStartPos, nameEndPos - nameStartPos);
        templ->setName(name);
    }
    
    // Extract the structure/nodes
    size_t structurePos = json.find("\"structure\":");
    if (structurePos != std::string::npos) {
        size_t nodeStart = json.find("{", structurePos);
        
        while (nodeStart != std::string::npos) {
            // Extract node ID
            size_t nodeIdPos = json.find("\"id\":", nodeStart);
            size_t nodeIdStartPos = json.find("\"", nodeIdPos + 5) + 1;
            size_t nodeIdEndPos = json.find("\"", nodeIdStartPos);
            std::string nodeId = json.substr(nodeIdStartPos, nodeIdEndPos - nodeIdStartPos);
            
            // Extract node name
            size_t nodeNamePos = json.find("\"name\":", nodeStart);
            size_t nodeNameStartPos = json.find("\"", nodeNamePos + 7) + 1;
            size_t nodeNameEndPos = json.find("\"", nodeNameStartPos);
            std::string nodeName = json.substr(nodeNameStartPos, nodeNameEndPos - nodeNameStartPos);
            
            // Extract parent ID
            std::string parentId = "";
            size_t parentIdPos = json.find("\"parentId\":", nodeStart);
            if (parentIdPos != std::string::npos) {
                if (json.find("null", parentIdPos) != std::string::npos && 
                    json.find("null", parentIdPos) < json.find(",", parentIdPos)) {
                    // Parent ID is null
                    parentId = "";
                } else {
                    size_t parentIdStartPos = json.find("\"", parentIdPos + 11) + 1;
                    size_t parentIdEndPos = json.find("\"", parentIdStartPos);
                    parentId = json.substr(parentIdStartPos, parentIdEndPos - parentIdStartPos);
                }
            }
            
            // Create node and add to template
            auto node = std::make_shared<Node>(nodeId, nodeName, parentId);
            templ->addNode(node);
            
            // Find the next node
            size_t nextBrace = json.find("}", nodeStart);
            nodeStart = json.find("{", nextBrace);
        }
    }
    
    return templ;
}

bool JsonHandler::saveTemplateToFile(const Template& templ, const std::string& filePath) {
    std::string json = serializeTemplate(templ);
    std::ofstream outFile(filePath);
    
    if (!outFile.is_open()) {
        return false;
    }
    
    outFile << json;
    outFile.close();
    
    return true;
}

std::unique_ptr<Template> JsonHandler::loadTemplateFromFile(const std::string& filePath) {
    std::ifstream inFile(filePath);
    
    if (!inFile.is_open()) {
        return nullptr;
    }
    
    std::string json((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();
    
    return deserializeTemplate(json);
}

bool JsonHandler::saveTemplateToStorage(const Template& templ) {
    std::string storageDir = getTemplateStorageDirectory();
    std::string filePath = getTemplateFilePath(templ.getId());
    
    return saveTemplateToFile(templ, filePath);
}

std::unique_ptr<Template> JsonHandler::loadTemplateFromStorage(const std::string& templateId) {
    std::string filePath = getTemplateFilePath(templateId);
    
    return loadTemplateFromFile(filePath);
}

bool JsonHandler::deleteTemplateFromStorage(const std::string& templateId) {
    std::string filePath = getTemplateFilePath(templateId);
    
    return DeleteFileA(filePath.c_str()) != 0;
}

std::vector<std::pair<std::string, std::string>> JsonHandler::getAvailableTemplates() {
    std::vector<std::pair<std::string, std::string>> templates;
    std::string storageDir = getTemplateStorageDirectory();
    
    for (const auto& entry : fs::directory_iterator(storageDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            auto templ = loadTemplateFromFile(entry.path().string());
            if (templ) {
                templates.push_back(std::make_pair(templ->getId(), templ->getName()));
            }
        }
    }
    
    return templates;
}

std::string JsonHandler::getTemplateStorageDirectory() {
    char appDataPath[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
    
    std::string storagePath = std::string(appDataPath) + "\\FolderManager\\Templates";
    return storagePath;
}

bool JsonHandler::createStorageDirectory() {
    std::string storageDir = getTemplateStorageDirectory();
    
    // Create directory if it doesn't exist
    if (!fs::exists(storageDir)) {
        return fs::create_directories(storageDir);
    }
    
    return true;
}

std::string JsonHandler::getTemplateFilePath(const std::string& templateId) {
    std::string storageDir = getTemplateStorageDirectory();
    return storageDir + "\\" + templateId + ".json";
} 