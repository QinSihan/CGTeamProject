# Cyberpunk Recon

A C++ / OpenGL computer graphics project: explore a cyberpunk city, locate red anomaly signals, and scan as many as possible before the 90-second mission ends.

![Cyberpunk Recon title-screen artwork](assets/StartPage.jpg)

*Title-screen artwork included with the project.*

## What it demonstrates

- **Camera and interaction:** first-person movement, mouse look, vertical movement, and adjustable field of view for scanning distant targets.
- **Scene rendering:** an Assimp-loaded GLB city model with a baked texture and distance-based fog.
- **Post-processing:** off-screen rendering, multisample anti-aliasing, and a glitch effect.
- **Gameplay and UI:** timed target spawning, scan detection, a score display, mission briefing, and a restartable game loop built with Dear ImGui.

The project connects rendering, input, scene loading, and gameplay in a small application rather than relying on a game engine.

## Build and run

The development setup recorded in the project uses **Windows, Visual Studio 2022, CMake, and Ninja**.

Requirements:

- A C++17 compiler and CMake 3.15 or newer
- Ninja, available in the build environment
- A graphics driver that supports OpenGL 3.3
- Internet access for the first CMake configuration

CMake fetches GLFW 3.3.8, Dear ImGui 1.90.4, and Assimp 5.2.5. GLAD, GLM, and stb_image are included in the repository.

From a Visual Studio developer terminal with Ninja available:

```powershell
git clone https://github.com/QinSihan/CGTeamProject.git
cd CGTeamProject
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build
.\CGTeamProject.exe
```

Run the executable from its output directory. The application reads `assets/` and `shaders/` relative to the working directory; CMake copies both alongside the executable after building.

The commands above follow the checked-in build configuration. Other platforms have not been validated.

## Controls

| Input | Action |
| --- | --- |
| Enter | Advance through the title screen and briefing; restart after a mission |
| W / A / S / D | Move |
| Mouse | Look around |
| Space | Move upward |
| C or Left Ctrl | Move downward |
| Mouse wheel | Zoom in or out |
| Left mouse button | Scan the target under the crosshair |
| Left Alt | Toggle cursor capture |
| Esc | Quit |

Find the pulsing red targets in the city and scan them before time runs out. Distant targets require zooming to a field of view of **20° or less**.

## My contribution and team

**Sihan Qin (覃司翰)** built the project framework and dependency setup, implemented the camera and input systems, and integrated the modules. I also worked on model loading and scene rendering with 袁舜骐, and on UI and gameplay with 岳峻宇.

| Team member | Recorded responsibilities |
| --- | --- |
| 覃司翰 (Sihan Qin) | Core architecture, camera, input, module integration; shared scene and gameplay work |
| 袁舜骐 | Shared scene and rendering work |
| 孙鹏翔 | Framebuffers, glitch post-processing, and effect management |
| 岳峻宇 | Shared UI and gameplay work |

The original Chinese [development log](Log.md) preserves the task breakdown and collaboration history.

## Code map

| Path | Purpose |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Window setup, main loop, input, scene rendering, and UI |
| [src/Camera.h](src/Camera.h) | Camera movement, orientation, and zoom |
| [src/Model.h](src/Model.h) | Assimp model and mesh loading |
| [src/GameManager.cpp](src/GameManager.cpp) | Mission timer, target spawning, scoring, and scan checks |
| [src/PostProcessor.cpp](src/PostProcessor.cpp) | Framebuffer composition and post-processing |
| [shaders/](shaders/) | GLSL shaders |
| [assets/](assets/) | City model, texture, and title-screen artwork |

## Scope

This is a team coursework prototype focused on graphics and interaction. Collision and target-hit handling are simplified. Bloom-related code is present but disabled by default; the showcased visual effects are fog and glitch post-processing.
