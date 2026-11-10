#include "MediaScanner.h"

vector<filesystem::path> MediaScanner::discoverMedia(
        const vector<filesystem::path>& vaults
    ) {
    vector<filesystem::path> mediaFiles;
    unordered_set<filesystem::path> discovered;

    for (const auto& vault : vaults) {
        error_code ec;

        if (!filesystem::is_directory(vault, ec)) {
            continue;
        }

        filesystem::recursive_directory_iterator it(
            vault,
            filesystem::directory_options::skip_permission_denied,
            ec
        );

        filesystem::recursive_directory_iterator end;

        while (it != end) {
            const auto& entry = *it;
            const auto& path = entry.path();

            if (entry.is_directory(ec) && !ec) {
                // Don't traverse Obsidian configuration or Git directories.
                if (path.filename() == ".obsidian" ||
                    path.filename() == ".git" ||
                    path.filename() == ".optimized-media" ||
                    path.filename().string().starts_with(".compression-")) {
                    it.disable_recursion_pending();
                    }
            }
            else if (!ec && !entry.is_symlink(ec) && !ec &&
                     entry.is_regular_file(ec) && !ec &&
                     isSupportedMedia(path)) {

                // Normalize paths to avoid duplicates.
                error_code canonicalEc;
                auto normalized =
                    filesystem::weakly_canonical(path, canonicalEc);

                if (!canonicalEc &&
                    discovered.insert(normalized).second) {
                    mediaFiles.push_back(normalized);
                    }
                     }

            ec.clear();
            it.increment(ec);

            if (ec) {
                ec.clear();
            }
        }
    }

    return mediaFiles;
}

bool MediaScanner::isSupportedMedia(const filesystem::path &path) {
    string extension = path.extension().string();

    transform(extension.begin(), extension.end(), extension.begin(),
              [](unsigned char c) { return tolower(c); });

    return supportedExtensions.contains(extension);
}
