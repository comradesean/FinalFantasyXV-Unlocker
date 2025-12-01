#include "PatternScanner.h"
#include <Psapi.h>
#include <algorithm>

std::optional<uintptr_t> PatternScanner::findPattern(
    HANDLE processHandle,
    uintptr_t startAddress,
    size_t searchSize,
    const std::vector<uint8_t>& pattern)
{
    if (!processHandle || pattern.empty()) {
        return std::nullopt;
    }

    // Read memory in chunks to avoid large allocations
    constexpr size_t CHUNK_SIZE = 0x10000; // 64KB chunks
    std::vector<uint8_t> buffer(CHUNK_SIZE + pattern.size());

    for (size_t offset = 0; offset < searchSize; offset += CHUNK_SIZE) {
        size_t bytesToRead = std::min(CHUNK_SIZE + pattern.size(), searchSize - offset);

        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(processHandle,
                               reinterpret_cast<LPCVOID>(startAddress + offset),
                               buffer.data(),
                               bytesToRead,
                               &bytesRead)) {
            continue; // Skip unreadable regions
        }

        // Search for pattern in this chunk
        for (size_t i = 0; i + pattern.size() <= bytesRead; ++i) {
            if (matchPattern(buffer.data(), bytesRead, pattern, i)) {
                return startAddress + offset + i;
            }
        }
    }

    return std::nullopt;
}

std::optional<uintptr_t> PatternScanner::findPatternInModule(
    HANDLE processHandle,
    const wchar_t* moduleName,
    const std::vector<uint8_t>& pattern)
{
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;

    if (!getModuleInfo(processHandle, moduleName, baseAddress, moduleSize)) {
        return std::nullopt;
    }

    return findPattern(processHandle, baseAddress, moduleSize, pattern);
}

bool PatternScanner::getModuleInfo(
    HANDLE processHandle,
    const wchar_t* moduleName,
    uintptr_t& baseAddress,
    size_t& moduleSize)
{
    HMODULE modules[1024];
    DWORD cbNeeded;

    if (!EnumProcessModulesEx(processHandle, modules, sizeof(modules), &cbNeeded, LIST_MODULES_ALL)) {
        return false;
    }

    int moduleCount = cbNeeded / sizeof(HMODULE);

    for (int i = 0; i < moduleCount; ++i) {
        wchar_t modName[MAX_PATH];
        if (GetModuleBaseNameW(processHandle, modules[i], modName, MAX_PATH)) {
            if (_wcsicmp(modName, moduleName) == 0) {
                MODULEINFO modInfo;
                if (GetModuleInformation(processHandle, modules[i], &modInfo, sizeof(modInfo))) {
                    baseAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                    moduleSize = modInfo.SizeOfImage;
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<uint8_t> PatternScanner::readMemory(
    HANDLE processHandle,
    uintptr_t address,
    size_t size)
{
    std::vector<uint8_t> buffer(size);
    SIZE_T bytesRead = 0;

    if (ReadProcessMemory(processHandle,
                          reinterpret_cast<LPCVOID>(address),
                          buffer.data(),
                          size,
                          &bytesRead)) {
        buffer.resize(bytesRead);
    } else {
        buffer.clear();
    }

    return buffer;
}

bool PatternScanner::matchPattern(
    const uint8_t* data,
    size_t dataSize,
    const std::vector<uint8_t>& pattern,
    size_t offset)
{
    if (offset + pattern.size() > dataSize) {
        return false;
    }

    for (size_t i = 0; i < pattern.size(); ++i) {
        if (data[offset + i] != pattern[i]) {
            return false;
        }
    }

    return true;
}
