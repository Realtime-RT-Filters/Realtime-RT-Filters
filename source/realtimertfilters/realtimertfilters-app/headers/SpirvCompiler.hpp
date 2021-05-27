#pragma once
#include "../project_defines.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <utility>
#include <iostream>
#include <fstream>
#include <string_view>
#include <chrono>
#include "../../base/DataPath.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// https://stackoverflow.com/questions/15435994/how-do-i-open-an-exe-from-another-c-exe
namespace rtf
{
	// calls the glslc.exe on windows and passes the shader file path
	// returns true if the compilation failed
	bool callGlslCompiler(std::string& inputFileFullPath)
	{
		LPCTSTR lpApplicationName = RT_GLSLC_EXECUTABLE_PATH;
		// additional information
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		std::string commandLine = std::string(lpApplicationName) + " " + inputFileFullPath + " -o " + inputFileFullPath + ".spv";

		// start the program up
		CreateProcess(lpApplicationName,   // the path
			(LPSTR)commandLine.c_str(),        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);

		// wait for compilation to finish
		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode{0};
		GetExitCodeProcess(pi.hProcess, &exitCode);
	
		// Close process and thread handles.
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		// returns true if an error occurs
		return exitCode != 0;
	}
}
#else
namespace rtf
{
	// TODO: Call glslc on linux
	// return true if compilation fails
	bool callGlslCompiler(std::string& inputFile)
	{
		return true;
	}
}
#endif



namespace fs = std::filesystem;

namespace rtf
{
	class SpirvCompiler
	{
	public:
		SpirvCompiler() = default;
		~SpirvCompiler() = default;

		bool verbose{ false };

		// file endings that can be detected and directed to a shader type
		std::vector<char*> validFileEndings =
		{
			"frag",
			"vert",
			"rchit",
			"rmiss",
			"rgen",
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
			// get base path
			std::string dataDir(VK_EXAMPLE_DATA_DIR);
			dataDir += "shaders/glsl/";

			if (shaders.size() > 0)
			{
				// compile set of shaders given via input parameter shaders
				for (std::string& shaderFile : shaders)
				{
					std::string fullPath = dataDir + shaderFile;
					if (compileShaderFile(fullPath))
					{
						return true;
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
						if (compileShaderFile(fullPath))
						{
							return true;
						}
					}
				}
			}
			return false;
		}

		bool compileShaderFile(std::string& shaderFileFullPath)
		{
			if (!isValidShaderFile(shaderFileFullPath))
			{
				if(verbose)
					std::cout << "unsupported: " << shaderFileFullPath << std::endl;
				return false;
			}
			if (needsCompilation(shaderFileFullPath))
			{
				std::cout << "compiling: " << shaderFileFullPath << std::endl;
				return callGlslCompiler(shaderFileFullPath);
			}
			else
			{
				if (verbose)
					std::cout << "skipped: " << shaderFileFullPath << std::endl;
				return false;
			}
		};

		// checks if the source file is newer than the binary and needs a recompilation
		bool needsCompilation(std::string& shaderFilePath)
		{
			fs::path pathSource(shaderFilePath);
			fs::path pathOutput(shaderFilePath + ".spv");

			fs::file_time_type ftimeSource = fs::last_write_time(pathSource);
			fs::file_time_type ftimeOutput = fs::last_write_time(pathOutput);
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ftimeOutput - ftimeSource);
			return milliseconds.count() < 0;
		}

		bool isValidShaderFile(std::string& shaderFile)
		{
			for (auto& fileEnding : validFileEndings)
			{
				if (endsWith(shaderFile, std::string(fileEnding)))
					return true;
			}

			// cannot identify shader type by one of the preset file endings
			return false;
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
