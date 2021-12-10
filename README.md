# Conway's Game of Life in 3D

This was built in C++ using SFML and OpenGL.

## Rules

The rules can be modified on line 21 on /src/Main.cpp. The numbers are inclusive.
```cpp
int rules[2][2] = {
	{ *Lower bound*, *Upper bound* }, // Alive between
	{ *Lower bound*, *Upper bound* } // Dead between
};
```

Default rules are `4` `7` `10` `4`

## Compile

To complile it for Windows or Linux, open the directory in VS Code. Press `Ctrl + Shift + B` and select `Build Production`. It will then be compiled in /build
