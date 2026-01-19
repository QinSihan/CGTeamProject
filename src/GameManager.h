#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Camera.h"
#include "Shader.h"

struct Target {
    glm::vec3 position;
    float activeTime;
    bool isActive;
};

class GameManager {
public:
    GameManager();
    ~GameManager();

    void Init();
    void Update(float deltaTime);
    void Render(Shader& shader);
    bool CheckShot(Camera& camera, bool& outHit); // Returns true if click was processed

    // Game Control
    void StartGame();
    void ResetGame();

    // Getters
    float GetTimeLeft() const { return timeLeft; }
    int GetScore() const { return score; }
    bool IsGameOver() const { return isGameOver; }
    
private:
    // Game State
    float timeLeft;
    int score;
    bool isGameOver;
    float spawnTimer;
    
    // Config
    const float GAME_DURATION = 90.0f; // [Modified] 1.5 minutes (90s)
    const float SPAWN_INTERVAL = 3.0f; // Spawn every 3 seconds
    const float MAX_TARGETS = 10;
    const float HIT_DISTANCE_THRESHOLD = 1.0f; // Precision required
    const float FAR_DISTANCE_THRESHOLD = 15.0f; // Distance to require zoom
    const float REQUIRED_ZOOM = 20.0f; // FOV must be less than this

    // Targets
    std::vector<Target> targets;
    unsigned int cubeVAO, cubeVBO;

    // Helpers
    void SpawnTarget();
    void InitRenderData();
    bool RaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 sphereCenter, float sphereRadius);
};

#endif
