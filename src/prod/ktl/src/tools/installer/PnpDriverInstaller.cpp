/*++

Copyright (c) Microsoft Corporation

Module Name:

    PnpDriverInstaller.cpp

Abstract:

    Functions related to installing and controlling 
    a pnp KMDF driver.

Environment:

    User-mode

--*/
#include "PnpDriverInstaller.h"
#include "Constants.h"
#include "Path.h"
#include "public.h"
#include "WexString.h"
#include "WexTestClass.h"

using namespace std;
using namespace WEX::Common;
using namespace WEX::Logging;

std::wstring GetUtilityPath()
{
    return PathHelper::GetDirectoryName(PathHelper::GetModuleLocation());
}

UINT __stdcall InstallDrivers()
{
    std::wstring utilityPath = GetUtilityPath();
    Log::Comment(String().Format(L"utility directory=%ls", utilityPath.c_str()));

    DWORD result = PnpDriverInstaller::TestSignDrivers();
    if (result != Constants::ExitCodeOK)
    {
        Log::Error(String().Format(L"TestSignDrivers failed with error %u", result));
        return ERROR_INSTALL_FAILURE;
    }

	//
	// Uninstall first
	//
	UninstallKtlLoggerDriver();
	
	BOOL ok;

	ok = InstallKtlLoggerDriver();
	if (! ok)
	{
		Log::Error(String().Format(L"InstallDriver KtlLogger returned error failed"));
		return ERROR_INSTALL_FAILURE;
	}

    return ERROR_SUCCESS;
}

UINT __stdcall UninstallDrivers()
{
    std::wstring utilityPath = GetUtilityPath();
    Log::Comment(String().Format(L"utility directory=%ls", utilityPath.c_str()));


	BOOL ok;
	
	ok = UninstallKtlLoggerDriver();
	if (! ok)
	{
		Log::Error(String().Format(L"UninstallDriver KtlLogger returned error failed"));
		return ERROR_INSTALL_FAILURE;
	}

    return ERROR_SUCCESS;
}

DWORD PnpDriverInstaller::TestSignDrivers()
{
    std::wstring utilityPath = GetUtilityPath();
    wstring certPath = PathHelper::Combine(utilityPath, L"KtlTestCert.cer");
    wstring deleteRootCert = L"certutil.exe -delstore root KtlTestCert";
    wstring deleteTrustedPubCert = L"certutil.exe -delstore trustedpublisher KtlTestCert";
    wstring deleteStore = L"certutil.exe -user -delstore KtlTestCertStore KtlTestCert";
    wstring makeCert = PathHelper::Combine(utilityPath, L"makecert.exe") + L" -r -pe -ss KtlTestCertStore -n \"CN=KtlTestCert\" \"" + certPath + L"\"";
    wstring addRootCert = L"certutil.exe -addstore root \"" + certPath + L"\"";
    wstring addTrustedPubCert = L"certutil.exe -addstore trustedpublisher \"" + certPath + L"\"";
    ExecuteCommand(deleteRootCert);
    ExecuteCommand(deleteTrustedPubCert);
    ExecuteCommand(deleteStore);

    DWORD result = ExecuteCommand(makeCert);
    if (result != Constants::ExitCodeOK)
    {
        Log::Error(String().Format(L"MakeCert command:%ls returned error:%lu", makeCert.c_str(), result));
        return result;
    }

    result = ExecuteCommand(addRootCert);
    if (result != Constants::ExitCodeOK)
    {
        Log::Error(String().Format(L"AddRootCert command:%ls returned error:%lu", addRootCert.c_str(), result));
        ::DeleteFileW(certPath.c_str());
        return result;
    }

    result = ExecuteCommand(addTrustedPubCert);
    if (result != Constants::ExitCodeOK)
    {
        Log::Error(String().Format(L"AddTrustedPubCert command:%ls returned error:%lu", addTrustedPubCert.c_str(), result));
        ::DeleteFileW(certPath.c_str());
        return result;
    }

    ::DeleteFileW(certPath.c_str());

    wstring signToolExe = PathHelper::Combine(utilityPath, L"signtool.exe");
    StringCollection filesToBeSigned;
    SplitStrings(Constants::FilesToBeSigned, filesToBeSigned, L",");
    for (auto it = filesToBeSigned.begin(); it != filesToBeSigned.end(); it++)
    {
        Log::Comment(String().Format(L"FileName:%ls", it->c_str()));
        wstring file = PathHelper::Combine(utilityPath, *it);
        wstring signCommand = signToolExe + L" sign /s KtlTestCertStore /n KtlTestCert \"" + file + L"\"";
        result = ExecuteCommand(signCommand);
        if (result != Constants::ExitCodeOK)
        {
            Log::Error(String().Format(L"SignCommand:%ls returned error:%lu", signCommand.c_str(), result));
            return result;
        }
    }

    return Constants::ExitCodeOK;
}

DWORD PnpDriverInstaller::ExecuteCommand(wstring const & commandLine)
{
    Log::Comment(String().Format(L"ExecuteCommand - Executing command %ls", commandLine.c_str()));
    vector<wchar_t> commandLineBuffer(0);
    if (!TryPrepareCommandLineBuffer(commandLineBuffer, commandLine))
    {
        return Constants::ExitCodeFail;
    }

    // prepare parameters for CreateProcess
    LPTSTR lpCommandLine = &(commandLineBuffer.front());
    STARTUPINFO         startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    if (!::CreateProcessW(
        NULL,
        lpCommandLine,
        NULL,
        NULL,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW,
        NULL,
        NULL,
        &startupInfo,
        &processInfo))
    {
        DWORD lastError = GetLastError();
        Log::Error(String().Format(L"CreateProcessW failed (%lu)", lastError));
        return lastError;
    }

    ::WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode;
    ::GetExitCodeProcess(processInfo.hProcess, &exitCode);
    Log::Comment(String().Format(L"ExecuteCommand - Return Code (%lu)", exitCode));

    // Close process and thread handles. 
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return exitCode;
}

LPTSTR PnpDriverInstaller::GetDeviceId(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop)
    /*++
    Arguments:

    Devs    - HDEVINFO containing DevInfo
    DevInfo - Specific device
    Prop    - SPDRP_HARDWAREID or SPDRP_COMPATIBLEIDS

    Return Value:

    String
    returns NULL on failure

    --*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    DWORD sizeChars;

    size = 8192;
    buffer = new TCHAR[(size / sizeof(TCHAR)) + 2];
    if (!buffer)
    {
        return NULL;
    }

    Log::Comment(L"GetDeviceId::Calling SetupDiGetDeviceRegistryProperty");
    while (!SetupDiGetDeviceRegistryProperty(Devs, DevInfo, Prop, &dataType, (LPBYTE)buffer, size, &reqSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            goto failed;
        }
        if (dataType != REG_MULTI_SZ)
        {
            goto failed;
        }
        size = reqSize;
        delete[] buffer;
        buffer = new TCHAR[(size / sizeof(TCHAR)) + 2];
        if (!buffer)
        {
            goto failed;
        }
    }

    sizeChars = reqSize / sizeof(TCHAR);
    buffer[sizeChars] = TEXT('\0');
    buffer[sizeChars + 1] = TEXT('\0');
    return buffer;

failed:
    if (buffer)
    {
        delete[] buffer;
    }
    return NULL;
}

DWORD PnpDriverInstaller::InstallDriver(__in LPCTSTR infFile, __in LPCTSTR hwid)
    /*++

    Routine Description:

    Creates a root enumerated devnode and installs drivers on it

    Arguments:

    infFile  - driver inf file
    hwid - hardware device ID <ROOT\leaslayr>

    Return Value:

    EXIT_xxxx

    --*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN + 4];
    TCHAR InfPath[MAX_PATH];
    int failcode = Constants::ExitCodeFail;
    wstring sddlFormat;
    vector<wstring> initialSIDStrings;
    TCHAR sddl[LINE_LEN + 4];
    HRESULT ret;
    wstring devInstancePath(hwid);

    //ASSERT_IF(userSIDs.size() == 0, "Count of user sids passed to this function should not be 0");

    if (!infFile[0])
    {
        Log::Error(L"Input inf file is invalid.");
        return Constants::ExitCodeUsage;
    }

    if (!hwid[0])
    {
        Log::Error(L"Input device ID is invalid.");
        return Constants::ExitCodeUsage;
    }

    if (GetFullPathName(infFile, MAX_PATH, InfPath, NULL) >= MAX_PATH)
    {
        Log::Error(L"Input inf path name is too long.");
        return Constants::ExitCodeFail;
    }

    //
    // First Uninstall to make sure the driver can be installed from a clean state;
    // Ignore the return result
    //
    DWORD uninstallResult = UninstallDriver(infFile, hwid);
    Log::Comment(String().Format(L"UninstallDriver returned %lu", uninstallResult));

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList, sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList, LINE_LEN, hwid)))
    {
        Log::Error(L"Copy hwid failed.");
        goto final;
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    Log::Comment(L"InstallDriver:: Calling SetupDiGetINFClass");
    if (!SetupDiGetINFClass(InfPath, &ClassGUID, ClassName, sizeof(ClassName) / sizeof(ClassName[0]), 0))
    {
        Log::Error(String().Format(L"Failed to get class GUID because %lu.", ::GetLastError()));
        goto final;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    Log::Comment(L"InstallDriver:: Calling SetupDiCreateDeviceInfoList");
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, 0);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        Log::Error(String().Format(L"Failed to create device information set because %lu.", ::GetLastError()));
        goto final;
    }


    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    LPCWCHAR deviceName = devInstancePath.c_str() + (sizeof("ROOT"));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    Log::Comment(L"InstallDriver:: Calling SetupDiCreateDeviceInfo");
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        deviceName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        ULONG lastError = ::GetLastError();
        Log::Error(String().Format(L"Failed to create device element because %lu", lastError));
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    Log::Comment(L"InstallDriver:: Calling SetupDiSetDeviceRegistryProperty");
    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        static_cast<DWORD>(wcslen(hwIdList) + 1 + 1)*sizeof(TCHAR)))
    {
        Log::Error(String().Format(L"Failed to add device's hardware id property because %lu", ::GetLastError()));
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    Log::Comment(L"InstallDriver:: Calling SetupDiCallClassInstaller");
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        Log::Error(String().Format(L"Failed to create actual device node because %lu", ::GetLastError()));
        goto final;
    }

    //
    // Set the initial ACLs
    //
    initialSIDStrings.push_back(SDDL_BUILTIN_ADMINISTRATORS);
    initialSIDStrings.push_back(SDDL_LOCAL_SYSTEM);

    sddlFormat = L"D:P";
    for (auto it = initialSIDStrings.begin(); it != initialSIDStrings.end(); ++it)
    {
        sddlFormat.append(L"(A;;GA;;;");
        sddlFormat.append(*it);
        sddlFormat.append(L")");
    }

    ZeroMemory(sddl, sizeof(sddl));
    ret = StringCchCopy(sddl, LINE_LEN, sddlFormat.c_str());
    if (FAILED(ret))
    {
        Log::Error(String().Format(L"SDDL string copy failed because %lu.", ret));
        goto final;
    }

    Log::Comment(String().Format(L"SDDL string is %ls", sddl));
    Log::Comment(L"InstallDriver:: Calling SetupDiSetDeviceRegistryProperty");
    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_SECURITY_SDS,
        (LPBYTE)sddl,
        static_cast<DWORD>(wcslen(sddl) + 1 + 1)*sizeof(TCHAR)))
    {
        Log::Error(String().Format(L"Failed to set device's security property because %lu.", ::GetLastError()));
        goto final;
    }

    //
    // update the driver for the device we just created
    //
    failcode = UpdateDriver(InfPath, hwIdList);
    
    Log::Comment(L"Driver Installation succeeded");

final:
    
    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}

DWORD PnpDriverInstaller::UpdateDriver(__in LPCTSTR infPath, __in LPCTSTR hwidList)
    /*++

    Routine Description:
    UPDATE
    update driver for existing device(s)

    Arguments:

    infFile  - driver inf file
    hwid - hardware device ID <ROOT\leaslayr>

    Return Value:

    EXIT_xxxx

    --*/
{
    HMODULE newdevMod = NULL;
    DWORD failcode = Constants::ExitCodeFail;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    DWORD flags = 0;
    LPCTSTR inf;

    inf = infPath;
    flags |= INSTALLFLAG_FORCE;

    //
    // make use of UpdateDriverForPlugAndPlayDevices
    //
    Log::Comment(L"UpdateDriver:: Loading newdev.dll");
    newdevMod = LoadLibraryEx(TEXT("newdev.dll"), NULL, 0);
    if (!newdevMod)
    {
        Log::Error(String().Format(L"Failed to load newdev.dll because %lu", ::GetLastError()));
        goto final;
    }

    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if (!UpdateFn)
    {
        Log::Error(String().Format(L"Fail to create updating function for %ls from %ls because %lu.", hwidList, inf, ::GetLastError()));
        goto final;
    }

    Log::Comment(L"UpdateDriver:: calling UpdateDriverForPlugAndPlayDevices");
    if (!UpdateFn(NULL, hwidList, inf, flags, &reboot))
    {
        Log::Error(String().Format(L"Updating drivers failed for %ls from %ls because %lu.", hwidList, inf, ::GetLastError()));
        goto final;
    }
    Log::Comment(L"Drivers installed successfully");

    failcode = reboot ? Constants::ExitCodeReboot : Constants::ExitCodeOK;

    final :

        if (newdevMod) {
            FreeLibrary(newdevMod);
        }

    return failcode;
}

DWORD PnpDriverInstaller::RemoveDriver(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in LPVOID Context)
    /*++

    Routine Description:

    REMOVE
    remove device

    --*/

{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;
    LPCTSTR action = NULL;
    //
    // need hardware ID before trying to remove, as we won't have it after
    //        

    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    devInfoListDetail.cbSize = sizeof(devInfoListDetail);

    Log::Comment(L"RemoveDriver:: calling SetupDiGetDeviceInfoListDetail");
    if (!SetupDiGetDeviceInfoListDetail(Devs, &devInfoListDetail))
    {
        Log::Error(L"LeasLayrRemove: GetDeviceInfoListDetail failed.");
        return Constants::ExitCodeOK;
    }

    ULONG deviceIdLength;
    Log::Comment(L"RemoveDriver:: calling CM_Get_Device_ID_Size_Ex");
    CONFIGRET errorCode = CM_Get_Device_ID_Size_Ex(&deviceIdLength, DevInfo->DevInst, 0, devInfoListDetail.RemoteMachineHandle);
    if (errorCode != CR_SUCCESS)
    {
        Log::Error(String().Format(L"LeasLayrRemove: Failed to get DeviceId size using CM_Get_Device_ID_Size_Ex. Error: %lu", errorCode));
        return Constants::ExitCodeFail;
    }

    vector<WCHAR> devID(deviceIdLength + 1);
    Log::Comment(L"RemoveDriver:: calling CM_Get_Device_ID_Ex");
    if (CM_Get_Device_ID_Ex(DevInfo->DevInst, devID.data(), deviceIdLength + 1, 0, devInfoListDetail.RemoteMachineHandle) != CR_SUCCESS)
    {
        //
        // skip this
        //
        Log::Error(L"RemoveDriver: CM_Get_Device_ID_Ex failed.");
        return Constants::ExitCodeOK;
    }

    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    Log::Comment(L"RemoveDriver:: calling SetupDiCallClassInstaller");
    if (!SetupDiSetClassInstallParams(Devs, DevInfo, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) ||
        !SetupDiCallClassInstaller(DIF_REMOVE, Devs, DevInfo))
    {
        //
        // failed to invoke DIF_REMOVE
        //
        Log::Error(String().Format(L"LeasLayrRemove:  failed to invoke DIF_REMOVE because %lu.", ::GetLastError()));
        action = TEXT("No devices were removed.");
    }
    else
    {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        Log::Comment(L"RemoveDriver:: calling SetupDiGetDeviceInstallParams");
        if (SetupDiGetDeviceInstallParams(Devs, DevInfo, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)))
        {
            //
            // reboot required
            //
            Log::Error(L"LeasLayrRemove: Device is still used by some process. To remove the device, reboot the system.");
            action = TEXT("device(s) are ready to be removed, reboot is needed. ");
            pControlContext->reboot = TRUE;
        }
        else
        {
            //
            // appears to have succeeded
            //
            action = TEXT("device(s) were removed.");
        }
        pControlContext->count++;
    }

    return Constants::ExitCodeOK;
}

BOOL PnpDriverInstaller::RemoveDriverStore(__in LPCTSTR sourceInfName)
{
    BOOL ret = TRUE;
    PWSTR oemInfName = new WCHAR[MAX_PATH];
    if (!oemInfName)
    {
        Log::Error(String().Format(L"Failed to allocate resource for oemInfName because %lu.", ::GetLastError()));
        return FALSE;
    }
    DWORD requiredSz = 0;
    DWORD lastError;

    Log::Comment(L"RemoveDriverStore:: calling SetupCopyOEMInf");
    ret = SetupCopyOEMInf(
        sourceInfName,
        NULL,
        SPOST_NONE,
        SP_COPY_REPLACEONLY,
        oemInfName,
        MAX_PATH,
        &requiredSz,
        NULL);

    oemInfName[requiredSz] = L'\0';
    if (!ret)
    {
        lastError = GetLastError();
        Log::Error(String().Format(L"Copy OEM inf failed because the file is not in the system INF directory, error %lu", lastError));
    }
    else
    {
        wstring oemInfNoPathStr = PathHelper::GetFileName(oemInfName);
        PCWSTR oemInfNoPath = oemInfNoPathStr.c_str();
        Log::Comment(L"RemoveDriverStore:: calling SetupUninstallOEMInf %ws", oemInfNoPath);
        ret = SetupUninstallOEMInf(
            oemInfNoPath,
            SUOI_FORCEDELETE,
            NULL);

        if (!ret)
        {
            lastError = GetLastError();
            Log::Error(String().Format(L"Uninstall OEM inf failed, error %lu.", lastError));
        }
    }

    delete[] oemInfName;
    return ret;

}

DWORD PnpDriverInstaller::UninstallDriver(__in LPCTSTR sourceInfName, __in LPCTSTR hwid)
    /*++

    Routine Description:

    REMOVE
    remove devices

    Return Value:

    EXIT_xxxx

    --*/
{
    GenericContext context;
    DWORD failcode = Constants::ExitCodeFail;

    context.reboot = FALSE;
    context.count = 0;
    failcode = SearchDevNode(sourceInfName, hwid, &context);

    return failcode;
}

DWORD PnpDriverInstaller::SearchDevNode(__in LPCTSTR sourceInfName, __in LPCTSTR hwid, __in LPVOID Context)
    /*++

    Routine Description:

    Find if the dev node exists

    Arguments:

    HWid

    Return Value:

    EXIT_xxxx

    --*/
{
    HDEVINFO devs = INVALID_HANDLE_VALUE;
    DWORD failcode = Constants::ExitCodeFail;
    DWORD retcode = 0;
    DWORD devIndex;
    SP_DEVINFO_DATA devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    BOOL match = false;
    GUID cls;
    DWORD numClass = 0;

    UNREFERENCED_PARAMETER(Context);

    // Get all device in system class
    Log::Comment(L"SearchDevNode:: calling SetupDiClassGuidsFromNameEx");
    if (!SetupDiClassGuidsFromNameEx(TEXT("System"), &cls, 1, &numClass, NULL, NULL) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        Log::Error(String().Format(L"SetupDiClassGuidsFromNameEx failed because %lu.", ::GetLastError()));
        goto final;
    }
    if (!numClass)
    {
        Log::Error(String().Format(L"numClass %lu.", numClass));
        failcode = Constants::ExitCodeOK;
        goto final;
    }

    Log::Comment(L"SearchDevNode:: calling SetupDiGetClassDevsEx");
    devs = SetupDiGetClassDevsEx(
        numClass ? &cls : NULL,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL);

    if (devs == INVALID_HANDLE_VALUE)
    {
        Log::Error(String().Format(L"LeasLayrSearchDevNode: devs == INVALID_HANDLE_VALUE because %lu.", ::GetLastError()));
        goto final;
    }

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    Log::Comment(L"SearchDevNode:: calling SetupDiGetDeviceInfoListDetail");
    if (!SetupDiGetDeviceInfoListDetail(devs, &devInfoListDetail))
    {
        Log::Error(String().Format(L"LeasLayrSearchDevNode: devInfoListDetail fail because %lu.", ::GetLastError()));
        goto final;
    }

    devInfo.cbSize = sizeof(devInfo);
    for (devIndex = 0; SetupDiEnumDeviceInfo(devs, devIndex, &devInfo); devIndex++)
    {
        match = false;

        TCHAR devID[MAX_DEVICE_ID_LEN];
        LPTSTR hwIdsToCompare = NULL;
        //
        // determine instance ID
        //
        Log::Comment(L"SearchDevNode:: calling CM_Get_Device_ID_Ex");
        if (CM_Get_Device_ID_Ex(devInfo.DevInst, devID, MAX_DEVICE_ID_LEN, 0, devInfoListDetail.RemoteMachineHandle) != CR_SUCCESS)
        {
            devID[0] = TEXT('\0');
        }

        hwIdsToCompare = GetDeviceId(devs, &devInfo, SPDRP_HARDWAREID);

        if (!hwIdsToCompare)
        {
            Log::Error(String().Format(L"Failed to get device ID when enumerating the device list in system because %lu.", ::GetLastError()));
        }


        if (!_tcsicmp(hwid, hwIdsToCompare))
        {
            match = true;
        }

        delete[] hwIdsToCompare;

        if (match)
        {
            retcode = RemoveDriverStore(sourceInfName);

            retcode = RemoveDriver(devs, &devInfo, Context);
            if (retcode)
            {
                failcode = retcode;
                goto final;
            }
        }

    }

    failcode = Constants::ExitCodeOK;

    final:

    if (devs != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(devs);
    }
    return failcode;

}

void PnpDriverInstaller::SplitStrings(wstring const & originalString, StringCollection & tokens, wstring const & delimiter)
{
    wstring::size_type startTokenPos = 0;
    wstring::size_type endTokenPos = originalString.length() ? 0 : wstring::npos;

    // Till end of string is reached
    while (wstring::npos != endTokenPos)
    {
        endTokenPos = originalString.find_first_of(delimiter, startTokenPos);
        if (wstring::npos != startTokenPos)
        {
            wstring token = originalString.substr(startTokenPos, endTokenPos - startTokenPos);
            if (!token.empty())
            {
                tokens.push_back(token);
            }

            startTokenPos = endTokenPos + 1;
        }
    }
}

bool PnpDriverInstaller::TryPrepareCommandLineBuffer(
    vector<wchar_t> & commandLineBuffer,
    wstring const & commandLine)
{
    size_t commandLineBufferLen = commandLine.length() + 1;
    commandLineBuffer.resize(commandLineBufferLen);

    if (wcscpy_s(&commandLineBuffer.front(), commandLineBufferLen, commandLine.c_str()) != 0)
    {
        DWORD error = ::GetLastError();
        Log::Error(String().Format(L"PrepareCommandLineBuffer: wcscpy_s failed with %lu while trying to create a mutable command line buffer: CommandLine=%s",
            error,
            commandLine.c_str()));

        return false;
    }

    return true;
}
