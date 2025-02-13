#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Function to add a path to Windows Defender whitelist
bool addToWindowsDefenderWhitelist(const std::wstring &path)
{
    std::wstring command = L"powershell -Command \"Add-MpPreference -ExclusionPath '" + path + L"'\"";
    return _wsystem(command.c_str()) == 0;
}

// Function to move translation files
bool moveTranslationFiles(const std::wstring &source, const std::wstring &destination)
{
    try
    {
        fs::path srcPath = source;
        fs::path dstPath = destination;
        fs::create_directories(dstPath.parent_path());
        fs::copy(srcPath, dstPath, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        return true;
    }
    catch (const std::exception &e)
    {
        std::wcerr << L"Error moving folder: " << e.what() << std::endl;
        return false;
    }
}

int wmain(int argc, wchar_t *argv[])
{
    std::locale::global(std::locale(""));

    if (argc < 2)
    {
        std::wcerr << L"Usage: " << argv[0] << L" <command> [args...]" << std::endl;
        std::wcerr << L"Press Enter to exit..." << std::endl;
        std::wcin.get();
        return 1;
    }

    std::wstring command = argv[1];

    if (command == L"whitelist")
    {
        if (argc < 3)
        {
            std::wcerr << L"Usage for whitelist: " << argv[0] << L" whitelist <path1> <path2> ..." << std::endl;
            std::wcerr << L"Press Enter to exit..." << std::endl;
            std::wcin.get();
            return 1;
        }

        bool allSucceeded = true;
        for (int i = 2; i < argc; ++i)
        {
            std::wstring path = argv[i];
            if (addToWindowsDefenderWhitelist(path))
            {
                std::wcout << L"Path added to Windows Defender whitelist successfully: " << path << std::endl;
            }
            else
            {
                std::wcerr << L"Failed to add path to Windows Defender whitelist: " << path << std::endl;
                allSucceeded = false;
            }
        }
        if (!allSucceeded)
        {
            std::wcerr << L"One or more paths failed to be added to the whitelist." << std::endl;
            std::wcerr << L"Press Enter to exit..." << std::endl;
            std::wcin.get();
        }
        return allSucceeded ? 0 : 1;
    }
    else if (command == L"translation")
    {
        if (argc != 4)
        {
            std::wcerr << L"Usage for translation: " << argv[0] << L" translation <source_path> <destination_path>" << std::endl;
            std::wcerr << L"Press Enter to exit..." << std::endl;
            std::wcin.get();
            return 1;
        }

        std::wstring source = argv[2];
        std::wstring destination = argv[3];
        if (moveTranslationFiles(source, destination))
        {
            std::wcout << L"Translation files moved successfully from " << source << L" to " << destination << std::endl;
            return 0;
        }
        else
        {
            std::wcerr << L"Failed to move translation files." << std::endl;
            std::wcerr << L"Press Enter to exit..." << std::endl;
            std::wcin.get();
            return 1;
        }
    }
    else
    {
        std::wcerr << L"Unknown command: " << command << std::endl;
        std::wcerr << L"Press Enter to exit..." << std::endl;
        std::wcin.get();
        return 1;
    }
}
