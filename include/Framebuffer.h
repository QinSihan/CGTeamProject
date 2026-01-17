#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>
#include <iostream>

class Framebuffer {
public:
    unsigned int ID;
    unsigned int TextureID; // Color attachment
    unsigned int RBO;       // Depth/Stencil attachment

    int Width, Height;
    bool IsMultisampled;

    Framebuffer(int width, int height, bool multisampled = false);
    ~Framebuffer();

    void Bind();
    void Unbind();
    
    // Blit operations for MSAA resolution
    void BlitTo(Framebuffer* other); 
    void BlitToScreen(); // Blit to default framebuffer 0

    void Rescale(int width, int height);

private:
    void Init();
};

#endif
