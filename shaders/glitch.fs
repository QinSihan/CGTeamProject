#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur; // [NEW]
uniform bool bloom;          // [NEW]
uniform float exposure;      // [NEW]
uniform float time;

// Simple pseudo-random function
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = TexCoords;
    
    // Time based glitch factor
    float glitchStrength = 0.0;
    
    // Periodically trigger glitch
    float t = time * 2.0; 
    // Trigger every few seconds
    if (mod(t, 5.0) > 4.5 || mod(t, 20.0) > 19.5) {
        glitchStrength = 0.5 * sin(time * 10.0);
    }
    
    if (glitchStrength != 0.0) {
        // XY jitter
        float rnd = rand(vec2(time, floor(uv.y * 10.0))); // Blocky effect
        if (rnd < 0.3) {
             uv.x += rnd * 0.05 * sin(time) * glitchStrength;
        }
    }
    
    // Chromatic aberration
    float redShift = 0.01 * glitchStrength;
    float greenShift = 0.005 * glitchStrength;
    
    float r = texture(screenTexture, uv + vec2(redShift, 0)).r;
    float g = texture(screenTexture, uv + vec2(-greenShift, 0)).g;
    float b = texture(screenTexture, uv).b;
    
    vec3 col = vec3(r, g, b);
    
    if(bloom) {
         // Apply bloom to the glitched UVs as well? 
         // Yes, for consistency.
         vec3 bloomColor = texture(bloomBlur, uv).rgb;
         col += bloomColor;
    }
    
    // Tone mapping & Gamma (Match screen.fs)
    const float gamma = 2.2;
    vec3 result = vec3(1.0) - exp(-col * exposure);
    result = pow(result, vec3(1.0 / gamma));

    FragColor = vec4(result, 1.0);
}
