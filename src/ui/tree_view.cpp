#include "tree_view.h"
#include <windowsx.h>
#include <map>

// 确保在任何其他代码之前定义UNICODE和_UNICODE
#define UNICODE
#define _UNICODE

// 确保Windows.h包含正确的版本
#include <windows.h>

#define IDC_TREEVIEW 101

// Item data map to associate tree items with node IDs
static std::map<HTREEITEM, std::string> itemNodeMap;

// 用于保存this指针的map
static std::map<HWND, TreeView*> treeViewInstances;

// Helper function declarations
static std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

// Add helper function to convert from wstring to string (UTF-8)
static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

TreeView::TreeView(HWND parentHwnd) 
    : hwndTree(NULL), parentHwnd(parentHwnd), imageList(NULL), oldTreeProc(NULL), deleteCallback(NULL) {
}

TreeView::~TreeView() {
    // Clean up
    if (imageList) {
        ImageList_Destroy(imageList);
    }
    
    // 移除窗口子类化
    if (hwndTree && oldTreeProc) {
        SetWindowLongPtr(hwndTree, GWLP_WNDPROC, (LONG_PTR)oldTreeProc);
    }
    
    // 从实例映射中移除
    if (hwndTree) {
        treeViewInstances.erase(hwndTree);
    }
}

bool TreeView::create() {
    // Call the parameterized create method with default position and size
    return create(parentHwnd, 0, 0, 0, 0);
}

bool TreeView::create(HWND parent, int x, int y, int width, int height) {
    // Store the parent window handle
    parentHwnd = parent;
    
    // Create the tree view control with specified position and size
    hwndTree = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEWW,
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS,
        x, y, width, height,
        parentHwnd,
        (HMENU)IDC_TREEVIEW,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!hwndTree) {
        DWORD error = GetLastError();
        std::wstring errorMsg = L"Failed to create tree view control. Error code: " + std::to_wstring(error);
        MessageBoxW(NULL, errorMsg.c_str(), L"TreeView Creation Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // 子类化TreeView控件以处理键盘消息
    try {
        treeViewInstances[hwndTree] = this;
        oldTreeProc = (WNDPROC)SetWindowLongPtr(hwndTree, GWLP_WNDPROC, (LONG_PTR)treeViewProc);
        
        if (!oldTreeProc) {
            DWORD error = GetLastError();
            if (error != 0) { // 只有当真正有错误时才显示
                std::wstring errorMsg = L"Failed to subclass tree view. Error code: " + std::to_wstring(error);
                MessageBoxW(NULL, errorMsg.c_str(), L"TreeView Subclassing Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        
        // Create and set image list with folder icons
        imageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 0);
        if (!imageList) {
            DWORD error = GetLastError();
            std::wstring errorMsg = L"Failed to create image list. Error code: " + std::to_wstring(error);
            MessageBoxW(NULL, errorMsg.c_str(), L"ImageList Creation Error", MB_OK | MB_ICONERROR);
            // 不是关键错误，继续执行
        } else {
            // Load standard folder icon from system
            SHFILEINFOW sfi = {0};
            if (SHGetFileInfoW(L"C:\\", 0, &sfi, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON)) {
                if (sfi.hIcon) {
                    ImageList_AddIcon(imageList, sfi.hIcon);
                    ImageList_AddIcon(imageList, sfi.hIcon); // Use same icon for both states
                    TreeView_SetImageList(hwndTree, imageList, TVSIL_NORMAL);
                    DestroyIcon(sfi.hIcon); // Clean up the icon
                }
            }
        }
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "TreeView Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}

// 定义TreeView控件的子类窗口过程
LRESULT CALLBACK TreeView::treeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 获取与该hwnd关联的TreeView实例
    TreeView* pThis = treeViewInstances[hwnd];
    if (!pThis) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    switch (msg) {
        case WM_KEYDOWN:
            if (wParam == VK_DELETE) {
                // 如果设置了回调函数，调用它
                if (pThis->deleteCallback) {
                    pThis->deleteCallback();
                    return 0;
                }
            }
            break;
            
        case WM_LBUTTONDOWN:
            {
                // 获取点击的树项
                TVHITTESTINFO hitTest = {0};  // 确保完全初始化结构体
                hitTest.pt.x = GET_X_LPARAM(lParam);
                hitTest.pt.y = GET_Y_LPARAM(lParam);
                
                // 执行击中测试
                HTREEITEM hitItem = TreeView_HitTest(hwnd, &hitTest);
                
                // 如果点击不在任何项目上，清除选择
                if (hitItem == NULL && !(hitTest.flags & TVHT_ONITEM)) {
                    // 这将取消选择当前选中的项目
                    TreeView_SelectItem(hwnd, NULL);
                }
                // 注意：我们不返回0，让原始处理器也能处理此消息
            }
            break;
    }
    
    // 调用原始窗口过程
    return CallWindowProc(pThis->oldTreeProc, hwnd, msg, wParam, lParam);
}

void TreeView::refresh(const Template& templ) {
    clear();
    populateTreeFromTemplate(templ);
}

void TreeView::clear() {
    TreeView_DeleteAllItems(hwndTree);
    itemNodeMap.clear();
}

HTREEITEM TreeView::addNode(const std::string& name, HTREEITEM parent) {
    // 初始化结构体
    TVINSERTSTRUCTW tvInsert;
    ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCTW));
    
    tvInsert.hParent = parent;
    tvInsert.hInsertAfter = TVI_LAST;
    
    // 设置item字段
    tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    
    // 转换UTF-8字符串为宽字符
    std::wstring wname = StringToWString(name);
    tvInsert.item.pszText = const_cast<LPWSTR>(wname.c_str());
    tvInsert.item.cchTextMax = (int)wname.length() + 1; // 包括null终止符
    tvInsert.item.iImage = 0;  // Folder icon
    tvInsert.item.iSelectedImage = 0;  // Same icon when selected
    
    // 使用宽字符版本的TreeView_InsertItem
    return (HTREEITEM)SendMessageW(hwndTree, TVM_INSERTITEMW, 0, (LPARAM)&tvInsert);
}

bool TreeView::deleteNode(HTREEITEM item) {
    if (!item) return false;
    
    // Recursively delete child items to clean up itemNodeMap
    HTREEITEM child = TreeView_GetChild(hwndTree, item);
    while (child) {
        HTREEITEM nextChild = TreeView_GetNextSibling(hwndTree, child);
        deleteNode(child);
        child = nextChild;
    }
    
    // Remove from node map and delete from tree
    itemNodeMap.erase(item);
    return TreeView_DeleteItem(hwndTree, item);
}

bool TreeView::renameNode(HTREEITEM item, const std::string& newName) {
    if (!item) return false;
    
    // 初始化结构体
    TVITEMW tvi;
    ZeroMemory(&tvi, sizeof(TVITEMW));
    
    tvi.hItem = item;
    tvi.mask = TVIF_TEXT;
    
    // 确保字符串转换正确
    std::wstring wname = StringToWString(newName);
    tvi.pszText = const_cast<LPWSTR>(wname.c_str());
    tvi.cchTextMax = (int)wname.length() + 1;  // 包括null终止符
    
    // 使用宽字符版本的SendMessage
    return (bool)SendMessageW(hwndTree, TVM_SETITEMW, 0, (LPARAM)&tvi);
}

HTREEITEM TreeView::getSelectedItem() const {
    return TreeView_GetSelection(hwndTree);
}

std::string TreeView::getItemText(HTREEITEM item) const {
    if (!item) return "";
    
    // 使用足够大的缓冲区
    wchar_t buffer[512] = {0};
    
    // 初始化结构体
    TVITEMW tvi;
    ZeroMemory(&tvi, sizeof(TVITEMW));
    
    tvi.hItem = item;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = buffer;
    tvi.cchTextMax = 511; // 保留空间给null终止符
    
    // 使用宽字符版本的SendMessage
    if (SendMessageW(hwndTree, TVM_GETITEMW, 0, (LPARAM)&tvi)) {
        return WStringToString(buffer);
    }
    
    return "";
}

void TreeView::setItemNodeId(HTREEITEM item, const std::string& nodeId) {
    if (item) {
        itemNodeMap[item] = nodeId;
    }
}

std::string TreeView::getItemNodeId(HTREEITEM item) const {
    auto it = itemNodeMap.find(item);
    if (it != itemNodeMap.end()) {
        return it->second;
    }
    return "";
}

HTREEITEM TreeView::findItemByNodeId(const std::string& nodeId) const {
    for (const auto& pair : itemNodeMap) {
        if (pair.second == nodeId) {
            return pair.first;
        }
    }
    return NULL;
}

void TreeView::onSize(int width, int height) {
    // Resize the tree view control to fill the specified area
    if (hwndTree) {
        MoveWindow(hwndTree, 0, 0, width, height, TRUE);
    }
}

void TreeView::populateTreeFromTemplate(const Template& templ) {
    // First add root nodes
    auto rootNodes = templ.getRootNodes();
    for (const auto& node : rootNodes) {
        HTREEITEM item = addNode(node->getName());
        setItemNodeId(item, node->getId());
        // Recursively add children
        addChildrenToTree(templ, node->getId(), item);
    }
}

void TreeView::addChildrenToTree(const Template& templ, const std::string& parentId, HTREEITEM parentItem) {
    auto children = templ.getChildNodes(parentId);
    for (const auto& child : children) {
        HTREEITEM item = addNode(child->getName(), parentItem);
        setItemNodeId(item, child->getId());
        // Recursively add this node's children
        addChildrenToTree(templ, child->getId(), item);
    }
} 