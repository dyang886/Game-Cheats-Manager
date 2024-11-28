#include <iostream>
#include <string>

bool addToWindowsDefenderWhitelist(const std::string& path) {
    std::string command = "powershell -Command \"Add-MpPreference -ExclusionPath '" + path + "'\"";
    return system(command.c_str()) == 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path1> <path2> ... <pathN>" << std::endl;
        return 1;
    }

    bool allSucceeded = true;
    for (int i = 1; i < argc; ++i) {
        std::string path = argv[i];
        if (addToWindowsDefenderWhitelist(path)) {
            std::cout << "Path added to Windows Defender whitelist successfully: " << path << std::endl;
        } else {
            std::cerr << "Failed to add path to Windows Defender whitelist: " << path << std::endl;
            allSucceeded = false;
        }
    }

    return allSucceeded ? 0 : 1;
}
