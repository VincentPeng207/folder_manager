# 问题与解决方案记录

## 问题1: EditableComboBox编辑模板名称导致按钮组消失

### 问题描述
当在EditableComboBox输入框中编辑模板名称时，界面上的按钮组（"+模板"、"+ 目录"、"- 目录"、"导入"）会消失。点击保存按钮后，这些按钮组又会重新显示出来。

### 原因分析
这个问题是由两个设计逻辑缺陷共同导致的：

1. UI条件渲染逻辑问题：
   - 在主UI渲染中，renderTemplatePanel的调用被条件语句包裹：`if (selectedTemplateIndex >= 0 && selectedTemplateIndex < (int)availableTemplates.size())`
   - 当selectedTemplateIndex不满足条件时，整个模板面板（包括按钮组）都不会被渲染

2. EditableComboBox实现逻辑问题：
   - 在EditableComboBox中，当用户输入的文本与现有模板名称不匹配时，代码会将currentItem（即selectedTemplateIndex）设置为-1
   - 这导致上面提到的条件判断失败，进而使整个按钮组不显示

这是一个典型的UI状态管理设计不周的例子，两个本应独立的功能（编辑名称和显示按钮组）产生了意外的耦合。

### 解决方案

1. 修改主UI渲染逻辑，去除条件判断，始终渲染模板面板：
   ```cpp
   if (ImGui::BeginChild("TemplatePanel", ImVec2(contentWidth, templatePanelHeight), true, windowFlags)) {
       // 垂直居中
       ImGui::SetCursorPosY((templatePanelHeight - contentHeight) * 0.5f);
       ImGui::Dummy(ImVec2(0,0)); // 添加Dummy以验证光标位置
       renderTemplatePanel(contentWidth);
       ImGui::EndChild();
   }
   ```

2. 修改EditableComboBox实现，在找不到匹配项时保持currentItem值不变，而不是设置为-1：
   ```cpp
   if (!found && items.size() > 0) {
       // 保持 currentItem 不变，而不是设置为 -1
       // 如果需要标记为自定义值，可以在外部单独处理
   }
   ```

3. 添加明确的注释，说明按钮组应始终显示：
   ```cpp
   // 添加目录操作按钮 - 始终显示按钮组，无论selectedTemplateIndex是否有效
   ```

这个修复确保了即使在编辑模板名称时，UI的其他部分仍然保持正常显示和功能。

## 问题2: EditableComboBox自动加载匹配名称的模板

### 问题描述
当在EditableComboBox输入框中修改模板名称时，如果输入的内容与已有的某个模板名称匹配，系统会自动加载该模板，导致以下问题：
1. 如果当前模板尚未保存，会丢失正在编辑的内容
2. 在状态栏显示"加载模板失败"的错误信息
3. 有时会意外加载已保存模板列表中的第一个模板

### 原因分析
EditableComboBox的实现混合了两种不同的交互模式：
1. 自由文本编辑（用户输入新模板名称）
2. 选择现有项目（从下拉列表选择）

在原实现中，当用户通过键盘输入修改文本时，代码会自动搜索匹配项：
```cpp
if (ImGui::InputText("##EditableInput", buffer, bufferSize)) {
    // 搜索匹配项并更新currentItem
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i] == buffer) {
            *currentItem = static_cast<int>(i);
            found = true;
            break;
        }
    }
}
```

然后在renderTemplatePanel函数中，当currentItem变化时会加载对应的模板：
```cpp
if (currentTemplate->getId() == availableTemplates[selectedTemplateIndex].id) {
    currentTemplate->setName(templateNameInput);
} else {
    // 如果选择了不同的模板，则加载该模板
    loadTemplate(availableTemplates[selectedTemplateIndex].id);
}
```

这种设计违反了用户预期，因为用户在编辑模板名称时可能并不希望触发加载操作。

### 解决方案

1. 修改EditableComboBox函数，区分文本输入和下拉选择两种交互方式：
   ```cpp
   std::pair<bool, bool> EditableComboBox(const char* label, char* buffer, size_t bufferSize,
                                        const std::vector<std::string>& items, int* currentItem);
   ```
   返回一个pair，第一个元素表示值是否被修改，第二个元素表示是否通过下拉列表选择。

2. 在EditableComboBox实现中，移除文本输入时自动查找匹配项的逻辑：
   ```cpp
   if (ImGui::InputText("##EditableInput", buffer, bufferSize)) {
       modified = true;
       // 输入文本已修改，但不搜索匹配项也不更新currentItem
   }
   ```

3. 在下拉列表选择时设置标记：
   ```cpp
   if (ImGui::Selectable(items[i].c_str(), static_cast<int>(i) == *currentItem)) {
       // ...
       selectedFromDropdown = true; // 标记为通过下拉列表选择
   }
   ```

4. 在renderTemplatePanel中根据交互方式区分处理：
   ```cpp
   if (selectedFromDropdown) {
       // 用户通过下拉列表选择了模板，执行加载操作
       // ...
   } else {
       // 用户仅通过键盘输入修改了文本，只更新当前模板名称
       // ...
   }
   ```

这个修复确保了EditableComboBox的行为符合用户预期：输入文本只会修改当前模板的名称，而不会触发模板加载；只有当用户明确从下拉列表中选择一个项目时，才会加载相应的模板。
