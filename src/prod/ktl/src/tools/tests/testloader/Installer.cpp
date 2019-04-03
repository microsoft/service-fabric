/*++

Copyright (c) Microsoft Corporation

Module Name:

    Installer.cpp

Abstract:

    Functions related to installing and controlling 
    a non-pnp KMDF driver.

Environment:

    User-mode

--*/

#include "TestLoader.h"
#include "Installer.h"

#define MAX_COMMANDLINE 1024


KmdfNonPnpInstaller::KmdfNonPnpInstaller(__in PCWSTR WdfVersionString,
    __in PCWSTR DriverName, __in PCWSTR InfFileName, 
    __in PCWSTR WdfSectionNameInInfFile, __in BOOL RemovalOnExit
    ) : wdfVersionString_(WdfVersionString), driverName_(DriverName),
    infFileName_(InfFileName), wdfSectionNameInInfFile_(WdfSectionNameInInfFile),
    removalOnExit_(RemovalOnExit), schSCManager_(NULL),
    wdfCoInstallerHandle_(NULL), 
    pfnWdfPreDeviceInstallEx_(NULL), pfnWdfPostDeviceInstall_(NULL),
    pfnWdfPreDeviceRemove_(NULL), pfnWdfPostDeviceRemove_(NULL)    

/*++

Routine Description:

    Constructor.

Arguments:

    WdfVersionString - Version of the WDF CoInstaller in MMmmm format. 
        MM is the major version; mmm is the minor version.
            
    DriverName - Name of the driver binary without the .sys suffix.
        The driver binary is assumed to exist under the current directory.

    InfFileName - Name of the INF file. The INF file is assumed to 
        exist under the current directory.

    WdfSectionNameInInfFile - Name of the WDF section in the INF file.

    RemovalOnExit - TRUE indicates this object will try to remove the driver
        at destruction.
        
Return Value:

    None.

--*/

{    
}

KmdfNonPnpInstaller::~KmdfNonPnpInstaller(
    )

/*++

Routine Description:

    Destructor.

Arguments:

    None.
        
Return Value:

    None.

--*/

{
    if (removalOnExit_) {
        StopDriver();
        RemoveDriver();
        RemoveCertificate();
    }
    
    if (schSCManager_) {
        CloseServiceHandle(schSCManager_);
        schSCManager_ = NULL;
    }
    
    pfnWdfPreDeviceInstallEx_ = NULL;
    pfnWdfPostDeviceInstall_ = NULL;
    pfnWdfPreDeviceRemove_ = NULL;
    pfnWdfPostDeviceRemove_ = NULL;    
    
    if (wdfCoInstallerHandle_) {
        FreeLibrary(wdfCoInstallerHandle_);    
        wdfCoInstallerHandle_ = NULL;
    }
}

DWORD
KmdfNonPnpInstaller::EnsureWdfCoInstallerLoaded(
    )

/*++

Routine Description:

    This routine makes sure the WDF CoInstaller DLL is loaded and
    all function pointers are obtained.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    HRESULT     hr;
    DWORD       error = ERROR_SUCCESS;
    WCHAR       currentDirectory[MAX_PATH] = {0};
    WCHAR       coInstallerPath[MAX_PATH] = {0};

    if (!wdfCoInstallerHandle_) {
        if (!GetCurrentDirectoryW(ARRAYSIZE(currentDirectory), currentDirectory)) {
            error = GetLastError();            
            printf("GetCurrentDirectory failed!  Error = %d \n", error);
            return error;
        }
        
        hr = StringCchPrintfW(coInstallerPath,
                              ARRAYSIZE(coInstallerPath),
                              L"%s\\WdfCoInstaller%s.dll",
                              currentDirectory, wdfVersionString_);    
        if (FAILED(hr)) {     
            error = WIN32_FROM_HRESULT(hr);
            printf("StringCchPrintfW for coinstaller binary path failed!  Error = %d \n", error);
            return error;
        }
                
        wdfCoInstallerHandle_ = LoadLibraryW(coInstallerPath);
        if (wdfCoInstallerHandle_ == NULL) {
            error = GetLastError();
            printf("LoadLibrary(%S) failed: %d\n", coInstallerPath, error);
            return error;
        }        
    }

    if (!pfnWdfPreDeviceInstallEx_) {
        pfnWdfPreDeviceInstallEx_ =
            (PFN_WDFPREDEVICEINSTALLEX)GetProcAddress(wdfCoInstallerHandle_, "WdfPreDeviceInstallEx");
        if (pfnWdfPreDeviceInstallEx_ == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPreDeviceInstallEx\") failed: %d\n", error);
            return error;
        }        
    }

    if (!pfnWdfPostDeviceInstall_) {
        pfnWdfPostDeviceInstall_ =
            (PFN_WDFPOSTDEVICEINSTALL)GetProcAddress(wdfCoInstallerHandle_, "WdfPostDeviceInstall");
        if (pfnWdfPostDeviceInstall_ == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPostDeviceInstall\") failed: %d\n", error);
            return error;
        }        
    }

    if (!pfnWdfPreDeviceRemove_) {
        pfnWdfPreDeviceRemove_ =
            (PFN_WDFPREDEVICEREMOVE)GetProcAddress(wdfCoInstallerHandle_, "WdfPreDeviceRemove");
        if (pfnWdfPreDeviceRemove_ == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPreDeviceRemove\") failed: %d\n", error);
            return error;
        }        
    }

    if (!pfnWdfPostDeviceRemove_) {
        pfnWdfPostDeviceRemove_ =
            (PFN_WDFPREDEVICEREMOVE)GetProcAddress(wdfCoInstallerHandle_, "WdfPostDeviceRemove");
        if (pfnWdfPostDeviceRemove_ == NULL) {
            error = GetLastError();
            printf("GetProcAddress(\"WdfPostDeviceRemove\") failed: %d\n", error);
            return error;
        }        
    }

    return error;
}

DWORD
KmdfNonPnpInstaller::EnsureSCMManagerOpen(
    )

/*++

Routine Description:

    This routine makes sure the SCM manager handle is opened.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD   error = ERROR_SUCCESS;
    
    if (schSCManager_) {
        return ERROR_SUCCESS;
    }
    
    schSCManager_ = OpenSCManager(NULL,                 // local machine
                                  NULL,                 // local database
                                  SC_MANAGER_ALL_ACCESS // access required
                                  );
    if (!schSCManager_) {
        error = GetLastError();
        printf("Open SC Manager failed! Error = %d\n", error);
    }            

    return error;    
}

DWORD
KmdfNonPnpInstaller::EnsurePrecondition(
    )

/*++

Routine Description:

    This routine makes sure all preconditions for installing and controlling
    a driver are met.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD       error = ERROR_SUCCESS;
    HRESULT     hr;    
    WCHAR       currentDirectory[MAX_PATH] = {0};

    error = EnsureSCMManagerOpen();
    if (error != ERROR_SUCCESS) {
        return error;
    }

    error = EnsureWdfCoInstallerLoaded();
    if (error != ERROR_SUCCESS) {
        return error;
    }
    
    if (GetCurrentDirectoryW(ARRAYSIZE(currentDirectory), currentDirectory) == 0) {
        error = GetLastError();
        printf("GetCurrentDirectoryW failed!  Error = %d \n", error);
        return error;
    }

    hr = StringCchPrintfW(infFileFullPath_, ARRAYSIZE(infFileFullPath_), L"%s\\%s",
            currentDirectory, infFileName_);
    if (FAILED(hr)) {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for inf file path failed!  Error = %d \n", error);        
        return error;
    }
    
    hr = StringCchPrintfW(driverFullPath_, ARRAYSIZE(driverFullPath_), L"%s\\%s.sys",
            currentDirectory, driverName_);
    if (FAILED(hr)) {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for driver file path failed!  Error = %d \n", error);        
        return error;
    }

    return error;
}


DWORD
KmdfNonPnpInstaller::InstallCertificate(
    )

/*++

Routine Description:

    This routine creates, signs, and installs the certificate for the driver.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD      error = ERROR_SUCCESS;
    HRESULT    hr = S_OK;

    error = EnsurePrecondition();
    if (error != ERROR_SUCCESS) {
        return error;
    }

    // Prepare the commands
    WCHAR certificateFile[MAX_PATH] = { 0 };
    hr = StringCchPrintfW(certificateFile, ARRAYSIZE(certificateFile), L"%sCert.cer", driverName_);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for certificate filename failed!  Error = %d\n", error);
        return error;
    }

    WCHAR currentDirectory[MAX_PATH] = { 0 };
    if (!GetCurrentDirectoryW(ARRAYSIZE(currentDirectory), currentDirectory))
    {
        error = GetLastError();
        printf("GetCurrentDirectory failed!  Error = %d \n", error);
        return error;
    }

	WCHAR windowsDirectory[MAX_PATH] = { 0 };
    if (!GetWindowsDirectoryW(windowsDirectory, ARRAYSIZE(windowsDirectory)))
    {
        error = GetLastError();
        printf("GetWindowsDirectory failed!  Error = %d \n", error);
        return error;
    }

    WCHAR makeCertificate[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(makeCertificate, ARRAYSIZE(makeCertificate), L"\"%s\\makecert.exe\" -r -pe -ss %sCertStore -n \"CN=%sCert\" %s", currentDirectory, driverName_, driverName_, certificateFile);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for make certificate command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR addRootCertificate[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(addRootCertificate, ARRAYSIZE(addRootCertificate), L"\"%s\\certutil.exe\" -addstore root %s", currentDirectory, certificateFile);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for add root certificate command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR addTrustedPublisherCertificate[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(addTrustedPublisherCertificate, ARRAYSIZE(addTrustedPublisherCertificate), L"\"%s\\certutil.exe\" -addstore trustedpublisher %s", currentDirectory, certificateFile);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for add trusted publisher certificate command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR signDriver[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(signDriver, ARRAYSIZE(signDriver), L"\"%s\\signtool.exe\" sign /s %sCertStore /n %sCert \"%s\"", currentDirectory, driverName_, driverName_, driverFullPath_);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for driver signing command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR copyNtdll[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(copyNtdll, ARRAYSIZE(copyNtdll), L"cmd /c copy \"%s\\system32\\ntdll.dll\" \"%s\"", windowsDirectory, currentDirectory);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for copy ntdll command failed!  Error = %d\n", error);
        return error;
    }
    printf("Copy NTDLL - %ws\n", copyNtdll);
    error = ExecuteCommand(copyNtdll);
    if (error != ERROR_SUCCESS)
    {
        printf("Failed to copy ntdll!  Error = %d\n", error);
        return error;
    }
	
    // Execute the commands
    printf("Creating a new certificate for signing:  %S\n", makeCertificate);
    error = ExecuteCommand(makeCertificate);
    if (error != ERROR_SUCCESS)
    {
        printf("Failed to create a new certificate for signing!  Error = %d\n", error);
        return error;
    }

    printf("Adding the certificate to the root store:  %S\n", addRootCertificate);
    error = ExecuteCommand(addRootCertificate);
    if (error != ERROR_SUCCESS)
    {
        printf("Failed to add the certificate to the root store!  Error = %d\n", error);
        return error;
    }

    printf("Adding the certificate to the trusted publisher store:  %S\n", addTrustedPublisherCertificate);
    error = ExecuteCommand(addTrustedPublisherCertificate);
    if (error != ERROR_SUCCESS)
    {
        printf("Failed to add the certificate to the trusted publisher store!  Error = %d\n", error);
        return error;
    }

    // Delete the certificate file
    ::DeleteFileW(certificateFile);

    printf("Signing the driver .sys file:  %S\n", signDriver);
    error = ExecuteCommand(signDriver);
    if (error != ERROR_SUCCESS)
    {
        printf("Failed to sign the driver .sys file!  Error = %d\n", error);
        return error;
    }

    return error;
}


DWORD
KmdfNonPnpInstaller::RemoveCertificate(
    )

/*++

Routine Description:

    This routine removes the certificate for the driver.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD      error = ERROR_SUCCESS;
    HRESULT    hr = S_OK;

    // Prepare the commands
    WCHAR currentDirectory[MAX_PATH] = { 0 };
    if (!GetCurrentDirectoryW(ARRAYSIZE(currentDirectory), currentDirectory))
    {
        error = GetLastError();
        printf("GetCurrentDirectory failed!  Error = %d \n", error);
        return error;
    }

    WCHAR deleteRootCertificate[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(deleteRootCertificate, ARRAYSIZE(deleteRootCertificate), L"\"%s\\certutil.exe\" -delstore root %sCert", currentDirectory, driverName_);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for delete root certificate command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR deleteTrustedPublisherCertificate[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(deleteTrustedPublisherCertificate, ARRAYSIZE(deleteTrustedPublisherCertificate), L"\"%s\\certutil.exe\" -delstore trustedpublisher %sCert", currentDirectory, driverName_);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for delete trusted publisher certificate command failed!  Error = %d\n", error);
        return error;
    }

    WCHAR deleteCertificateStore[MAX_COMMANDLINE] = { 0 };
    hr = StringCchPrintfW(deleteCertificateStore, ARRAYSIZE(deleteCertificateStore), L"\"%s\\certutil.exe\" -user -delstore %sCertStore %sCert", currentDirectory, driverName_, driverName_);
    if (FAILED(hr))
    {
        error = WIN32_FROM_HRESULT(hr);
        printf("StringCchPrintfW for delete certificate store command failed!  Error = %d\n", error);
        return error;
    }

    // Execute the commands.  Ignore failures in certificate removal
    ExecuteCommand(deleteRootCertificate);
    ExecuteCommand(deleteTrustedPublisherCertificate);
    ExecuteCommand(deleteCertificateStore);

    return error;
}


DWORD 
KmdfNonPnpInstaller::InstallDriver(
    )

/*++

Routine Description:

    This routine installs the driver.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD                               error = ERROR_SUCCESS;
    SC_HANDLE                           schService = NULL;
    WDF_COINSTALLER_INSTALL_OPTIONS     clientOptions;    

    error = EnsurePrecondition();
    if (error != ERROR_SUCCESS) {        
        return error;
    }

    WDF_COINSTALLER_INSTALL_OPTIONS_INIT(&clientOptions);

    error = pfnWdfPreDeviceInstallEx_(infFileFullPath_, wdfSectionNameInInfFile_, &clientOptions);
    if (error != ERROR_SUCCESS) {
        if (error == ERROR_SUCCESS_REBOOT_REQUIRED) {
            printf("System needs to be rebooted, before the driver installation can proceed.\n");
        } else {
            printf("Failed to install the kernel device driver!  Error = %d\n", error);
        }
        
        return error;
    }

    schService = CreateServiceW(schSCManager_,          // handle of service control manager database
                                driverName_,            // address of name of service to start
                                driverName_,            // address of display name
                                SERVICE_ALL_ACCESS,     // type of access to service
                                SERVICE_KERNEL_DRIVER,  // type of service
                                SERVICE_DEMAND_START,   // when to start service
                                SERVICE_ERROR_NORMAL,   // severity if service fails to start
                                driverFullPath_,        // address of name of binary file
                                NULL,                   // service does not belong to a group
                                NULL,                   // no tag requested
                                NULL,                   // no dependency names
                                NULL,                   // use LocalSystem account
                                NULL                    // no password for service account
                               );
    if (schService) {
        CloseServiceHandle(schService);    
        schService = NULL;

        error = pfnWdfPostDeviceInstall_(infFileFullPath_, wdfSectionNameInInfFile_);                
    } else {
        error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {

            //
            // Ignore this error.
            //

            error = ERROR_SUCCESS;
        } else {
            printf("CreateService failed!  Error = %d \n", error);
        }
    } 

    return error;
}

DWORD 
KmdfNonPnpInstaller::RemoveDriver(
    )

/*++

Routine Description:

    This routine removes the driver.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD                               error = ERROR_SUCCESS;
    SC_HANDLE                           schService = NULL;

    error = EnsurePrecondition();
    if (error != ERROR_SUCCESS) {        
        return error;
    }

    error = pfnWdfPreDeviceRemove_(infFileFullPath_, wdfSectionNameInInfFile_);
    if (error != ERROR_SUCCESS) {
        return  error;
    }

    schService = OpenServiceW(schSCManager_,
                              driverName_,
                              SERVICE_ALL_ACCESS);
    if (schService) {
        if (DeleteService(schService)) {
            error = pfnWdfPostDeviceRemove_(infFileFullPath_, wdfSectionNameInInfFile_);            
        } else {
            error = GetLastError();                
        }

        CloseServiceHandle(schService);        
        schService = NULL;        
    } else {
        error = GetLastError();        
    }

    return error;
}

DWORD 
KmdfNonPnpInstaller::StartDriver(
    )

/*++

Routine Description:

    This routine starts the driver.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD                               error = ERROR_SUCCESS;
    SC_HANDLE                           schService = NULL;

    error = EnsureSCMManagerOpen();
    if (error != ERROR_SUCCESS) {
        return error;        
    }

    schService = OpenServiceW(schSCManager_,
                              driverName_,
                              SERVICE_ALL_ACCESS);
    if (schService) {
        if (!StartService(schService, 0, NULL)) {
            error = GetLastError();
            if (error == ERROR_SERVICE_ALREADY_RUNNING) {
            
                //
                // Ignore this error.
                //
                error = ERROR_SUCCESS;
            
            } else {
                printf("StartService failure! Error = %d\n", error);
            }
        }

        CloseServiceHandle(schService);        
        schService = NULL;        
    } else {
        error = GetLastError();        
        printf("OpenService failed!  Error = %d \n", error);    
    }

    return error;
}

DWORD 
KmdfNonPnpInstaller::StopDriver(
    )

/*++

Routine Description:

    This routine stops the driver.

Arguments:

    None.
        
Return Value:

    Win32 error code.

--*/

{
    DWORD                       error = ERROR_SUCCESS;
    SC_HANDLE                   schService = NULL;
    SERVICE_STATUS              serviceStatus;

    error = EnsureSCMManagerOpen();
    if (error != ERROR_SUCCESS) {
        return error;        
    }    

    schService = OpenServiceW(schSCManager_,
                              driverName_,
                              SERVICE_ALL_ACCESS);
    if (schService) {
        if (!ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) {
            error = GetLastError();
        }

        CloseServiceHandle(schService);        
        schService = NULL;        
    } else {
        error = GetLastError();        
    }

    return error;    
}


DWORD
KmdfNonPnpInstaller::ExecuteCommand(
    __in LPWSTR CommandLine
    )

/*++

Routine Description:

    This routine creates a process to execute the commandline supplied, and
    blocks until the process completes.

Arguments:

    CommandLine - the command to execute in a separate process.

Return Value:

    Win32 error code.

--*/

{
    DWORD      error = ERROR_SUCCESS;

    STARTUPINFO si;
    RtlZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;    
    RtlZeroMemory(&pi, sizeof(pi));

    BOOL b = ::CreateProcessW(
        nullptr,        // lpApplicationName
        CommandLine,    // lpCommandLine
        nullptr,        // lpProcessAttributes
        nullptr,        // lpThreadAttributes  
        FALSE,          // bInheritHandles
        0,              // dwCreationFlags
        nullptr,        // lpEnvironment 
        nullptr,        // lpCurrentDirectory 
        &si,
        &pi);
    if (!b)
    {
        error = GetLastError();
        printf("Failed to create process!  Error = %d\n", error);
        return error;
    }

    //
    // Wait until the command finishes.
    //
    if (::WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
    {
        error = GetLastError();
        printf("Failed waiting for process to complete!  Error = %d\n", error);
        return error;
    }

    ::GetExitCodeProcess(pi.hProcess, &error);
	
    if (pi.hProcess != INVALID_HANDLE_VALUE && pi.hProcess != NULL)
    {
        ::CloseHandle(pi.hProcess);
    }

    if (pi.hThread != INVALID_HANDLE_VALUE && pi.hThread != NULL)
    {
        ::CloseHandle(pi.hThread);
    }

    return error;
}
