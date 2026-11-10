#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include "VaultScanner.h"
#include "MediaScanner.h"
#include "CompressionEngine.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char* argv[]) {
    bool compress = false;
    string filePath;
    for (int i = 1; i < argc; ++i) {
        const string argument = argv[i];
        if (argument == "--help") {
            cout << "Usage: obsidian_optimizer [--compress] [--vaults-file PATH]\n"
                    "--compress replaces original images when lossless compression saves space.\n"
                    "Use --vaults-file for unattended runs. Relative vault paths are\n"
                    "resolved against the vault paths file's directory.\n";
            return 0;
        } else if (argument == "--compress") {
            compress = true;
        } else if (argument == "--vaults-file" && i + 1 < argc && filePath.empty()) {
            filePath = argv[++i];
        } else {
            cerr << "Invalid argument: " << argument << ". Use --help.\n";
            return 2;
        }
    }
    if (filePath.empty()) {
        if (!isatty(STDIN_FILENO)) {
            cerr << "Unattended runs require --vaults-file PATH\n";
            return 2;
        }
        cout << "Enter absolute path to 'vaultpaths.txt' ";
        getline(cin, filePath);
    }

    ifstream file(filePath);

    if (!file.is_open()) {
        cerr << "Failed to open vault paths file!" << endl;
        return 1;
    }
    string line;

    vector<filesystem::path> roots = {};
    const auto configDirectory = fs::absolute(filePath).parent_path();
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        auto root = fs::path(line);
        if (root.is_relative()) root = configDirectory / root;
        error_code ec;
        if (!fs::is_directory(root, ec)) {
            cerr << "Vault root is unavailable: " << root << endl;
            return 1;
        }
        roots.push_back(root);
    }
    if (file.bad() || roots.empty()) {
        cerr << "Vault paths file is empty or could not be read\n";
        return 1;
    }
    file.close();

    cout << "Roots loaded: " << roots.size() << endl;

    VaultScanner vaultScanner;

    cout << "Discovering vaults..." << endl;
    const auto vaults = vaultScanner.discoverVaults(roots);

    cout << "Vaults discovered: " << vaults.size() << endl;

    MediaScanner mediaScanner;

    cout << "Discovering media..." << endl;
    const auto mediaFiles = mediaScanner.discoverMedia(vaults);

    cout << "Media files discovered: " << mediaFiles.size() << endl;

    for (auto& mFile: mediaFiles)
        cout << mFile << endl;

    if (compress) {
        CompressionEngine engine;
        size_t compressed = 0;
        bool failed = false;
        for (const auto& image : mediaFiles) {
            if (engine.compress(image)) ++compressed;
            failed = failed || engine.hadError();
            cout << image << ": " << engine.lastMessage() << endl;
        }
        cout << "Images compressed in place: " << compressed << endl;
        if (failed) return 1;
    }

    return 0;
}
