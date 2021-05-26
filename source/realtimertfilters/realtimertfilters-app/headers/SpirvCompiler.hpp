#pragma once
#include "../project_defines.hpp"

#ifndef RT_USE_SHADERC_SPIRV_GENERATION
namespace rtf
{
	class SpirvCompiler
	{
	public:
		SpirvCompiler = default;
		~SpirvCompiler() = default;
		
		bool compileShaders() { return false; }

		// shaderc library is not loaded. Do nothing.
		bool compileShaders(std::vector<std::string>& shaders = nullptr) { return false; }
	};
}
#else
#include <shaderc/shaderc.hpp>
#include <shaderc/glslc/src/file_includer.h>
#include <shaderc/libshaderc_util/include/libshaderc_util/file_finder.h>
#include <string>
#include <vector>
#include <filesystem>
#include <utility>
#include <iostream>
#include <fstream>
#include <string_view>
#include <chrono>
#include "../../base/DataPath.h"

namespace fs = std::filesystem;

namespace rtf
{
	class SpirvCompiler
	{
	public:
		SpirvCompiler() = default;
		~SpirvCompiler() = default;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
	
		// file endings that can be detected and directed to a shader type
		std::vector<std::pair<char*, shaderc_shader_kind>> validFileEndings =
		{
			{"frag",  shaderc_shader_kind::shaderc_fragment_shader},
			{"vert",  shaderc_shader_kind::shaderc_vertex_shader},
			{"rchit", shaderc_shader_kind::shaderc_closesthit_shader},
			{"rmiss", shaderc_shader_kind::shaderc_miss_shader},
			{"rgen",  shaderc_shader_kind::shaderc_raygen_shader},
		};

		// call compiled shaders with no specific shaders passed
		bool compileShaders()
		{
			return compileShaders(std::vector<std::string>());
		}

		// compiles either all shaders passed by the vector or if left to null, compiles
		// all shaders in data/shaders/
		bool compileShaders(std::vector<std::string>& shaders)
		{
			// use latest spirv version to be able to use rt extensions
			options.SetTargetSpirv(shaderc_spirv_version::shaderc_spirv_version_1_5);

			// include interface to solve #include directives in glsl shaders
			shaderc_util::FileFinder fileFinder;
			options.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));

			// get base path
			std::string dataDir(VK_EXAMPLE_DATA_DIR);
			dataDir += "shaders/glsl/";

			std::cout << "+++++++++++++++ Compiling shaders +++++++++++++++ \n";
			bool errors = false;
			if (shaders.size() > 0)
			{
				// compile set of shaders given via input parameter shaders
				for (std::string& shaderFile : shaders)
				{
					std::string fullPath = dataDir + shaderFile;
					auto shader_kind = validateShaderFile(shaderFile);
					if (shader_kind == shaderc_shader_kind::shaderc_glsl_infer_from_source)
					{
						std::cout << "unknown shadertype(might fail): " << shaderFile << std::endl;
					}
					if (needsCompilation(fullPath))
					{
						errors = errors || generateSpirv(fullPath, shader_kind);
					}
					else
					{
						std::cout << "skipped: " << fullPath << std::endl;
					}
				}
			}
			else
			{
				// parse all shaders in data directory
				for (auto& path : fs::recursive_directory_iterator(dataDir))
				{
					if (!path.is_directory())
					{
						std::string fullPath = path.path().string();

						auto shader_kind = validateShaderFile(fullPath);
						if (shader_kind == shaderc_shader_kind::shaderc_glsl_infer_from_source)
						{
							// do not auto compile shaders with unknown ending
							continue;
						}

						if (needsCompilation(fullPath))
						{
							errors = errors || generateSpirv(fullPath, shader_kind);
						}
						else
						{
							std::cout << "skipped: " << fullPath << std::endl;
						}
					}
				}
			}
			return errors;
		}

		bool generateSpirv(std::string& sourceFile, shaderc_shader_kind shaderkind)
		{
			std::string sourceText;
			readSourceFile(sourceFile,
				&sourceText);
			sourceText.append("\0");
			shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(sourceText, shaderkind, sourceFile.c_str(), options);
			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				std::cout << "Failed " << "\n"
					<<  "Error message: " << result.GetErrorMessage() << "\n\n";
				return true;
			}
			std::cout << "compiled: " << sourceFile << "\n";
			// write spirv file
			std::vector<uint32_t> spirv;
			spirv.assign(result.cbegin(), result.cend());

			auto myfile = std::fstream(sourceFile+".spv", std::ios::out | std::ios::binary);
			myfile.write((char*)spirv.data(), spirv.size());
			myfile.close();
			return false;
		}

		// reads a shader source file and outputs its content in buffer
		void readSourceFile(const std::string& filename, std::string* outputString)
		{
			std::ifstream fh(filename, std::ios::in | std::ios::out | std::ios::ate);
			if (!fh.is_open())
			{
				std::cout << "error: could not open file: " << filename << std::endl;
				return;
			}
			const size_t sz = fh.tellg();
			if (sz <= 0) {
				return;
			}
			fh.seekg(0, std::ios::beg);
			// Initalizes a std::string with length sz, filled with null
			*outputString = std::string(sz, ' ');
			fh.read(outputString->data(), sz);
			fh.close();
		}

		// checks if the source file is newer than the binary and needs a recompilation
		bool needsCompilation(std::string& shaderFilePath)
		{
			return true;
			fs::path pathSource(shaderFilePath);
			fs::path pathOutput(shaderFilePath + ".spv");

			fs::file_time_type ftimeSource = fs::last_write_time(pathSource);
			fs::file_time_type ftimeOutput = fs::last_write_time(pathOutput);
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ftimeOutput - ftimeSource);
			return milliseconds.count() < 0;
		}

		shaderc_shader_kind validateShaderFile(std::string& shaderFile)
		{
			for (auto& pair : validFileEndings)
			{
				if (endsWith(shaderFile, std::string(pair.first)))
					return pair.second;
			}

			// cannot identify shader type by one of the preset file endings
			return shaderc_shader_kind::shaderc_glsl_infer_from_source;
		}

		// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
		bool endsWith(std::string_view str, std::string_view suffix)
		{
			return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
		}

		bool startsWith(std::string_view str, std::string_view prefix)
		{
			return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
		}
	};
}
#endif
