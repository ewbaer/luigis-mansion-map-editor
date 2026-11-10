#pragma once

#include <string>

namespace DolphinLauncher {
    // Returns an error message if Dolphin could not be started.
    std::string LaunchDiscImage(const std::string& dolphinPath, const std::string& imagePath);
}
