#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include "VaultScanner.h"
#include "MediaScanner.h"
#include "CompressionEngine.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char* argv[]) {
    const bool compress = argc == 2 && string(argv[1]) == "--compress";
    if (argc > 1 && !compress) {
        cerr << "Usage: obsidian_optimizer [--compress]" << endl;
        return 1;
    }
    string filePath;
    cout << "Enter absolute path to 'vaultpaths.txt' ";
    getline(cin, filePath);

    ifstream file(filePath);

    if (!file.is_open()) {
        cerr << "Failed to open vault paths file!" << endl;
        return 1;
    }
    string line;

    vector<filesystem::path> roots = {};
    while (getline(file, line)) {
        roots.emplace_back(line);
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
        for (const auto& image : mediaFiles) {
            const auto output = image.parent_path() / ".optimized-media" / image.filename();
            if (engine.compress(image, output)) ++compressed;
            cout << image << ": " << engine.lastMessage() << endl;
        }
        cout << "Smaller copies created: " << compressed << endl;
    }

    return 0;
}
