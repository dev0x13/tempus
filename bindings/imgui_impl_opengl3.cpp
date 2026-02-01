// dear imgui: Renderer Backend for modern OpenGL with shaders / programmatic pipeline
// Simplified implementation for Tempus app

#include "imgui_impl_opengl3.h"
#include "imgui.h"

#include <GL/glew.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

// OpenGL Data
struct ImGui_ImplOpenGL3_Data
{
    GLuint          GlVersion;
    char            GlslVersionString[32];
    GLuint          FontTexture;
    GLuint          ShaderHandle;
    GLint           AttribLocationTex;
    GLint           AttribLocationProjMtx;
    GLuint          AttribLocationVtxPos;
    GLuint          AttribLocationVtxUV;
    GLuint          AttribLocationVtxColor;
    GLuint          VboHandle, ElementsHandle;
    GLsizeiptr      VertexBufferSize;
    GLsizeiptr      IndexBufferSize;
    bool            HasClipOrigin;

    ImGui_ImplOpenGL3_Data() { memset((void*)this, 0, sizeof(*this)); }
};

static ImGui_ImplOpenGL3_Data* ImGui_ImplOpenGL3_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplOpenGL3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Forward declarations
static void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object);

bool ImGui_ImplOpenGL3_Init(const char* glsl_version)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplOpenGL3_Data* bd = IM_NEW(ImGui_ImplOpenGL3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_opengl3";

    // Query for GL version
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0)
    {
        // Query GL_VERSION in desktop GL 2.x
        const char* gl_version = (const char*)glGetString(GL_VERSION);
        sscanf(gl_version, "%d.%d", &major, &minor);
    }
    bd->GlVersion = (GLuint)(major * 100 + minor * 10);

    if (glsl_version == nullptr)
    {
#ifdef __APPLE__
        glsl_version = "#version 150";
#else
        glsl_version = "#version 130";
#endif
    }

    IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(bd->GlslVersionString));
    strcpy(bd->GlslVersionString, glsl_version);
    strcat(bd->GlslVersionString, "\n");

    // Check for clip origin support
    bd->HasClipOrigin = (bd->GlVersion >= 450);
#ifdef GL_CLIP_ORIGIN
    GLint clip_origin = 0;
    if (bd->HasClipOrigin)
        glGetIntegerv(GL_CLIP_ORIGIN, &clip_origin);
    bd->HasClipOrigin = (clip_origin == GL_UPPER_LEFT);
#endif

    return true;
}

void ImGui_ImplOpenGL3_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplOpenGL3_NewFrame()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOpenGL3_Init()?");

    if (!bd->ShaderHandle)
        ImGui_ImplOpenGL3_CreateDeviceObjects();
}

static void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);

    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

    if (bd->HasClipOrigin)
    {
        float tmp = T; T = B; B = tmp;
    }

    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };
    glUseProgram(bd->ShaderHandle);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

    glBindSampler(0, 0);

    glBindVertexArray(vertex_array_object);

    glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
    glEnableVertexAttribArray(bd->AttribLocationVtxPos);
    glEnableVertexAttribArray(bd->AttribLocationVtxUV);
    glEnableVertexAttribArray(bd->AttribLocationVtxColor);
    glVertexAttribPointer(bd->AttribLocationVtxPos,   2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(bd->AttribLocationVtxUV,    2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
}

void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLuint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&last_program);
    GLuint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&last_texture);
    GLuint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&last_sampler);
    GLuint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&last_array_buffer);
    GLuint last_vertex_array_object; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&last_vertex_array_object);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup desired GL state
    GLuint vertex_array_object = 0;
    glGenVertexArrays(1, &vertex_array_object);
    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        GLsizeiptr vtx_buffer_size = (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        GLsizeiptr idx_buffer_size = (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        if (bd->VertexBufferSize < vtx_buffer_size)
        {
            bd->VertexBufferSize = vtx_buffer_size;
            glBufferData(GL_ARRAY_BUFFER, bd->VertexBufferSize, nullptr, GL_STREAM_DRAW);
        }
        if (bd->IndexBufferSize < idx_buffer_size)
        {
            bd->IndexBufferSize = idx_buffer_size;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->IndexBufferSize, nullptr, GL_STREAM_DRAW);
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, vtx_buffer_size, (const GLvoid*)cmd_list->VtxBuffer.Data);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idx_buffer_size, (const GLvoid*)cmd_list->IdxBuffer.Data);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                glScissor((int)clip_min.x, (int)((float)fb_height - clip_max.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));

                // Bind texture, Draw
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Destroy the temporary VAO
    glDeleteVertexArrays(1, &vertex_array_object);

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindSampler(0, last_sampler);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_stencil_test) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->FontTexture);
    glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->FontTexture)
    {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

static bool CheckShader(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: ImGui_ImplOpenGL3: failed to compile %s!\n", desc);
    if (log_length > 1)
    {
        char buf[1024];
        glGetShaderInfoLog(handle, 1024, nullptr, buf);
        fprintf(stderr, "%s\n", buf);
    }
    (void)bd;
    return (GLboolean)status == GL_TRUE;
}

static bool CheckProgram(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: ImGui_ImplOpenGL3: failed to link %s!\n", desc);
    if (log_length > 1)
    {
        char buf[1024];
        glGetProgramInfoLog(handle, 1024, nullptr, buf);
        fprintf(stderr, "%s\n", buf);
    }
    (void)bd;
    return (GLboolean)status == GL_TRUE;
}

bool ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    // Parse GLSL version string
    int glsl_version = 130;
    sscanf(bd->GlslVersionString, "#version %d", &glsl_version);

    const GLchar* vertex_shader_glsl_130 =
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_130 =
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar* vertex_shader = nullptr;
    const GLchar* fragment_shader = nullptr;
    vertex_shader = vertex_shader_glsl_130;
    fragment_shader = fragment_shader_glsl_130;

    // Create shaders
    const GLchar* vertex_shader_with_version[2] = { bd->GlslVersionString, vertex_shader };
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 2, vertex_shader_with_version, nullptr);
    glCompileShader(vert_handle);
    CheckShader(vert_handle, "vertex shader");

    const GLchar* fragment_shader_with_version[2] = { bd->GlslVersionString, fragment_shader };
    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 2, fragment_shader_with_version, nullptr);
    glCompileShader(frag_handle);
    CheckShader(frag_handle, "fragment shader");

    // Link
    bd->ShaderHandle = glCreateProgram();
    glAttachShader(bd->ShaderHandle, vert_handle);
    glAttachShader(bd->ShaderHandle, frag_handle);
    glLinkProgram(bd->ShaderHandle);
    CheckProgram(bd->ShaderHandle, "shader program");

    glDetachShader(bd->ShaderHandle, vert_handle);
    glDetachShader(bd->ShaderHandle, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->AttribLocationTex = glGetUniformLocation(bd->ShaderHandle, "Texture");
    bd->AttribLocationProjMtx = glGetUniformLocation(bd->ShaderHandle, "ProjMtx");
    bd->AttribLocationVtxPos = (GLuint)glGetAttribLocation(bd->ShaderHandle, "Position");
    bd->AttribLocationVtxUV = (GLuint)glGetAttribLocation(bd->ShaderHandle, "UV");
    bd->AttribLocationVtxColor = (GLuint)glGetAttribLocation(bd->ShaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &bd->VboHandle);
    glGenBuffers(1, &bd->ElementsHandle);

    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    return true;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->VboHandle)      { glDeleteBuffers(1, &bd->VboHandle); bd->VboHandle = 0; }
    if (bd->ElementsHandle) { glDeleteBuffers(1, &bd->ElementsHandle); bd->ElementsHandle = 0; }
    if (bd->ShaderHandle)   { glDeleteProgram(bd->ShaderHandle); bd->ShaderHandle = 0; }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}
