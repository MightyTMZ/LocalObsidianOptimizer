
#include "VaultScanner.h"

#include <filesystem>
#include <vector>
#include <unordered_set>
#include <system_error>

using namespace std;
namespace fs = std::filesystem;

const vector<fs::path> VaultScanner::discoverVaults(
    const vector<fs::path>& roots
) {
    vector<fs::path> vaults;
    unordered_set<fs::path> discovered;

    for (const auto& root : roots) {
        error_code ec;

        if (!fs::is_directory(root, ec)) {
            continue;
        }

        // Check the root directory itself.
        if (isValidVault(root)) {
            fs::path normalized = fs::weakly_canonical(root, ec);

            if (!ec && discovered.insert(normalized).second) {
                vaults.push_back(normalized);
            }
        }

        // Recursively search subdirectories.
        fs::recursive_directory_iterator it(
            root,
            fs::directory_options::skip_permission_denied,
            ec
        );

        fs::recursive_directory_iterator end;

        while (it != end) {
            const fs::directory_entry& entry = *it;

            // Skip .obsidian and .git directories.
            if (entry.path().filename() == ".obsidian" ||
                entry.path().filename() == ".git") {
                it.disable_recursion_pending();
                }
            else if (entry.is_directory(ec) && !ec) {
                if (isValidVault(entry.path())) {
                    fs::path normalized =
                        fs::weakly_canonical(entry.path(), ec);

                    if (!ec && discovered.insert(normalized).second) {
                        vaults.push_back(normalized);
                    }
                }
            }

            ec.clear();
            it.increment(ec);

            if (ec) {
                ec.clear();
            }
        }
    }

    return vaults;
}

bool VaultScanner::isValidVault(const fs::path& path) {
    error_code ec;

    return fs::is_directory(path / ".obsidian", ec) && !ec;
}
