#include "CompressionEngine.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;
namespace fs = std::filesystem;

bool CompressionEngine::compress(const fs::path& input, const fs::path& output) {
    message.clear();
    fs::path temporaryDirectory;
    auto cleanup = [&] {
        std::error_code ec;
        if (!temporaryDirectory.empty()) fs::remove_all(temporaryDirectory, ec);
    };
    try {
        const auto source = fs::absolute(input);
        const auto destination = fs::absolute(output);
        if (fs::is_symlink(source) || !fs::is_regular_file(source)) {
            message = "Input must be a regular file, not a symlink";
            return false;
        }
        if (fs::exists(destination) || fs::is_symlink(destination)) {
            message = "Output already exists";
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
        fs::create_directories(destination.parent_path());
        auto pattern = (destination.parent_path() / ".compression-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        if (!mkdtemp(buffer.data())) {
            message = "Could not create temporary directory";
            return false;
        }
        temporaryDirectory = buffer.data();
        const auto candidate = temporaryDirectory / ("image" + extension);
        std::vector<std::string> arguments;
        if (extension == ".png") {
            // Recompress IDAT only: retain bit depth, color type, and ancillary chunks.
            arguments = {"optipng", "-quiet", "-o2", "-nx", "-out", candidate.string(), source.string()};
        } else {
            arguments = {"jpegtran", "-copy", "all", "-optimize", "-outfile", candidate.string(), source.string()};
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
            message = "Already optimized; no smaller copy produced";
            cleanup();
            return false;
        }
        // Publish without replacing any existing file, even if created during compression.
        fs::create_hard_link(candidate, destination);
        message = "Saved " + std::to_string(originalSize - compressedSize) + " bytes";
        cleanup();
        return true;
    } catch (const fs::filesystem_error& error) {
        message = error.what();
        cleanup();
        return false;
    }
}
