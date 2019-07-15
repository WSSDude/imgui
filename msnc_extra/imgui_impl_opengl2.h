// ImGui Renderer for: OpenGL2 (legacy OpenGL, fixed pipeline)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in imgui_impl_opengl3.cpp**
// This code is mostly provided as a reference to learn how ImGui integration works, because it is shorter to read.
// If your code is using GL3+ context or any semi modern OpenGL calls, using this is likely to make everything more
// complicated, will require your code to reset every single OpenGL attributes to their initial state, and might
// confuse your GPU driver. 
// The GL2 code is unable to reset attributes or even call e.g. "glUseProgram(0)" because they don't exist in that API.

#pragma once

#include "imgui.h"

IMGUI_IMPL_API void ImGui_ImplOpenGL2_Init(const ImVec2& displaySize);
IMGUI_IMPL_API void ImGui_ImplOpenGL2_Shutdown();
IMGUI_IMPL_API void ImGui_ImplOpenGL2_NewFrame();
IMGUI_IMPL_API void ImGui_ImplOpenGL2_EndFrame();
IMGUI_IMPL_API void ImGui_ImplOpenGL2_SetDisplaySize(const ImVec2& size);
IMGUI_IMPL_API void ImGui_ImplOpenGL2_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API void ImGui_ImplOpenGL2_RenderSnapshot();
IMGUI_IMPL_API void ImGui_ImplOpenGL2_TakeSnapshot();

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API void ImGui_ImplOpenGL2_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplOpenGL2_DestroyFontsTexture();
