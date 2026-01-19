#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;
uniform vec3 lightDirection;
uniform vec3 viewPos;
uniform int objectType; // 0 = Default (City), 1 = Target (Red Dot)
uniform float time; // For pulsing effect

void main()
{
    if (objectType == 1) {
        // Pulsing Red Target
        float pulse = (sin(time * 5.0) + 1.0) * 0.5; // 0.0 to 1.0
        vec3 redColor = vec3(1.0, 0.0, 0.0);
        vec3 emission = redColor * (0.8 + 0.4 * pulse); // Always bright
        FragColor = vec4(emission, 1.0);
        return;
    }

    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(-lightDirection); 
    
    // 1. diffuse 
    // Baked Texture Mode: Ignore dynamic lighting direction, rely on the texture content
    vec3 texColor = texture(material.texture_diffuse1, TexCoords).rgb;
    
    // [Debug] If texture is black/invisible, output a solid color or check UVs
    // Since it's a "Bake", we treat it as unlit (the light is in the texture)
    vec3 result = texColor; 

    // --- Fog Effect (Night Mists - Cyberpunk) ---
    float dist = length(viewPos - FragPos);
    
    // Smoothstep Fog for nicer transition
    float fogStart = 10.0;
    float fogEnd = 100.0; // Dense fog for atmosphere
    float fogFactor = smoothstep(fogStart, fogEnd, dist);
    
    // Cyberpunk Fog Color (Deep Blue/Purple Haze)
    vec3 fogColor = vec3(0.05, 0.05, 0.1); 
    
    // Mix
    result = mix(result, fogColor, fogFactor);

    // Add slight Dynamic Light influence if needed (optional)
    // vec3 lightDir = normalize(-lightDirection); 
    // float diff = max(dot(norm, lightDir), 0.0);
    // result *= (0.8 + 0.2 * diff);
    FragColor = vec4(result, 1.0);
}