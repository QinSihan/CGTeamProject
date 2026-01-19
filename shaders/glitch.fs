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
    
    // Continuous glitch effect (Permanent)
    // Varying strength but always active
    glitchStrength = 0.05 + 0.05 * sin(time * 3.0); 
    
    vec2 originalUV = uv; // Keep original for bloom etc
    
    if (glitchStrength != 0.0) {
        // XY jitter
        float rnd = rand(vec2(time, floor(uv.y * 10.0))); // Blocky effect
        
        // [NEW] Vignette mask: Glitch affects edges more than center
        float dist = distance(uv, vec2(0.5));
        float mask = smoothstep(0.1, 0.7, dist); // 0 at center, 1 at edges
        
        if (rnd < 0.3) {
             uv.x += rnd * 0.05 * sin(time) * glitchStrength * mask; // Apply mask
        }
    }
    
    // Chromatic aberration
    float redShift = 0.01 * glitchStrength;
    float greenShift = 0.005 * glitchStrength;
    
    // Use distorted UV for simple texture lookup? 
    // Usually chromatic aberration uses slightly different offsets.
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

    // --- Improved Cyberpunk Rain (Depth & Glitch) ---
    // Slant calculation
    vec2 rUV = TexCoords;
    rUV.x -= rUV.y * 0.1; // Slanted rain
    
    float rainTotal = 0.0;
    
    // Layer 1: Background (Dense, slow, faint) - "Mist Rain"
    {
        vec2 st = rUV * vec2(100.0, 1.0); // Very thin
        float t = time * 0.5; // Slow
        float col = floor(st.x);
        float y = st.y + t + rand(vec2(col, 1.0));
        if(rand(vec2(col, floor(y))) > 0.95) {
            float f = fract(y);
            rainTotal += pow(f, 5.0) * 0.1; 
        }
    }

    // Layer 2: Midground (Normal rain)
    {
        vec2 st = rUV * vec2(60.0, 1.0);
        float t = time * 1.5;
        float col = floor(st.x);
        float y = st.y + t + rand(vec2(col, 2.0));
        if(rand(vec2(col, floor(y))) > 0.90) { // More drops
            float f = fract(y);
            // Shape drop
            float xDist = abs(fract(st.x) - 0.5);
            if (xDist < 0.4) {
                 rainTotal += pow(f, 10.0) * 0.4;
            }
        }
    }
    
    // Layer 3: Foreground (Fast, bright, few) - "Camera Hits"
    {
        vec2 st = rUV * vec2(30.0, 1.0);
        float t = time * 3.0; // Fast
        float col = floor(st.x);
        float off = rand(vec2(col, 3.5));
        float y = st.y + t * (1.2 + off * 0.5);
        if(rand(vec2(col, floor(y))) > 0.97) {
            float f = fract(y);
            // Thick drops
            float xDist = abs(fract(st.x) - 0.5);
            float thickness = 1.0 - smoothstep(0.0, 0.5, xDist);
            rainTotal += pow(f, 20.0) * thickness * 0.8; 
        }
    }
    
    // Rain Color (Cyberpunk Cyan/White)
    vec3 rainColor = vec3(0.7, 0.9, 1.0);
    // Add additive blend
    result += rainColor * rainTotal;
    
    result = pow(result, vec3(1.0 / gamma));

    FragColor = vec4(result, 1.0);
}
