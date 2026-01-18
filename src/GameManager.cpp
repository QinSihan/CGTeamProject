#include "GameManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>

GameManager::GameManager() : timeLeft(0.0f), score(0), isGameOver(false), spawnTimer(0.0f), cubeVAO(0), cubeVBO(0) {
    std::srand(static_cast<unsigned int>(std::time(0)));
}

GameManager::~GameManager() {
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
}

void GameManager::Init() {
    InitRenderData();
    StartGame();
}

void GameManager::StartGame() {
    timeLeft = GAME_DURATION;
    score = 0;
    isGameOver = false;
    spawnTimer = 0.0f;
    targets.clear();
}

void GameManager::ResetGame() {
    StartGame();
}

void GameManager::Update(float deltaTime) {
    if (isGameOver) return;

    timeLeft -= deltaTime;
    if (timeLeft <= 0.0f) {
        timeLeft = 0.0f;
        isGameOver = true;
        return;
    }

    // Spawn Logic
    spawnTimer += deltaTime;
    if (spawnTimer >= SPAWN_INTERVAL) {
        spawnTimer = 0.0f;
        if (targets.size() < MAX_TARGETS) {
            SpawnTarget();
        }
    }

    // Remove old targets? Maybe not, keep them until found.
}

void GameManager::SpawnTarget() {
    // Defines a rough bounding box for the city scene
    // Expanded range to cover more of the city
    // Center is roughly (0, -5, -10)
    
    Target t;
    // X: -60 to 60
    float x = (std::rand() % 1200) / 10.0f - 60.0f;
    // Y: -4 to 20 (Lowered max height from 35)
    float y = (std::rand() % 240) / 10.0f - 4.0f; 
    // Z: -70 to 30
    float z = (std::rand() % 1000) / 10.0f - 70.0f;
    
    t.position = glm::vec3(x, y, z);
    t.isActive = true;
    t.activeTime = 0.0f;
    
    targets.push_back(t);
}

bool GameManager::CheckShot(Camera& camera, bool& outHit) {
    if (isGameOver) return false;

    outHit = false;
    glm::vec3 rayOrigin = camera.Position;
    glm::vec3 rayDir = camera.Front;

    for (auto it = targets.begin(); it != targets.end(); ) {
        if (!it->isActive) {
            ++it;
            continue;
        }

        // 1. Ray Intersection (Simple Sphere Text against Cube center)
        // Target visual size is roughly 0.5 unit radius
        if (RaySphereIntersect(rayOrigin, rayDir, it->position, 0.8f)) {
            
            // 2. Distance Check
            float dist = glm::distance(camera.Position, it->position);
            
            // 3. Zoom Requirement
            if (dist > FAR_DISTANCE_THRESHOLD) {
               if (camera.Zoom > REQUIRED_ZOOM) {
                   // Hit valid, but not zoomed enough
                   // Maybe give feedback? For now just fail the hit.
                   ++it;
                   continue;
               }
            }

            // Hit Confirmed!
            it = targets.erase(it);
            score++;
            outHit = true;
            return true; // Single valid shot only hits one per click
        } else {
            ++it;
        }
    }
    return false;
}

void GameManager::Render(Shader& shader) {
    if (targets.empty() && isGameOver) return; // 'targets' not 'target'

    glBindVertexArray(cubeVAO);
    
    // We can reuse the same shader, but we need to set color uniforms
    // If the shader is "textured.fs", it expects textures.
    // We can just bind the white texture and set a specific "tint" or light color.
    // Hacker way: Set lightDirection to be pure ambient or use a "isSolidColor" uniform if we added one.
    // Or just draw them as "lights" (bright emission).
    
    // Assuming the shader used is `ourShader` (Textured).
    // Let's rely on standard rendering but maybe toggle a "useTexture" uniform if it existed.
    // Since we don't have that, we rely on the white texture we bound globally before this call?
    // No, main loop binds textues.
    
    shader.setInt("objectType", 1); // [Added] Switch to Target Rendering Mode (Red Pulse)

    for (const auto& t : targets) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, t.position);
        model = glm::scale(model, glm::vec3(0.5f)); // Size
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    shader.setInt("objectType", 0); // [Added] Reset just in case
}

void GameManager::InitRenderData() {
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

// Simple Ray-Sphere Intersection
bool GameManager::RaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 sphereCenter, float sphereRadius) {
    glm::vec3 oc = rayOrigin - sphereCenter;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
}
