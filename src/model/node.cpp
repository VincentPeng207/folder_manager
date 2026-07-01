#include "node.h"

Node::Node(const std::string& id, const std::string& name, const std::string& parentId)
    : id(id), name(name), parentId(parentId), treeItem(NULL) {
}

Node::~Node() {
    // Nothing to do here for now
} 