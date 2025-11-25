# Final-Project

1. Open the simulator
The interface loads showing the Input Tab. You can choose between Scalar, Vector, or Forces modes.

2. Select one mode
Switching modes clears all inputs instantly because clear_userInput() is triggered.
Each mode shows different fields:
Scalar mode: regular kinematics values; speeds, angle, height, time, distance, max height.
Vector mode: i/j components instead of speed + angle, plus the same shared fields.
Forces mode: friction μ, force, time, and mass.

3. Start typing your known values
Each value has a min/max range defined in ParameterInfo.
When you type numbers, the program checks if they’re allowed. If something is out of range, you see an error right away.

4. Program checks dependencies based on mode
Only Vector + Forces modes have dependency rules:
Vector mode requires both i & j components (initial and final).
Forces mode requires force, time, and mass together when µ is used.
Missing dependencies give errors like:
“Missing required dependency for parameter…”
Scalar mode has no dependencies.

5. If you enter both initial & final values, they must match
The program enforces consistency rules:
initial_speed must equal final_speed
initial_i must equal final_i
initial_j must equal final_j
If they don’t match, the solver stops and displays the specific mismatch error.

6. Enter at least three required parameters
Your solver only activates if the mode has 3 or more required fields entered.
If not, the program shows:
“Not enough required inputs.”

7. Press PLAY
This runs cleanup_input(), which fully validates:
numeric range checks
dependency rules
consistency checks
required input threshold
If anything is invalid, the exact cause is shown in the Error Box.

8. If validation passes, kinematics are solved (Scalar & Vector only)
The program calls parabola_outline() which:
normalizes vectors (Vector Mode)
determines launch angle
checks which values are zero/non-zero
picks one of your 20 projectile-motion cases
solves the missing values (time, max height, range, apex time, etc.)
If the computed max height becomes negative, an inconsistency error appears.
For Forces mode; solver not implemented yet; so no calculations occur.

9. Results appear in the read-only output boxes
The GUI shows calculated values:
time of apex
max height
absolute max height
range
These are locked fields so the user cannot edit them.

10. Press CLEAR when you want to restart
All values reset to default and error messages disappear, allowing the user to run a completely new scenario.

**Setup:**
```
0. Install git and CMake (sudo apt install)
1. sudo apt update
2. sudo apt install \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libudev-dev \
    libfreetype-dev \
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libfreetype-dev
3. cd Final-Project
4. cmake -B build
5. cmake --build build
6. Finally, you can use ./build/bin/main to run the code
```
