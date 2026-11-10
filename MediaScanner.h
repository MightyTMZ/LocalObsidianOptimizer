#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#ifndef OBSIDIAN_OPTIMIZER_MEDIASCANNER_H
#define OBSIDIAN_OPTIMIZER_MEDIASCANNER_H

using namespace std;
namespace fs = std::filesystem;

class MediaScanner {
public:
    vector<filesystem::path> discoverMedia(
        const vector<filesystem::path>& vaults
    );
private:
    const unordered_set<string> supportedExtensions = {
        ".png",
        ".jpg",
        ".jpeg",
    };
    bool isSupportedMedia(
        const filesystem::path& path
    );
};


#endif //OBSIDIAN_OPTIMIZER_MEDIASCANNER_H
