#pragma once

#include <QObject>
#include <QString>
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include "Patches.h"

/**
 * @brief Memory manipulation interface for FFXV process
 *
 * Provides safe memory read/write operations with automatic memory
 * protection handling. Supports two types of modifications:
 *
 * 1. AOB Pattern Patches: Scans for byte patterns and modifies code
 * 2. Byte Table Writes: Direct writes to known addresses for unlock items
 *
 * Thread Safety: Not thread-safe. All operations should be called from
 * the main Qt thread.
 */
class MemoryEditor : public QObject {
    Q_OBJECT

public:
    explicit MemoryEditor(QObject* parent = nullptr);
    ~MemoryEditor();

    // === Process Management ===
    bool attachToProcess(const std::wstring& processName);
    void detach();
    bool isAttached() const;
    std::wstring getProcessName() const;
    DWORD getProcessId() const;

    // === AOB Pattern-Based Patches ===
    bool applyPatch(Patches::Patch& patch);
    bool removePatch(Patches::Patch& patch);
    bool togglePatch(Patches::Patch& patch);
    bool applyAllPatches(std::vector<Patches::Patch*>& patches);
    bool removeAllPatches(std::vector<Patches::Patch*>& patches);
    bool isPatchApplied(const Patches::Patch& patch) const;

    // === Direct Byte Table Unlocks ===
    bool enableUnlock(Patches::UnlockItem& item);
    bool disableUnlock(Patches::UnlockItem& item);
    bool toggleUnlock(Patches::UnlockItem& item);
    bool enableAllUnlocks(std::vector<Patches::UnlockItem*>& items);
    bool disableAllUnlocks(std::vector<Patches::UnlockItem*>& items);
    bool isUnlockEnabled(const Patches::UnlockItem& item) const;

    // === Bundle Operations (Multiple Addresses) ===
    bool enableBundle(Patches::UnlockBundle& bundle);
    bool disableBundle(Patches::UnlockBundle& bundle);
    bool toggleBundle(Patches::UnlockBundle& bundle);
    bool enableAllBundles(std::vector<Patches::UnlockBundle*>& bundles);
    bool disableAllBundles(std::vector<Patches::UnlockBundle*>& bundles);
    bool isBundleEnabled(const Patches::UnlockBundle& bundle) const;

    // === Low-Level Access ===
    bool writeByte(uintptr_t address, uint8_t value);
    uint8_t readByte(uintptr_t address);
    std::string getLastError() const;

signals:
    void processAttached(const QString& processName, DWORD pid);
    void processDetached();
    void patchApplied(const QString& patchName);
    void patchRemoved(const QString& patchName);
    void unlockEnabled(const QString& itemName);
    void unlockDisabled(const QString& itemName);
    void bundleEnabled(const QString& bundleName);
    void bundleDisabled(const QString& bundleName);
    void errorOccurred(const QString& error);

private:
    // Process state
    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    std::wstring m_processName;
    std::string m_lastError;

    // Pattern cache: avoids rescanning for same patterns
    std::map<std::string, uintptr_t> m_patternCache;

    // Internal helpers
    DWORD findProcessByName(const std::wstring& processName);
    uintptr_t findPatternAddress(const Patches::Patch& patch);
    bool writeMemory(uintptr_t address, const std::vector<uint8_t>& data);
    std::vector<uint8_t> readMemory(uintptr_t address, size_t size);
    bool writeProtectedMemory(uintptr_t address, const std::vector<uint8_t>& data);
    bool setMemoryProtection(uintptr_t address, size_t size, DWORD newProtection, DWORD& oldProtection);
};
