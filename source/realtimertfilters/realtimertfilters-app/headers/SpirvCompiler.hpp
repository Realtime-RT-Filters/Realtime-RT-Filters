#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <utility>
#include <iostream>
#include <fstream>
#include <string_view>
#include <chrono>
#include "../project_defines.hpp"

namespace fs = std::filesystem;

namespace rtf
{
	struct ShaderFileInfo
	{
		fs::path m_SourcePathFull{};
		fs::path m_OutPathFull{};
	};

	inline std::ostream& operator<<(std::ostream& stream, ShaderFileInfo& shaderFileInfo)
	{
		stream << "Source: " << shaderFileInfo.m_SourcePathFull << " Output: " << shaderFileInfo.m_OutPathFull;
		return stream;
	}


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
	// https://stackoverflow.com/questions/15435994/how-do-i-open-an-exe-from-another-c-exe
	using SPV_STR = std::wstring;

	inline SPV_STR PathToString(const fs::path& path)
	{
		return path.wstring();
	}

	// calls the glslc.exe on windows and passes the shader file path
	// returns false if the compilation failed
	inline bool callGlslCompiler(const ShaderFileInfo& shaderFileInfo)
	{
		LPCWSTR lpApplicationName = SPIRV_COMPILER_CMD_NAME_W;
		if (lpApplicationName == nullptr || (int)lpApplicationName[0] == 0)
		{
			return false;
		}
		// additional information
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		SPV_STR commandLine = SPV_STR(lpApplicationName) + L" --target-spv=spv1.5 " + PathToString(shaderFileInfo.m_SourcePathFull) + L" -o " + PathToString(shaderFileInfo.m_OutPathFull);
		// start the program up
		CreateProcessW(lpApplicationName,   // the path
			(LPWSTR)commandLine.c_str(),        // Command line
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

		DWORD exitCode{ 0 };
		GetExitCodeProcess(pi.hProcess, &exitCode);

		// Close process and thread handles.
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		// returns true if an error occurs
		return exitCode == 0;
	}

#define SPIRV_FILEENDING std::wstring(L".spv")
#else
	using SPV_STR = std::string;

	inline SPV_STR PathToString(const fs::path& path)
	{
		return path.string();
	}

	// returns false if compilation fails
	inline bool callGlslCompiler(const ShaderFileInfo& shaderFileInfo)
	{
		std::string command("/bin/glslc --target-spv=spv1.5 " + PathToString(shaderFileInfo.m_SourcePathFull) + " -o " + PathToString(shaderFileInfo.m_OutPathFull));
		int returnvalue = std::system(command.c_str());
		return returnvalue == 0;
	}

#define SPIRV_FILEENDING std::string(".spv")
#endif


	class SpirvCompiler
	{
	public:
		SPV_STR m_SourceDir = SPV_STR();
		SPV_STR m_OutputDir = SPV_STR();
		bool m_Verbose = false;
		bool m_ThrowException = true;

		SpirvCompiler() = default;
		SpirvCompiler(const SPV_STR& sourceDir, const SPV_STR& outDir, bool verbose = false, bool throwException = true) : m_SourceDir(sourceDir), m_OutputDir(outDir), m_Verbose(verbose), m_ThrowException(throwException) {}

		// file endings that can be detected and directed to a shader type
		std::vector<SPV_STR> validFileEndings =
		{
	#ifdef _WIN32
				L".frag",
				L".vert",
				L".rchit",
				L".rmiss",
				L".rgen",
				L".comp",
	#else
				".frag",
				".vert",
				".rchit",
				".rmiss",
				".rgen",
				".comp",
	#endif
		};

		bool CompileAll()
		{
			if (m_SourceDir.empty() || m_OutputDir.empty())
			{
				return false;
			}

			// parse all shaders in data directory
			for (auto& pathIterator : fs::recursive_directory_iterator(m_SourceDir))
			{
				if (!pathIterator.is_directory())
				{
					fs::path shaderFile = pathIterator.path();
					compileShaderFile(shaderFile);
				}
			}
			return true;
		}

		bool compileShaderFile(fs::path shaderFilePath)
		{
			if (!isValidSourceFile(shaderFilePath))
			{
				if (m_Verbose)
					std::cout << "unsupported: " << shaderFilePath << std::endl;
				return false;
			}

			SPV_STR outputFullPath = PathToString(m_OutputDir) + PathToString(fs::relative(shaderFilePath, m_SourceDir)) + SPIRV_FILEENDING;
			ShaderFileInfo shaderFileInfo
			{
				shaderFilePath,
				fs::path(outputFullPath)
			};

			if (!needsCompiling(shaderFileInfo))
			{
				if (m_Verbose)
					std::cout << "skipped: " << shaderFileInfo << std::endl;
				return true;
			}

			bool compileResult = callGlslCompiler(shaderFileInfo);

			if (compileResult)
			{
				std::cout << "compiled: " << shaderFileInfo << std::endl;
			}
			if (!compileResult)
			{
				std::cout << "Failed to compile: " << shaderFileInfo << std::endl;
				if (m_ThrowException)
				{
					throw new std::runtime_error("Failed to compile a shader!");
				}
			}

			// If you get a runtime exception here, check console for shader compile error messages
			return compileResult;
		}

	protected:
		bool isValidSourceFile(const fs::path& shaderFilePath)
		{
			SPV_STR pathName = PathToString(shaderFilePath);
			for (auto& fileEnding : validFileEndings)
			{
				if (endsWith(pathName, fileEnding))
					return true;
			}

			// cannot identify shader type by one of the preset file endings
			return false;
		}

		bool needsCompiling(const ShaderFileInfo& shaderFile)
		{
			if (!fs::exists(shaderFile.m_OutPathFull))
			{
				return true;
			}

			fs::file_time_type ftimeSource = fs::last_write_time(shaderFile.m_SourcePathFull);
			fs::file_time_type ftimeOutput = fs::last_write_time(shaderFile.m_OutPathFull);
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ftimeOutput - ftimeSource);
			return milliseconds.count() < 0;
		}

		// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
		bool endsWith(const SPV_STR& str, const SPV_STR& suffix)
		{
			return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
		}

		bool startsWith(const SPV_STR& str, const SPV_STR& prefix)
		{
			return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
		}
	};

}