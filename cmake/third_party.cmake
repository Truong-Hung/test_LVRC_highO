# Download and create target for third party libraries
# instead of manually keeping code in ./external. (Better for quick upgrade)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# .................................
# GLFW
# .................................

message(STATUS "Installing Glfw3...")
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "Done")

# .................................
# IMGUI
# .................................

message(STATUS "Installing ImGui...")
FetchContent_Declare(
    imgui_sources
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9-docking)
FetchContent_Populate(imgui_sources)

file(GLOB IMGUI_SOURCES 
	${imgui_sources_SOURCE_DIR}/*.cpp
	${imgui_sources_SOURCE_DIR}/*.h
    ${imgui_sources_SOURCE_DIR}/backends/imgui_impl_glfw*
	${imgui_sources_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${imgui_sources_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_sources_SOURCE_DIR})
target_link_libraries(imgui PUBLIC glfw)
message(STATUS "Done")

# .................................
# IMGUIZMO
# .................................

message(STATUS "Installing ImGuizmo...")
FetchContent_Declare(
    imguizmo_sources
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
    GIT_TAG be3d9cd)
FetchContent_Populate(imguizmo_sources)

file(GLOB IMGUIZMO_SOURCES 
	${imguizmo_sources_SOURCE_DIR}/*.cpp
	${imguizmo_sources_SOURCE_DIR}/*.h)
add_library(imguizmo ${IMGUIZMO_SOURCES})
target_include_directories(imguizmo PUBLIC ${imguizmo_sources_SOURCE_DIR})
target_link_libraries(imguizmo PUBLIC imgui)
message(STATUS "Done")

# .................................
# GLM
# .................................

message(STATUS "Installing Glm...")
FetchContent_Declare(
    glm_sources
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(glm_sources)
message(STATUS "Done")

# .................................
# Glad
# .................................

message(STATUS "Installing Glad...")
set(glad_SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/GLAD)
add_library(glad 
    ${glad_SOURCE_DIR}/src/glad.c
    ${glad_SOURCE_DIR}/include/glad/glad.h
    ${glad_SOURCE_DIR}/include/KHR/khrplatform.h)
target_include_directories(glad PUBLIC ${glad_SOURCE_DIR}/include)
message(STATUS "Done")

# .................................
# OptiX
# .................................

if(OPTIX)
    enable_language(CUDA)

    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-O3 -res-usage -gencode arch=compute_86,code=sm_86 --ptx -allow-unsupported-compiler -ccbin /usr/bin/g++-11)
    set(CMAKE_CUDA_ARCHITECTURES "86")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

    find_package(OptiX REQUIRED VERSION 7)
    find_package(CUDA REQUIRED)

    find_program(BIN2C bin2c DOC "Path to the cuda-sdk bin2c executable.")

    macro(cuda_compile_and_embed output_var cuda_file)
        set(c_var_name ${output_var})

        cuda_compile_ptx(
            ptx_files
            ${cuda_file} 
            # Default options
            OPTIONS -use_fast_math --keep --relocatable-device-code=true)
            # Debug options
            # OPTIONS --generate-line-info -use_fast_math --keep --relocatable-device-code=true)
            
            list(GET ptx_files 0 ptx_file)
            set(embedded_file ${ptx_file}_embedded.c)

        add_custom_command(
            OUTPUT ${embedded_file}
            COMMAND ${BIN2C} -c --padd 0 --type char --name ${c_var_name} ${ptx_file} > ${embedded_file}
            DEPENDS ${ptx_file}
            COMMENT "compiling (and embedding ptx from) ${cuda_file}")

        set(${output_var} ${embedded_file})
    endmacro()

    cuda_include_directories(
            ${OptiX_INCLUDE}
            ${CUDA_TOOLKIT_INCLUDE}
            ${CMAKE_SOURCE_DIR}/include/)

    cuda_compile_and_embed(embedded_ptx_code ${CMAKE_SOURCE_DIR}/resources/cuda/OptixRenderer/volumetricRayCasting.cu) 

    include_directories(${OptiX_INCLUDE})

    add_compile_definitions(USING_OPTIX)
endif()
