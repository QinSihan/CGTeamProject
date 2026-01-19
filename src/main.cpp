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
#include "GameManager.h" // Include Game Logic

#include <filesystem> // 

// --- 函数声明 ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);  
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset); 
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 摄像机
Camera camera(glm::vec3(0.0f, -3.0f, 25.0f)); // Position back to 25.0f
PostProcessor* postProcessor; // VFX Manager
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 计时器 (解决不同电脑速度不一样的问题)
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool isCursorVisible = false; // Cursor state toggle

GameManager gameManager; // Game Manager Instance

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

    // Initialize Game
    gameManager.Init();

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
    Model ourModel("assets/CuberpunkCityWithKaws.glb");

    // [New] Manually load and assign City_Bake_4K.png texture
    // This ensures the model uses the provided texture even if the GLB doesn't reference it correctly.
    {
        string textName = "City_Bake_4K.png";
        string textPath = "assets/" + textName;
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false); // [Check] Bake textures usually don't need flip if UVs match standard GLTF
        unsigned char* data = stbi_load(textPath.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) {
            std::cout << "Loaded texture: " << textName << " (" << width << "x" << height << ")" << std::endl;
            unsigned int cityTextureID;
            glGenTextures(1, &cityTextureID);
            glBindTexture(GL_TEXTURE_2D, cityTextureID);
            
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            // Apply to all meshes
            Texture t;
            t.id = cityTextureID;
            t.type = "texture_diffuse";
            t.path = textName;

            for (auto& mesh : ourModel.meshes) {
                mesh.textures.clear(); // Remove existing/embedded textures
                mesh.textures.push_back(t);
            }
        } else {
            std::cout << "Failed to load texture: " << textPath << " (Reason: " << stbi_failure_reason() << ")" << std::endl;
            // Fallback: Create a MAGENTA texture to indicate error visibly
            unsigned int errorTexture;
            glGenTextures(1, &errorTexture);
            glBindTexture(GL_TEXTURE_2D, errorTexture);
            unsigned char magenta[] = { 255, 0, 255 }; 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, magenta);
             
            Texture t; t.id = errorTexture; t.type = "texture_diffuse"; t.path = "error";
            for (auto& mesh : ourModel.meshes) { mesh.textures.clear(); mesh.textures.push_back(t); }
        }
    }

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

    // [New] Load Start Page Texture for Menu
    unsigned int startPageTexture = 0;
    int startPageWidth = 0;
    int startPageHeight = 0;
    {
        string textPath = "assets/StartPage.jpg";
        int nrChannels;
        stbi_set_flip_vertically_on_load(false); 
        unsigned char* data = stbi_load(textPath.c_str(), &startPageWidth, &startPageHeight, &nrChannels, 0);
        if (data) {
             glGenTextures(1, &startPageTexture);
             glBindTexture(GL_TEXTURE_2D, startPageTexture);
             // [Fix] Set alignment to 1 to handle arbitrary dimensions (prevents distortion/skewing)
             glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
             
             GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
             glTexImage2D(GL_TEXTURE_2D, 0, format, startPageWidth, startPageHeight, 0, format, GL_UNSIGNED_BYTE, data);
             glGenerateMipmap(GL_TEXTURE_2D);
             // Restore alignment default
             glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
             stbi_image_free(data);
             std::cout << "Loaded StartPage texture." << std::endl;
        } else {
             std::cout << "Failed to load StartPage texture." << std::endl;
        }
    }

    // 设置光照方向 (从上往下照)
    glm::vec3 lightDirection(-0.2f, -1.0f, -0.3f);

    // [Modified] Game State Machine
    // 0: Start Screen
    // 1: Story/Background
    // 2: Instructions
    // 3: Game Running
    int gameState = 0; 
    bool enterPressed = false; // Debounce for Enter key

    while (!glfwWindowShouldClose(window)) {
        // [New] Menu & State Handling
        if (gameState != 3) {
            // Input Poll
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            // State Transition Logic
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                if (!enterPressed) {
                    enterPressed = true;
                    if (gameState == 0) gameState = 1;      // Start -> Story
                    else if (gameState == 1) gameState = 2; // Story -> Instr
                    else if (gameState == 2) {              // Instr -> Game
                        gameState = 3;
                        gameManager.ResetGame();
                        lastFrame = static_cast<float>(glfwGetTime()); 
                    }
                }
            } else {
                enterPressed = false;
            }

            // Render Start Screen
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // User Window for Background
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); 
            
            if (ImGui::Begin("MenuState", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground)) {
                
                if (gameState == 0) {
                    // --- STATE 0: Start Page ---
                    // Draw Full Screen Image with Crop (Aspect Fill)
                    if (startPageTexture != 0) {
                         float screenAspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
                         float imageAspect = (float)startPageWidth / (float)startPageHeight;
                         ImVec2 uv0(0, 0); ImVec2 uv1(1, 1);

                         if (screenAspect > imageAspect) {
                             // Screen is wider. Crop top/bottom.
                             float range = imageAspect / screenAspect;
                             uv0.y = 0.5f - range * 0.5f;
                             uv1.y = 0.5f + range * 0.5f;
                         } else {
                             // Screen is taller. Crop left/right.
                             float range = screenAspect / imageAspect;
                             uv0.x = 0.5f - range * 0.5f;
                             uv1.x = 0.5f + range * 0.5f;
                         }

                         ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)startPageTexture, ImVec2(0, 0), ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), uv0, uv1);
                    } else {
                         ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), IM_COL32(0,0,0,255));
                    }

                    // Press Enter Text
                    const char* startText = "PRESS ENTER";
                    ImGui::SetWindowFontScale(3.0f);
                    ImVec2 txtSz = ImGui::CalcTextSize(startText);
                    ImGui::SetCursorPos(ImVec2((SCR_WIDTH - txtSz.x)/2, SCR_HEIGHT - 100));
                    ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", startText);
                } 
                else if (gameState == 1) {
                    // --- STATE 1: Story Background ---
                    // Dark Background
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), IM_COL32(10, 15, 20, 255));
                    
                    // Text
                    ImGui::SetWindowFontScale(2.5f);
                    const char* title = "MISSION BRIEFING";
                    ImVec2 tSz = ImGui::CalcTextSize(title);
                    ImGui::SetCursorPos(ImVec2((SCR_WIDTH - tSz.x)/2, 80));
                    ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s", title);
                    
                    ImGui::SetWindowFontScale(1.5f);
                    ImGui::SetCursorPos(ImVec2(100, 200));
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "ID: INSPECTOR 7734");
                    ImGui::SetCursorPosX(100);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "SECTOR: NEO-KYOTO DISTRICT 9");
                    
                    ImGui::Dummy(ImVec2(0, 30));
                    ImGui::SetCursorPosX(100);
                    ImGui::TextWrapped("You are an inspector in the Cyberpunk City.");
                    ImGui::SetCursorPosX(100);
                    ImGui::TextWrapped("Headquarters has ordered an investigation into this district.");
                    ImGui::SetCursorPosX(100);
                    ImGui::TextWrapped("Numerous 'Anomalies' (Red Signals) have been reported.");
                    ImGui::Dummy(ImVec2(0, 20));
                    ImGui::SetCursorPosX(100);
                    ImGui::TextWrapped("OBJECTIVE: Find and scan as many Anomalies as possible within the TIME LIMIT.");
                    ImGui::SetCursorPosX(100);
                    ImGui::TextWrapped("The data you collect is vital to confirming the threat level.");
                    
                    // Next Prompt
                    ImGui::SetWindowFontScale(1.2f);
                    ImGui::SetCursorPos(ImVec2(SCR_WIDTH - 300, SCR_HEIGHT - 80));
                    if (sin(glfwGetTime()*5) > 0)
                        ImGui::TextColored(ImVec4(0, 1, 1, 1), "PRESS ENTER TO CONTINUE >>");
                }
                else if (gameState == 2) {
                    // --- STATE 2: Instructions ---
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), IM_COL32(10, 15, 20, 255));
                    
                    ImGui::SetWindowFontScale(2.5f);
                    const char* title = "OPERATIONAL GUIDE";
                    ImVec2 tSz = ImGui::CalcTextSize(title);
                    ImGui::SetCursorPos(ImVec2((SCR_WIDTH - tSz.x)/2, 80));
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1), "%s", title);
                    
                    float centerX = SCR_WIDTH / 2 - 200;
                    ImGui::SetWindowFontScale(1.8f);

                    ImGui::SetCursorPos(ImVec2(centerX, 200));
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "[ MOVEMENT ]");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("W A S D - Move");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("Space / C - Fly Up / Down");
                    
                    ImGui::Dummy(ImVec2(0, 20));
                    ImGui::SetCursorPosX(centerX);
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "[ OPTICS ]");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("Mouse - Look");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("Scroll Wheel - ZOOM IN / OUT");
                    
                    ImGui::Dummy(ImVec2(0, 20));
                    ImGui::SetCursorPosX(centerX);
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ SCANNING ]");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("1. Locate RED DOT on buildings");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("2. ZOOM IN until FOV > 20");
                    ImGui::SetCursorPosX(centerX + 20); ImGui::Text("3. LEFT CLICK to Scan");
                    
                     // Start Prompt
                    ImGui::SetWindowFontScale(1.5f);
                    ImGui::SetCursorPos(ImVec2((SCR_WIDTH - 400)/2, SCR_HEIGHT - 100));
                    if (sin(glfwGetTime()*8) > 0)
                        ImGui::TextColored(ImVec4(0, 1, 1, 1), "PRESS ENTER TO START MISSION");
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(2);

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClear(GL_COLOR_BUFFER_BIT); // Clear previous
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window);
            continue; // Skip the rest of the loop
        }

        // 计算每一帧的时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // [Added] Update Game Logic
        gameManager.Update(deltaTime);

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
        ourShader.setFloat("time", static_cast<float>(glfwGetTime())); // [Added] Pass time
        ourShader.setInt("objectType", 0); // [Added] Default to City Rendering

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
        model = glm::scale(model, glm::vec3(10.0f));

        ourShader.setMat4("model", model);

        // 5. 绘制
        // [Fix] 在绘制前绑定白色的一像素纹理到 TU0
        // 如果模型有点材质(texture_diffuse)，Mesh::Draw 里的代码会覆盖绑定
        // 如果模型没有材质，Shader 就会采样这个白色纹理，而不会采样到上一帧的屏幕纹理（导致闪烁）
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);

        ourModel.Draw(ourShader);

        // [Added] Render Targets (Red Dots)
        // Reset Model Matrix for Targets
        gameManager.Render(ourShader);

        // --- 3D 场景渲染结束 ---

        // 2. Post Processing (Resolve MSAA -> Draw Quad -> Screen)
        postProcessor->EndRender(static_cast<float>(glfwGetTime()));

        // [Modified] UI replaced with Game UI
        // Use a full screen window for HUD to control positioning better
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        
        if (gameManager.IsGameOver()) {
            // Darken background
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0,0), ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), IM_COL32(0,0,0,200));
            
            // Center the text
            const char* title = "MISSION COMPLETE";
            ImGui::SetWindowFontScale(3.0f);
            float titleW = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPos(ImVec2((SCR_WIDTH - titleW)/2, SCR_HEIGHT/3));
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", title); // Cyan Title
            
            ImGui::SetWindowFontScale(2.0f);
            char scoreText[64];
            sprintf(scoreText, "FINAL SCORE: %d", gameManager.GetScore());
            float scoreW = ImGui::CalcTextSize(scoreText).x;
            ImGui::SetCursorPos(ImVec2((SCR_WIDTH - scoreW)/2, SCR_HEIGHT/3 + 60));
            ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", scoreText);

            // Instructions
            const char* sub = "Press Enter to Restart";
            ImGui::SetWindowFontScale(1.5f);
            float subW = ImGui::CalcTextSize(sub).x;
            ImGui::SetCursorPos(ImVec2((SCR_WIDTH - subW)/2, SCR_HEIGHT/3 + 120));
            if (sin(glfwGetTime() * 5.0f) > 0.0f) // Blink
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", sub);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                gameManager.ResetGame();
            }
        } else {
            // HUD Top Bar
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0,0), ImVec2(SCR_WIDTH, 60), IM_COL32(0,0,0,150));
            ImGui::GetWindowDrawList()->AddLine(ImVec2(0,60), ImVec2(SCR_WIDTH, 60), IM_COL32(0, 255, 255, 255), 2.0f);

            // Timer
            float t = gameManager.GetTimeLeft();
            int minutes = (int)(t / 60);
            int seconds = (int)(t) % 60;
            char timeStr[32];
            sprintf(timeStr, "%02d:%02d", minutes, seconds);

            ImGui::SetWindowFontScale(2.0f);
            ImGui::SetCursorPos(ImVec2(20, 10));
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "TIME: %s", timeStr);

            // Score
            char scoreStr[32];
            sprintf(scoreStr, "TARGETS: %d", gameManager.GetScore());
            float scoreW = ImGui::CalcTextSize(scoreStr).x;
            ImGui::SetCursorPosX(SCR_WIDTH - scoreW - 20);
            ImGui::SetCursorPosY(10);
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", scoreStr);
            
            // Instructions (Bottom Left)
            ImGui::SetWindowFontScale(1.0f);
            ImGui::SetCursorPos(ImVec2(20, SCR_HEIGHT - 120));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "INSTRUCTIONS:");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Find RED DOTS");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Zoom Scroll < 20 FOV");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "- Left Click to Scan");
        }
        
        // Draw Crosshair
        if (!gameManager.IsGameOver()) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCircle(ImVec2(SCR_WIDTH/2, SCR_HEIGHT/2), 5.0f, IM_COL32(255, 0, 0, 200));
            draw_list->AddCircle(ImVec2(SCR_WIDTH/2, SCR_HEIGHT/2), 20.0f, IM_COL32(255, 0, 0, 100), 0, 1.0f);
            draw_list->AddLine(ImVec2(SCR_WIDTH/2 - 10, SCR_HEIGHT/2), ImVec2(SCR_WIDTH/2 + 10, SCR_HEIGHT/2), IM_COL32(255, 0, 0, 150));
            draw_list->AddLine(ImVec2(SCR_WIDTH/2, SCR_HEIGHT/2 - 10), ImVec2(SCR_WIDTH/2, SCR_HEIGHT/2 + 10), IM_COL32(255, 0, 0, 150));
        }

        ImGui::End();

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

    // [Added] Handle Shooting (Mouse Left Click)
    static bool leftMousePressed = false;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!leftMousePressed) {
            bool validHit = false;
            if (gameManager.CheckShot(camera, validHit)) {
                 std::cout << "Target Neutralized!" << std::endl;
            } else {
                 // std::cout << "Missed or Zoom insufficient." << std::endl;
            }
            leftMousePressed = true;
        }
    } else {
        leftMousePressed = false;
    }

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