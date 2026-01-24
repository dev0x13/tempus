// dear imgui: Platform Backend for GLFW
// This needs to be linked against GLFW (tested against 3.x)
// Backend: imgui_impl_glfw.cpp

#pragma once
#include "imgui.h"

struct GLFWwindow;
struct GLFWmonitor;

IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API void     ImGui_ImplGlfw_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGlfw_NewFrame();

// GLFW callbacks install
IMGUI_IMPL_API void     ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window);
IMGUI_IMPL_API void     ImGui_ImplGlfw_RestoreCallbacks(GLFWwindow* window);

// GFLW callbacks options:
IMGUI_IMPL_API void     ImGui_ImplGlfw_SetCallbacksChainForAllWindows(bool chain_for_all_windows);

// GLFW callbacks (individual callbacks to call yourself if you didn't install callbacks)
IMGUI_IMPL_API void     ImGui_ImplGlfw_WindowFocusCallback(GLFWwindow* window, int focused);
IMGUI_IMPL_API void     ImGui_ImplGlfw_CursorEnterCallback(GLFWwindow* window, int entered);
IMGUI_IMPL_API void     ImGui_ImplGlfw_CursorPosCallback(GLFWwindow* window, double x, double y);
IMGUI_IMPL_API void     ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_IMPL_API void     ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);
IMGUI_IMPL_API void     ImGui_ImplGlfw_MonitorCallback(GLFWmonitor* monitor, int event);
