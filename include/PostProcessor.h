#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "Framebuffer.h"

class PostProcessor {
public:
    Shader* ScreenShader;
    Shader* GlitchShader;
    Shader* BlurShader; // [NEW]
    Shader* BloomExtractShader; // [NEW]
    
    unsigned int Width, Height;

    bool UseGlitch;
    bool UseBloom = false; // [NEW]
    
    // Constructor requires paths for shaders, or assumes default locations
    PostProcessor(unsigned int width, unsigned int height);
    ~PostProcessor();

    // 1. Prepare for scene rendering (Bind MSAA FBO)
    void BeginRender(); 
    
    // 2. Resolve MSAA and Render Quad to Screen with effects
    void EndRender(float time);

    void UpdateSize(int width, int height);

private:
    unsigned int VAO, VBO;
    Framebuffer* MSAAFBO;
    Framebuffer* IntermediateFBO;
    Framebuffer* PingPongFBO[2]; // [NEW]

    void InitRenderData();
};
#endif
