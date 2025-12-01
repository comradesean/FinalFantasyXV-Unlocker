/**
 * @file MemoryEditor.cpp
 * @brief Memory manipulation interface for FFXV process
 *
 * Provides safe memory read/write operations for:
 * - AOB (Array of Bytes) pattern-based code patches
 * - Direct byte table writes for unlock items
 * - Bundle operations (multiple addresses per unlock)
 *
 * Memory Protection:
 * All write operations temporarily change page protection to PAGE_EXECUTE_READWRITE
 * and restore the original protection after writing. This is required because
 * game code sections are typically marked as read-only/execute.
 *
 * Pattern Caching:
 * Found patterns are cached by name to avoid repeated scans. Cache is cleared
 * on detach to ensure fresh scans on next attach (in case game memory changed).
 */

#include "MemoryEditor.h"
#include "PatternScanner.h"
#include <TlHelp32.h>
#include <Psapi.h>

// ============================================================================
// Construction / Destruction
// ============================================================================

MemoryEditor::MemoryEditor(QObject* parent)
    : QObject(parent)
{
}

MemoryEditor::~MemoryEditor()
{
    detach();
}

// ============================================================================
// Process Attachment
// ============================================================================

bool MemoryEditor::attachToProcess(const std::wstring& processName)
{
    if (m_processHandle) {
        detach();
    }

    DWORD pid = findProcessByName(processName);
    if (pid == 0) {
        m_lastError = "Process not found: " + std::string(processName.begin(), processName.end());
        return false;
    }

    m_processHandle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE,
        pid
    );

    if (!m_processHandle) {
        m_lastError = "Failed to open process. Run as administrator? Error: " + std::to_string(GetLastError());
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    m_processId = pid;
    m_processName = processName;
    m_patternCache.clear();

    emit processAttached(QString::fromStdWString(processName), pid);
    return true;
}

void MemoryEditor::detach()
{
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
        m_processId = 0;
        m_processName.clear();
        m_patternCache.clear();
        emit processDetached();
    }
}

bool MemoryEditor::isAttached() const
{
    if (!m_processHandle) return false;

    // Verify process is still running
    DWORD exitCode;
    if (GetExitCodeProcess(m_processHandle, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

std::wstring MemoryEditor::getProcessName() const
{
    return m_processName;
}

DWORD MemoryEditor::getProcessId() const
{
    return m_processId;
}

std::string MemoryEditor::getLastError() const
{
    return m_lastError;
}

// ============================================================================
// AOB Pattern-Based Patches
// ============================================================================

bool MemoryEditor::applyPatch(Patches::Patch& patch)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    uintptr_t address = findPatternAddress(patch);
    if (address == 0) {
        m_lastError = "Pattern not found: " + patch.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    // Apply offset to get actual patch location
    address += patch.offset;

    if (!writeProtectedMemory(address, patch.patched)) {
        m_lastError = "Failed to write patch: " + patch.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    patch.enabled = true;
    emit patchApplied(QString::fromStdString(patch.name));
    return true;
}

bool MemoryEditor::removePatch(Patches::Patch& patch)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    // Try cache first, then rescan
    uintptr_t address = 0;
    auto it = m_patternCache.find(patch.name);
    if (it != m_patternCache.end()) {
        address = it->second;
    } else {
        address = findPatternAddress(patch);
    }

    if (address == 0) {
        m_lastError = "Cannot find patch location: " + patch.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    address += patch.offset;

    if (!writeProtectedMemory(address, patch.original)) {
        m_lastError = "Failed to restore original bytes: " + patch.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    patch.enabled = false;
    emit patchRemoved(QString::fromStdString(patch.name));
    return true;
}

bool MemoryEditor::togglePatch(Patches::Patch& patch)
{
    return patch.enabled ? removePatch(patch) : applyPatch(patch);
}

bool MemoryEditor::applyAllPatches(std::vector<Patches::Patch*>& patches)
{
    bool allSuccess = true;
    for (auto* patch : patches) {
        if (!patch->enabled && !applyPatch(*patch)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::removeAllPatches(std::vector<Patches::Patch*>& patches)
{
    bool allSuccess = true;
    for (auto* patch : patches) {
        if (patch->enabled && !removePatch(*patch)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::isPatchApplied(const Patches::Patch& patch) const
{
    return patch.enabled;
}

// ============================================================================
// Direct Memory Unlock Operations (Byte Table)
// ============================================================================

bool MemoryEditor::enableUnlock(Patches::UnlockItem& item)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    if (!writeByte(item.address, 0x01)) {
        m_lastError = "Failed to enable unlock: " + item.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    item.enabled = true;
    emit unlockEnabled(QString::fromStdString(item.name));
    return true;
}

bool MemoryEditor::disableUnlock(Patches::UnlockItem& item)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    if (!writeByte(item.address, 0x00)) {
        m_lastError = "Failed to disable unlock: " + item.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    item.enabled = false;
    emit unlockDisabled(QString::fromStdString(item.name));
    return true;
}

bool MemoryEditor::toggleUnlock(Patches::UnlockItem& item)
{
    return item.enabled ? disableUnlock(item) : enableUnlock(item);
}

bool MemoryEditor::enableAllUnlocks(std::vector<Patches::UnlockItem*>& items)
{
    bool allSuccess = true;
    for (auto* item : items) {
        if (!item->enabled && !enableUnlock(*item)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::disableAllUnlocks(std::vector<Patches::UnlockItem*>& items)
{
    bool allSuccess = true;
    for (auto* item : items) {
        if (item->enabled && !disableUnlock(*item)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::isUnlockEnabled(const Patches::UnlockItem& item) const
{
    return item.enabled;
}

// ============================================================================
// Bundle Operations (Multiple Addresses per Unlock)
// ============================================================================

bool MemoryEditor::enableBundle(Patches::UnlockBundle& bundle)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    bool allSuccess = true;
    for (uintptr_t addr : bundle.addresses) {
        if (!writeByte(addr, 0x01)) {
            allSuccess = false;
        }
    }

    if (allSuccess) {
        bundle.enabled = true;
        emit bundleEnabled(QString::fromStdString(bundle.name));
    } else {
        m_lastError = "Failed to enable bundle (partial): " + bundle.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
    }

    return allSuccess;
}

bool MemoryEditor::disableBundle(Patches::UnlockBundle& bundle)
{
    if (!isAttached()) {
        m_lastError = "Not attached to process";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return false;
    }

    bool allSuccess = true;
    for (uintptr_t addr : bundle.addresses) {
        if (!writeByte(addr, 0x00)) {
            allSuccess = false;
        }
    }

    if (allSuccess) {
        bundle.enabled = false;
        emit bundleDisabled(QString::fromStdString(bundle.name));
    } else {
        m_lastError = "Failed to disable bundle (partial): " + bundle.name;
        emit errorOccurred(QString::fromStdString(m_lastError));
    }

    return allSuccess;
}

bool MemoryEditor::toggleBundle(Patches::UnlockBundle& bundle)
{
    return bundle.enabled ? disableBundle(bundle) : enableBundle(bundle);
}

bool MemoryEditor::enableAllBundles(std::vector<Patches::UnlockBundle*>& bundles)
{
    bool allSuccess = true;
    for (auto* bundle : bundles) {
        if (!bundle->enabled && !enableBundle(*bundle)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::disableAllBundles(std::vector<Patches::UnlockBundle*>& bundles)
{
    bool allSuccess = true;
    for (auto* bundle : bundles) {
        if (bundle->enabled && !disableBundle(*bundle)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool MemoryEditor::isBundleEnabled(const Patches::UnlockBundle& bundle) const
{
    return bundle.enabled;
}

// ============================================================================
// Low-Level Memory Operations
// ============================================================================

bool MemoryEditor::writeByte(uintptr_t address, uint8_t value)
{
    if (!isAttached()) return false;

    DWORD oldProtection;
    if (!setMemoryProtection(address, 1, PAGE_EXECUTE_READWRITE, oldProtection)) {
        return false;
    }

    SIZE_T bytesWritten;
    bool success = WriteProcessMemory(
        m_processHandle,
        reinterpret_cast<LPVOID>(address),
        &value,
        1,
        &bytesWritten
    ) && bytesWritten == 1;

    // Always restore protection, even if write failed
    DWORD temp;
    setMemoryProtection(address, 1, oldProtection, temp);

    return success;
}

uint8_t MemoryEditor::readByte(uintptr_t address)
{
    if (!isAttached()) return 0;

    uint8_t value = 0;
    SIZE_T bytesRead;
    ReadProcessMemory(
        m_processHandle,
        reinterpret_cast<LPCVOID>(address),
        &value,
        1,
        &bytesRead
    );
    return value;
}

bool MemoryEditor::writeMemory(uintptr_t address, const std::vector<uint8_t>& data)
{
    SIZE_T bytesWritten;
    return WriteProcessMemory(
        m_processHandle,
        reinterpret_cast<LPVOID>(address),
        data.data(),
        data.size(),
        &bytesWritten
    ) && bytesWritten == data.size();
}

std::vector<uint8_t> MemoryEditor::readMemory(uintptr_t address, size_t size)
{
    std::vector<uint8_t> buffer(size);
    SIZE_T bytesRead;

    if (ReadProcessMemory(
            m_processHandle,
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

// ============================================================================
// Internal Helpers
// ============================================================================

DWORD MemoryEditor::findProcessByName(const std::wstring& processName)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        m_lastError = "Failed to create process snapshot";
        emit errorOccurred(QString::fromStdString(m_lastError));
        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    DWORD pid = 0;
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return pid;
}

uintptr_t MemoryEditor::findPatternAddress(const Patches::Patch& patch)
{
    // Check cache first to avoid expensive rescans
    auto it = m_patternCache.find(patch.name);
    if (it != m_patternCache.end()) {
        return it->second;
    }

    // Scan for pattern in main game module
    auto result = PatternScanner::findPatternInModule(
        m_processHandle,
        L"ffxv_s.exe",
        patch.pattern
    );

    if (result.has_value()) {
        m_patternCache[patch.name] = result.value();
        return result.value();
    }

    return 0;
}

bool MemoryEditor::writeProtectedMemory(uintptr_t address, const std::vector<uint8_t>& data)
{
    DWORD oldProtection;
    if (!setMemoryProtection(address, data.size(), PAGE_EXECUTE_READWRITE, oldProtection)) {
        m_lastError = "Failed to change memory protection";
        return false;
    }

    bool success = writeMemory(address, data);

    // Always restore protection
    DWORD temp;
    setMemoryProtection(address, data.size(), oldProtection, temp);

    return success;
}

bool MemoryEditor::setMemoryProtection(uintptr_t address, size_t size, DWORD newProtection, DWORD& oldProtection)
{
    return VirtualProtectEx(
        m_processHandle,
        reinterpret_cast<LPVOID>(address),
        size,
        newProtection,
        &oldProtection
    );
}
