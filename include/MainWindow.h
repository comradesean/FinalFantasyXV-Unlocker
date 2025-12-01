#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <vector>
#include <map>

#include "MemoryEditor.h"
#include "HttpServer.h"
#include "Patches.h"

/**
 * @brief Main application window for FFXV Unlocker
 *
 * Provides UI for:
 * - Process attachment/detachment to ffxv_s.exe
 * - HTTP server control for Twitch Prime URL spoofing
 * - Individual unlock item toggles (byte table modifications)
 * - Platform exclusive unlock patches (code-level modifications)
 *
 * Architecture notes:
 * - Uses two unlock mechanisms:
 *   1. Byte table: Direct memory writes for selectable items
 *   2. Code patches: AOB-based patches for anti-tamper protected items
 * - Steam/Promotional items cannot be individually selected due to game's
 *   anti-tamper protection; they require the "Platform Exclusives" patches
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // === Process Management ===
    void onAttachClicked();
    void onDetachClicked();
    void checkForProcess();

    // === Server & URL Redirect ===
    void onServerToggled(bool checked);
    void onURLRedirectToggled(bool checked);

    // === Unlock Controls ===
    void onUnlockAllToggled(bool checked);
    void onCategoryToggled(bool checked);
    void onIndividualUnlockToggled(bool checked);
    void onBundleToggled(bool checked);
    void onTwitchPrimeCategoryToggled(bool checked);

    // === Platform Exclusives (mutually exclusive options) ===
    void onUnlockWithoutWorkshopToggled(bool checked);
    void onUnlockWithWorkshopToggled(bool checked);

    // === Event Handlers ===
    void onProcessAttached(const QString& name, DWORD pid);
    void onProcessDetached();
    void onPatchApplied(const QString& name);
    void onPatchRemoved(const QString& name);
    void onUnlockEnabled(const QString& name);
    void onUnlockDisabled(const QString& name);
    void onBundleEnabled(const QString& name);
    void onBundleDisabled(const QString& name);
    void onServerStarted(quint16 port);
    void onServerStopped();
    void onRequestReceived(const QString& method, const QString& path);
    void onError(const QString& error);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    // === Initialization ===
    void setupUI();
    void setupConnections();
    void setupSystemTray();

    // === UI Helpers ===

    /**
     * @brief Creates a collapsible group box for a category of unlock items
     * @param title Category display name
     * @param items List of unlock items in this category
     * @param allCheck Output: reference to the "Enable All" checkbox
     * @param itemChecks Output: list of individual item checkboxes
     * @param disableCategoryCheck If true, category checkbox is permanently disabled
     *        (used for Steam/Promotional items that require code patches)
     * @param startCollapsed If true, content starts hidden
     */
    QGroupBox* createUnlockCategoryGroup(
        const QString& title,
        std::vector<Patches::UnlockItem*> items,
        QCheckBox*& allCheck,
        std::vector<QCheckBox*>& itemChecks,
        bool disableCategoryCheck = false,
        bool startCollapsed = false);

    void toggleCategoryCollapse(QToolButton* button, QWidget* content);
    void updateStatus();
    void setUnlocksEnabled(bool enabled);
    void log(const QString& message);

    // === Platform Exclusives Patch Management ===
    void applyUnlockAllExclusives(bool withWorkshop);
    void removeUnlockAllExclusives();

    // === Checkbox State Helpers ===

    /**
     * @brief Blocks signals, unchecks, and disables a checkbox
     * Used when Platform Exclusives are enabled to prevent conflicts
     */
    void disableAndUncheckControl(QCheckBox* checkbox);

    /**
     * @brief Updates the master "Unlock All" checkbox based on category states
     * Only considers enabled categories (excludes Steam/Promotional)
     */
    void updateMasterUnlockCheckbox();

    /**
     * @brief Re-enables unlock controls after Platform Exclusives are disabled
     * Respects original enable states (selectable items, non-Steam/Promotional)
     */
    void restoreUnlockControlStates();

    // === Core Components ===
    MemoryEditor* m_memoryEditor;
    HttpServer* m_httpServer;
    QTimer* m_processCheckTimer;

    // === UI Widgets ===
    QWidget* m_centralWidget;

    // Status section
    QGroupBox* m_statusGroup;
    QLabel* m_processStatusLabel;
    QLabel* m_serverStatusLabel;
    QPushButton* m_attachButton;
    QPushButton* m_detachButton;

    // URL redirect controls
    QCheckBox* m_urlRedirectCheck;
    QCheckBox* m_serverCheck;

    // Master unlock control
    QCheckBox* m_unlockAllCheck;

    // Platform exclusives section
    QGroupBox* m_unlockAllExclusivesGroup;
    QCheckBox* m_unlockWithoutWorkshopCheck;  // Unlock 3 (no Workshop items)
    QCheckBox* m_unlockWithWorkshopCheck;     // Unlock 1+2 (includes Workshop)

    // Category "Enable All" checkboxes
    QCheckBox* m_normallyUnavailableAllCheck;
    QCheckBox* m_twitchPrimeAllCheck;
    QCheckBox* m_steamAllCheck;
    QCheckBox* m_originAllCheck;
    QCheckBox* m_msStoreAllCheck;
    QCheckBox* m_promotionalAllCheck;

    // Checkbox mappings for programmatic access
    std::map<Patches::UnlockItem*, QCheckBox*> m_unlockCheckboxes;
    std::map<Patches::UnlockBundle*, QCheckBox*> m_bundleCheckboxes;

    // Checkbox collections for bulk operations
    std::vector<QCheckBox*> m_allCategoryChecks;
    std::vector<QCheckBox*> m_allItemChecks;
    std::vector<QCheckBox*> m_allBundleChecks;

    // Collapse button to content widget mapping
    std::map<QToolButton*, QWidget*> m_collapseButtons;

    // Log section
    QGroupBox* m_logGroup;
    QTextEdit* m_logText;

    // System tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;

    // === State ===
    bool m_autoAttach = true;  // Auto-attach on startup, disabled on manual detach
    bool m_twitchPrimeWarningShown = false;  // Show info popup once per session
    static constexpr const wchar_t* TARGET_PROCESS = L"ffxv_s.exe";
};
