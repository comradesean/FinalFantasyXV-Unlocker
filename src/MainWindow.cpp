/**
 * @file MainWindow.cpp
 * @brief Main window implementation for FFXV Unlocker
 *
 * This file implements the UI and control logic for unlocking exclusive items
 * in Final Fantasy XV Windows Edition. It handles two distinct unlock mechanisms:
 *
 * 1. BYTE TABLE UNLOCKS: Direct memory writes to the game's unlock byte table
 *    at base address 0x140752038. Each item has an enable byte that can be
 *    set to 0x01 (enabled) or 0x00 (disabled).
 *
 * 2. CODE PATCHES: AOB (Array of Bytes) pattern-based patches for items
 *    protected by the game's anti-tamper system. These modify executable
 *    code and cannot be individually toggled without crashing the game.
 *
 * The "Platform Exclusives" section uses code patches (Unlock 1, 2, 3) to
 * bypass ownership checks for Steam, Origin, and promotional items that
 * cannot be individually selected.
 */

#include "MainWindow.h"
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QMessageBox>
#include <QCloseEvent>
#include <QIcon>
#include <QScrollArea>

// ============================================================================
// Construction / Destruction
// ============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_memoryEditor(new MemoryEditor(this))
    , m_httpServer(new HttpServer(this))
    , m_processCheckTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    setupSystemTray();

    // Poll for game process every 2 seconds for auto-attach
    m_processCheckTimer->setInterval(2000);
    m_processCheckTimer->start();

    updateStatus();
    log("FFXV Unlocker initialized");
}

MainWindow::~MainWindow() = default;

// ============================================================================
// UI Setup
// ============================================================================

void MainWindow::setupUI()
{
    setWindowTitle("FFXV Unlocker");
    setWindowIcon(QIcon(":/icon.png"));
    setMinimumSize(450, 600);
    resize(450, 650);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // --- Status Section ---
    m_statusGroup = new QGroupBox("Status", this);
    auto* statusMainLayout = new QHBoxLayout(m_statusGroup);

    // Left: process info and attach/detach buttons
    auto* statusLeftLayout = new QVBoxLayout();
    m_processStatusLabel = new QLabel("Process: Not attached", this);
    m_serverStatusLabel = new QLabel("Server: Stopped", this);

    auto* buttonLayout = new QHBoxLayout();
    m_attachButton = new QPushButton("Attach", this);
    m_detachButton = new QPushButton("Detach", this);
    m_detachButton->setEnabled(false);
    buttonLayout->addWidget(m_attachButton);
    buttonLayout->addWidget(m_detachButton);
    buttonLayout->addStretch();

    statusLeftLayout->addWidget(m_processStatusLabel);
    statusLeftLayout->addWidget(m_serverStatusLabel);
    statusLeftLayout->addLayout(buttonLayout);

    // Right: URL redirect controls for Twitch Prime spoofing
    auto* urlGroup = new QGroupBox("Twitch URL Redirect", m_statusGroup);
    urlGroup->setStyleSheet("QGroupBox { font-size: 9pt; } QCheckBox { font-size: 8pt; }");
    urlGroup->setToolTip("Redirects Twitch API calls to local server,\nenabling Twitch Prime promotion.");

    auto* urlLayout = new QVBoxLayout(urlGroup);
    urlLayout->setSpacing(2);
    urlLayout->setContentsMargins(6, 6, 6, 6);

    m_serverCheck = new QCheckBox("Enable HTTP Server (port 443)", urlGroup);
    m_urlRedirectCheck = new QCheckBox("Redirect Twitch URLs to localhost", urlGroup);
    m_urlRedirectCheck->setEnabled(false);

    urlLayout->addWidget(m_serverCheck);
    urlLayout->addWidget(m_urlRedirectCheck);

    statusMainLayout->addLayout(statusLeftLayout, 1);
    statusMainLayout->addWidget(urlGroup, 0);
    mainLayout->addWidget(m_statusGroup);

    // --- Master Unlock All Checkbox ---
    m_unlockAllCheck = new QCheckBox("UNLOCK ALL ITEMS", this);
    m_unlockAllCheck->setEnabled(false);
    m_unlockAllCheck->setStyleSheet("QCheckBox { font-weight: bold; font-size: 11pt; }");
    mainLayout->addWidget(m_unlockAllCheck);

    // --- Scrollable Categories Section ---
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget();
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(5);

    // Category: Normally Unavailable (selectable via byte table)
    std::vector<QCheckBox*> normallyUnavailableChecks;
    scrollLayout->addWidget(createUnlockCategoryGroup(
        "Normally Unavailable",
        Patches::getNormallyUnavailableItems(),
        m_normallyUnavailableAllCheck,
        normallyUnavailableChecks));

    // Category: Origin Exclusives (selectable via byte table)
    std::vector<QCheckBox*> originChecks;
    scrollLayout->addWidget(createUnlockCategoryGroup(
        "Origin Exclusives",
        Patches::getOriginItems(),
        m_originAllCheck,
        originChecks));

    // Category: Microsoft Store (selectable via byte table)
    std::vector<QCheckBox*> msStoreChecks;
    scrollLayout->addWidget(createUnlockCategoryGroup(
        "Microsoft (UWP) Store Exclusive",
        Patches::getMicrosoftStoreItems(),
        m_msStoreAllCheck,
        msStoreChecks));

    // Category: Twitch Prime (uses bundles - paired items)
    auto* twitchPrimeGroup = new QGroupBox(scrollContent);
    twitchPrimeGroup->setTitle("");
    auto* twitchLayout = new QVBoxLayout(twitchPrimeGroup);
    twitchLayout->setSpacing(2);
    twitchLayout->setContentsMargins(6, 6, 6, 6);

    // Twitch Prime header with collapse button
    auto* twitchHeader = new QWidget(twitchPrimeGroup);
    auto* twitchHeaderLayout = new QHBoxLayout(twitchHeader);
    twitchHeaderLayout->setContentsMargins(0, 0, 0, 0);
    twitchHeaderLayout->setSpacing(4);

    auto* twitchCollapseBtn = new QToolButton(twitchHeader);
    twitchCollapseBtn->setArrowType(Qt::DownArrow);
    twitchCollapseBtn->setAutoRaise(true);
    twitchCollapseBtn->setFixedSize(16, 16);
    twitchCollapseBtn->setToolTip("Collapse/Expand");
    twitchHeaderLayout->addWidget(twitchCollapseBtn);

    m_twitchPrimeAllCheck = new QCheckBox("Enable All Twitch Prime Drops", twitchHeader);
    m_twitchPrimeAllCheck->setEnabled(false);
    m_twitchPrimeAllCheck->setStyleSheet("QCheckBox { font-weight: bold; }");
    twitchHeaderLayout->addWidget(m_twitchPrimeAllCheck);
    twitchHeaderLayout->addStretch();
    twitchLayout->addWidget(twitchHeader);

    // Twitch Prime bundle checkboxes (indented)
    auto* twitchIndent = new QWidget(twitchPrimeGroup);
    auto* twitchIndentLayout = new QVBoxLayout(twitchIndent);
    twitchIndentLayout->setContentsMargins(20, 0, 0, 0);
    twitchIndentLayout->setSpacing(1);

    for (auto* bundle : Patches::getTwitchPrimeBundles()) {
        auto* check = new QCheckBox(QString::fromStdString(bundle->name), twitchIndent);
        check->setEnabled(false);
        check->setToolTip(QString::fromStdString(bundle->description));
        check->setProperty("unlockBundle", QVariant::fromValue(reinterpret_cast<quintptr>(bundle)));
        twitchIndentLayout->addWidget(check);
        m_bundleCheckboxes[bundle] = check;
        m_allBundleChecks.push_back(check);
    }
    twitchLayout->addWidget(twitchIndent);

    m_collapseButtons[twitchCollapseBtn] = twitchIndent;
    connect(twitchCollapseBtn, &QToolButton::clicked, this, [this, twitchCollapseBtn, twitchIndent]() {
        toggleCategoryCollapse(twitchCollapseBtn, twitchIndent);
    });
    scrollLayout->addWidget(twitchPrimeGroup);

    // Category: Steam Exclusives (NOT selectable - requires code patches)
    std::vector<QCheckBox*> steamChecks;
    scrollLayout->addWidget(createUnlockCategoryGroup(
        "Steam Exclusives",
        Patches::getSteamItems(),
        m_steamAllCheck,
        steamChecks,
        true,   // disableCategoryCheck: cannot individually select
        true)); // startCollapsed: less visual clutter

    // Category: Promotional Items (NOT selectable - requires code patches)
    std::vector<QCheckBox*> promotionalChecks;
    scrollLayout->addWidget(createUnlockCategoryGroup(
        "Promotional Items",
        Patches::getPromotionalItems(),
        m_promotionalAllCheck,
        promotionalChecks,
        true,   // disableCategoryCheck
        true)); // startCollapsed

    scrollLayout->addStretch();
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // --- Platform Exclusives Section (Code Patches) ---
    m_unlockAllExclusivesGroup = new QGroupBox(this);
    m_unlockAllExclusivesGroup->setTitle("");
    m_unlockAllExclusivesGroup->setToolTip(
        "Steam Exclusives and Promotional items cannot be individually selected\n"
        "due to FFXV's anti-tamper protection. Use this option to unlock them all.");

    auto* exclusivesLayout = new QVBoxLayout(m_unlockAllExclusivesGroup);
    exclusivesLayout->setSpacing(2);
    exclusivesLayout->setContentsMargins(6, 6, 6, 6);

    // Header with collapse button
    auto* exclusivesHeader = new QWidget(m_unlockAllExclusivesGroup);
    auto* exclusivesHeaderLayout = new QHBoxLayout(exclusivesHeader);
    exclusivesHeaderLayout->setContentsMargins(0, 0, 0, 0);
    exclusivesHeaderLayout->setSpacing(4);

    auto* exclusivesCollapseBtn = new QToolButton(exclusivesHeader);
    exclusivesCollapseBtn->setArrowType(Qt::RightArrow);  // Start collapsed
    exclusivesCollapseBtn->setAutoRaise(true);
    exclusivesCollapseBtn->setFixedSize(16, 16);
    exclusivesCollapseBtn->setToolTip("Collapse/Expand");
    exclusivesHeaderLayout->addWidget(exclusivesCollapseBtn);

    auto* exclusivesLabel = new QLabel("Unlock All Platform Exclusives", exclusivesHeader);
    exclusivesLabel->setStyleSheet("QLabel { font-weight: bold; }");
    exclusivesHeaderLayout->addWidget(exclusivesLabel);
    exclusivesHeaderLayout->addStretch();
    exclusivesLayout->addWidget(exclusivesHeader);

    // Two mutually exclusive options for platform exclusives
    auto* exclusivesContent = new QWidget(m_unlockAllExclusivesGroup);
    auto* exclusivesContentLayout = new QVBoxLayout(exclusivesContent);
    exclusivesContentLayout->setContentsMargins(20, 4, 0, 0);
    exclusivesContentLayout->setSpacing(2);

    m_unlockWithoutWorkshopCheck = new QCheckBox("Everything without Steam Workshop", exclusivesContent);
    m_unlockWithoutWorkshopCheck->setToolTip(
        "Unlocks all Steam Exclusives, Origin Exclusives, MS Store, and Promotional items.\n"
        "Does NOT unlock Steam Workshop items (HEV Suit, Scientist Glasses, Crowbar variants).\n"
        "Recommended for single-player only.");
    m_unlockWithoutWorkshopCheck->setEnabled(false);

    m_unlockWithWorkshopCheck = new QCheckBox("Everything with Steam Workshop", exclusivesContent);
    m_unlockWithWorkshopCheck->setToolTip(
        "Unlocks ALL exclusive items including Steam Workshop variants.\n"
        "Warning: May affect multiplayer/workshop functionality.");
    m_unlockWithWorkshopCheck->setEnabled(false);

    exclusivesContentLayout->addWidget(m_unlockWithoutWorkshopCheck);
    exclusivesContentLayout->addWidget(m_unlockWithWorkshopCheck);
    exclusivesLayout->addWidget(exclusivesContent);
    exclusivesContent->setVisible(false);  // Start collapsed

    m_collapseButtons[exclusivesCollapseBtn] = exclusivesContent;
    connect(exclusivesCollapseBtn, &QToolButton::clicked, this, [exclusivesCollapseBtn, exclusivesContent]() {
        bool isVisible = exclusivesContent->isVisible();
        exclusivesContent->setVisible(!isVisible);
        exclusivesCollapseBtn->setArrowType(isVisible ? Qt::RightArrow : Qt::DownArrow);
    });
    mainLayout->addWidget(m_unlockAllExclusivesGroup);

    // Store category checkboxes for iteration
    m_allCategoryChecks = {
        m_normallyUnavailableAllCheck,
        m_twitchPrimeAllCheck,
        m_steamAllCheck,
        m_originAllCheck,
        m_msStoreAllCheck,
        m_promotionalAllCheck
    };

    // --- Log Section ---
    m_logGroup = new QGroupBox("Log", this);
    auto* logLayout = new QVBoxLayout(m_logGroup);

    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(120);
    m_logText->setStyleSheet("QTextEdit { font-family: Consolas, monospace; font-size: 9pt; }");
    logLayout->addWidget(m_logText);

    mainLayout->addWidget(m_logGroup);
}

QGroupBox* MainWindow::createUnlockCategoryGroup(
    const QString& title,
    std::vector<Patches::UnlockItem*> items,
    QCheckBox*& allCheck,
    std::vector<QCheckBox*>& itemChecks,
    bool disableCategoryCheck,
    bool startCollapsed)
{
    auto* group = new QGroupBox(this);
    group->setTitle("");
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(2);
    layout->setContentsMargins(6, 6, 6, 6);

    // Header with collapse button and "Enable All" checkbox
    auto* headerWidget = new QWidget(group);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto* collapseBtn = new QToolButton(headerWidget);
    collapseBtn->setArrowType(startCollapsed ? Qt::RightArrow : Qt::DownArrow);
    collapseBtn->setAutoRaise(true);
    collapseBtn->setFixedSize(16, 16);
    collapseBtn->setToolTip("Collapse/Expand");
    headerLayout->addWidget(collapseBtn);

    allCheck = new QCheckBox(QString("Enable All %1").arg(title), headerWidget);
    allCheck->setEnabled(false);

    if (disableCategoryCheck) {
        // Items in this category require code patches, not byte table writes
        allCheck->setStyleSheet("QCheckBox { font-weight: bold; color: gray; }");
        allCheck->setToolTip("Use 'Unlock All Platform Exclusives' to unlock these items");
        allCheck->setProperty("permanentlyDisabled", true);
    } else {
        allCheck->setStyleSheet("QCheckBox { font-weight: bold; }");
    }
    allCheck->setProperty("category", title);
    headerLayout->addWidget(allCheck);
    headerLayout->addStretch();
    layout->addWidget(headerWidget);

    // Individual item checkboxes (indented, collapsible)
    auto* indentWidget = new QWidget(group);
    auto* indentLayout = new QVBoxLayout(indentWidget);
    indentLayout->setContentsMargins(20, 0, 0, 0);
    indentLayout->setSpacing(1);

    for (auto* item : items) {
        auto* check = new QCheckBox(QString::fromStdString(item->name), indentWidget);
        check->setEnabled(false);
        check->setToolTip(QString::fromStdString(item->description));
        check->setProperty("unlockItem", QVariant::fromValue(reinterpret_cast<quintptr>(item)));
        check->setProperty("categoryCheck", QVariant::fromValue(reinterpret_cast<quintptr>(allCheck)));
        check->setProperty("selectable", item->selectable);

        if (!item->selectable) {
            // Anti-tamper protected: visual indication and extended tooltip
            check->setStyleSheet("QCheckBox { color: gray; }");
            check->setToolTip(QString::fromStdString(item->description) +
                "\n\nThis item cannot be individually selected due to FFXV's anti-tamper protection.\n"
                "Use 'Unlock All Platform Exclusives' option instead.");
        }

        indentLayout->addWidget(check);
        itemChecks.push_back(check);
        m_unlockCheckboxes[item] = check;
        m_allItemChecks.push_back(check);
    }
    layout->addWidget(indentWidget);

    if (startCollapsed) {
        indentWidget->setVisible(false);
    }

    m_collapseButtons[collapseBtn] = indentWidget;
    connect(collapseBtn, &QToolButton::clicked, this, [this, collapseBtn, indentWidget]() {
        toggleCategoryCollapse(collapseBtn, indentWidget);
    });

    return group;
}

void MainWindow::toggleCategoryCollapse(QToolButton* button, QWidget* content)
{
    bool willExpand = !content->isVisible();
    content->setVisible(willExpand);
    button->setArrowType(willExpand ? Qt::DownArrow : Qt::RightArrow);
}

// ============================================================================
// Signal/Slot Connections
// ============================================================================

void MainWindow::setupConnections()
{
    // Process management
    connect(m_attachButton, &QPushButton::clicked, this, &MainWindow::onAttachClicked);
    connect(m_detachButton, &QPushButton::clicked, this, &MainWindow::onDetachClicked);
    connect(m_processCheckTimer, &QTimer::timeout, this, &MainWindow::checkForProcess);

    // Memory editor signals
    connect(m_memoryEditor, &MemoryEditor::processAttached, this, &MainWindow::onProcessAttached);
    connect(m_memoryEditor, &MemoryEditor::processDetached, this, &MainWindow::onProcessDetached);
    connect(m_memoryEditor, &MemoryEditor::patchApplied, this, &MainWindow::onPatchApplied);
    connect(m_memoryEditor, &MemoryEditor::patchRemoved, this, &MainWindow::onPatchRemoved);
    connect(m_memoryEditor, &MemoryEditor::unlockEnabled, this, &MainWindow::onUnlockEnabled);
    connect(m_memoryEditor, &MemoryEditor::unlockDisabled, this, &MainWindow::onUnlockDisabled);
    connect(m_memoryEditor, &MemoryEditor::bundleEnabled, this, &MainWindow::onBundleEnabled);
    connect(m_memoryEditor, &MemoryEditor::bundleDisabled, this, &MainWindow::onBundleDisabled);
    connect(m_memoryEditor, &MemoryEditor::errorOccurred, this, &MainWindow::onError);

    // HTTP server signals
    connect(m_httpServer, &HttpServer::serverStarted, this, &MainWindow::onServerStarted);
    connect(m_httpServer, &HttpServer::serverStopped, this, &MainWindow::onServerStopped);
    connect(m_httpServer, &HttpServer::requestReceived, this, &MainWindow::onRequestReceived);
    connect(m_httpServer, &HttpServer::errorOccurred, this, &MainWindow::onError);

    // URL redirect toggles
    connect(m_serverCheck, &QCheckBox::toggled, this, &MainWindow::onServerToggled);
    connect(m_urlRedirectCheck, &QCheckBox::toggled, this, &MainWindow::onURLRedirectToggled);

    // Master unlock all
    connect(m_unlockAllCheck, &QCheckBox::toggled, this, &MainWindow::onUnlockAllToggled);

    // Platform exclusives (mutually exclusive options)
    connect(m_unlockWithoutWorkshopCheck, &QCheckBox::toggled, this, &MainWindow::onUnlockWithoutWorkshopToggled);
    connect(m_unlockWithWorkshopCheck, &QCheckBox::toggled, this, &MainWindow::onUnlockWithWorkshopToggled);

    // Category toggles
    for (auto* categoryCheck : m_allCategoryChecks) {
        if (categoryCheck == m_twitchPrimeAllCheck) {
            // Twitch Prime uses bundles, not individual items
            connect(categoryCheck, &QCheckBox::toggled, this, &MainWindow::onTwitchPrimeCategoryToggled);
        } else {
            connect(categoryCheck, &QCheckBox::toggled, this, &MainWindow::onCategoryToggled);
        }
    }

    // Individual item toggles
    for (auto* itemCheck : m_allItemChecks) {
        connect(itemCheck, &QCheckBox::toggled, this, &MainWindow::onIndividualUnlockToggled);
    }

    // Bundle toggles
    for (auto* bundleCheck : m_allBundleChecks) {
        connect(bundleCheck, &QCheckBox::toggled, this, &MainWindow::onBundleToggled);
    }
}

void MainWindow::setupSystemTray()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icon.png"));
    m_trayIcon->setToolTip("FFXV Unlocker");

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("Show", this, [this]() {
        show();
        raise();
        activateWindow();
    });
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Exit", qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

// ============================================================================
// Process Management
// ============================================================================

void MainWindow::onAttachClicked()
{
    if (!m_memoryEditor->isAttached()) {
        m_autoAttach = true;  // Re-enable auto-attach when manually attaching
        log("Attempting to attach to ffxv_s.exe...");
        m_memoryEditor->attachToProcess(TARGET_PROCESS);
    }
}

void MainWindow::onDetachClicked()
{
    if (!m_memoryEditor->isAttached()) return;

    m_autoAttach = false;  // Disable auto-attach until Attach is clicked again

    // Clean up: disable all active unlocks and patches before detaching
    auto allItems = Patches::getAllUnlockItems();
    m_memoryEditor->disableAllUnlocks(allItems);

    auto bundles = Patches::getTwitchPrimeBundles();
    m_memoryEditor->disableAllBundles(bundles);

    auto urlPatches = Patches::getURLPatches();
    m_memoryEditor->removeAllPatches(urlPatches);

    removeUnlockAllExclusives();

    m_memoryEditor->detach();
}

void MainWindow::checkForProcess()
{
    bool wasAttached = m_memoryEditor->isAttached();

    if (!wasAttached && m_autoAttach) {
        // Auto-attach when game is detected
        m_memoryEditor->attachToProcess(TARGET_PROCESS);
    } else if (wasAttached && !m_memoryEditor->isAttached()) {
        // Game was closed while we were attached
        onProcessDetached();
    }
}

// ============================================================================
// Server & URL Redirect Handlers
// ============================================================================

void MainWindow::onServerToggled(bool checked)
{
    if (checked) {
        if (m_httpServer->start(443)) {
            m_urlRedirectCheck->setEnabled(m_memoryEditor->isAttached());
        } else {
            m_serverCheck->setChecked(false);
        }
    } else {
        m_httpServer->stop();
        m_urlRedirectCheck->setEnabled(false);
        m_urlRedirectCheck->setChecked(false);
    }
}

void MainWindow::onURLRedirectToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    auto urlPatches = Patches::getURLPatches();
    if (checked) {
        m_memoryEditor->applyAllPatches(urlPatches);
    } else {
        m_memoryEditor->removeAllPatches(urlPatches);
    }
}

// ============================================================================
// Unlock Control Handlers
// ============================================================================

void MainWindow::onUnlockAllToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    // Block all signals during bulk update to prevent cascading
    for (auto* catCheck : m_allCategoryChecks) catCheck->blockSignals(true);
    for (auto* itemCheck : m_allItemChecks) itemCheck->blockSignals(true);
    for (auto* bundleCheck : m_allBundleChecks) bundleCheck->blockSignals(true);

    // Only operate on selectable items (non-selectable require Platform Exclusives)
    auto selectableItems = Patches::getSelectableItems();
    auto allBundles = Patches::getTwitchPrimeBundles();

    if (checked) {
        m_memoryEditor->enableAllUnlocks(selectableItems);
        m_memoryEditor->enableAllBundles(allBundles);
    } else {
        m_memoryEditor->disableAllUnlocks(selectableItems);
        m_memoryEditor->disableAllBundles(allBundles);
    }

    // Update UI to reflect new state (only enabled checkboxes)
    for (auto* catCheck : m_allCategoryChecks) {
        if (catCheck->isEnabled()) {
            catCheck->setChecked(checked);
        }
    }
    for (auto& [item, checkbox] : m_unlockCheckboxes) {
        if (item->selectable) {
            checkbox->setChecked(checked);
        }
    }
    for (auto& [bundle, checkbox] : m_bundleCheckboxes) {
        checkbox->setChecked(checked);
    }

    // Restore signal handling
    for (auto* catCheck : m_allCategoryChecks) catCheck->blockSignals(false);
    for (auto* itemCheck : m_allItemChecks) itemCheck->blockSignals(false);
    for (auto* bundleCheck : m_allBundleChecks) bundleCheck->blockSignals(false);
}

void MainWindow::onCategoryToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    auto* sender = qobject_cast<QCheckBox*>(QObject::sender());
    if (!sender) return;

    // Map sender to category items
    std::vector<Patches::UnlockItem*> items;
    if (sender == m_normallyUnavailableAllCheck) {
        items = Patches::getNormallyUnavailableItems();
    } else if (sender == m_steamAllCheck) {
        items = Patches::getSteamItems();
    } else if (sender == m_originAllCheck) {
        items = Patches::getOriginItems();
    } else if (sender == m_msStoreAllCheck) {
        items = Patches::getMicrosoftStoreItems();
    } else if (sender == m_promotionalAllCheck) {
        items = Patches::getPromotionalItems();
    }

    if (items.empty()) return;

    // Filter to selectable items only
    std::vector<Patches::UnlockItem*> selectableItems;
    for (auto* item : items) {
        if (item->selectable) {
            selectableItems.push_back(item);
        }
    }

    // Apply changes to memory
    if (checked) {
        m_memoryEditor->enableAllUnlocks(selectableItems);
    } else {
        m_memoryEditor->disableAllUnlocks(selectableItems);
    }

    // Update individual checkboxes
    for (auto* item : items) {
        auto it = m_unlockCheckboxes.find(item);
        if (it != m_unlockCheckboxes.end() && item->selectable) {
            it->second->blockSignals(true);
            it->second->setChecked(checked);
            it->second->blockSignals(false);
        }
    }

    updateMasterUnlockCheckbox();
}

void MainWindow::onIndividualUnlockToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    auto* sender = qobject_cast<QCheckBox*>(QObject::sender());
    if (!sender) return;

    auto itemPtr = sender->property("unlockItem").value<quintptr>();
    auto* item = reinterpret_cast<Patches::UnlockItem*>(itemPtr);
    if (!item) return;

    // Non-selectable items cannot be individually toggled
    if (!item->selectable) {
        sender->blockSignals(true);
        sender->setChecked(false);
        sender->blockSignals(false);
        return;
    }

    if (checked) {
        m_memoryEditor->enableUnlock(*item);
    } else {
        m_memoryEditor->disableUnlock(*item);
    }

    // Update category checkbox state
    auto catPtr = sender->property("categoryCheck").value<quintptr>();
    auto* categoryCheck = reinterpret_cast<QCheckBox*>(catPtr);

    if (categoryCheck) {
        bool allInCategoryChecked = true;
        for (auto& [itm, cb] : m_unlockCheckboxes) {
            auto cbCatPtr = cb->property("categoryCheck").value<quintptr>();
            if (cbCatPtr == catPtr && !cb->isChecked()) {
                allInCategoryChecked = false;
                break;
            }
        }
        categoryCheck->blockSignals(true);
        categoryCheck->setChecked(allInCategoryChecked);
        categoryCheck->blockSignals(false);
    }

    // Update master checkbox
    bool allChecked = true;
    for (auto* catCheck : m_allCategoryChecks) {
        if (!catCheck->isChecked()) {
            allChecked = false;
            break;
        }
    }
    m_unlockAllCheck->blockSignals(true);
    m_unlockAllCheck->setChecked(allChecked);
    m_unlockAllCheck->blockSignals(false);
}

void MainWindow::onBundleToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    auto* sender = qobject_cast<QCheckBox*>(QObject::sender());
    if (!sender) return;

    auto bundlePtr = sender->property("unlockBundle").value<quintptr>();
    auto* bundle = reinterpret_cast<Patches::UnlockBundle*>(bundlePtr);
    if (!bundle) return;

    // Show informational popup once per session when enabling Twitch Prime items
    if (checked && !m_twitchPrimeWarningShown) {
        m_twitchPrimeWarningShown = true;
        QMessageBox::information(this, "Twitch Prime Rewards",
            "These items can also be unlocked using the Twitch URL Redirect feature, "
            "which simulates the original Twitch Prime login flow.\n\n"
            "To use the web-based method:\n"
            "1. Enable the HTTP Server (port 443)\n"
            "2. Enable \"Redirect Twitch URLs to localhost\"\n"
            "3. Access the Twitch Prime menu in-game\n\n"
            "The direct memory unlock you're using now works immediately, "
            "but the web-based method provides a more authentic experience.");
    }

    if (checked) {
        m_memoryEditor->enableBundle(*bundle);
    } else {
        m_memoryEditor->disableBundle(*bundle);
    }

    // Update Twitch Prime category checkbox
    bool allBundlesChecked = true;
    for (auto& [b, cb] : m_bundleCheckboxes) {
        if (!cb->isChecked()) {
            allBundlesChecked = false;
            break;
        }
    }
    m_twitchPrimeAllCheck->blockSignals(true);
    m_twitchPrimeAllCheck->setChecked(allBundlesChecked);
    m_twitchPrimeAllCheck->blockSignals(false);

    updateMasterUnlockCheckbox();
}

void MainWindow::onTwitchPrimeCategoryToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    // Show informational popup once per session when enabling Twitch Prime items
    if (checked && !m_twitchPrimeWarningShown) {
        m_twitchPrimeWarningShown = true;
        QMessageBox::information(this, "Twitch Prime Rewards",
            "These items can also be unlocked using the Twitch URL Redirect feature, "
            "which simulates the original Twitch Prime login flow.\n\n"
            "To use the web-based method:\n"
            "1. Enable the HTTP Server (port 443)\n"
            "2. Enable \"Redirect Twitch URLs to localhost\"\n"
            "3. Access the Twitch Prime menu in-game\n\n"
            "The direct memory unlock you're using now works immediately, "
            "but the web-based method provides a more authentic experience.");
    }

    auto bundles = Patches::getTwitchPrimeBundles();

    if (checked) {
        m_memoryEditor->enableAllBundles(bundles);
    } else {
        m_memoryEditor->disableAllBundles(bundles);
    }

    // Update individual bundle checkboxes
    for (auto* bundle : bundles) {
        auto it = m_bundleCheckboxes.find(bundle);
        if (it != m_bundleCheckboxes.end()) {
            it->second->blockSignals(true);
            it->second->setChecked(checked);
            it->second->blockSignals(false);
        }
    }

    updateMasterUnlockCheckbox();
}

// ============================================================================
// Platform Exclusives Handlers
// ============================================================================

void MainWindow::onUnlockWithoutWorkshopToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    if (checked) {
        // Uncheck the mutually exclusive option
        m_unlockWithWorkshopCheck->blockSignals(true);
        m_unlockWithWorkshopCheck->setChecked(false);
        m_unlockWithWorkshopCheck->blockSignals(false);

        // Disable all other unlock controls (Platform Exclusives takes over)
        disableAndUncheckControl(m_unlockAllCheck);
        for (auto* catCheck : m_allCategoryChecks) disableAndUncheckControl(catCheck);
        for (auto& [item, checkbox] : m_unlockCheckboxes) disableAndUncheckControl(checkbox);
        for (auto& [bundle, checkbox] : m_bundleCheckboxes) disableAndUncheckControl(checkbox);

        // Clear any active byte table unlocks
        auto allItems = Patches::getAllUnlockItems();
        m_memoryEditor->disableAllUnlocks(allItems);
        auto allBundles = Patches::getTwitchPrimeBundles();
        m_memoryEditor->disableAllBundles(allBundles);

        applyUnlockAllExclusives(false);  // Unlock 3 only
    } else {
        if (!m_unlockWithWorkshopCheck->isChecked()) {
            removeUnlockAllExclusives();
            restoreUnlockControlStates();
        }
    }
}

void MainWindow::onUnlockWithWorkshopToggled(bool checked)
{
    if (!m_memoryEditor->isAttached()) return;

    if (checked) {
        // Uncheck the mutually exclusive option
        m_unlockWithoutWorkshopCheck->blockSignals(true);
        m_unlockWithoutWorkshopCheck->setChecked(false);
        m_unlockWithoutWorkshopCheck->blockSignals(false);

        // Disable all other unlock controls
        disableAndUncheckControl(m_unlockAllCheck);
        for (auto* catCheck : m_allCategoryChecks) disableAndUncheckControl(catCheck);
        for (auto& [item, checkbox] : m_unlockCheckboxes) disableAndUncheckControl(checkbox);
        for (auto& [bundle, checkbox] : m_bundleCheckboxes) disableAndUncheckControl(checkbox);

        // Clear any active byte table unlocks
        auto allItems = Patches::getAllUnlockItems();
        m_memoryEditor->disableAllUnlocks(allItems);
        auto allBundles = Patches::getTwitchPrimeBundles();
        m_memoryEditor->disableAllBundles(allBundles);

        applyUnlockAllExclusives(true);  // Unlock 1 + Unlock 2
    } else {
        if (!m_unlockWithoutWorkshopCheck->isChecked()) {
            removeUnlockAllExclusives();
            restoreUnlockControlStates();
        }
    }
}

void MainWindow::applyUnlockAllExclusives(bool withWorkshop)
{
    if (!m_memoryEditor->isAttached()) return;

    removeUnlockAllExclusives();  // Clear any existing patches first

    if (withWorkshop) {
        // Unlock 1 + Unlock 2: Everything including Steam Workshop items
        for (auto* patch : Patches::getUnlockAllWithWorkshopPatches()) {
            if (m_memoryEditor->applyPatch(*patch)) {
                log(QString("Applied: %1").arg(QString::fromStdString(patch->name)));
            }
        }
        log("Platform exclusives unlocked (with Steam Workshop)");
    } else {
        // Unlock 3: Everything except Steam Workshop items
        for (auto* patch : Patches::getUnlockAllWithoutWorkshopPatches()) {
            if (m_memoryEditor->applyPatch(*patch)) {
                log(QString("Applied: %1").arg(QString::fromStdString(patch->name)));
            }
        }
        log("Platform exclusives unlocked (without Steam Workshop)");
    }
}

void MainWindow::removeUnlockAllExclusives()
{
    if (!m_memoryEditor->isAttached()) return;

    // Remove in reverse order of dependencies
    auto* unlock3 = Patches::getUnlock3Patch();
    if (unlock3->enabled) {
        m_memoryEditor->removePatch(*unlock3);
        log("Removed: Unlock 3 - DL Bypass");
    }

    auto* unlock2 = Patches::getUnlock2Patch();
    auto* unlock1 = Patches::getUnlock1Patch();
    if (unlock2->enabled) {
        m_memoryEditor->removePatch(*unlock2);
        log("Removed: Unlock 2 - Steam Bypass");
    }
    if (unlock1->enabled) {
        m_memoryEditor->removePatch(*unlock1);
        log("Removed: Unlock 1 - Bounds Bypass");
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void MainWindow::disableAndUncheckControl(QCheckBox* checkbox)
{
    checkbox->blockSignals(true);
    checkbox->setChecked(false);
    checkbox->setEnabled(false);
    checkbox->blockSignals(false);
}

void MainWindow::updateMasterUnlockCheckbox()
{
    bool allCategoriesChecked = true;
    for (auto* catCheck : m_allCategoryChecks) {
        // Only consider enabled categories (excludes permanently disabled Steam/Promotional)
        if (catCheck->isEnabled() && !catCheck->isChecked()) {
            allCategoriesChecked = false;
            break;
        }
    }
    m_unlockAllCheck->blockSignals(true);
    m_unlockAllCheck->setChecked(allCategoriesChecked);
    m_unlockAllCheck->blockSignals(false);
}

void MainWindow::restoreUnlockControlStates()
{
    m_unlockAllCheck->setEnabled(true);

    for (auto* catCheck : m_allCategoryChecks) {
        // Steam and Promotional categories remain permanently disabled
        if (catCheck != m_steamAllCheck && catCheck != m_promotionalAllCheck) {
            catCheck->setEnabled(true);
        }
    }

    for (auto& [item, checkbox] : m_unlockCheckboxes) {
        if (item->selectable) {
            checkbox->setEnabled(true);
        }
    }

    for (auto& [bundle, checkbox] : m_bundleCheckboxes) {
        checkbox->setEnabled(true);
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

void MainWindow::onProcessAttached(const QString& name, DWORD pid)
{
    log(QString("Attached to %1 (PID: %2)").arg(name).arg(pid));
    updateStatus();

    m_urlRedirectCheck->setEnabled(m_httpServer->isRunning());
    setUnlocksEnabled(true);

    m_attachButton->setEnabled(false);
    m_detachButton->setEnabled(true);
}

void MainWindow::onProcessDetached()
{
    log("Detached from process");
    updateStatus();

    // Reset URL redirect controls
    m_urlRedirectCheck->setChecked(false);
    m_urlRedirectCheck->setEnabled(false);

    // Reset all unlock checkboxes without triggering signals
    m_unlockAllCheck->blockSignals(true);
    m_unlockAllCheck->setChecked(false);
    m_unlockAllCheck->blockSignals(false);

    m_unlockWithoutWorkshopCheck->blockSignals(true);
    m_unlockWithoutWorkshopCheck->setChecked(false);
    m_unlockWithoutWorkshopCheck->blockSignals(false);

    m_unlockWithWorkshopCheck->blockSignals(true);
    m_unlockWithWorkshopCheck->setChecked(false);
    m_unlockWithWorkshopCheck->blockSignals(false);

    for (auto* catCheck : m_allCategoryChecks) {
        catCheck->blockSignals(true);
        catCheck->setChecked(false);
        catCheck->blockSignals(false);
    }

    for (auto& [item, checkbox] : m_unlockCheckboxes) {
        checkbox->blockSignals(true);
        checkbox->setChecked(false);
        item->enabled = false;
        checkbox->blockSignals(false);
    }

    for (auto& [bundle, checkbox] : m_bundleCheckboxes) {
        checkbox->blockSignals(true);
        checkbox->setChecked(false);
        bundle->enabled = false;
        checkbox->blockSignals(false);
    }

    // Reset all patch states
    for (auto* patch : Patches::getAllPatches()) {
        patch->enabled = false;
    }

    setUnlocksEnabled(false);

    m_attachButton->setEnabled(true);
    m_detachButton->setEnabled(false);
}

void MainWindow::setUnlocksEnabled(bool enabled)
{
    m_unlockAllCheck->setEnabled(enabled);
    m_unlockWithoutWorkshopCheck->setEnabled(enabled);
    m_unlockWithWorkshopCheck->setEnabled(enabled);

    for (auto* catCheck : m_allCategoryChecks) {
        bool permanentlyDisabled = catCheck->property("permanentlyDisabled").toBool();
        catCheck->setEnabled(enabled && !permanentlyDisabled);
    }

    for (auto* itemCheck : m_allItemChecks) {
        bool selectable = itemCheck->property("selectable").toBool();
        itemCheck->setEnabled(enabled && selectable);
    }

    for (auto* bundleCheck : m_allBundleChecks) {
        bundleCheck->setEnabled(enabled);
    }
}

void MainWindow::onPatchApplied(const QString& name)
{
    log(QString("Patch applied: %1").arg(name));
}

void MainWindow::onPatchRemoved(const QString& name)
{
    log(QString("Patch removed: %1").arg(name));
}

void MainWindow::onUnlockEnabled(const QString& name)
{
    log(QString("Unlock enabled: %1").arg(name));
}

void MainWindow::onUnlockDisabled(const QString& name)
{
    log(QString("Unlock disabled: %1").arg(name));
}

void MainWindow::onBundleEnabled(const QString& name)
{
    log(QString("Bundle enabled: %1").arg(name));
}

void MainWindow::onBundleDisabled(const QString& name)
{
    log(QString("Bundle disabled: %1").arg(name));
}

void MainWindow::onServerStarted(quint16 port)
{
    log(QString("HTTP server started on port %1").arg(port));
    updateStatus();
}

void MainWindow::onServerStopped()
{
    log("HTTP server stopped");
    updateStatus();
}

void MainWindow::onRequestReceived(const QString& method, const QString& path)
{
    log(QString("[HTTP] %1 %2").arg(method).arg(path));
}

void MainWindow::onError(const QString& error)
{
    log(QString("[ERROR] %1").arg(error));
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    }
}

// ============================================================================
// UI Updates
// ============================================================================

void MainWindow::updateStatus()
{
    if (m_memoryEditor->isAttached()) {
        m_processStatusLabel->setText(QString("Process: %1 (PID: %2)")
            .arg(QString::fromStdWString(m_memoryEditor->getProcessName()))
            .arg(m_memoryEditor->getProcessId()));
        m_processStatusLabel->setStyleSheet("QLabel { color: green; }");
    } else {
        m_processStatusLabel->setText("Process: Not attached");
        m_processStatusLabel->setStyleSheet("QLabel { color: red; }");
    }

    if (m_httpServer->isRunning()) {
        m_serverStatusLabel->setText(QString("Server: Running on port %1").arg(m_httpServer->port()));
        m_serverStatusLabel->setStyleSheet("QLabel { color: green; }");
    } else {
        m_serverStatusLabel->setText("Server: Stopped");
        m_serverStatusLabel->setStyleSheet("QLabel { color: gray; }");
    }
}

void MainWindow::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logText->append(QString("[%1] %2").arg(timestamp).arg(message));
}
