cmake_minimum_required (VERSION 3.8)

set(SHADER_COMPILE_DIR "./")
set(SHADERS_TO_COMPILE "prepass/rasterprepass.frag" "prepass/rasterprepass.vert")

set(SHADER_COMPILE_COMMANDS)

foreach (SHADER_TO_COMPILE IN LISTS SHADERS_TO_COMPILE)

	set (SHADER_COMPILE_COMMANDS "${SHADER_COMPILE_COMMANDS}$ENV{VULKAN_SDK}/Bin/glslc.exe ${SHADER_TO_COMPILE} -o ${SHADER_TO_COMPILE}.spv --target-env=vulkan1.2\n")

#	message("Compiling ${SHADER_TO_COMPILE}")
#	execute_process(WORKING_DIRECTORY "./" COMMAND "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe ${SHADER_TO_COMPILE} -o ${SHADER_TO_COMPILE}.spv --target-env vulkan1.2")
#	add_custom_command(OUTPUT "${SHADER_TO_COMPILE}.spv" WORKING_DIRECTORY "${SHADER_COMPILE_DIR}" COMMAND "$ENV{VULKAN_SDK}/Bin/glslc.exe ${SHADER_TO_COMPILE} -o ${SHADER_TO_COMPILE}.spv --target-env vulkan1.2")
endforeach()

configure_file("${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl/compile_all_shaders.bat.in" "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl/compile_all_shaders.bat")
