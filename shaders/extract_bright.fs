#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;

void main()
{
    vec3 color = texture(scene, TexCoords).rgb;
    // Check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.8) // Threshold
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
