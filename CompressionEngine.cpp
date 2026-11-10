#include "CompressionEngine.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <array>
#include <fstream>
#ifdef __APPLE__
#include <copyfile.h>
#endif
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;
namespace fs = std::filesystem;

namespace {
bool identicalFiles(const fs::path& left, const fs::path& right) {
    std::ifstream a(left, std::ios::binary), b(right, std::ios::binary);
    if (!a || !b) return false;
    std::array<char, 65536> first{}, second{};
    do {
        a.read(first.data(), first.size());
        b.read(second.data(), second.size());
        if (a.bad() || b.bad() || a.gcount() != b.gcount() ||
            !std::equal(first.begin(), first.begin() + a.gcount(), second.begin())) return false;
    } while (!a.eof() && !b.eof());
    return a.eof() && b.eof();
}
}

bool CompressionEngine::compress(const fs::path& input) {
    message.clear();
    error = true;
    fs::path temporaryDirectory;
    auto cleanup = [&] {
        std::error_code ec;
        if (!temporaryDirectory.empty()) fs::remove_all(temporaryDirectory, ec);
    };
    try {
        const auto source = fs::absolute(input);
        if (fs::is_symlink(source) || !fs::is_regular_file(source)) {
            message = "Input must be a regular file, not a symlink";
            return false;
        }
        if (access(source.c_str(), W_OK) != 0 || fs::hard_link_count(source) != 1) {
            message = "Input must be writable and have no additional hard links";
            return false;
        }
        auto extension = source.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg") {
            message = "Unsupported image format";
            return false;
        }
        const auto originalSize = fs::file_size(source);
        const auto originalTime = fs::last_write_time(source);
        const auto originalPermissions = fs::status(source).permissions();
        auto pattern = (source.parent_path() / ".compression-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        if (!mkdtemp(buffer.data())) {
            message = "Could not create temporary directory";
            return false;
        }
        temporaryDirectory = buffer.data();
        const auto candidate = temporaryDirectory / ("image" + extension);
        const auto snapshot = temporaryDirectory / ("original" + extension);
        fs::copy_file(source, snapshot);
        std::vector<std::string> arguments;
        if (extension == ".png") {
            // Recompress IDAT only: retain bit depth, color type, and ancillary chunks.
            arguments = {"optipng", "-quiet", "-o2", "-nx", "-out", candidate.string(), snapshot.string()};
        } else {
            arguments = {"jpegtran", "-copy", "all", "-optimize", "-outfile", candidate.string(), snapshot.string()};
        }
        std::vector<char*> argv;
        for (auto& argument : arguments) argv.push_back(argument.data());
        argv.push_back(nullptr);
        pid_t child;
        const int spawnError = posix_spawnp(&child, argv[0], nullptr, nullptr, argv.data(), environ);
        if (spawnError != 0) {
            message = "Cannot launch " + arguments.front() + "; install it and ensure it is on PATH";
            cleanup();
            return false;
        }
        int status = 0;
        pid_t waited;
        do { waited = waitpid(child, &status, 0); } while (waited == -1 && errno == EINTR);
        if (waited == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            message = "Image optimizer failed";
            cleanup();
            return false;
        }
        if (!fs::is_regular_file(candidate) || fs::file_size(candidate) == 0) {
            message = "Optimizer produced no image";
            cleanup();
            return false;
        }
        const auto compressedSize = fs::file_size(candidate);
        if (compressedSize >= originalSize) {
            message = "Already optimized; original unchanged";
            error = false;
            cleanup();
            return false;
        }
        if (fs::is_symlink(source) || !fs::is_regular_file(source) ||
            fs::hard_link_count(source) != 1 || fs::last_write_time(source) != originalTime ||
            fs::status(source).permissions() != originalPermissions ||
            !identicalFiles(source, snapshot)) {
            message = "Input changed during compression; original unchanged";
            cleanup();
            return false;
        }
#ifdef __APPLE__
        if (copyfile(source.c_str(), candidate.c_str(), nullptr, COPYFILE_METADATA) != 0) {
            message = "Could not preserve file metadata; original unchanged";
            cleanup();
            return false;
        }
#endif
        fs::permissions(candidate, originalPermissions);
        fs::last_write_time(candidate, originalTime);
        // Both paths are on the same filesystem: rename publishes the complete file atomically.
        fs::rename(candidate, source);
        message = "Saved " + std::to_string(originalSize - compressedSize) + " bytes";
        error = false;
        cleanup();
        return true;
    } catch (const fs::filesystem_error& error) {
        message = error.what();
        cleanup();
        return false;
    }
}
