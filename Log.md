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
- [ ] **[Camera]** 实现 `Camera` 类 (欧拉角, 视角移动)
- [ ] **[Input]** 实现 WASD 移动与鼠标视角控制 (对接 Camera)
- [ ] **[Feature]** 实现“义眼变焦”功能 (动态调整 FOV)

### 👤 成员 B - 场景与渲染 (Renderer)
* **职责:** 模型加载、光照系统、天空盒
- [ ] **[Model]** 封装 `Mesh` 和 `Model` 类 (基于 Assimp)
- [ ] **[Scene]** 寻找并加载赛博朋克城市模型 (.obj/.gltf)
- [ ] **[Lighting]** 实现多光源 Blinn-Phong Shader (支持霓虹灯光效)
- [ ] **[Skybox]** 实现立方体贴图天空盒 (夜景)

### 👤 成员 C - 视觉特效 (VFX)
* **职责:** 帧缓冲、后处理 (Bloom, Glitch)
- [ ] **[FBO]** 封装 `Framebuffer` 类 (支持多重采样/离屏渲染)
- [ ] **[Bloom]** 实现辉光特效 Shader (提取高亮 -> 高斯模糊 -> 叠加)
- [ ] **[Glitch]** 实现故障艺术 Shader (UV 抖动, 色彩偏移)
- [ ] **[Manage]** 编写 PostProcessor 管理类，统管特效开关

### 👤 成员 D - 交互与逻辑 (Gameplay)
* **职责:** 游戏循环、UI 界面、判定逻辑
- [ ] **[UI]** 使用 ImGui 绘制倒计时、分数板、调试面板
- [ ] **[Logic]** 实现“视觉锁定”算法 (计算 ViewDir 与 Target 夹角)
- [ ] **[GameLoop]** 实现游戏状态机 (Start -> Play -> Win/Lose)
- [ ] **[Integrate]** 整合所有人的模块，微调参数

---

## 📝 更新日志 (Changelog)
*提交代码时请在下方追加一行记录：`[日期] [成员] 内容`*

* **[覃司翰]**: 初始化项目。配置 CMakeLists.txt，自动集成 Assimp/GLFW/ImGui。解决 Ninja 编译锁死问题。上传基础骨架。