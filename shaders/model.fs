#version 330 core
out vec4 FragColor;

in vec3 Normal; // 我们只需要法线

void main()
{
    // 【赛博朋克调试色】
    // 将法线方向映射为颜色：
    // 朝上(Y+)是绿色，朝右(X+)是红色，朝前(Z+)是蓝色
    // 这样你能瞬间看清街道、楼房的轮廓！
    vec3 debugColor = normalize(Normal) * 0.5 + 0.5;
    
    // 加一点亮度，防止太暗
    FragColor = vec4(debugColor, 1.0);
}