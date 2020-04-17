cmake_minimum_required(VERSION 3.13)

project(SuperSDL VERSION 0.1.0)

# Do things right: https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/

execute_process(
	COMMAND
	git rev-parse HEAD
	RESULT_VARIABLE
	COMMIT_HASH_RESULT
	OUTPUT_VARIABLE
	SUPERSDL_COMMIT_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)


# Needed so that cmake uses our find modules.
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(toml11 REQUIRED)

set(SOURCE_FILES
	src/game.cpp
	src/engine.cpp
	src/loggable.cpp
	src/util.cpp
	src/graphics/shader.cpp
	src/graphics/color.cpp
	src/graphics/renderer.cpp
	)

add_library(SuperSDL SHARED ${SOURCE_FILES})
target_compile_features(SuperSDL PRIVATE cxx_std_17)

target_include_directories(SuperSDL
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(SuperSDL PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wpedantic>)

# TODO: Create a IMPORTED target for glm?
target_link_libraries(SuperSDL
    PUBLIC
		SDL2::SDL2 Vulkan::Vulkan Freetype::Freetype glm spdlog::spdlog toml11::toml11
)

# Install instructions

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/SuperSDL)

install(TARGETS SuperSDL
	EXPORT SuperSDL-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export the targets to a script
install(EXPORT SuperSDL-targets
    FILE
		SuperSDLTargets.cmake
    NAMESPACE
		SuperSDL::
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/SuperSDLConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/SuperSDLConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SuperSDLConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SuperSDLConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SuperSDLConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

export(EXPORT SuperSDL-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/SuperSDLTargets.cmake
    NAMESPACE SuperSDL::)

#Register package in user's package registry
export(PACKAGE SuperSDL)

# Example Executable

add_executable(SuperSDLExample examples/example.cpp)
target_compile_features(SuperSDLExample PRIVATE cxx_std_17)
target_link_libraries(SuperSDLExample SuperSDL)

