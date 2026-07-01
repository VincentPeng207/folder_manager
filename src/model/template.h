#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "node.h"

// Template class represents a folder structure template
class Template {
public:
    Template(const std::string& id = "", const std::string& name = "New Template");
    ~Template();

    // Getters
    std::string getId() const { return id; }
    std::string getName() const { return name; }
    
    // Setters
    void setName(const std::string& newName) { name = newName; }
    void setId(const std::string& newId) { id = newId; }
    
    // Node management
    void addNode(std::shared_ptr<Node> node);
    void removeNode(const std::string& nodeId);
    std::shared_ptr<Node> getNode(const std::string& nodeId) const;
    std::vector<std::shared_ptr<Node>> getRootNodes() const;
    std::vector<std::shared_ptr<Node>> getChildNodes(const std::string& parentId) const;
    std::vector<std::shared_ptr<Node>> getAllNodes() const;
    int getChildrenCount(const std::string& nodeId) const;
    void clear();
    bool isEmpty() const { return nodes.empty(); }
    int getNodeCount() const { return nodes.size(); }

private:
    std::string id;                                       // Unique identifier for the template
    std::string name;                                     // Name of the template
    std::map<std::string, std::shared_ptr<Node>> nodes;  // Map of nodes by ID
};

#endif // TEMPLATE_H 