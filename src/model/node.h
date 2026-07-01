#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include <commctrl.h>

// Node class represents a folder in the template
class Node {
public:
    Node(const std::string& id, const std::string& name, const std::string& parentId = "");
    ~Node();

    // Getters
    std::string getId() const { return id; }
    std::string getName() const { return name; }
    std::string getParentId() const { return parentId; }
    HTREEITEM getTreeItem() const { return treeItem; }
    
    // Setters
    void setName(const std::string& newName) { name = newName; }
    void setParentId(const std::string& newParentId) { parentId = newParentId; }
    void setTreeItem(HTREEITEM item) { treeItem = item; }

private:
    std::string id;        // Unique identifier for the node
    std::string name;      // Name of the folder
    std::string parentId;  // ID of the parent node (empty for root nodes)
    HTREEITEM treeItem;    // Handle to the corresponding tree view item
};

#endif // NODE_H 