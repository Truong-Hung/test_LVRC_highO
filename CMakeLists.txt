cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(LVRC LANGUAGES CXX C)


option(LVRC_BUILD_DOCS "Generate LVRC doxygen documentation" OFF)
option(OPTIX "Use NVIDIA OptiX for accelerated RTX rendering" OFF)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compilation and optimization flags
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra")

# Download and install third party libraries
include(cmake/third_party.cmake)

find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/include/*.hpp ${CMAKE_SOURCE_DIR}/include/*.h)

if(NOT OPTIX)
        list(FILTER SOURCES EXCLUDE REGEX ${CMAKE_SOURCE_DIR}/src/renderer/cuda/*)
        list(FILTER HEADERS EXCLUDE REGEX ${CMAKE_SOURCE_DIR}/include/renderer/cuda/*)
endif()

# For better display in IDEs
add_executable(lvrc ${SOURCES} ${HEADERS} ${embedded_ptx_code})
target_include_directories(lvrc PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_options(lvrc PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--ptxas-options=-v>
)

# Link libraries
target_link_libraries(lvrc PRIVATE imgui glm glad glfw imguizmo)
target_link_libraries(lvrc PRIVATE glm)
target_link_libraries(lvrc PRIVATE glad)

if(OPTIX)
        target_link_libraries(lvrc PRIVATE ${optix_LIBRARY} ${CUDA_LIBRARIES} ${CUDA_CUDA_LIBRARY})
        target_include_directories(lvrc PUBLIC ${OptiX_INCLUDE} ${CUDA_TOOLKIT_INCLUDE})
endif()

# Build documentation
if(LVRC_BUILD_DOCS)
        find_package(Doxygen OPTIONAL_COMPONENTS dot mscgen dia)

        if(DOXYGEN_FOUND)
                message(STATUS "Generating Doxygen documentation")
        
                set(DOXYGEN_GENERATE_HTML YES)
                set(DOXYGEN_DISABLE_INDEX NO)
                set(DOXYGEN_GENERATE_TREEVIEW NO)
                set(DOXYGEN_EXTRACT_PRIVATE YES)
                set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.md)

                doxygen_add_docs(
                        lvrc_doc
                        ${SOURCES}
                        ${HEADERS}
                        ${CMAKE_SOURCE_DIR}/README.md
                        WORKING_DIRECTORY 
                        ${CMAKE_BINARY_DIR}
                        COMMENT "Generate LVRC documentation")
        else()
                message(WARNING "Documentation generation enabled, but unable to find Doxygen.")
        endif()
endif()
