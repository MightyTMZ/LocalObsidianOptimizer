#ifndef OBSIDIAN_OPTIMIZER_COMPRESSIONENGINE_H
#define OBSIDIAN_OPTIMIZER_COMPRESSIONENGINE_H

#include <filesystem>
#include <string>

class CompressionEngine {
public:
    // Writes a smaller lossless copy. Never overwrites input or an existing output.
    // Returns false for errors or when compression cannot reduce the file size.
    bool compress(const std::filesystem::path& input,
                  const std::filesystem::path& output);
    const std::string& lastMessage() const { return message; }

private:
    std::string message;
};

#endif
