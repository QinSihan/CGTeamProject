#include "PostProcessor.h"
#include <iostream>

PostProcessor::PostProcessor(unsigned int width, unsigned int height)
    : Width(width), Height(height), UseGlitch(false), ScreenShader(nullptr), GlitchShader(nullptr), BlurShader(nullptr), BloomExtractShader(nullptr), MSAAFBO(nullptr), IntermediateFBO(nullptr)
{
    // Initialize Shaders
    ScreenShader = new Shader("shaders/screen.vs", "shaders/screen.fs");
    GlitchShader = new Shader("shaders/screen.vs", "shaders/glitch.fs");
    BlurShader = new Shader("shaders/screen.vs", "shaders/blur.fs"); // [NEW]
    BloomExtractShader = new Shader("shaders/screen.vs", "shaders/extract_bright.fs"); // [NEW]
    
    // Initialize Framebuffers
    MSAAFBO = new Framebuffer(width, height, true); // Multisampled
    IntermediateFBO = new Framebuffer(width, height, false); // Normal texture
    
    // Initialize Ping Pong Framebuffers for Bloom
    PingPongFBO[0] = new Framebuffer(width, height, false);
    PingPongFBO[1] = new Framebuffer(width, height, false);
    
    InitRenderData();
}

PostProcessor::~PostProcessor() {
    delete ScreenShader;
    delete GlitchShader;
    delete BlurShader;
    delete BloomExtractShader;
    delete MSAAFBO;
    delete IntermediateFBO;
    delete PingPongFBO[0];
    delete PingPongFBO[1];
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void PostProcessor::InitRenderData() {
    // Quad covering the screen in NDC (-1 to 1)
    float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void PostProcessor::BeginRender() {
    MSAAFBO->Bind();
    glEnable(GL_DEPTH_TEST);
    // [Modified] Darker Night Background
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::EndRender(float time) {
    // 1. Blit MSAA -> Intermediate
    MSAAFBO->BlitTo(IntermediateFBO);
    
    // 2. Bloom Pipeline
    if (UseBloom) {
        // 2a. Extract Bright Colors
        PingPongFBO[0]->Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        BloomExtractShader->use();
        BloomExtractShader->setInt("scene", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, IntermediateFBO->TextureID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // 2b. Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        BlurShader->use();
        for (unsigned int i = 0; i < amount; i++)
        {
            PingPongFBO[horizontal]->Bind();
            BlurShader->setInt("horizontal", horizontal);
            
            glActiveTexture(GL_TEXTURE0);
            // Bind texture of other framebuffer (or extracted brights if first iteration)
            // Wait, logic:
            // Extract -> PingPong[0].
            // Pass 1 (Hor): Read PingPong[0], Write PingPong[1].
            // Pass 2 (Ver): Read PingPong[1], Write PingPong[0].
            
            // Correction: Extract writes to PingPong[0].
            // First blur iteration: Use PingPong[0] as input? No, extraction output is input.
            // Actually, simpler:
            // Extract -> PingPong[0] texture.
            // Loop:
            //   Bind PingPong[1], Read PingPong[0], Blur Horizontal.
            //   Bind PingPong[0], Read PingPong[1], Blur Vertical.
            
            glBindTexture(GL_TEXTURE_2D, first_iteration ? PingPongFBO[0]->TextureID : PingPongFBO[!horizontal]->TextureID); 
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
    }

    // 3. Render Quad to Screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default
    glDisable(GL_DEPTH_TEST); // We don't care about depth for the screen quad
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    Shader* shaderToUse = UseGlitch ? GlitchShader : ScreenShader;
    shaderToUse->use();
    shaderToUse->setInt("screenTexture", 0);
    shaderToUse->setInt("bloomBlur", 1);
    shaderToUse->setInt("bloom", UseBloom);
    shaderToUse->setFloat("exposure", 1.0f); // Simple exposure

    if (UseGlitch) {
        shaderToUse->setFloat("time", time);
    }

    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, IntermediateFBO->TextureID);
    
    glActiveTexture(GL_TEXTURE1);
    // The last write was to PingPong[!horizontal] (because we flipped horizontal at end of loop)
    // Actually, simplest is to just blindly use PingPong[0] or [1] depending on logic.
    // Loop ran 10 times. Even number.
    // Iter 0: Reads [0], writes [1]. hor=false.
    // Iter 1: Reads [1], writes [0]. hor=true.
    // ...
    // Iter 9: Reads [1], writes [0].
    // So result is in PingPong[0].
    if (UseBloom)
        glBindTexture(GL_TEXTURE_2D, PingPongFBO[0]->TextureID);
    else
        glBindTexture(GL_TEXTURE_2D, 0); // Bind nothing or black

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void PostProcessor::UpdateSize(int width, int height) {
    Width = width;
    Height = height;
    MSAAFBO->Rescale(width, height);
    IntermediateFBO->Rescale(width, height);
    PingPongFBO[0]->Rescale(width, height);
    PingPongFBO[1]->Rescale(width, height);
}
