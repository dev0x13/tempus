from conan import ConanFile
from conan.tools.cmake import cmake_layout


class TempusRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("imgui/1.91.0")
        self.requires("glfw/3.4")
        self.requires("glew/2.2.0")
        self.requires("sqlitecpp/3.3.1")
        self.requires("portable-file-dialogs/0.1.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("cpr/1.10.5")
        self.requires("icu/74.2")
        self.requires("ghc-filesystem/1.5.14")

    def layout(self):
        cmake_layout(self)
