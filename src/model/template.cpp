#include "template.h"

Template::Template(const std::string& id, const std::string& name)
    : id(id), name(name) {
}

Template::~Template() {
    clear();
}

void Template::addNode(std::shared_ptr<Node> node) {
    if (node) {
        nodes[node->getId()] = node;
    }
}

void Template::removeNode(const std::string& nodeId) {
    // First, collect all child nodes to remove
    std::vector<std::string> nodesToRemove;
    nodesToRemove.push_back(nodeId);
    
    // Find all descendants recursively
    for (size_t i = 0; i < nodesToRemove.size(); i++) {
        std::string currentId = nodesToRemove[i];
        for (const auto& pair : nodes) {
            if (pair.second->getParentId() == currentId) {
                nodesToRemove.push_back(pair.first);
            }
        }
    }
    
    // Remove all collected nodes
    for (const auto& id : nodesToRemove) {
        nodes.erase(id);
    }
}

std::shared_ptr<Node> Template::getNode(const std::string& nodeId) const {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Node>> Template::getRootNodes() const {
    std::vector<std::shared_ptr<Node>> rootNodes;
    for (const auto& pair : nodes) {
        if (pair.second->getParentId().empty()) {
            rootNodes.push_back(pair.second);
        }
    }
    return rootNodes;
}

std::vector<std::shared_ptr<Node>> Template::getChildNodes(const std::string& parentId) const {
    std::vector<std::shared_ptr<Node>> childNodes;
    for (const auto& pair : nodes) {
        if (pair.second->getParentId() == parentId) {
            childNodes.push_back(pair.second);
        }
    }
    return childNodes;
}

std::vector<std::shared_ptr<Node>> Template::getAllNodes() const {
    std::vector<std::shared_ptr<Node>> allNodes;
    for (const auto& pair : nodes) {
        allNodes.push_back(pair.second);
    }
    return allNodes;
}

int Template::getChildrenCount(const std::string& nodeId) const {
    int count = 0;
    for (const auto& pair : nodes) {
        if (pair.second->getParentId() == nodeId) {
            count++;
        }
    }
    return count;
}

void Template::clear() {
    nodes.clear();
} 