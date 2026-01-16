#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <iostream>

#include "Camera.h"
#include "Shader.h"
#include "Model.h"

#include <filesystem> // 调试

// --- 函数声明 ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos); // [NEW]
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset); // [NEW]
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 摄像机
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f)); // 初始位置放高一点，往后一点
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 计时器 (解决不同电脑速度不一样的问题)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    std::cout << "Current path is: " << std::filesystem::current_path() << std::endl; // 调试
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Cyberpunk Recon", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 注册鼠标回调
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //  隐藏光标并捕捉
    //  注意：按 ESC 退出前可能看不到鼠标，这是正常的
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- 加载模型与着色器 ---

    // 1. 启用深度测试 (避免模型透视)
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // 临时

    // 2. 编译着色器 (注意路径！)
    // 相对路径 "shaders/model.vs"
    Shader ourShader("shaders/model.vs", "shaders/model.fs");

    // 3. 加载模型 (会打印 Assimp 日志)
    // 注意：路径必须指向 assets 里的 .gltf 文件
    Model ourModel("assets/scene.gltf");

    // 设置光照方向 (从上往下照)
    glm::vec3 lightDirection(-0.2f, -1.0f, -0.3f);

    while (!glfwWindowShouldClose(window)) {
        // 计算每一帧的时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. 清屏
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. 激活 Shader
        ourShader.use();

        // 3. 设置摄像机 (View & Projection)
        // 这里把 Far Plane (远平面) 设置到 1000.0f，防止远处被切掉
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // 4. 设置模型 (Model)
        glm::mat4 model = glm::mat4(1.0f);

        // 往下移 -5 (让人站在地面)，往远推 -10 (确保在视野内)
        model = glm::translate(model, glm::vec3(0.0f, -5.0f, -10.0f));

        // 自动旋转，方便确认它是立体的
        model = glm::rotate(model, (float)glfwGetTime() * 0.2f, glm::vec3(0.0f, 1.0f, 0.0f));

        // 既然你试出来 0.01 是合适的，那就用 0.01
        model = glm::scale(model, glm::vec3(0.01f));

        ourShader.setMat4("model", model);

        // 5. 绘制
        ourModel.Draw(ourShader);
        // --- 3D 场景渲染结束 ---

        // 2. 渲染 ImGui (必须放在 3D 渲染之后，保证 UI 覆盖在画面最上层)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 3. 交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD 控制
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 垂直飞行 (可选)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    // 实现“义眼变焦” 
    // 长按鼠标右键，FOV 变小 (放大看)
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessZoom(2.0f); // 快速缩小 FOV
    }
    else {
        // 松开回弹 (简单的插值回弹逻辑，之后可以写更平滑的)
        if (camera.Zoom < 45.0f) camera.Zoom += 2.0f;
    }
}

// 鼠标移动回调：控制视角
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 注意这里是反的，因为Y坐标是从底部往上的

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// 滚轮回调 (备用)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessZoom(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}