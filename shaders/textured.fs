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
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 texColor = texture(material.texture_diffuse1, TexCoords).rgb;

    // --- Cyberpunk Tinting (More Aggressive) ---
    // Make dark areas purple/blue, bright areas cyan/pink
    float lum = dot(texColor, vec3(0.299, 0.587, 0.114));
    
    // Gradient: Deep Purple -> Hot Pink -> Cyan
    vec3 colLow = vec3(0.2, 0.0, 0.4); // Purple
    vec3 colHigh = vec3(0.0, 1.0, 1.0); // Cyan
    vec3 cyberColor = mix(colLow, colHigh, lum);
    
    // Mix original texture with cyber color (80% Cyber)
    vec3 finalAlbedo = mix(texColor, cyberColor, 0.8); 

    // Add rim lighting (Neon outline effect)
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = smoothstep(0.5, 1.0, rim);
    vec3 rimColor = vec3(1.0, 0.0, 0.8) * rim * 1.5; // Magenta rim
    
    vec3 diffuse = diff * finalAlbedo;
    
    // 2. specular (Neon highlights)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.0, 1.0, 0.9) * spec * 2.0; // Bright Cyan highlights
    
    // 3. ambient
    vec3 ambient = vec3(0.3, 0.1, 0.4) * finalAlbedo; // Stronger ambient
    
    vec3 result = ambient + diffuse + specular + rimColor;
    FragColor = vec4(result, 1.0);
}