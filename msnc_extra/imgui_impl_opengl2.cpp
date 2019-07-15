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

// CHANGELOG 
// (minor and older changes stripped away, please see git history for details)
//  2018-08-03: OpenGL: Disabling/restoring GL_LIGHTING and GL_COLOR_MATERIAL to increase compatibility with legacy OpenGL applications.
//  2018-06-08: Misc: Extracted imgui_impl_opengl2.cpp/.h away from the old combined GLFW/SDL+OpenGL2 examples.
//  2018-06-08: OpenGL: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplGlfwGL2_RenderDrawData() in the .h file so you can call it yourself.
//  2017-09-01: OpenGL: Save and restore current polygon mode.
//  2016-09-10: OpenGL: Uploading font texture as RGBA32 to increase compatibility with users shaders (not ideal).
//  2016-09-05: OpenGL: Fixed save and restore of current scissor rectangle.

#include "imgui_impl_opengl2.h"

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif
#if defined(_WIN32) && !defined(WINGDIAPI)
#define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Disable few warnings regarding ptr <-> uint casting (start)
#if _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4311)
#pragma warning(disable: 4302)
#pragma warning(disable: 4312)
#endif

// OpenGL Data
static GLsizei g_ViewWidth = 1280;
static GLsizei g_ViewHeight = 720;
static GLsizei g_LastViewWidth = 1280;
static GLsizei g_LastViewHeight = 720;
static GLuint  g_LastFrameTexture = 0;
static GLuint  g_FontTexture = 0;

#define G_LAST_FRAME_SIZE_X        8192
#define G_LAST_FRAME_SIZE_Y        8192
#define G_LAST_FRAME_COLOR_BYTES      3
#define G_LAST_FRAME_BYTES_COUNT   (G_LAST_FRAME_SIZE_X * G_LAST_FRAME_SIZE_Y * G_LAST_FRAME_COLOR_BYTES)

// Functions
void ImGui_ImplOpenGL2_Init(const ImVec2& displaySize) {
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.45f, 0.55f, 0.60f, 1.00f);

	// Setup buffer for last frame snapshot (shown during eg. load to prevent drawing from data modified in other thread)
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &g_LastFrameTexture);
	glBindTexture(GL_TEXTURE_2D, g_LastFrameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, G_LAST_FRAME_SIZE_X, G_LAST_FRAME_SIZE_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	// Setup model view matrix as identity (not changed anywhere, no need to do this every frame)
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Init font texture
	ImGui_ImplOpenGL2_CreateFontsTexture();

	// Set display size
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = displaySize;
}

void ImGui_ImplOpenGL2_Shutdown() {
	// Restore modified state
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Deinit last frame texture
	glDeleteTextures(1, &g_LastFrameTexture);
	g_LastFrameTexture = 0;

	// Deinit font textures
	ImGui_ImplOpenGL2_DestroyFontsTexture();
}

void ImGui_ImplOpenGL2_NewFrame() {
	// Clear screen
	glClear(GL_COLOR_BUFFER_BIT);

	// Setup viewport, orthographic projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, g_ViewWidth, g_ViewHeight, 0.0f, -1.0f, 1.0f);
}

void ImGui_ImplOpenGL2_EndFrame() {
	// Restore modified state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void ImGui_ImplOpenGL2_SetDisplaySize(const ImVec2& size) {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = size;

	g_ViewWidth = (GLsizei)size.x;
	g_ViewHeight = (GLsizei)size.y;
	glViewport(0, 0, g_ViewWidth, g_ViewHeight);
}

// OpenGL2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
void ImGui_ImplOpenGL2_RenderDrawData(ImDrawData* draw_data) {
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	if (!g_ViewWidth || !g_ViewHeight) {
		return;
	}

	// Render command lists
	ImVec2 pos = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; ++n) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
		                (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
		                  (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
		               (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback) {
				// User callback (registered via ImDrawList::AddCallback)
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				ImVec4 clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y, pcmd->ClipRect.z - pos.x,
				                          pcmd->ClipRect.w - pos.y);
				if ((clip_rect.x < g_ViewWidth) && (clip_rect.y < g_ViewHeight) && (clip_rect.z >= 0.0f) && (clip_rect.w >= 0.0f
				)) {
					// Apply scissor/clipping rectangle
					glScissor((GLint)clip_rect.x, (GLint)(g_ViewHeight - clip_rect.w), (GLint)(clip_rect.z - clip_rect.x),
					          (GLint)(clip_rect.w - clip_rect.y));

					// Bind texture, Draw
					glBindTexture(GL_TEXTURE_2D, (GLuint)pcmd->TextureId);
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
					               (sizeof(ImDrawIdx) == 2) ? (GL_UNSIGNED_SHORT) : (GL_UNSIGNED_INT), idx_buffer);
				}
			}
			idx_buffer += pcmd->ElemCount;
		}
	}
}

void ImGui_ImplOpenGL2_RenderSnapshot() {
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	if (!g_ViewWidth || !g_ViewHeight) {
		return;
	}
	// Apply scissor/clipping rectangle
	glScissor(0, 0, (GLint)g_ViewWidth, (GLint)g_ViewHeight);

	// Render quad with latest snapshot
	glBindTexture(GL_TEXTURE_2D, g_LastFrameTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, (float)g_LastViewHeight / G_LAST_FRAME_SIZE_Y);
	glVertex2f(0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, (float)g_ViewHeight);
	glTexCoord2f((float)g_LastViewWidth / G_LAST_FRAME_SIZE_X, 0.0f);
	glVertex2f((float)g_ViewWidth, (float)g_ViewHeight);
	glTexCoord2f((float)g_LastViewWidth / G_LAST_FRAME_SIZE_X, (float)g_LastViewHeight / G_LAST_FRAME_SIZE_Y);
	glVertex2f((float)g_ViewWidth, 0.0f);
	glEnd();
}

void ImGui_ImplOpenGL2_TakeSnapshot() {
	// Copy framebuffer to last frame texture
	glBindTexture(GL_TEXTURE_2D, g_LastFrameTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_ViewWidth, g_ViewHeight);

	// Copy last window size (so snapshot can be stretchet if needed)
	g_LastViewWidth = g_ViewWidth;
	g_LastViewHeight = g_ViewHeight;
}

void ImGui_ImplOpenGL2_CreateFontsTexture() {
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
	// Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (ImTextureID)(intptr_t)g_FontTexture;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
}

void ImGui_ImplOpenGL2_DestroyFontsTexture() {
	if (g_FontTexture) {
		ImGuiIO& io = ImGui::GetIO();
		glDeleteTextures(1, &g_FontTexture);
		io.Fonts->TexID = nullptr;
		g_FontTexture = 0;
	}
}

// Disable few warnings regarding ptr <-> uint casting (end)
#if _MSC_VER
#pragma warning(pop) 
#endif