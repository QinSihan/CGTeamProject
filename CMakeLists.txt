cmake_minimum_required(VERSION 3.15)
project(CGTeamProject)

# --- 1. 基础设置 ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 确保 assets 和 shaders 文件夹被复制到生成目录，方便 exe 读取
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/shaders)
# 下面这行有模型文件了再取消注释
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/assets)

# --- 2. 包含路径 ---
# 包含根目录
include_directories(${CMAKE_SOURCE_DIR})
# 包含 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(OpenGL REQUIRED)

# --- 3. 自动下载依赖库 ---

# GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip
)
FetchContent_MakeAvailable(glfw)

# ImGui
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.4.zip
)
FetchContent_MakeAvailable(imgui)

# 准备 ImGui 的源文件列表
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Assimp (模型加载库)
# 这一步第一次运行会比较慢
message(STATUS "Fetching Assimp... (Wait for it...)")
FetchContent_Declare(
    assimp
    URL https://github.com/assimp/assimp/archive/refs/tags/v5.2.5.zip
)
# 关闭 Assimp 不用的功能以加速编译
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE) 
FetchContent_MakeAvailable(assimp)

# --- 4. 生成可执行文件 ---
# 注意：一定要包含 src/glad.c 和 src/main.cpp
add_executable(${PROJECT_NAME} 
    "src/main.cpp"
    "src/glad.c"
    "src/stb_image_impl.cpp"
    "src/Framebuffer.cpp"
    "src/PostProcessor.cpp"
    ${IMGUI_SOURCES}
)

# 每次编译后，自动把 assets 和 shaders 文件夹复制到 exe 旁边
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "正在自动搬运资源文件 (Assets & Shaders)..."
)

# --- 5. 链接库 ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    assimp
)

# 告诉 ImGui 它的头文件在哪里
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Windows 系统需要的额外配置
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
endif()

message(STATUS "Setup complete. Ready to code Cyberpunk!")