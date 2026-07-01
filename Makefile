CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -DUNICODE -D_UNICODE -O2 -ffunction-sections -fdata-sections -flto
LDFLAGS = -static-libgcc -static-libstdc++ -mwindows -s -Wl,--gc-sections -flto
LIBS = -lcomctl32 -lcomdlg32 -lole32 -luuid -lopengl32 -lglu32 -lgdi32

# ImGui相关设置
IMGUI_DIR = include/imgui
GLFW_DIR = include/glfw
STB_DIR = include/stb

# 包含目录
INCLUDES = -I$(IMGUI_DIR) -I$(GLFW_DIR)/include -I$(STB_DIR)

# ImGui源文件
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp \
                $(IMGUI_DIR)/imgui_draw.cpp \
                $(IMGUI_DIR)/imgui_tables.cpp \
                $(IMGUI_DIR)/imgui_widgets.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# 源代码和对象文件
SRC_DIR = src
OBJ_DIR = build/obj
BIN_DIR = build/bin
RES_DIR = resources

# 资源文件
RC_FILE = $(RES_DIR)/app.rc
RES_OBJ = $(OBJ_DIR)/resources/app.res

# 查找所有源文件
SRC_FILES = $(SRC_DIR)/main_imgui.cpp \
            $(SRC_DIR)/app_imgui.cpp \
            $(SRC_DIR)/ui/imgui_main_window.cpp \
            $(SRC_DIR)/ui/dialogs_imgui.cpp \
            $(SRC_DIR)/model/template.cpp \
            $(SRC_DIR)/model/node.cpp \
            $(SRC_DIR)/util/uuid_generator.cpp \
            $(SRC_DIR)/util/json_handler.cpp \
            $(SRC_DIR)/util/stb_image_impl.cpp \
            $(SRC_DIR)/filesystem/directory_ops.cpp \
            $(IMGUI_SOURCES)

# 生成对象文件名
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter-out $(IMGUI_SOURCES), $(SRC_FILES))) \
            $(patsubst $(IMGUI_DIR)/%.cpp, $(OBJ_DIR)/imgui/%.o, $(IMGUI_SOURCES))

# 输出可执行文件
EXE = $(BIN_DIR)/FolderManagerImGui.exe

# 默认目标
all: directories $(EXE)

# 创建必要的目录
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/ui
	@mkdir -p $(OBJ_DIR)/model
	@mkdir -p $(OBJ_DIR)/util
	@mkdir -p $(OBJ_DIR)/filesystem
	@mkdir -p $(OBJ_DIR)/imgui
	@mkdir -p $(OBJ_DIR)/imgui/backends
	@mkdir -p $(OBJ_DIR)/resources

# 编译源文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 编译ImGui源文件
$(OBJ_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 编译资源文件
$(RES_OBJ): $(RC_FILE)
	@mkdir -p $(@D)
	windres -i $< -o $@ --input-format=rc --output-format=coff

# 链接可执行文件
$(EXE): $(OBJ_FILES) $(RES_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) -L$(GLFW_DIR)/lib-mingw -lglfw3

# 清理构建文件
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# 运行程序
run: all
	$(EXE)

.PHONY: all directories clean run 