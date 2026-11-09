#pragma once

#ifndef OBSIDIAN_OPTIMIZER_VAULTSCANNER_H
#define OBSIDIAN_OPTIMIZER_VAULTSCANNER_H

#include <filesystem>
#include <vector>

using namespace std;

class VaultScanner {
public:
    const vector<filesystem::path> discoverVaults(
        const vector<filesystem::path>& roots
    );

private:
    bool isValidVault(const filesystem::path& path);
};


#endif //OBSIDIAN_OPTIMIZER_VAULTSCANNER_H
