from conan import ConanFile
from conan.tools.cmake import cmake_layout


class TimeTrackerRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("imgui/1.91.0")
        self.requires("glfw/3.4")
        self.requires("glew/2.2.0")
        self.requires("sqlitecpp/3.3.1")
        self.requires("portable-file-dialogs/0.1.0")

    def layout(self):
        cmake_layout(self)
