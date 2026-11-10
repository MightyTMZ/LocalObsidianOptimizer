#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include "VaultScanner.h"
#include "MediaScanner.h"

namespace fs = std::filesystem;
using namespace std;

int main() {
    string filePath;
    cout << "Enter absolute path to 'vaultpaths.txt' ";
    cin >> filePath;

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

    return 0;
}