#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <iostream>

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "PostProcessor.h"

#include <filesystem> // 

// --- 函数声明 ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos); // [NEW]
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset); // [NEW]
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 摄像机
Camera camera(glm::vec3(0.0f, -3.0f, 10.0f)); // 初始位置调整到地面高度 (对应地面碰撞逻辑的 -3.0f)
PostProcessor* postProcessor; // VFX Manager
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 计时器 (解决不同电脑速度不一样的问题)
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool isCursorVisible = false; // [NEW] Cursor state toggle

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
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE); // Optional improved aiming

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize PostProcessor
    postProcessor = new PostProcessor(SCR_WIDTH, SCR_HEIGHT);
    // [Modified] Enable Glitch by default, Disable Bloom by default
    postProcessor->UseGlitch = true;
    postProcessor->UseBloom = false;

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
    // 相对路径 "shaders/textured.vs"
    Shader ourShader("shaders/textured.vs", "shaders/textured.fs");

    // 3. 加载模型 (会打印 Assimp 日志)
    // 注意：路径必须指向 assets 里的 .gltf 文件
    Model ourModel("assets/cyberpunk_city_-_1.glb");

    // [New] 创建一个默认的白色纹理，防止模型没有纹理时采样到错误的显存（如上一帧的屏幕）导致闪烁
    // [Modified] Change white texture to Gray to detect if texture mapping is failing
    unsigned int whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whiteData[] = { 128, 128, 128 }; // Gray
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whiteData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

        // 1.  (Render to MSAA FBO)
        postProcessor->BeginRender();
        // glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Moved to BeginRender
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Moved to BeginRender

        // 2. 激活 Shader
        ourShader.use();
        
        // [NEW] 传递光照所需的 Uniforms
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("lightDirection", lightDirection);

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

        // [FIX] 修正姿态：由于 Assimp 读取 GLTF 时可能保留了 Z-Up，导致模型看起来是躺着的
        // 绕 X 轴旋转 -90 度，让 Z 轴变成 Y 轴
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // [FIX] 修正朝向：模型如果是反的(比如背对摄像机)，通常需要绕 Y 轴转 180 度
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // 缩放
        model = glm::scale(model, glm::vec3(0.1f));

        ourShader.setMat4("model", model);

        // 5. 绘制
        // [Fix] 在绘制前绑定白色的一像素纹理到 TU0
        // 如果模型有点材质(texture_diffuse)，Mesh::Draw 里的代码会覆盖绑定
        // 如果模型没有材质，Shader 就会采样这个白色纹理，而不会采样到上一帧的屏幕纹理（导致闪烁）
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);

        ourModel.Draw(ourShader);
        // --- 3D 场景渲染结束 ---

        // 2. Post Processing (Resolve MSAA -> Draw Quad -> Screen)
        postProcessor->EndRender(static_cast<float>(glfwGetTime()));

        // [Removed] UI removed as requested
        /*
        ImGui::Begin("VFX Control Panel");
        ImGui::Text("Press Left ALT to toggle Mouse/Camera");
        ImGui::End();
        */

        // 2. ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 3. 交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    delete postProcessor;
    
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle Cursor Visibility [Alt Key]
    static bool altKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        if (!altKeyPressed) {
            isCursorVisible = !isCursorVisible;
            if (isCursorVisible)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            
            // Only toggle once per press
            altKeyPressed = true;
        }
    } else {
        altKeyPressed = false;
    }

    // [Modified] Disable camera ROTATION when cursor is visible, but allow MOVEMENT (WASD)
    // if (isCursorVisible) return; // Removed global block

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
    // [Changed] Use Left Control instead of Shift to avoid IME conflict
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    // [Removed] Right Mouse Button Zoom Logic
    // User requested to use Scroll Wheel instead.
    // The previous logic was resetting the Zoom every frame, making scroll wheel ineffective.
    
    /*
    // 实现“义眼变焦” 
    // 长按鼠标右键，FOV 变小 (放大看)
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessZoom(2.0f); // 快速缩小 FOV
    }
    else {
        // 松开回弹 (简单的插值回弹逻辑，之后可以写更平滑的)
        if (camera.Zoom < 45.0f) camera.Zoom += 2.0f;
    }
    */

    // [New] Simple Ground Collision
    // Keep camera above a certain height (e.g. -3.0 which is roughly eye level relative to model at -5.0)
    // Adjust this value based on where the floor visually is.
    // [Modified] Improved collision logic - Prevent walking through walls (Simple AABB)
    
    // 1. 地面碰撞 (防止掉到地图下方)
    if (camera.Position.y < -3.0f) 
        camera.Position.y = -3.0f;

    // 2. 简单的建筑碰撞箱 (示例)
    // 假设地图中心有一个不可进入的区域 (比如大楼实体)
    // 你需要根据实际模型调整这个范围 min/max
    /*
    bool collisionX = camera.Position.x > -5.0f && camera.Position.x < 5.0f;
    bool collisionZ = camera.Position.z > -15.0f && camera.Position.z < -5.0f;
    
    if (collisionX && collisionZ) {
        // 如果进入了建筑内部，把位置弹回去 (简单回弹)
        // 这里只是一个简单的逻辑，如果要完美的防穿模，需要专门的物理引擎(如PhysX)检测 Mesh
        float pushStrength = 0.1f;
        if (camera.Position.x > 0) camera.Position.x += pushStrength;
        else camera.Position.x -= pushStrength;
    }
    */
}

// 鼠标移动回调：控制视角
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (isCursorVisible) return; // Don't move camera if cursor is visible
    
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
    if (postProcessor)
        postProcessor->UpdateSize(width, height);
}