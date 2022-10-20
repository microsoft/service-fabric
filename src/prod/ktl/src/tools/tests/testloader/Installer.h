/*++

Copyright (c) Microsoft Corporation

Module Name:

    Installer.h

Abstract:

    Functions related to installing and controlling 
    a non-pnp KMDF driver.

Environment:

    User-mode

--*/

#pragma once

#include <wdfinstaller.h>

class KmdfNonPnpInstaller
{
public:
    KmdfNonPnpInstaller(__in PCWSTR WdfVersionString, __in PCWSTR DriverName, 
        __in PCWSTR InfFileName, __in PCWSTR WdfSectionNameInInfFile,
        __in BOOL RemovalOnExit);
    ~KmdfNonPnpInstaller();

    DWORD InstallCertificate();
    DWORD RemoveCertificate();

    DWORD InstallDriver();
    DWORD RemoveDriver();

    DWORD StartDriver();
    DWORD StopDriver();

private:
    KmdfNonPnpInstaller();    

    DWORD EnsureWdfCoInstallerLoaded();
    DWORD EnsureSCMManagerOpen();    

    static DWORD ExecuteCommand(__in LPWSTR CommandLine);

    //
    // Make sure all common data structures are properly built
    // and handles are opened.
    //
    DWORD EnsurePrecondition();

    PCWSTR  wdfVersionString_;
    PCWSTR  driverName_;
    PCWSTR  infFileName_;
    PCWSTR  wdfSectionNameInInfFile_;
    BOOL    removalOnExit_;

    //
    // Full path of the driver binary file.
    // The driver binary is assumed to exist under the current directory.    
    //
    WCHAR   driverFullPath_[MAX_PATH];

    //
    // Full path of the INF file. 
    // The INF file is assumed to exist under the current directory.
    //
    WCHAR   infFileFullPath_[MAX_PATH];

    //
    // Handle to the service control manager.
    //
    SC_HANDLE   schSCManager_;

    //
    // Handle to a loaded Wdf CoInstaller DLL.
    //    
    HMODULE wdfCoInstallerHandle_;

    //
    // Pointers to functions inside Wdf CoInstaller DLL.
    //    
    PFN_WDFPREDEVICEINSTALLEX   pfnWdfPreDeviceInstallEx_;
    PFN_WDFPOSTDEVICEINSTALL    pfnWdfPostDeviceInstall_;
    PFN_WDFPREDEVICEREMOVE      pfnWdfPreDeviceRemove_;
    PFN_WDFPOSTDEVICEREMOVE     pfnWdfPostDeviceRemove_;    
};



