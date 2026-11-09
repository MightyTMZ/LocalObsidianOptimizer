#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include "VaultScanner.h"

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

    VaultScanner scanner;

    // Find valid Obsidian vaults
    const auto vaults = scanner.discoverVaults(roots);

    for (const auto& vault:vaults) {
        cout << "Found vault: " << vault << endl;
    }

    return 0;
}