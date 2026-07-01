#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include <windows.h>
#include <commctrl.h>
#include <memory>
#include <string>
#include "../model/template.h"

// TreeView class to display and manage folder hierarchy
class TreeView {
public:
    TreeView(HWND parentHwnd);
    ~TreeView();

    // Tree view operations
    bool create();
    bool create(HWND parent, int x, int y, int width, int height);
    void refresh(const Template& templ);
    void clear();
    
    // Node operations
    HTREEITEM addNode(const std::string& name, HTREEITEM parent = TVI_ROOT);
    bool deleteNode(HTREEITEM item);
    bool renameNode(HTREEITEM item, const std::string& newName);
    
    // Selection
    HTREEITEM getSelectedItem() const;
    std::string getItemText(HTREEITEM item) const;
    
    // Mapping between tree items and nodes
    void setItemNodeId(HTREEITEM item, const std::string& nodeId);
    std::string getItemNodeId(HTREEITEM item) const;
    HTREEITEM findItemByNodeId(const std::string& nodeId) const;

    // Window handle
    HWND getHandle() const { return hwndTree; }
    
    // Event handlers
    void onSize(int width, int height);
    
    // 设置删除功能回调
    typedef void (*DeleteFolderCallback)();
    void setDeleteFolderCallback(DeleteFolderCallback callback) { deleteCallback = callback; }

private:
    HWND hwndTree;     // Tree view control handle
    HWND parentHwnd;   // Parent window handle
    HIMAGELIST imageList; // Image list for folder icons
    WNDPROC oldTreeProc; // 原始的窗口过程
    DeleteFolderCallback deleteCallback; // 删除文件夹的回调函数
    
    // Helper methods
    void populateTreeFromTemplate(const Template& templ);
    void addChildrenToTree(const Template& templ, const std::string& parentId, HTREEITEM parentItem);
    
    // 子类化的TreeView窗口过程
    static LRESULT CALLBACK treeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif // TREE_VIEW_H 