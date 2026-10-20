# Cyber-Robot Runner: Chrome Dino Style Monster Game 🤖👾

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B11-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/Graphics-OpenGL%20%7C%20GLUT-5586A4?style=for-the-badge&logo=opengl&logoColor=white" alt="OpenGL" />
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-4EAA25?style=for-the-badge" alt="Platform" />
  <img src="https://img.shields.io/badge/License-MIT-F7B731?style=for-the-badge" alt="License: MIT" />
</p>

A fast-paced 2D Endless Runner Game built using **C++ and OpenGL with GLUT / Native Win32 Compatibility**.

Inspired by the iconic **Google Chrome Dino Game**, the player controls an agile humanoid robot sprinting across a futuristic cyber-terrain, jumping over approaching alien monsters, low-flying cyber drones, neon spikes, and hazard crates!

---

## 🌟 Key Highlights & Gameplay Features

- **Chrome Dino Style Runner Mechanics**:
  - Continuous running with procedural obstacle & monster spawning.
  - Responsive, gravity-based jump physics: press **`Space`**, **`W`**, or **`Up Arrow`** to launch into the air.
  - Multi-tier enemy threats:
    1. **Chomping Alien Monster**: Animated snapping jaws, sharp fangs, glowing menace eyes, and scuttling legs.
    2. **Cyber-Bat Drone**: Low-flying aerial hazard with flapping wings and jet thruster flames.
    3. **Neon Hazard Spikes**: Ground steel spikes with directional highlights.
    4. **Power Crate**: Reinforced container with flashing beacon.
  - **Accurate Collision Detection & Crash FX**:
    - Generous AABB (Axis-Aligned Bounding Box) collision detection.
    - Upon impact: screen shake, electric spark explosion particles, fallen robot pose with red 'X' 'X' eyes, and Game Over screen!
  - **Dynamic Speed Progression & Score System**:
    - Live score ticker and High Score tracker.
    - Running speed escalates as your score increases (just like Chrome Dino!).
  - **Game States**:
    - *Start Screen*: Welcome modal with instructions.
    - *Playing State*: Active running, jumping, and dodging.
    - *Game Over Screen*: Displays final score, best score, and retry prompt.
    - *Pause State*: Press `P` anytime to freeze the game.

- **Hierarchical Robot Modeling & Animation**:
  - Full scene graph hierarchy: Torso $\rightarrow$ Head/Visor/Antenna $\rightarrow$ Shoulders/Arms/Claws $\rightarrow$ Hips/Thighs/Knees/Feet.
  - High-speed running limb oscillation with natural forward sprint tilt and body bobbing.
  - Dynamic jumping pose (tucked knees, arms raised) with shrinking and fading contact shadow.
  - Jump dust and landing dust particle effects.

- **Parallax Scrolling Background**:
  - Distant mountain silhouettes scrolling at 15% speed.
  - Midground hills scrolling at 40% speed.
  - Tech runway with road markers scrolling at 100% world speed.
  - Twilight sky with gradient, twinkling stars, and cyber moon.

---

## 🎮 Keyboard Controls

| Key | Action | Description |
|:---:|:---|:---|
| **`Space`** | **Jump / Start / Retry** | Jumps over monsters, starts game from menu, or retries after game over |
| **`W`** or **`↑`** | **Alternative Jump** | W key or Up Arrow key to jump |
| **`P`** or **`p`** | **Pause / Resume** | Freezes the game and resumes play |
| **`R`** or **`r`** | **Instant Restart** | Restarts the game from score 0 |
| **`Esc`** | **Exit** | Cleanly exits the game |

---

## 📐 Computer Graphics Concepts Explained (Viva Prep)

### 1. 2D Coordinate System & Orthographic Projection
- **Function**: `gluOrtho2D(0.0, 1000.0, 0.0, 600.0)`
- **Concept**: Configures a 2D parallel orthographic projection. The bottom-left corner of the window is $(0, 0)$ and the top-right is $(1000, 600)$. Unlike perspective projection, objects maintain their size regardless of distance from the viewer, which is ideal for 2D technical drawings, schematics, and side-scrolling simulations.

### 2. 2D Geometric Primitives
- **`GL_QUADS`**: Used for ground slabs, sky gradient, rectangular body chassis, and hazard stripes.
- **`GL_TRIANGLES`**: Used for mountain peaks, rock facets, and spike obstacles.
- **`GL_TRIANGLE_FAN`**: Approximates solid discs and circles (reactor core, wheels, joints, antenna bulb, moon). A central vertex is specified, followed by vertices along the circle circumference:
  $$x = c_x + r \cdot \cos(\theta), \quad y = c_y + r \cdot \sin(\theta)$$
- **`GL_LINE_LOOP` & `GL_LINES`**: Used for wireframe outlines, pavement expansion joints, neon guide lines, and claws.
- **`GL_POINTS`**: Used for randomized background stars.

### 3. Hierarchical Modeling & Matrix Stack
- **Functions**: `glPushMatrix()`, `glPopMatrix()`, `glTranslatef()`, `glRotatef()`
- **Concept**: A complex humanoid figure is represented as a tree structure (Scene Graph):
  ```
  Robot Root (World Position: robotX, robotY)
   ├── Torso & Head
   │    ├── Visor & Face Details
   │    └── Antenna & Sensor Beacon
   ├── Left & Right Shoulders (Pivot Joints)
   │    └── Upper Arm
   │         └── Elbow Joint
   │              └── Forearm & Claws
   └── Left & Right Hips (Pivot Joints)
        └── Thigh
             └── Knee Joint
                  └── Shin & Foot
  ```
- By pushing and popping transformation matrices, rotating the hip automatically moves the thigh, knee, shin, and foot together around their respective anatomical joints without needing manual trigonometry for each vertex.

### 4. Sinusoidal Walking Kinematics & Counter-Phase
- **Concept**: Natural bipedal walking uses periodic harmonic oscillation:
  $$\theta_{\text{leg}} = A_{\text{max}} \cdot \sin(\text{walkPhase})$$
  $$\theta_{\text{opposite\_leg}} = -A_{\text{max}} \cdot \sin(\text{walkPhase})$$
- Arms swing in counter-phase to the respective leg ($\theta_{\text{arm}} = -\theta_{\text{leg}}$) to maintain balance and mimic biological locomotion.
- A vertical bobbing displacement ($\Delta y = |\sin(2 \cdot \text{walkPhase})| \cdot 3.5$) is added to the torso so the robot naturally bounces with each step.

### 5. Parabolic Jump Trajectory (Kinematics)
- **Formula**:
  $$y(t) = y_{\text{ground}} + 4 \cdot H_{\text{max}} \cdot t \cdot (1 - t) \quad \text{for } t \in [0, 1]$$
- At $t = 0.0$ (launch): $y = y_{\text{ground}}$.
- At $t = 0.5$ (peak): $y = y_{\text{ground}} + H_{\text{max}}$.
- At $t = 1.0$ (touchdown): $y = y_{\text{ground}}$.
- This mathematical quadratic curve models constant gravitational acceleration cleanly and deterministically.

### 6. Double Buffering
- **Function**: `glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB)` and `glutSwapBuffers()`
- **Concept**: In single-buffering, drawing directly to the active screen causes visible flickering as shapes are drawn and erased. Double buffering uses two memory buffers:
  - **Back Buffer**: The off-screen buffer where OpenGL draws the new frame.
  - **Front Buffer**: The buffer currently displayed on the monitor.
  - Once rendering completes, `glutSwapBuffers()` instantly exchanges the two buffers during the monitor's vertical blanking interval.

### 7. Animation Regulation via Timer Callbacks
- **Function**: `glutTimerFunc(16, update, 0)`
- **Concept**: Calling `glutTimerFunc` with 16 milliseconds schedules the update function to execute ~60 times per second ($1000\text{ ms} / 60 \approx 16.6\text{ ms}$). This decouples animation speed from CPU clock speed, preventing the robot from moving uncontrollably fast on high-performance machines.

### 8. Alpha Blending (Transparency & Shadows)
- **Function**: `glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);`
- **Concept**: Computes the final pixel color as a blend between incoming color (source) and existing framebuffer color (destination):
  $$C = C_{\text{src}} \cdot \alpha_{\text{src}} + C_{\text{dest}} \cdot (1 - \alpha_{\text{src}})$$
  Enables soft ground shadows, glassmorphic HUD cards, and atmospheric glow.

---

## 🛠️ Step-by-Step Setup & Compilation Guides

### Option 1: Visual Studio Code (VS Code with MinGW) — Zero Setup Required!

This project includes a self-contained native Windows OpenGL compatibility layer (`glut_compat.cpp`). **You do NOT need to download any external FreeGLUT or GLUT files!** Everything compiles using your existing Windows OpenGL and GDI libraries out of the box.

1. **Build and Run (VS Code Shortcuts)**:
   - Press **`Ctrl + Shift + B`** $\rightarrow$ compiles the project and generates `robot.exe`.
   - Press **`F5`** $\rightarrow$ launches and runs the walking robot!

2. **Or Run via VS Code Integrated Terminal**:
   - Open terminal (**`Ctrl + ~`**) and simply run:
     ```powershell
     .\run_mingw.bat
     ```
   - Or manually compile and run:
     ```powershell
     g++ -O2 -Wall -DFREEGLUT_STATIC main.cpp glut_compat.cpp -o robot.exe -Iinclude -lopengl32 -lglu32 -lgdi32 -static-libgcc -static-libstdc++
     .\robot.exe
     ```

---

### Option 2: Code::Blocks (Most Common in College Labs)

1. **Install FreeGLUT in MinGW / Code::Blocks**:
   - Download the FreeGLUT MinGW package (from TransmissionZero or your instructor).
   - Copy files from the package:
     - `include/GL/*.h` $\rightarrow$ `C:\Program Files\CodeBlocks\MinGW\include\GL\`
     - `lib/*.a` $\rightarrow$ `C:\Program Files\CodeBlocks\MinGW\lib\`
     - `bin/freeglut.dll` (or `glut32.dll`) $\rightarrow$ `C:\Windows\System32\` **and** into your project folder.
2. **Create New Project**:
   - Open Code::Blocks $\rightarrow$ **File** $\rightarrow$ **New** $\rightarrow$ **Project...** $\rightarrow$ **GLUT Project** (or **Console Application** in C++).
   - Replace the generated `main.cpp` with the `main.cpp` from this repository.
3. **Configure Linker Libraries**:
   - Go to **Project** $\rightarrow$ **Build options...**
   - Select the project name in the left panel.
   - Go to the **Linker settings** tab.
   - In **Other linker options**, paste:
     ```
     -lfreeglut -lopengl32 -lglu32
     ```
     *(If using original GLUT, use `-lglut32 -lopengl32 -lglu32`)*
4. **Build & Run**:
   - Press **F9** (Build and Run).

---

### Option 2: Dev-C++ (5.11 / Embarcadero Dev-C++)

1. Open Dev-C++.
2. Go to **File** $\rightarrow$ **New** $\rightarrow$ **Project...** $\rightarrow$ Select **Console Application** and **C++ Project**.
3. Replace the code in `main.cpp` with this project's `main.cpp`.
4. Go to **Project** $\rightarrow$ **Project Options** (or press `Alt + P`).
5. Select the **Parameters** tab.
6. Under **Linker**, paste:
   ```
   -lfreeglut
   -lopengl32
   -lglu32
   ```
   *(or `-lglut32 -lopengl32 -lglu32`)*
7. Place `freeglut.dll` (or `glut32.dll`) in the folder containing your `.exe`.
8. Press **F11** (Compile & Run).

---

### Option 3: Visual Studio (2019 / 2022)

1. Open Visual Studio $\rightarrow$ **Create a new project** $\rightarrow$ **Empty Project (C++)**.
2. Add `main.cpp` to **Source Files**.
3. Open **Project** $\rightarrow$ **Manage NuGet Packages...**
4. Search for `freeglut` (or `nupkg-freeglut`) and click **Install**.
5. Press **Ctrl + F5** to run! Visual Studio automatically links OpenGL and includes the required DLL.

---

### Option 4: Windows Command Line (MinGW g++)

If you have FreeGLUT installed in your MinGW include/lib directories:
```cmd
g++ main.cpp -o robot.exe -lfreeglut -lopengl32 -lglu32
robot.exe
```
*(Or simply double-click `run_mingw.bat`)*

---

### Option 5: Linux (Ubuntu / Debian / Linux Mint)

1. Install FreeGLUT and OpenGL development packages:
   ```bash
   sudo apt-get update
   sudo apt-get install build-essential freeglut3-dev libglu1-mesa-dev
   ```
2. Compile and run:
   ```bash
   g++ -O2 main.cpp -o robot -lglut -lGL -lGLU
   ./robot
   ```
   *(Or run `make && make run`)*

---

### Option 6: macOS

```bash
clang++ -std=c++11 -O2 main.cpp -o robot -framework OpenGL -framework GLUT
./robot
```

---

## 🎓 College Viva Voce Questions & Answers Cheat Sheet

**Q1: What is the purpose of `glPushMatrix()` and `glPopMatrix()`?**  
> **A:** They manage OpenGL's transformation matrix stack. `glPushMatrix()` saves the current transformation state, allowing local transformations (like rotating an arm or leg) without affecting other objects. `glPopMatrix()` restores the previous matrix state.

**Q2: Why do we use Double Buffering instead of Single Buffering?**  
> **A:** Single buffering renders directly to the active screen, causing noticeable visual flicker. Double buffering renders to an off-screen back buffer and swaps it with the front buffer (`glutSwapBuffers()`) during the vertical refresh interval, providing smooth animation.

**Q3: How is the walking animation generated mathematically?**  
> **A:** Limb angles are driven by a continuous phase variable `walkPhase`. Using `sin(walkPhase) * maxAngle`, limbs swing forward and backward smoothly. Arms use opposite signs to achieve natural counter-phase movement.

**Q4: How does the robot detect obstacles?**  
> **A:** The system checks horizontal proximity: if $(x_{\text{obstacle}} - x_{\text{robot}})$ is within a defined margin (between 20px and 75px) and the obstacle has not yet been cleared, an evasive jump sequence is triggered.

**Q5: What equation is used for the jumping arc?**  
> **A:** A normalized parabola: $y = 4 \cdot H \cdot t \cdot (1 - t)$, where $t \in [0, 1]$ represents normalized progress through the jump and $H$ is the peak jump height.

**Q6: What is the role of `gluOrtho2D`?**  
> **A:** It defines a 2D orthographic projection matrix mapping world coordinates directly to the viewport without perspective foreshortening.

**Q7: How is the frame rate regulated to ~60 FPS?**  
> **A:** Using `glutTimerFunc(16, update, 0)`. The function schedules the animation update every 16 milliseconds ($1000 / 60 \approx 16.6\text{ ms}$).

**Q8: What primitive is used to draw circles in standard OpenGL 1.x?**  
> **A:** `GL_TRIANGLE_FAN`. A central origin vertex is placed, and triangular slices fan around it using trigonometric points $(\cos\theta, \sin\theta)$.

---

## 📁 Project File Structure

```
CG project/
│
├── main.cpp         # Complete C++ OpenGL source code with detailed comments
├── Makefile         # Multi-platform build file (Windows, Linux, macOS)
├── CMakeLists.txt   # CMake configuration for modern IDEs / VS Code
├── run_mingw.bat    # One-click Windows MinGW compilation & launch script
└── README.md        # Documentation, CG concepts & viva guide
```

---

## 📄 License & Credits
Developed as an educational Computer Graphics mini project for undergraduate computer science and engineering coursework. Free to study, modify, and present.
