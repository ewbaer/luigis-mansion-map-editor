#include "DolphinLauncher.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>

extern char** environ;
#endif

std::string DolphinLauncher::LaunchDiscImage(const std::string& dolphinPath, const std::string& imagePath)
{
    namespace fs = std::filesystem;

    if (dolphinPath.empty()){
        return "Set Dolphin Path in Edit > Options first.";
    }

    try {
        const fs::path image = fs::absolute(fs::path(std::u8string(imagePath.begin(), imagePath.end())));
        auto extension = image.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char character){ return std::tolower(character); });
        if ((extension != ".iso" && extension != ".gcm") || !fs::is_regular_file(image)){
            return "Select an existing .iso or .gcm disc image.";
        }

#ifdef _WIN32
        const fs::path executable = fs::absolute(fs::path(std::u8string(dolphinPath.begin(), dolphinPath.end())));
        if (!fs::is_regular_file(executable)){
            return "Dolphin executable was not found. Check Dolphin Path in Edit > Options.";
        }

        // Windows filenames cannot contain double quotes. Both paths name files.
        std::wstring commandLine = L"\"" + executable.native() + L"\" -e \"" + image.native() + L"\"";
        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(executable.c_str(), commandLine.data(), nullptr, nullptr,
                FALSE, 0, nullptr, executable.parent_path().c_str(), &startupInfo, &process)){
            return "Could not start Dolphin. Windows error " + std::to_string(GetLastError()) + ".";
        }
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
#else
        std::string executable = dolphinPath;
        std::string argument = image.string();
        char execFlag[] = "-e";
        char* arguments[] = { executable.data(), execFlag, argument.data(), nullptr };
        pid_t process;
        const int error = posix_spawnp(&process, executable.c_str(), nullptr, nullptr, arguments, environ);
        if (error != 0){
            return "Could not start Dolphin: " + std::error_code(error, std::generic_category()).message();
        }
        std::thread([process]{
            while (waitpid(process, nullptr, 0) == -1 && errno == EINTR){}
        }).detach();
#endif
        return {};
    } catch (const fs::filesystem_error& error) {
        return "Could not start Dolphin: " + error.code().message();
    }
}
