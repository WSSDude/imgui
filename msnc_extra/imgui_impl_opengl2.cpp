// dear imgui: Renderer for OpenGL2 (legacy OpenGL, fixed pipeline)
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
//  2019-04-30: OpenGL: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2019-02-11: OpenGL: Projecting clipping rectangles correctly using draw_data->FramebufferScale to allow multi-viewports for retina display.
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-08-03: OpenGL: Disabling/restoring GL_LIGHTING and GL_COLOR_MATERIAL to increase compatibility with legacy OpenGL applications.
//  2018-06-08: Misc: Extracted imgui_impl_opengl2.cpp/.h away from the old combined GLFW/SDL+OpenGL2 examples.
//  2018-06-08: OpenGL: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplGlfwGL2_RenderDrawData() in the .h file so you can call it yourself.
//  2017-09-01: OpenGL: Save and restore current polygon mode.
//  2016-09-10: OpenGL: Uploading font texture as RGBA32 to increase compatibility with users shaders (not ideal).
//  2016-09-05: OpenGL: Fixed save and restore of current scissor rectangle.

#include "AppDefs.h"

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif
#if defined(_WIN32) && !defined(WINGDIAPI)
#define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// OpenGL Data
static GLsizei      g_ViewWidth       = APP_RES_X;
static GLsizei      g_ViewHeight      = APP_RES_Y;
static GLsizei      g_LastViewWidth   = APP_RES_X;
static GLsizei      g_LastViewHeight  = APP_RES_Y;
static GLuint       g_FontTexture     = 0;
static GLuint       g_SnapshotTexture = 0;

#define GL_SNAPSHOT_SIZE_X        4096
#define GL_SNAPSHOT_SIZE_Y        4096

// Functions
bool    ImGui_ImplOpenGL2_Init()
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "OpenGL 2.0";

    ImGui_ImplOpenGL2_CreateFontsTexture();

    // Setup buffer for last frame snapshot (shown during eg. load to prevent drawing from data modified in other thread)
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &g_SnapshotTexture);
    glBindTexture(GL_TEXTURE_2D, g_SnapshotTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GL_SNAPSHOT_SIZE_X, GL_SNAPSHOT_SIZE_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    return true;
}

void    ImGui_ImplOpenGL2_Shutdown()
{
    // Deinit last frame texture
    glDeleteTextures(1, &g_SnapshotTexture);
    g_SnapshotTexture = 0;

    ImGui_ImplOpenGL2_DestroyFontsTexture();
}

void    ImGui_ImplOpenGL2_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplOpenGL2_CreateFontsTexture();
}

static void ImGui_ImplOpenGL2_SetupRenderState(GLint d_x, GLint d_y, GLsizei d_width, GLsizei d_height, GLsizei fb_width, GLsizei fb_height)
{
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glViewport(0, 0, fb_width, fb_height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(d_x, d_x + d_width, d_y + d_height, d_y, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

// OpenGL2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
void ImGui_ImplOpenGL2_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    g_ViewWidth = (GLsizei)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    g_ViewHeight = (GLsizei)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (!g_ViewWidth || !g_ViewHeight)
        return;

    // Backup GL state
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);

    // Setup desired GL state
    GLint x = (GLint)draw_data->DisplayPos.x;
    GLint y = (GLint)draw_data->DisplayPos.y;
    GLsizei width = (GLsizei)draw_data->DisplaySize.x;
    GLsizei height = (GLsizei)draw_data->DisplaySize.y;
    ImGui_ImplOpenGL2_SetupRenderState(x, y, width, height, g_ViewWidth, g_ViewHeight);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL2_SetupRenderState(x, y, width, height, g_ViewWidth, g_ViewHeight);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < g_ViewWidth && clip_rect.y < g_ViewHeight && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Apply scissor/clipping rectangle
                    glScissor((GLint)clip_rect.x, (GLint)(g_ViewHeight - clip_rect.w), (GLsizei)(clip_rect.z - clip_rect.x), (GLsizei)(clip_rect.w - clip_rect.y));

                    // Bind texture, Draw
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
                }
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
    
    // Restore modified GL state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]); glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGui_ImplOpenGL2_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

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

    return true;
}

void ImGui_ImplOpenGL2_DestroyFontsTexture()
{
    if (g_FontTexture)
    {
        ImGuiIO& io = ImGui::GetIO();
        glDeleteTextures(1, &g_FontTexture);
        io.Fonts->TexID = (ImTextureID)(intptr_t)0;
        g_FontTexture = 0;
    }
}

void ImGui_ImplOpenGL2_RenderSnapshot() 
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    if (!g_ViewWidth || !g_ViewHeight)
        return;

    // Setup desired GL state
    ImGui_ImplOpenGL2_SetupRenderState(0, 0, g_ViewWidth, g_ViewHeight, g_ViewWidth, g_ViewHeight);

    // Apply scissor/clipping rectangle
    glScissor(0, 0, (GLsizei)g_ViewWidth, (GLsizei)g_ViewHeight);
    
    // Render quad with latest snapshot
    glBindTexture(GL_TEXTURE_2D, g_SnapshotTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, (float)g_LastViewHeight / GL_SNAPSHOT_SIZE_Y);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, (float)g_ViewHeight);
    glTexCoord2f((float)g_LastViewWidth / GL_SNAPSHOT_SIZE_X, 0.0f);
    glVertex2f((float)g_ViewWidth, (float)g_ViewHeight);
    glTexCoord2f((float)g_LastViewWidth / GL_SNAPSHOT_SIZE_X, (float)g_LastViewHeight / GL_SNAPSHOT_SIZE_Y);
    glVertex2f((float)g_ViewWidth, 0.0f);
    glEnd();
}

void ImGui_ImplOpenGL2_TakeSnapshot() 
{
    // Copy framebuffer to last frame texture
    glBindTexture(GL_TEXTURE_2D, g_SnapshotTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_ViewWidth, g_ViewHeight);
    
    // Copy last window size (so snapshot can be stretched if needed)
    g_LastViewWidth = g_ViewWidth;
    g_LastViewHeight = g_ViewHeight;
}