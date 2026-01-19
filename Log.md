# 🕵️‍ Cyberpunk Recon - 开发协作日志

> **Prompt Context:**
> 请先阅读下方的 [当前架构] 和 [任务分配看板]。
> 完成 Assigned 栏目下未勾选的 Task，确保代码符合整体工程规范。

## 🏗️ 当前工程架构 (Architecture)
* **构建系统:** CMake + Ninja (VS2022)
* **依赖库:** GLFW, GLAD, GLM, ImGui, Assimp (全部通过 CMake FetchContent 自动管理)
* **目录结构:**
    * `/src`: `.cpp` 源文件 (main.cpp, glad.c)
    * `/include`: `.h` 头文件
    * `/shaders`: `.vs`, `.fs` 着色器
    * `/assets`: 模型与贴图
    * `/doc`: 报告与文档

---

## 🚀 任务分配
*请各位组员在提交代码前，将自己完成的任务标记为 `[x]`*

### 👤 覃司翰 - 核心架构 & 漫游
* **职责:** 摄像机系统、输入处理、整体框架整合
- [x] **[Init]** 工程搭建：CMake, GLFW, ImGui, Assimp 环境配置 
- [x] **[Camera]** 实现 `Camera` 类 (欧拉角, 视角移动)
- [x] **[Input]** 实现 WASD 移动与鼠标视角控制 (对接 Camera)
- [x] **[Feature]** 实现“义眼变焦”功能 (动态调整 FOV)

### 👤 覃司翰 & 袁舜骐 - 场景与渲染 (Renderer)
* **职责:** 模型加载、光照系统、天空盒
- [x] **[Model]** 封装 `Mesh` 和 `Model` 类 (基于 Assimp)
- [x] **[Scene]** 寻找并加载赛博朋克城市模型 (.obj/.gltf)
- [ ] **[Lighting]** 实现多光源 Blinn-Phong Shader (支持霓虹灯光效)
- [ ] **[Skybox]** 实现立方体贴图天空盒 (夜景)

### 👤 孙鹏翔 - 视觉特效 (VFX)
* **职责:** 帧缓冲、后处理 (Bloom, Glitch)
- [x] **[FBO]** 封装 `Framebuffer` 类 (支持多重采样/离屏渲染)
- [ ] **[Bloom]** 实现辉光特效 Shader (提取高亮 -> 高斯模糊 -> 叠加)
- [x] **[Glitch]** 实现故障艺术 Shader (UV 抖动, 色彩偏移)
- [x] **[Manage]** 编写 PostProcessor 管理类，统管特效开关

### 👤 覃司翰 & 岳峻宇 - 交互与逻辑 (Gameplay)
* **职责:** 游戏循环、UI 界面、判定逻辑
- [x] **[UI]** 使用 ImGui 绘制倒计时、分数板、调试面板
- [x] **[Logic]** 实现“视觉锁定”算法 (计算 ViewDir 与 Target 夹角)
- [x] **[GameLoop]** 实现游戏状态机 (Start -> Play -> Win/Lose)
- [x] **[Integrate]** 整合所有人的模块，微调参数

一些变动：
最后没有首先Bloom，但是做了一个Fog效果，离得越远颜色越接近雾色