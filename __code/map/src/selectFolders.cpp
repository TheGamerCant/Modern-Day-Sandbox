#include <filesystem>
#include <fstream>
#include <regex>

#include "selectFolders.hpp"


#include <windows.h>
#include <shobjidl.h>
// Function to convert wide string (PWSTR) to a standard string
static std::string WideStringToString(PWSTR pszWideString) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, pszWideString, -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) {
        return std::string();
    }
    std::string str(bufferSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, pszWideString, -1, &str[0], bufferSize, NULL, NULL);
    return str;
}

// Function to display the folder selection dialog and return the selected path as a string
std::string SelectFolder() {
    std::string folderPath = "";

    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileDialog* pFileDialog = nullptr;

        // Create the FileOpenDialog object
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
        if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            pFileDialog->GetOptions(&dwOptions);
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);  // Set option to pick folders

            // Show the dialog
            hr = pFileDialog->Show(NULL);
            if (SUCCEEDED(hr)) {
                // Get the selected item
                IShellItem* pItem = nullptr;
                hr = pFileDialog->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    // Get the file path
                    PWSTR pszFilePath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Convert the file path to a string
                    if (SUCCEEDED(hr)) {
                        folderPath = WideStringToString(pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }
        CoUninitialize();
    }
    return folderPath;
}

static void removeTrailingNullChar(std::string& str) {
    if (!str.empty() && str.back() == '\0') {
        str.pop_back();  // Removes the last character from the string
    }
}

void returnModAndVanillaDirectories(std::filesystem::path& modDirectory, std::filesystem::path& vanillaDirectory) {
    std::filesystem::path currentDirectory = std::filesystem::current_path();
    std::string gameDirectoriesTXT = currentDirectory.string() + "\\file_directories.txt";
    std::ifstream file(gameDirectoriesTXT);
    std::string line;

    std::regex modDirectoryRegex("mod_directory\s*=\s*\"(.*?)\"");
    std::regex vanillaDirectoryRegex("vanilla_directory\s*=\s*\"(.*?)\"");;
    while (getline(file, line)) {
        std::smatch match;

        if (regex_search(line, match, modDirectoryRegex)) {
            std::string modDirString = match[1];
            removeTrailingNullChar(modDirString);
            modDirectory = modDirString;
        }
        else if (regex_search(line, match, vanillaDirectoryRegex)) {
            std::string vanillaDirString = match[1];
            removeTrailingNullChar(vanillaDirString);
            vanillaDirectory = vanillaDirString;
        }
    }

    if (!std::filesystem::is_directory(modDirectory) || !std::filesystem::is_directory(vanillaDirectory)) {
        returnDirectoriesFromGUI(modDirectory, vanillaDirectory);
    }
}