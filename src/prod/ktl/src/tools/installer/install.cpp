/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    install.c

Abstract:

    Win32 routines to dynamically load and unload a Windows NT kernel-mode
    driver using the Service Control Manager APIs.

Environment:

    User mode only

--*/

#define NANO_SERVER

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "public.h"

#ifndef NANO_SERVER
#include <wdfinstaller.h>
#endif

//-----------------------------------------------------------------------------
// 4127 -- Conditional Expression is Constant warning
//-----------------------------------------------------------------------------
#define WHILE(a) \
__pragma(warning(suppress:4127)) while(a)

BOOLEAN
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCWSTR    DriverName,
    IN LPCWSTR    ServiceExe
    );


BOOLEAN
RemoveDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCWSTR    DriverName
    );

BOOLEAN
StartDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCWSTR    DriverName
    );

BOOLEAN
StopDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCWSTR    DriverName
    );

#define ARRAY_SIZE(x)        (sizeof(x) /sizeof(x[0]))

#define SYSTEM32_DRIVERS L"\\System32\\Drivers\\"

LONG GetCurrentDirectoryWithoutLog(
    ULONG Size,
    PWCHAR Path
    );

#ifndef NANO_SERVER
VOID
UnloadWdfCoInstaller(
    HMODULE Library
    );

HMODULE
LoadWdfCoInstaller(
    VOID
    );

// TODO: Make into parameters
#define NONPNP_INF_FILENAME  L"\\ktllogger.inf"
#define WDF_SECTION_NAME L"KtlLogger.NT.Wdf"

// for example, WDF 1.9 is "01009". the size 6 includes the ending NULL marker
//
#define MAX_VERSION_SIZE 6

WCHAR G_coInstallerVersion[MAX_VERSION_SIZE] = {0};
BOOLEAN  G_fLoop = FALSE;
BOOL G_versionSpecified = FALSE;

BOOL
ValidateCoinstallerVersion(
    _In_ PWSTR Version
    )
{   BOOL ok = FALSE;
    INT i;

    for(i= 0; i<MAX_VERSION_SIZE ;i++){
        if( ! IsCharAlphaNumericW(Version[i])) {
            break;
        }
    }
    if (i == (MAX_VERSION_SIZE -sizeof(WCHAR))) {
        ok = TRUE;
    }
    return ok;
}

PWCHAR
GetCoinstallerVersion(
    VOID
    )
{
    if (FAILED( StringCchPrintf(G_coInstallerVersion,
                                MAX_VERSION_SIZE,
                                L"%02d%03d",    // for example, "01009"
                                KMDF_VERSION_MAJOR,
                                KMDF_VERSION_MINOR)))
    {
        printf("StringCchCopy failed with error \n");
    }

    return (PWCHAR)&G_coInstallerVersion;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
PFN_WDFPREDEVICEINSTALLEX pfnWdfPreDeviceInstallEx;
PFN_WDFPOSTDEVICEINSTALL   pfnWdfPostDeviceInstall;
PFN_WDFPREDEVICEREMOVE     pfnWdfPreDeviceRemove;
PFN_WDFPOSTDEVICEREMOVE   pfnWdfPostDeviceRemove;


LONG
GetPathToInf(
    _Out_writes_(InfFilePathSize) PWCHAR InfFilePath,
    IN ULONG InfFilePathSize
    )
{
    LONG    error = ERROR_SUCCESS;

    if (GetCurrentDirectoryWithoutLog(InfFilePathSize, InfFilePath) == 0) {
        error =  GetLastError();
        printf("InstallDriver failed!  Error = %d \n", error);
        return error;
    }
    
    if (FAILED( StringCchCatW(InfFilePath,
                              InfFilePathSize,
                              NONPNP_INF_FILENAME) )) {
        error = ERROR_BUFFER_OVERFLOW;
        return error;
    }
    return error;

}

HMODULE
LoadWdfCoInstaller(
    VOID
    )
{
    HMODULE library = NULL;
    DWORD   error = ERROR_SUCCESS;
    WCHAR   szCurDir[MAX_PATH];
    WCHAR   tempCoinstallerName[MAX_PATH];
    PWCHAR  coinstallerVersion;

    do {

        if (GetSystemDirectory(szCurDir, MAX_PATH) == 0) {

            printf("GetSystemDirectory failed!  Error = %d \n", GetLastError());
            break;
        }
        coinstallerVersion = GetCoinstallerVersion();
        if (FAILED( StringCchPrintf(tempCoinstallerName,
                                  MAX_PATH,
                                  L"\\WdfCoInstaller%ws.dll",
                                  coinstallerVersion) )) {
            break;
        }
        if (FAILED( StringCchCat(szCurDir, MAX_PATH, tempCoinstallerName) )) {
            break;
        }
        
        library = LoadLibrary(szCurDir);

        if (library == NULL) {
            if (GetCurrentDirectoryWithoutLog(MAX_PATH, szCurDir) == 0) {

                printf("GetCurrentDirectory failed!  Error = %d \n", GetLastError());
                break;
            }
            coinstallerVersion = GetCoinstallerVersion();
            if (FAILED( StringCchPrintf(tempCoinstallerName,
                                      MAX_PATH,
                                      L"\\WdfCoInstaller%ws.dll",
                                      coinstallerVersion) )) {
                break;
            }
            if (FAILED( StringCchCat(szCurDir, MAX_PATH, tempCoinstallerName) )) {
                break;
            }

            library = LoadLibrary(szCurDir);
        }

        if (library == NULL) {
            error = GetLastError();
            printf("LoadLibrary(%ws) failed: %d\n", szCurDir, error);
            break;
        }

        pfnWdfPreDeviceInstallEx =
            (PFN_WDFPREDEVICEINSTALLEX) GetProcAddress( library, "WdfPreDeviceInstallEx" );

        if (pfnWdfPreDeviceInstallEx == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPreDeviceInstallEx\") failed: %d\n", error);
			break;
        }

        pfnWdfPostDeviceInstall =
            (PFN_WDFPOSTDEVICEINSTALL) GetProcAddress( library, "WdfPostDeviceInstall" );

        if (pfnWdfPostDeviceInstall == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPostDeviceInstall\") failed: %d\n", error);
			break;
        }

        pfnWdfPreDeviceRemove =
            (PFN_WDFPREDEVICEREMOVE) GetProcAddress( library, "WdfPreDeviceRemove" );

        if (pfnWdfPreDeviceRemove == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPreDeviceRemove\") failed: %d\n", error);
			break;
        }

        pfnWdfPostDeviceRemove =
            (PFN_WDFPREDEVICEREMOVE) GetProcAddress( library, "WdfPostDeviceRemove" );

        if (pfnWdfPostDeviceRemove == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPostDeviceRemove\") failed: %d\n", error);
			break;
        }

    } WHILE (0);

    if (error != ERROR_SUCCESS) {
        if (library) {
            FreeLibrary( library );
        }
        library = NULL;
    }

    return library;
}

VOID
UnloadWdfCoInstaller(
    HMODULE Library
    )
{
    if (Library) {
        FreeLibrary( Library );
    }
}


#endif


LONG GetCurrentDirectoryWithoutLog(
    ULONG Size,
    PWCHAR Path
    )
{
    LONG error;
    
    if (GetCurrentDirectoryW(Size, Path) == 0) {
        error =  GetLastError();
        printf("GetCurrentDirectoryW failed!  Error = %d \n", error);
        return 0;
    }

    //
    // The current working directory when running under runtests is
    // bin\ktltest\log while running under te it is bin\ktltest.
    //
    PWCHAR tail;
    HRESULT hr;
    size_t len;

    hr = StringCchLength(Path, Size, &len);
    if (FAILED(hr))
    {
        return 0;
    }

    if (len > 4)
    {
        tail = &Path[len - 4];
        
        if (_wcsicmp(tail, L"\\Log") == 0)
        {
            Path[len - 4] = 0;
        }
    }
        
    return 1;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCWSTR    DriverName,
    IN LPCWSTR    ServiceExe
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    SC_HANDLE   schService;
    DWORD       err;

#ifndef NANO_SERVER
    WCHAR      infPath[MAX_PATH];
    WDF_COINSTALLER_INSTALL_OPTIONS clientOptions;

    WDF_COINSTALLER_INSTALL_OPTIONS_INIT(&clientOptions);

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //
    //
    // PRE-INSTALL for WDF support
    //
    err = GetPathToInf(infPath, ARRAY_SIZE(infPath) );
    if (err != ERROR_SUCCESS) {
        printf("DriverInstall: GetPathToInf return %d\n", err);
        return  FALSE;
    }
    err = pfnWdfPreDeviceInstallEx(infPath, WDF_SECTION_NAME, &clientOptions);

    if (err != ERROR_SUCCESS) {
        printf("WdfPreDeviceInstallEx returns %d\n", err);
        if (err == ERROR_SUCCESS_REBOOT_REQUIRED) {
            printf("System needs to be rebooted, before the driver installation can proceed.\n");
        }
        
        return  FALSE;
    }
#endif

    //
    // Create a new a service object.
    //

    schService = CreateService(SchSCManager,           // handle of service control manager database
                               DriverName,             // address of name of service to start
                               DriverName,             // address of display name
                               SERVICE_ALL_ACCESS,     // type of access to service
                               SERVICE_KERNEL_DRIVER,  // type of service
                               SERVICE_AUTO_START,     // when to start service
                               SERVICE_ERROR_NORMAL,   // severity if service fails to start
                               ServiceExe,             // address of name of binary file
                               NULL,                   // service does not belong to a group
                               NULL,                   // no tag requested
                               NULL,                   // no dependency names
                               NULL,                   // use LocalSystem account
                               NULL                    // no password for service account
                               );

    if (schService == NULL) {

        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS) {

            //
            // Ignore this error.
            //

            return TRUE;

        } else {

            printf("CreateService failed!  Error = %d \n", err );

            //
            // Indicate an error.
            //

            return  FALSE;
        }
    }

    //
    // Close the service object.
    //
    CloseServiceHandle(schService);

#ifndef NANO_SERVER
    //
    // POST-INSTALL for WDF support
    //
    err = pfnWdfPostDeviceInstall( infPath, WDF_SECTION_NAME );

    if (err != ERROR_SUCCESS) {
        printf("WdfPostDeviceInstall returns %d\n", err);
        return  FALSE;
    }
#endif
    
    //
    // Indicate success.
    //

    return TRUE;

}   // InstallDriver

BOOLEAN
ManageDriver(
    IN LPCWSTR  DriverName,
    IN LPCWSTR  ServiceName,
    IN USHORT   Function
    )
{

    SC_HANDLE   schSCManager;

    BOOLEAN rCode = TRUE;

    //
    // Insure (somewhat) that the driver and service names are valid.
    //

    if (!DriverName || !ServiceName) {

        printf("Invalid Driver or Service provided to ManageDriver() \n");

        return FALSE;
    }

    //
    // Connect to the Service Control Manager and open the Services database.
    //

    schSCManager = OpenSCManager(NULL,                   // local machine
                                 NULL,                   // local database
                                 SC_MANAGER_ALL_ACCESS   // access required
                                 );

    if (!schSCManager) {

        printf("Open SC Manager failed! Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Do the requested function.
    //

    switch( Function ) {

        case DRIVER_FUNC_INSTALL:

            //
            // Install the driver service.
            //

            if (InstallDriver(schSCManager,
                              DriverName,
                              ServiceName
                              )) {

                //
                // Start the driver service (i.e. start the driver).
                //

                rCode = StartDriver(schSCManager,
                                    DriverName
                                    );

            } else {

                //
                // Indicate an error.
                //

                rCode = FALSE;
            }

            break;

        case DRIVER_FUNC_REMOVE:

            //
            // Stop the driver.
            //

            StopDriver(schSCManager,
                       DriverName
                       );

            //
            // Remove the driver service.
            //

            RemoveDriver(schSCManager,
                         DriverName
                         );

            //
            // Ignore all errors.
            //

            rCode = TRUE;

            break;

        default:

            printf("Unknown ManageDriver() function. \n");

            rCode = FALSE;

            break;
    }

    //
    // Close handle to service control manager.
    //
    CloseServiceHandle(schSCManager);

    return rCode;

}   // ManageDriver


BOOLEAN
RemoveDriver(
    IN SC_HANDLE    SchSCManager,
    IN LPCWSTR      DriverName
    )
{
    SC_HANDLE   schService;
    BOOLEAN     rCode;

#ifndef NANO_SERVER
    WCHAR      infPath[MAX_PATH];

    err = GetPathToInf(infPath, ARRAY_SIZE(infPath) );
    if (err != ERROR_SUCCESS) {
        return  FALSE;
    }

    //
    // PRE-REMOVE of WDF support
    //
    err = pfnWdfPreDeviceRemove( infPath, WDF_SECTION_NAME );

    if (err != ERROR_SUCCESS) {
        return  FALSE;
    }
#endif
    
    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        printf("OpenService failed!  Error = %d \n", GetLastError());

        //
        // Indicate error.
        //

        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database.
    //

    if (DeleteService(schService)) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        printf("DeleteService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //
    CloseServiceHandle(schService);

#ifndef NANO_SERVER
    //
    // POST-REMOVE of WDF support
    //
    err = pfnWdfPostDeviceRemove(infPath, WDF_SECTION_NAME );

    if (err != ERROR_SUCCESS) {
        rCode = FALSE;
    }
#endif
    
    return rCode;

}   // RemoveDriver



BOOLEAN
StartDriver(
    IN SC_HANDLE    SchSCManager,
    IN LPCWSTR      DriverName
    )
{
    SC_HANDLE   schService;
    DWORD       err;
    BOOL        ok;

    //
    // Open the handle to the existing service.
    //
    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {
        //
        // Indicate failure.
        //
        printf("OpenService failed!  Error = %d\n", GetLastError());
        return FALSE;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    //
    ok = StartService( schService, 0, NULL );

    if (!ok) {

        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            //
            // Ignore this error.
            //
            return TRUE;

        } else {
            //
            // Indicate failure.
            // Fall through to properly close the service handle.
            //
            printf("StartService failure! Error = %d\n", err );
            return FALSE;
        }
    }

    //
    // Close the service object.
    //
    CloseServiceHandle(schService);

    return TRUE;

}   // StartDriver



BOOLEAN
StopDriver(
    IN SC_HANDLE    SchSCManager,
    IN LPCWSTR      DriverName
    )
{
    BOOLEAN         rCode = TRUE;
    SC_HANDLE       schService;
    SERVICE_STATUS  serviceStatus;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        printf("OpenService failed!  Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Request that the service stop.
    //

    if (ControlService(schService,
                       SERVICE_CONTROL_STOP,
                       &serviceStatus
                       )) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        printf("ControlService failed!  Error = %d \n", GetLastError() );

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //
    CloseServiceHandle (schService);

    return rCode;

}   //  StopDriver


//
// Caller must free returned pathname string.
//
PWCHAR
BuildDriversDirPath(
    _In_ PWSTR DriverName
    )
{
    size_t  remain;
    size_t  len;
    PWCHAR   dir;

    if (!DriverName || wcslen(DriverName) == 0) {
        return NULL;
    }

    remain = MAX_PATH;

    //
    // Allocate string space
    //
    dir = (PWCHAR) malloc( remain + 1 );

    if (!dir) {
        return NULL;
    }

    //
    // Get the base windows directory path.
    //
    len = GetWindowsDirectory( dir, (UINT) remain );

    if (len == 0 ||
        (remain - len) < (sizeof(SYSTEM32_DRIVERS) / sizeof(WCHAR))) {
        free(dir);
        return NULL;
    }
    remain -= len;

    //
    // Build dir to have "%windir%\System32\Drivers\<DriverName>".
    //
    if (FAILED( StringCchCat(dir, remain, SYSTEM32_DRIVERS) )) {
        free(dir);
        return NULL;
    }

    remain -= sizeof(SYSTEM32_DRIVERS);
    len    += sizeof(SYSTEM32_DRIVERS);
    len    += wcslen(DriverName);

    if (remain < len) {
        free(dir);
        return NULL;
    }

    if (FAILED( StringCchCat(dir, remain, DriverName) )) {
        free(dir);
        return NULL;
    }

    dir[len] = '\0';  // keeps prefast happy

    return dir;
}


BOOLEAN
SetupDriverName(
    _In_ PWCHAR DriverName,
    _Inout_updates_all_(BufferLength) PWCHAR DriverLocation,
    _In_ ULONG BufferLength
    )
{
    HANDLE fileHandle;
    DWORD  driverLocLen = 0;
    BOOL   ok;
    PWCHAR  driversDir;
    WCHAR   driverNameWithSys[MAX_PATH];

    //
    // Setup path name to driver file.
    //
    driverLocLen =
        GetCurrentDirectoryWithoutLog(BufferLength, DriverLocation);

    if (!driverLocLen) {

        printf("GetCurrentDirectory failed!  Error = %d \n",
               GetLastError());

        return FALSE;
    }

    if (FAILED( StringCchCat(DriverLocation, BufferLength, L"\\") )) {
        return FALSE;
    }

    if (FAILED( StringCchCat(DriverLocation, BufferLength, DriverName) )) {
        return FALSE;
    }

    if (FAILED( StringCchCat(DriverLocation, BufferLength, L".sys") )) {
        return FALSE;
    }

    //
    // Insure driver file is in the specified directory.
    //
    fileHandle = CreateFile( DriverLocation,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if (fileHandle == INVALID_HANDLE_VALUE) {
        //
        // Indicate failure.
        //
        printf("Driver: %ws.SYS is not in the %ws directory. \n",
               DriverName, DriverLocation );
        return FALSE;
    }

    //
    // Build %windir%\System32\Drivers\<DRIVER_NAME> path.
    // Copy the driver to %windir%\system32\drivers
    //
    *driverNameWithSys = 0;
    if (FAILED( StringCchCat(driverNameWithSys, MAX_PATH, DriverName) )) {
        return FALSE;
    }

    if (FAILED( StringCchCat(driverNameWithSys, MAX_PATH, L".sys") )) {
        return FALSE;
    }

    driversDir = BuildDriversDirPath( driverNameWithSys );

    if (!driversDir) {
        printf("BuildDriversDirPath failed!\n");
        return FALSE;
    }

    ok = CopyFile( DriverLocation, driversDir, FALSE );

    if(!ok) {
        printf("CopyFile failed: error(%d) - \"%ws\"\n",
               GetLastError(), driversDir );
        free(driversDir);
        return FALSE;
    }

    if (FAILED( StringCchCopy(DriverLocation, BufferLength, L"\\systemroot\\System32\\Drivers\\") )) {
        free(driversDir);
        return FALSE;
    }

    if (FAILED( StringCchCat(DriverLocation, BufferLength, driverNameWithSys) )) {
        free(driversDir);
        return FALSE;
    }
        
    free(driversDir);

    //
    // Close open file handle.
    //
    if (fileHandle) {
        CloseHandle(fileHandle);
    }

    //
    // Indicate success.
    //
    return TRUE;

}   // SetupDriverName


BOOL InstallKtlLoggerDriver()
{
    WCHAR     driverLocation [MAX_PATH];
    BOOL     ok;
    PWCHAR driverName = L"KtlLogger";

#ifndef NANO_SERVER
    HMODULE  library = NULL;
    PWCHAR    coinstallerVersion;
    coinstallerVersion = GetCoinstallerVersion();
    
    //
    // Load WdfCoInstaller.dll.
    //
    library = LoadWdfCoInstaller();

    if (library == NULL) {
        printf("The WdfCoInstaller%ws.dll library needs to be "
               "in the system directory\n", coinstallerVersion);
        return(FALSE);
    }
#endif
	
    //
    // The driver is not started yet so let us the install the driver.
    // First setup full path to driver name.
    //
    ok = SetupDriverName( driverName, driverLocation, MAX_PATH );

    if (!ok) {
        return(ok);
    }

    ok = ManageDriver( driverName,
                       driverLocation,
                       DRIVER_FUNC_INSTALL );

    if (!ok) {

        printf("Unable to install driver. \n");

        //
        // Error - remove driver.
        //
        ManageDriver( driverName,
                      driverLocation,
                      DRIVER_FUNC_REMOVE );
        return(ok);
    }

#ifndef NANO_SERVER
    //
    // Unload WdfCoInstaller.dll
    //
    if ( library ) {
        UnloadWdfCoInstaller( library );
    }
#endif
	
    return(ok);
}

BOOL UninstallKtlLoggerDriver()
{
    WCHAR     driverLocation [MAX_PATH];
    BOOL     ok;
    PWCHAR driverName = L"KtlLogger";

#ifndef NANO_SERVER
    HMODULE  library = NULL;
    PWCHAR    coinstallerVersion;
    coinstallerVersion = GetCoinstallerVersion();
    
    //
    // Load WdfCoInstaller.dll.
    //
    library = LoadWdfCoInstaller();

    if (library == NULL) {
        printf("The WdfCoInstaller%ws.dll library needs to be "
               "in the system directory\n", coinstallerVersion);
        return(FALSE);
    }
#endif
    
    //
    // The driver is not started yet so let us the install the driver.
    // First setup full path to driver name.
    //
    ok = SetupDriverName( driverName, driverLocation, MAX_PATH );

    if (!ok) {
        return(ok);
    }

    //
    // Unload the driver.  Ignore any errors.
    //
    ok = ManageDriver( driverName,
                  driverLocation,
                  DRIVER_FUNC_REMOVE );

#ifndef NANO_SERVER
    //
    // Unload WdfCoInstaller.dll
    //
    if ( library ) {
        UnloadWdfCoInstaller( library );
    }
#endif
    
    return(ok);
}