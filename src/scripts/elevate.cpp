#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

// Function to add a path to Windows Defender whitelist
bool addToWindowsDefenderWhitelist(const std::string &path)
{
    std::string command = "powershell -Command \"Add-MpPreference -ExclusionPath '" + path + "'\"";
    return system(command.c_str()) == 0;
}

// Function to move translation files
bool moveTranslationFiles(const std::string &source, const std::string &destination)
{
    try
    {
        fs::create_directories(fs::path(destination).parent_path());
        fs::copy(source, destination, fs::copy_options::overwrite_existing | fs::copy_options::recursive);

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error moving folder: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <command> [args...]" << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    std::string command = argv[1];

    if (command == "whitelist")
    {
        if (argc < 3)
        {
            std::cerr << "Usage for whitelist: " << argv[0] << " whitelist <path1> <path2> ..." << std::endl;
            std::cerr << "Press Enter to exit..." << std::endl;
            std::cin.get();
            return 1;
        }

        bool allSucceeded = true;
        for (int i = 2; i < argc; ++i)
        {
            std::string path = argv[i];
            if (addToWindowsDefenderWhitelist(path))
            {
                std::cout << "Path added to Windows Defender whitelist successfully: " << path << std::endl;
            }
            else
            {
                std::cerr << "Failed to add path to Windows Defender whitelist: " << path << std::endl;
                allSucceeded = false;
            }
        }
        if (!allSucceeded)
        {
            std::cerr << "One or more paths failed to be added to the whitelist." << std::endl;
            std::cerr << "Press Enter to exit..." << std::endl;
            std::cin.get();
        }
        return allSucceeded ? 0 : 1;
    }
    else if (command == "translation")
    {
        if (argc != 4)
        {
            std::cerr << "Usage for translation: " << argv[0] << " translation <source_path> <destination_path>" << std::endl;
            std::cerr << "Press Enter to exit..." << std::endl;
            std::cin.get();
            return 1;
        }

        std::string source = argv[2];
        std::string destination = argv[3];
        if (moveTranslationFiles(source, destination))
        {
            std::cout << "Translation files moved successfully from " << source << " to " << destination << std::endl;
            return 0;
        }
        else
        {
            std::cerr << "Failed to move translation files." << std::endl;
            std::cerr << "Press Enter to exit..." << std::endl;
            std::cin.get();
            return 1;
        }
    }
    else
    {
        std::cerr << "Unknown command: " << command << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
}
