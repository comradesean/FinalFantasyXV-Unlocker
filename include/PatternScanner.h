#pragma once

#include <Windows.h>
#include <vector>
#include <cstdint>
#include <optional>

class PatternScanner {
public:
    // Find a pattern in the target process memory
    // Returns the address where pattern was found, or nullopt if not found
    static std::optional<uintptr_t> findPattern(
        HANDLE processHandle,
        uintptr_t startAddress,
        size_t searchSize,
        const std::vector<uint8_t>& pattern
    );

    // Find pattern in a specific module
    static std::optional<uintptr_t> findPatternInModule(
        HANDLE processHandle,
        const wchar_t* moduleName,
        const std::vector<uint8_t>& pattern
    );

    // Get module base address and size
    static bool getModuleInfo(
        HANDLE processHandle,
        const wchar_t* moduleName,
        uintptr_t& baseAddress,
        size_t& moduleSize
    );

private:
    // Read memory from target process
    static std::vector<uint8_t> readMemory(
        HANDLE processHandle,
        uintptr_t address,
        size_t size
    );

    // Pattern matching helper
    static bool matchPattern(
        const uint8_t* data,
        size_t dataSize,
        const std::vector<uint8_t>& pattern,
        size_t offset
    );
};
