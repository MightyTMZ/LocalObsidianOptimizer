#ifndef OBSIDIAN_OPTIMIZER_COMPRESSIONENGINE_H
#define OBSIDIAN_OPTIMIZER_COMPRESSIONENGINE_H

#include <filesystem>
#include <string>

class CompressionEngine {
public:
    // Atomically replaces input only when lossless optimization makes it smaller.
    // Returns false for errors or when compression cannot reduce the file size.
    bool compress(const std::filesystem::path& input);
    const std::string& lastMessage() const { return message; }
    bool hadError() const { return error; }

private:
    std::string message;
    bool error = false;
};

#endif
