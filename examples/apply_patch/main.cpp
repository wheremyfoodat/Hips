#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>

#include "../../include/hips.hpp"
#include "../utils/io_file.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::printf("Invalid arguments. Usage: ./main <input file path> <patch path>\n");
        return -1;
    }

    // Retrieve the paths of the input file and the patch file
    auto inputPath = std::filesystem::path(argv[1]);
    auto patchPath = std::filesystem::path(argv[2]);

    IOFile input(inputPath, "rb");
    IOFile patch(patchPath, "rb");

    if (!input.isOpen() || !patch.isOpen()) {
        std::printf("Failed to open input or patch file.\n");
        return -1;
    }

    // Read the files into arrays
    const auto inputSize = *input.size();
    const auto patchSize = *patch.size();

    std::unique_ptr<uint8_t[]> inputData(new uint8_t[inputSize]);
    std::unique_ptr<uint8_t[]> patchData(new uint8_t[patchSize]);

    input.rewind();
    patch.rewind();
    input.readBytes(inputData.get(), inputSize);
    patch.readBytes(patchData.get(), patchSize);

    // Finally, detect the patch type, do the patching and check for errors
    Hips::PatchType patchType;
    auto extension = patchPath.extension();

    if (extension == ".ips") {
        patchType = Hips::PatchType::IPS;
    } else if (extension == ".ups") {
        patchType = Hips::PatchType::UPS;
    } else if (extension == ".bps") {
        patchType = Hips::PatchType::BPS;
    } else {
        printf("Unknown patch format\n");
        return -1;
    }

    auto [bytes, result] = Hips::patch(inputData.get(), inputSize, patchData.get(), patchSize, patchType);
    if (result == Hips::Result::Success) {
        printf("Patch applied successfully\n");
        return 0;
    } else {
        printf("Patching failed :(\n)");
        return -1;
    }
}