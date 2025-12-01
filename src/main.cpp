#include <QApplication>
#include <QMessageBox>
#include <Windows.h>
#include "MainWindow.h"

bool isRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin;
}

bool requestElevation()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.hwnd = nullptr;
    sei.nShow = SW_NORMAL;

    return ShellExecuteExW(&sei);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FFXV Unlocker");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FFXVUnlocker");

    // Check for admin privileges (required for memory editing)
    if (!isRunningAsAdmin()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            nullptr,
            "Administrator Required",
            "This application requires administrator privileges to modify game memory.\n\n"
            "Would you like to restart with elevated privileges?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            if (requestElevation()) {
                return 0; // Exit this instance, elevated one will start
            } else {
                QMessageBox::critical(nullptr, "Error",
                    "Failed to request administrator privileges.");
                return 1;
            }
        } else {
            QMessageBox::warning(nullptr, "Warning",
                "Running without administrator privileges.\n"
                "Memory editing features will not work.");
        }
    }

    MainWindow window;
    window.show();

    return app.exec();
}
