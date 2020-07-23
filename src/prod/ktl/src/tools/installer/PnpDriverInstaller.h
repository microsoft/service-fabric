/*++

Copyright (c) Microsoft Corporation

Module Name:

    PnpDriverInstaller.h

Abstract:

    Functions related to installing and controlling 
    a pnp KMDF driver.

Environment:

    User-mode

--*/

#pragma once

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
// your application.  The macros work by enabling all features available on platform versions up to and 
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                  // Specifies that the minimum required platform is Windows 2000.
#define WINVER 0x0500           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows 2000.
#define _WIN32_WINNT 0x0500     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_IE               // Specifies that the minimum required platform is Internet Explorer 5.0.
#define _WIN32_IE 0x0500        // Change this to the appropriate value to target other versions of IE.
#endif

#ifndef _WIN32_MSI              // Specifies that the minimum required MSI version is MSI 3.1
#define _WIN32_MSI 310          // Change this to the appropriate value to target other versions of MSI.
#endif

// Windows Header Files:
#include <windows.h>
#include <strsafe.h>
#include <string>
// WiX Header Files:
#include <msiquery.h>
#include <LM.h>
#include "tchar.h"

#include <wdfinstaller.h>
#include <cfgmgr32.h>
#include <newdev.h>
#include <sddl.h>
#include <vector>

//
// UpdateDriverForPlugAndPlayDevices
//
typedef BOOL(WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(
    __in HWND hwndParent,
    __in LPCTSTR HardwareId,
    __in LPCTSTR FullInfPath,
    __in DWORD InstallFlags,
    __out_opt PBOOL bRebootRequired
    );

typedef std::vector<std::wstring> StringCollection;

#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesA"
#endif

struct GenericContext {
    DWORD count;
    DWORD control;
    BOOL  reboot;
    LPCTSTR strSuccess;
    LPCTSTR strReboot;
    LPCTSTR strFail;
};

struct InfAndDevNode
{
    LPCWSTR Inf;
    LPCWSTR Devnode;
};

UINT __stdcall InstallDrivers();
UINT __stdcall UninstallDrivers();

class PnpDriverInstaller
{
public:
    static DWORD TestSignDrivers();

    static DWORD InstallDriver(__in LPCTSTR infFile, __in LPCTSTR hwid);

    static DWORD UninstallDriver(__in LPCTSTR infFile, __in LPCTSTR hwid);

private:
    static DWORD ExecuteCommand(const std::wstring& commandLine);
    static bool GetSecuritySids(__out StringCollection& userSids);
    static DWORD UpdateDriver(__in LPCTSTR infFile, __in LPCTSTR hwid);
    static DWORD RemoveDriver(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in LPVOID Context);
    static BOOL RemoveDriverStore(__in LPCTSTR sourceInfName);
    static DWORD SearchDevNode(__in LPCTSTR sourceInfName, __in LPCTSTR hwid, __in LPVOID Context);
    static LPTSTR GetDeviceId(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop);
    static void SplitStrings(const std::wstring& originalString, StringCollection & tokens, const std::wstring& delimiter);
    static bool TryPrepareCommandLineBuffer(std::vector<wchar_t>& commandLineBuffer, const std::wstring& commandLine);
};
