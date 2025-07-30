import os
import re

from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.files import load, rmdir, copy


class Recipe(ConanFile):
    url = "https://tentris.dice-research.org"
    topics = "triplestore", "sparql", "rdf", "sematic-web", "tensor"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "restinio/*:asio": "boost",
    }

    exports_sources = "libs/*", "CMakeLists.txt", "cmake/*"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("hypertrie/0.10.0", transitive_headers=True)
        self.requires("rdf4cpp/0.0.27.1", transitive_headers=True)
        self.requires("sparql-parser-base/0.3.6")
        self.requires("unordered_dense/4.4.0", transitive_headers=True, force=True)
        self.requires("cxxopts/2.2.1")
        self.requires("fmt/8.1.1", transitive_headers=True, force=True)
        self.requires("restinio/0.7.2")
        self.requires("expected-lite/0.8.0", override=True)
        self.requires("taskflow/3.4.0")
        self.requires("cppitertools/2.1")
        self.requires("spdlog/1.14.1")
        self.requires("rapidjson/cci.20220822")
        self.requires("metall/0.23.1")
        self.requires("nlohmann_json/3.11.2")
        self.requires("vincentlaucsb-csv-parser/2.1.3")
        self.requires("robin-hood-hashing/3.11.5", transitive_headers=True)
        self.requires("dice-hash/0.4.6", transitive_headers=True, force=True)
        self.requires("dice-sparse-map/0.2.5", transitive_headers=True)
        self.requires("dice-template-library/1.9.1", transitive_headers=True)
        self.requires("boost/1.84.0", transitive_headers=True, libs=False, force=True)

    def set_name(self):
        if not hasattr(self, 'name') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.name = re.search(r"project\(\s*([a-z\-]+)\s+VERSION", cmake_file).group(1)

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+.\d+)[^)]*\)", cmake_file).group(1)
        if not hasattr(self, 'description') or self.description is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.description = re.search(r"project\([^)]*DESCRIPTION\s+\"([^\"]+)\"[^)]*\)", cmake_file).group(1)

    def layout(self):
        cmake_layout(self)

    _cmake = None

    def _configure_cmake(self):
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.configure()
        return self._cmake

    def build(self):
        self._configure_cmake().build()

    def package(self):
        self._configure_cmake().install()
        for dir in ("res", "share", "cmake"):
            rmdir(self, os.path.join(self.package_folder, dir))
        copy(self, "LICENSE", src=self.folders.base_source, dst="licenses")

    def package_info(self):
        main_component = self.name
        self.cpp_info.set_property("cmake_target_name", f"{self.name}")
        self.cpp_info.components["global"].set_property("cmake_target_name", f"{self.name}::{main_component}")
        self.cpp_info.components["global"].names["cmake_find_package_multi"] = f"{self.name}"
        self.cpp_info.components["global"].names["cmake_find_package"] = f"{self.name}"
        self.cpp_info.set_property("cmake_file_name", f"{self.name}")
        self.cpp_info.components["global"].includedirs = [f"include/{self.name}/{main_component}/"]
        self.cpp_info.components["global"].requires = [
            "node-store", "rdf-tensor", "sparql2tensor", "triple-store", "endpoint",
            "boost::boost",
            "fmt::fmt",
            "restinio::restinio",
            "hypertrie::hypertrie",
            "metall::metall",
            "rdf4cpp::rdf4cpp",
            "sparql-parser-base::sparql-parser-base",
            "dice-hash::dice-hash",
            "cxxopts::cxxopts",
            "robin-hood-hashing::robin-hood-hashing",
            "expected-lite::expected-lite",
            "restinio::restinio",
            "taskflow::taskflow",
            "cppitertools::cppitertools",
            "spdlog::spdlog",
        ]

        if self.options.with_exec_deps:
            self.cpp_info.components["global"].requires += [
                "vincentlaucsb-csv-parser::vincentlaucsb-csv-parser",
                "nlohmann_json::nlohmann_json"]

        self.cpp_info.components["rdf-tensor"].requires = [
            "rdf4cpp::rdf4cpp",
            "hypertrie::hypertrie",
            "boost::boost",
            "metall::metall",
        ]

        self.cpp_info.components["node-store"].requires = [
            "rdf-tensor",
            "rdf4cpp::rdf4cpp"
        ]

        self.cpp_info.components["sparql2tensor"].requires = [
            "node-store",
            "robin-hood-hashing::robin-hood-hashing",
            "sparql-parser-base::sparql-parser-base",
        ]

        self.cpp_info.components["triple-store"].requires = [
            "sparql2tensor",
            "rdf-tensor",
        ]
        self.cpp_info.components["endpoint"].requires = [
            "rdf-tensor",
            "restinio::restinio",
            "taskflow::taskflow",
            "cppitertools::cppitertools",
            "spdlog::spdlog",
            "rapidjson::rapidjson",
        ]

        for component in ("node-store", "rdf-tensor", "sparql2tensor", "triple-store", "endpoint"):
            self.cpp_info.components[f"{component}"].includedirs = [f"include/{self.name}/{component}"]
            self.cpp_info.components[f"{component}"].names["cmake_find_package_multi"] = f"{component}"
            self.cpp_info.components[f"{component}"].names["cmake_find_package"] = f"{component}"

        for component in ("node-store", "sparql2tensor", "triple-store", "endpoint"):
            self.cpp_info.components[f"{component}"].libdirs = [f"lib/{self.name}/{component}"]
            self.cpp_info.components[f"{component}"].libs = [f"{self.name}-{component}"]
