// This contains the implementation of commands sent to the Service Fabric Block Device via DeviceIOControl.
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SFBDCtlLib.h"

BOOL IsSFBDDeviceAlreadyInstalled()
{
    DWORD dwLastError = 0;
    DWORD fIsInstalled = FALSE;

    GUID guid;
    DWORD dwRequiredGuids = 0;

    // Fetch the GUID for the device class
    BOOL fGotClassName = SetupDiClassGuidsFromNameEx(SFBD_CLASS_NAME, &guid, 1, &dwRequiredGuids, NULL, NULL);
    if (fGotClassName)
    {
        HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT);
        dwLastError = GetLastError();
        if (hDevInfoSet == INVALID_HANDLE_VALUE)
        {
            TRACE_ERROR("Unable to get Service Fabric Block Device class due to error: %08X.\n", dwLastError);
        }
        else
        {
            // Get the DeviceInfo Data for the only instance we have.
            SP_DEVINFO_DATA devInfoData;
            DWORD dwIndex = 0;
            {
                devInfoData.cbSize = sizeof(devInfoData);
                fIsInstalled = SetupDiEnumDeviceInfo(hDevInfoSet, dwIndex, &devInfoData);
                dwLastError = GetLastError();
                if (!fIsInstalled)
                {
                    if (dwLastError != ERROR_NO_MORE_ITEMS)
                    {
                        TRACE_ERROR("Unable to connect to the Service Fabric Block Device due to error: %08X.\n", dwLastError);
                    }
                }
            }

            SetupDiDestroyDeviceInfoList(hDevInfoSet);
        }
    }
    else
    {
        dwLastError = GetLastError();
        TRACE_ERROR("Unable to locate an installation of Service Fabric Block Device (error code: %08X).\n", dwLastError);
    }

    return fIsInstalled;
}

BOOL GetOEMInfFileName(LPCWCH sourceInfFilePath, PWCHAR outOemInfFileNameBuf, uint lengthOutOemInfFileNameBuf)
{
    DWORD requiredSize = 0;
    // Get the oem inf file path length for the buffer.
    BOOL foundOEMInfFileName = SetupCopyOEMInf(sourceInfFilePath, NULL, SPOST_NONE,
        SP_COPY_REPLACEONLY, NULL, 0, &requiredSize, NULL);

    if (!foundOEMInfFileName)
    {
        DWORD dwLastError = GetLastError();
        TRACE_ERROR("Unable to get oem inf filename for source file %s due to error : %08X.\n", sourceInfFilePath, dwLastError);
    }
    else
    {
        PWCHAR oemInfFilePath = NULL;
        PWSTR oemInfFileName = NULL;
        oemInfFilePath = new(std::nothrow) WCHAR[requiredSize];
        if (nullptr == oemInfFilePath)
        {
            TRACE_ERROR("Can't allocate WCHAR[ %d ] memory for Oem inf file path.\n", requiredSize);
        }
        else
        {
            ZeroMemory(oemInfFilePath, requiredSize);
            // get the oem inf file path in oemInfFilePath and filename starting pointer in oemInfFileName
            foundOEMInfFileName = SetupCopyOEMInf(sourceInfFilePath, NULL, SPOST_NONE,
                SP_COPY_REPLACEONLY, oemInfFilePath, requiredSize, NULL, &oemInfFileName);
            if (!foundOEMInfFileName)
            {
                DWORD dwLastError = GetLastError();
                TRACE_ERROR("Unable to get oem inf filename for source file %s due to error : %08X.\n", sourceInfFilePath, dwLastError);
            }
            else
            {
                oemInfFilePath[requiredSize - 1] = L'\0';
                if (NULL == oemInfFileName)
                {
                    TRACE_ERROR("Oemfilename is NULL when SetupCopyOEMInf returned success with oem inf file path : %s\n", oemInfFilePath);
                    foundOEMInfFileName = FALSE;
                }
                else
                {
                    errno_t cpErr = wcscpy_s(outOemInfFileNameBuf, lengthOutOemInfFileNameBuf, oemInfFileName);
                    if (cpErr != 0)
                    {
                        TRACE_ERROR("Failure in copying oem inf file name: %s to destination buffer due to error : %d", cpErr);
                    }
                }
            }
            delete[] oemInfFilePath;
        }
    }

    return foundOEMInfFileName;
}

SFBDCTLLIB_API UninstallErrorCodes UninstallSFBDDevice(LPCWSTR pPathToINF, BOOL& fRestartSystem, BOOL fDuringInstallation)
{
    UninstallErrorCodes fUninstalled = UninstallErrorCodes::NotFound;
    DWORD dwLastError = 0;
    GUID guid;
    DWORD dwRequiredGuids = 0;
    fRestartSystem = FALSE;

    // Get the oem inf file name before DiUninstallDevice otherwise we get ERROR_FILE_NOT_FOUND.
    WCHAR oemInfFileName[MAX_PATH];
    ZeroMemory(oemInfFileName, sizeof(oemInfFileName));
    BOOL foundOemInfFileName = GetOEMInfFileName(pPathToINF, oemInfFileName, MAX_PATH);
    if (!fDuringInstallation && !foundOemInfFileName)
    {
        TRACE_ERROR("Unable to find oem inf file during uninstallation.");
        return UninstallErrorCodes::UnInstallFailed;
    }

    // Fetch the GUID for the device class
    BOOL fGotClassName = SetupDiClassGuidsFromNameEx(SFBD_CLASS_NAME, &guid, 1, &dwRequiredGuids, NULL, NULL);
    if (fGotClassName)
    {
        HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT);
        dwLastError = GetLastError();
        if (hDevInfoSet == INVALID_HANDLE_VALUE)
        {
            TRACE_ERROR("Unable to get Service Fabric Block Device class due to error: %08X.\n", dwLastError);
        }
        else
        {
            // Get the DeviceInfo Data for the only instance we have.
            SP_DEVINFO_DATA devInfoData;
            DWORD dwIndex = 0;
            devInfoData.cbSize = sizeof(devInfoData);
            BOOL fRetVal = SetupDiEnumDeviceInfo(hDevInfoSet, dwIndex, &devInfoData);
            dwLastError = GetLastError();
            if (fRetVal)
            {
                BOOL uninstalled = DiUninstallDevice(NULL, hDevInfoSet, &devInfoData, 0, &fRestartSystem);
                dwLastError = GetLastError();
                if (uninstalled)
                {
                    TRACE_INFO("Device successfully uninstalled.\n");
                    if (foundOemInfFileName)
                    {
                        // uninstall oem file
                        if (SetupUninstallOEMInf(oemInfFileName, SUOI_FORCEDELETE, NULL))
                        {
                            TRACE_INFO("Removed driver from the driver store on uninstallation.\n");
                            fUninstalled = UninstallErrorCodes::UnInstalledSucessfully;
                            if (fRestartSystem)
                            {
                                TRACE_WARNING("Please restart the machine to complete uninstallation.\n");
                            }
                        }
                        else
                        {
                            dwLastError = GetLastError();
                            fUninstalled = UninstallErrorCodes::UnInstallFailed;
                            TRACE_ERROR("Failed to remove driver oem inf file : %s from the driver store on uninstallation due to error: %08X.\n", oemInfFileName, dwLastError);
                        }
                    }
                    else
                    {
                        ASSERT_IFNOT(fDuringInstallation, "Found no oem inf file during uninstallation");
                        TRACE_INFO("Found no oem inf file to uninstall during installation.");
                        fUninstalled = UninstallErrorCodes::UnInstalledSucessfully;
                    }
                }
                else
                {
                    fUninstalled = UninstallErrorCodes::UnInstallFailed;
                    TRACE_ERROR("Unable to uninstall Service Fabric Block Device due to error: %08X.\n", dwLastError);
                }

                // Make sure that we have only one device.
                dwIndex = 1;
                devInfoData.cbSize = sizeof(devInfoData);
                fRetVal = SetupDiEnumDeviceInfo(hDevInfoSet, dwIndex, &devInfoData);
                ASSERT_IF(fRetVal, "More than 1 SFBD device are present in PnP Hw tree.");
            }
            else
            {
                if (dwLastError == ERROR_NO_MORE_ITEMS)
                {
                    TRACE_INFO("Service Fabric Block Device Driver is not present.\n");
                }
                else
                {
                    TRACE_ERROR("Unable to connect to the Service Fabric Block Device due to error: %08X.\n", dwLastError);
                }
            }

            SetupDiDestroyDeviceInfoList(hDevInfoSet);
        }
    }
    else
    {
        dwLastError = GetLastError();
        TRACE_ERROR("Unable to locate an installation of Service Fabric Block Device (error code: %08X).\n", dwLastError);
    }

    return fUninstalled;
}

SFBDCTLLIB_API BOOL InstallSFBDDevice(LPCWSTR pPathToINF, BOOL& fRestartSystem)
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN + 4];
    LPCTSTR hwid = SFBD_HW_ID;
    fRestartSystem = FALSE;
    BOOL fInstalled = FALSE;
    DWORD dwLastError = 0;

    if (IsSFBDDeviceAlreadyInstalled())
    {
        TRACE_ERROR("Service Fabric Block Device is already installed. If you wish to reinstall the driver, please uninstall it first.\n");
        return TRUE;
    }

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList, sizeof(hwIdList));
    wcscpy_s(hwIdList, LINE_LEN, hwid);

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(pPathToINF, &ClassGUID, ClassName, sizeof(ClassName) / sizeof(ClassName[0]), 0))
    {
        dwLastError = GetLastError();
        TRACE_ERROR(L"Unable to get device class from the INF file at %s due to error: %08X.\n", pPathToINF, dwLastError);
    }
    else
    {
        //
        // Create the container for the to-be-created Device Information Element.
        //
        DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, 0);
        if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        {
            dwLastError = GetLastError();
            TRACE_ERROR("Unable to create device information list due to error: %08X.\n", dwLastError);
        }
        else
        {
            //
            // Now create the element.
            // Use the Class GUID and Name from the INF file.
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
                ClassName,
                &ClassGUID,
                NULL,
                0,
                DICD_GENERATE_ID,
                &DeviceInfoData))
            {
                dwLastError = GetLastError();
                TRACE_ERROR("Unable to create device info due to error: %08X.\n", dwLastError);
            }
            else
            {
                //
                // Add the HardwareID to the Device's HardwareID property.
                //
                if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                    &DeviceInfoData,
                    SPDRP_HARDWAREID,
                    (LPBYTE)hwIdList,
                    ((DWORD)_tcslen(hwIdList) + 1 + 1) * sizeof(TCHAR)))
                {
                    dwLastError = GetLastError();
                    TRACE_ERROR("Unable to set hardware ID property due to error: %08X.\n", dwLastError);
                }
                else
                {
                    //
                    // Transform the registry element into an actual devnode
                    // in the PnP HW tree.
                    //
                    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                        DeviceInfoSet,
                        &DeviceInfoData))
                    {
                        dwLastError = GetLastError();
                        TRACE_ERROR("Unable to create devnode in the PnP HW tree for the device due to error: %08X.\n", dwLastError);
                    }
                    else
                    {
                        unsigned int count = 0;
                        //trying 5 times because  there  were cases, when the driver update failed saying driver class is not found
                        while (fInstalled == FALSE && count < 5)
                        {
                            Sleep(DELAY_BETWEEN_DEV_NODE_CREATE_AND_UPDATE_DRIVER_IN_MS);
                            fInstalled = UpdateDriverForPlugAndPlayDevices(NULL, SFBD_HW_ID, pPathToINF, 0 /*No flags*/, &fRestartSystem);
                            count++;
                            if (!fInstalled)
                            {
                                dwLastError = GetLastError();
                                TRACE_WARNING("Driver installation attempt %d failed due to error: %08X.\n", count, dwLastError);
                            }
                        }

                        if (fInstalled)
                        {
                            TRACE_INFO(L"Driver successfully installed from %s\n", pPathToINF);
                            if (fRestartSystem)
                            {
                                TRACE_WARNING("Please restart the machine to complete driver installation.\n");
                            }
                        }
                    }
                }
            }
        }
    }

    return fInstalled;
}

bool GetSFBDDevicePath(PWCHAR pDevicePath, int Length, PDWORD pLastError)
{
    bool fFoundPath = false;

    DWORD dwError = ERROR_SUCCESS;
    *pLastError = dwError;

#if 0

    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;

    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&GUID_ServiceFabricBlockDeviceInterface, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfoSet == INVALID_HANDLE_VALUE)
    {
        *pLastError = GetLastError();
        TRACE_ERROR("Please install Service Fabric Block Device Driver on this machine.\n");
        return false;
    }

    // Enumerate the device interfaces
    DWORD dwIndex = 0;
    SP_DEVICE_INTERFACE_DATA devInterfaceData;
    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    BOOL fEnumeratedDeviceInterface = SetupDiEnumDeviceInterfaces(hDevInfoSet, NULL, &GUID_ServiceFabricBlockDeviceInterface, dwIndex, &devInterfaceData);
    if (fEnumeratedDeviceInterface)
    {
        // Fetch the DevicePath
        DWORD dwRequiredSize = 0;

        // Step 1 - Get the size of the buffer that is required.
        SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &devInterfaceData, NULL, 0, &dwRequiredSize, NULL);
        dwError = GetLastError();
        if (dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            TRACE_ERROR("Unable to fetch the interface details!\n");
            goto DisplayError;
        }

        // Step 2 - Now fetch the DevicePath.
        pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwRequiredSize);
        if (!pInterfaceDetailData)
        {
            TRACE_ERROR("Unable to allocate memory to fetch interface details.\n");
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto DisplayError;
        }

        // Make the call again with the requisite sized buffer.
        pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &devInterfaceData, pInterfaceDetailData, dwRequiredSize, NULL, NULL))
        {
            dwError = GetLastError();
            TRACE_ERROR("Unable to fetch the interface details!\n");
            goto DisplayError;
        }

        wcscpy_s(pDevicePath, Length, pInterfaceDetailData->DevicePath);
        fFoundPath = true;

        // Finally, free the buffer
        free(pInterfaceDetailData);
        pInterfaceDetailData = NULL;
        dwError = ERROR_NO_MORE_ITEMS;

        // Move to the next
        dwIndex++;
        //devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        //fEnumeratedDeviceInterface = SetupDiEnumDeviceInterfaces(hDevInfoSet, NULL, &GUID_ServiceFabricBlockDeviceInterface, dwIndex, &devInterfaceData);
    }
    else
    {
        dwError = ERROR_NO_MORE_ITEMS;
        TRACE_INFO("Please install Service Fabric Block Device driver first.\n");
    }

    // TRACE_INFO("Enumerated %d devices.\n", dwIndex);

DisplayError:
    if (dwError != ERROR_NO_MORE_ITEMS)
    {
        TRACE_ERROR("Device enumeration failed due to error: %08X\n", dwError);
        *pLastError = dwError;
    }

    // Delete the information set now that we are done using it.
    if (!SetupDiDestroyDeviceInfoList(hDevInfoSet))
    {
        dwError = GetLastError();
        TRACE_ERROR("Unable to delete device information set due to error: %08X\n", dwError);
    }

    if (pInterfaceDetailData != NULL)
    {
        free(pInterfaceDetailData);
    }

#else // !0
    wcscpy_s(pDevicePath, Length, SFBD_SYMBOLIC_PATH);
    fFoundPath = true;
#endif // 0

    return fFoundPath;
}

SFBDCTLLIB_API void DisplayErrorMessage(PSFBDCommand pCommand)
{
    TRACE_ERROR("Error details: ");

    switch (pCommand->cmdStatus)
    {
    case DeviceNotAvailable:
        TRACE_ERROR("No available device found on the adapter to setup LU.\n");
        break;
    case DeviceNotFound:
        TRACE_ERROR("Specified LU could not be located.\n");
        break;
    case InvalidArguments:
        TRACE_ERROR("Invalid arguments specified.\n");
        break;
    case FailedToRegisterLU:
        TRACE_ERROR("Unable to register the LU with the service.\n");
        break;
    case FailedToUnregisterLU:
        TRACE_ERROR("Unable to unregister the LU from the service.\n");
        break;
    case FailedToUpdateDeviceList:
        TRACE_ERROR("Unable to find the LU in the device list though it was unregistered.\n");
        break;
    case LUNotMounted:
        TRACE_ERROR("LU is not mounted.\n");
        break;
    case FailedToUnmountLU:
        TRACE_ERROR("Unable to unmount the LU in the service.\n");
        break;
    case LUAlreadyMounted:
        TRACE_ERROR("LU is already mounted.\n");
        break;
    case FailedToMountLU:
        TRACE_ERROR("Unable to mount the LU in the service.\n");
        break;
    case InvalidLuIndex:
        TRACE_ERROR("Invalid LU index specified.\n");
        break;
    case UnsupportedCommand:
        TRACE_ERROR("Invalid command specified.\n");
        break;
    }
}

SFBDCTLLIB_API HANDLE ConnectToDevice(PDWORD pLastError)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    WCHAR wszDevicePath[100];
    ZeroMemory(wszDevicePath, sizeof(wszDevicePath));

    *pLastError = ERROR_SUCCESS;

    // Fetch the device path
    if (GetSFBDDevicePath(wszDevicePath, _countof(wszDevicePath), pLastError) == true)
    {
        TRACE_INFO(L"Device Symbolic Path: %s\n", wszDevicePath);

        hDevice = CreateFile(wszDevicePath, GENERIC_READ | GENERIC_WRITE,  // Access rights requested
            0,                           // Share access - NONE
            0,                           // Security attributes - not used!
            OPEN_EXISTING,               // Device must exist to open it.
            FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
            0);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            *pLastError = GetLastError();
            TRACE_ERROR("Unable to connect to device! Error:%x", *pLastError);
        }
    }
    else
    {
        TRACE_ERROR("Unable to get device path!\n");

        if (*pLastError == ERROR_SUCCESS)
        {
            *pLastError = ERROR_DEVICE_NOT_AVAILABLE;
        }
    }

    return hDevice;
}

BOOL SendCommandToDriver(HANDLE hDevice, PSFBDCommand pCommand, PSFBDCommand pResponse)
{
    DWORD bytesReturned = 0;
    if (DeviceIoControl(hDevice, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, pCommand, sizeof(SFBDCommand), pResponse, sizeof(SFBDCommand), &bytesReturned, NULL) == 0)
    {
        DWORD dwLastError = GetLastError();

        if (pResponse->cmdStatus == Success)
        {
            pResponse->cmdStatus = GeneralFailure;
        }

        TRACE_ERROR("Unable to send command to driver due to error: %08X\n", dwLastError);
        return FALSE;
    }

    if (pResponse->cmdStatus == Success)
        return TRUE;
    else
        return FALSE;
}

BOOL SendRWCommandToDriver(HANDLE hDevice, PSFBDCommand pCommand, PDWORD pError)
{
    DWORD bytesReturned = 0;
    DWORD dwIoctlCode = (pCommand->command == ReadFromUsermodeBuffer) ? IOCTL_SFBD_READ_FROM_USERMODE : IOCTL_SFBD_READ_FROM_SRB;

    if (DeviceIoControl(hDevice, dwIoctlCode, pCommand, sizeof(SFBDCommand), pCommand->srbInfo.UserModeBuffer, pCommand->srbInfo.DataBufferLength, &bytesReturned, NULL) == 0)
    {
        *pError = GetLastError();

        TRACE_ERROR("Unable to send R/W command to driver due to error: %08X\n", *pError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

// Returns the R/W buffer address for the specified SRB in context of the process that invokes this function.
SFBDCTLLIB_API BOOL ReadFromUserBuffer(PVOID pSrb, PVOID pBuffer, ULONG Length, PDWORD pError)
{
    BOOL fRetVal = FALSE;

    HANDLE hDevice = ConnectToDevice(pError);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        SFBDCommand info;

        info.command = ReadFromUsermodeBuffer;
        info.srbInfo.SRBAddress = pSrb;
        info.srbInfo.DataBufferLength = Length;
        info.srbInfo.UserModeBuffer = pBuffer;

        *pError = 0;

        fRetVal = SendRWCommandToDriver(hDevice, &info, pError);

        CloseHandle(hDevice);
    }

    return fRetVal;
}

// Returns the R/W buffer address for the specified SRB in context of the process that invokes this function.
SFBDCTLLIB_API BOOL ReadFromSRBBuffer(PVOID pSrb, PVOID pBuffer, ULONG Length, PDWORD pError)
{
    BOOL fRetVal = FALSE;

    HANDLE hDevice = ConnectToDevice(pError);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        SFBDCommand info;

        info.command = ReadFromSrbDataBuffer;
        info.srbInfo.SRBAddress = pSrb;
        info.srbInfo.DataBufferLength = Length;
        info.srbInfo.UserModeBuffer = pBuffer;

        SFBDCommand responseCommand;
        memset(&responseCommand, 0, sizeof(SFBDCommand));

        *pError = 0;

        fRetVal = SendRWCommandToDriver(hDevice, &info, pError);

        CloseHandle(hDevice);
    }

    return fRetVal;
}

// Requests the driver to refresh the storage stack with the LUs registered with the service at the specified port
SFBDCTLLIB_API BOOL RefreshServiceLUList(DWORD servicePort)
{
    BOOL fRetVal = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    HANDLE hDevice = ConnectToDevice(&dwError);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        SFBDCommand info;

        info.command = RefreshLUList;
        info.servicePort = servicePort;

        SFBDCommand responseCommand;
        memset(&responseCommand, 0, sizeof(SFBDCommand));

        fRetVal = SendCommandToDriver(hDevice, &info, &responseCommand);

        CloseHandle(hDevice);
    }

    return fRetVal;
}

// Requests the driver to shutdown the volumes registered with the service at the specified port
SFBDCTLLIB_API BOOL ShutdownVolumesForService(DWORD servicePort)
{
    BOOL fRetVal = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    HANDLE hDevice = ConnectToDevice(&dwError);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        SFBDCommand info;

        info.command = ShutdownServiceVolumes;
        info.servicePort = servicePort;

        SFBDCommand responseCommand;
        memset(&responseCommand, 0, sizeof(SFBDCommand));

        fRetVal = SendCommandToDriver(hDevice, &info, &responseCommand);

        CloseHandle(hDevice);
    }

    return fRetVal;
}

// Displays details of the specified device.
SFBDCTLLIB_API BOOL DisplayDeviceInfo(HANDLE hDevice)
{
    SFBDCommand info;

    info.command = GetAdapterInfo;
    info.basicInfo.BusCount = 0;
    info.basicInfo.TargetCount = 0;
    info.basicInfo.LUCount = 0;

    SFBDCommand responseCommand;
    memset(&responseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &info, &responseCommand);
    if (fRetVal)
    {
        TRACE_INFO("Bus count: %d\n", responseCommand.basicInfo.BusCount);
        TRACE_INFO("Device count: %d\n", responseCommand.basicInfo.TargetCount);
        TRACE_INFO("LUN count: %d\n", responseCommand.basicInfo.LUCount);
    }
    else
    {
        TRACE_ERROR("Unable to get device info!\n");
    }

    return fRetVal;
}

// Simple helper to parse volume creation specific arguments.
BOOL ParseVolumeArgs(PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, LPWSTR pFileSystem, int& outSize, DiskSizeUnit& outSizeType, VDS_FILE_SYSTEM_TYPE& outFileSystemType)
{
    outSize = _wtoi(pSize);
    if (outSize < 1)
    {
        TRACE_ERROR("Invalid Disk size specified: %d\n", outSize);
        return FALSE;
    }

    outSizeType = MB;
    if (_wcsnicmp(pSizeType, L"MB", 2) == 0)
    {
        //Nothing to do.
    }
    else if (_wcsnicmp(pSizeType, L"GB", 2) == 0)
    {
        outSizeType = GB;
    }
    else if (_wcsnicmp(pSizeType, L"TB", 2) == 0)
    {
        outSizeType = TB;
    }
    else
    {
        TRACE_ERROR(L"Invalid DiskSizeType specified: %s\n", pSizeType);
        return FALSE;
    }

    if ((wcslen(pLUID) + 1) > MAX_WCHAR_SIZE_LUID)
    {
        TRACE_ERROR("LUN ID cannot be more than 50 characters in length!\n");
        return FALSE;
    }

    // Extract the file system - default is FAT.
    outFileSystemType = VDS_FST_RAW;
    if (_wcsnicmp(pFileSystem, L"RAW", 3) == 0)
    {
        // Nothing to do.
    }
    else if (_wcsnicmp(pFileSystem, L"FAT", 3) == 0)
    {
        outFileSystemType = VDS_FST_FAT;
    }
    else if (_wcsnicmp(pFileSystem, L"FAT32", 5) == 0)
    {
        outFileSystemType = VDS_FST_FAT32;
    }
    else if (_wcsnicmp(pFileSystem, L"NTFS", 4) == 0)
    {
        outFileSystemType = VDS_FST_NTFS;
    }
    else if (_wcsnicmp(pFileSystem, L"REFS", 4) == 0)
    {
        outFileSystemType = VDS_FST_REFS;
    }
    else
    {
        TRACE_ERROR(L"Invalid Filesystem type specified: %s\n", pFileSystem);
        return FALSE;
    }

    // All processing is completed successfully!
    return TRUE;
}

// Helper to determine if the specified directory exists or not.
BOOL DoesDirectoryExist(PWCHAR pwszPath)
{
  DWORD dwAttrib = GetFileAttributes(pwszPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// This is invoked by the service to provision a volume and checks if the volume already exists. If it does, it will
// simply reassign it the mountpoint passed in. Otherwise, it will create a new LUN.
SFBDCTLLIB_API BOOL ProvisionLUN(HANDLE hDevice, PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, LPWSTR pFileSystem, PWCHAR pMountPoint, DWORD dwServicePort)
{
    string sLUID = BlockStore::StringUtil::ToString(wstring(pLUID), true);
    SFBDCtlLibTrace::SetTracePrefix(sLUID);

    // ProvisionLUN's contract expects a mountpoint to be specified since it is invoked for automatic volume mounts.
    if ((pMountPoint == NULL) || (!DoesDirectoryExist(pMountPoint)))
    {
        TRACE_ERROR(L"Invalid mountpoint specified\n");
        return FALSE;
    }

    // If the volume with the specified LUID exists, then we will simply update its mount point.
    SFBDCommand cmdProvisionLu;
    memset(&cmdProvisionLu, 0, sizeof(cmdProvisionLu));
    BOOL fDoesVolumeExist = GetLUInfo(hDevice, pLUID, &cmdProvisionLu, FALSE);
    if (fDoesVolumeExist)
    {
        if (cmdProvisionLu.luInfo.Mounted)
        {
            // Does the configuration match?
            if (cmdProvisionLu.servicePort != dwServicePort)
            {
                TRACE_ERROR("Found an existing volume for LUID %s but service port mismatched. Expected: %d, Actual: %d\n", sLUID.data(), dwServicePort, cmdProvisionLu.servicePort);
                fDoesVolumeExist = FALSE;
            }
            else
            {
                // Locate the volume and update its mountpoint
                TRACE_INFO("Updating mountpoint of existing volume.\n");
                fDoesVolumeExist = VdsUpdateMountpointForVolume(cmdProvisionLu.luInfo.PathId, cmdProvisionLu.luInfo.TargetId, cmdProvisionLu.luInfo.Lun, pMountPoint);
            }
        }
        else
        {
            TRACE_ERROR("Found an existing volume for LUID %s but it is marked unmounted.\n", sLUID.data());
            fDoesVolumeExist = FALSE;
        }
    }
    else
    {
        // The volume does not exist, so create it now.
        TRACE_INFO("Provisioning volume: %s\n", sLUID.data());
        fDoesVolumeExist = CreateLUN(hDevice, pLUID, pSize, pSizeType, pFileSystem, pMountPoint, dwServicePort);
    }

    return fDoesVolumeExist;
}

// Create the specified LU
SFBDCTLLIB_API BOOL CreateLUN(HANDLE hDevice, PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, LPWSTR pFileSystem, PWCHAR pMountPoint, DWORD dwServicePort)
{
    int iSize = 0;
    DiskSizeUnit sizeType = MB;
    VDS_FILE_SYSTEM_TYPE fsType = VDS_FST_RAW;

    if (!ParseVolumeArgs(pLUID, pSize, pSizeType, pFileSystem, iSize, sizeType, fsType))
    {
        // Unable to process the volume args.
        return FALSE;
    }

    SFBDCommand cmdCreateLU;
    memset(&cmdCreateLU, 0, sizeof(cmdCreateLU));
    cmdCreateLU.command = SFBDCommandCode::CreateLU;
    cmdCreateLU.luInfo.diskSize = (ULONG)iSize;
    cmdCreateLU.luInfo.sizeUnit = sizeType;
    cmdCreateLU.servicePort = dwServicePort;
    wcscpy_s(cmdCreateLU.luInfo.wszLUID, _countof(cmdCreateLU.luInfo.wszLUID), pLUID);

    SFBDCommand responseCommand;
    memset(&responseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &cmdCreateLU, &responseCommand);
    if (!fRetVal)
    {
        TRACE_ERROR("Unable to create LU!\n");
        DisplayErrorMessage(&responseCommand);
    }
    else
    {
        fRetVal = FALSE;

        // Format the disk we just created.
        IVdsService *pVDSService = NULL;

        // Default partition type is MBR
        VDS_PARTITION_STYLE partitionStyle = VDS_PST_MBR;

        if ((sizeType == TB) && (iSize > 2))
        {
            // For disks > 2TB, we will use GPT partition type
            partitionStyle = VDS_PST_GPT;
        }

        TRACE_INFO("Initializing Virtual Disk Service...\n");
        HRESULT hrStatus = InitVDSService(&pVDSService);
        if (SUCCEEDED(hrStatus))
        {
            TRACE_INFO("Enumerating the VSD volumes...\n");

            int iLURefreshRetryCount = 0;

            while (iLURefreshRetryCount < MAX_LU_REFRESH_RETRY_COUNT)
            {
                // Delay some time in case we could not find the raw disks we just created because such operations are asynchronouse
                Sleep(DELAY_BETWEEN_LU_REFRESH_RETRY_IN_MS);

                hrStatus = pVDSService->Reenumerate();
                if (SUCCEEDED(hrStatus))
                {
                    if ((fsType != VDS_FST_NTFS) && (pMountPoint))
                    {
                        TRACE_WARNING("Mountpoint can only be set when formatting as NTFS. Will ignore the specified mountpoint and will assign the next available drive letter instead.\n");
                        pMountPoint = NULL;
                    }

                    fRetVal = InitializeSFBDDisk(pVDSService, partitionStyle, responseCommand.luInfo.PathId, responseCommand.luInfo.TargetId, responseCommand.luInfo.Lun, fsType, DEFAULT_VOLUME_LABEL, pMountPoint);
                    if (fRetVal)
                    {
                        TRACE_INFO("LU created on Bus: %d, Target: %d, LU: %d\n", responseCommand.luInfo.PathId, responseCommand.luInfo.TargetId, responseCommand.luInfo.Lun);
                    }
                }
                else
                {
                    TRACE_ERROR("Volume enumeration failed due to HRESULT: %08X\n", hrStatus);
                }

                if (!fRetVal)
                {
                    // Retry refreshing the storage stack for the created LU.
                    iLURefreshRetryCount++;
                    TRACE_WARNING("LU refresh retry:%d\n", iLURefreshRetryCount);
                }
                else
                {
                    // Found and initialized, so nothing to do.
                    break;
                }
            }

            ShutdownVDSService(pVDSService);
        }
        else
        {
            TRACE_ERROR("Unable to initialize Virtual Disk Service due to HRESULT: %08X\n", hrStatus);
        }

        if (!fRetVal)
        {
            // If we could not create a real disk, delete the LU.
            DeleteLU(hDevice, pLUID);
            TRACE_ERROR("Unable to create a disk for the LU.\n");
        }
    }

    return fRetVal;
}

// Displays the LU Info from the specified Command that is expected to contain the details (usually as a response from the driver).
SFBDCTLLIB_API void DisplayLUInfo(PSFBDCommand pCommand)
{
    TRACE_INFO("LU is created on Bus: %d, Target: %d, LU: %d\n", pCommand->luInfo.PathId, pCommand->luInfo.TargetId, pCommand->luInfo.Lun);
    TRACE_INFO(L"LU ID: %s\n", pCommand->luInfo.wszLUID);
    TRACE_INFO("Disk size: %d ", pCommand->luInfo.diskSize);

    switch (pCommand->luInfo.sizeUnit)
    {
    case DiskSizeUnit::MB:
        TRACE_INFO("MB");
        break;
    case DiskSizeUnit::GB:
        TRACE_INFO("GB");
        break;
    case DiskSizeUnit::TB:
        TRACE_INFO("TB");
        break;
    }

    TRACE_INFO("\n");
    TRACE_INFO("Enabled: %s\n", (pCommand->luInfo.Enabled == TRUE) ? "Yes" : "No");
    TRACE_INFO("Mounted: %s\n", (pCommand->luInfo.Mounted == TRUE) ? "Yes" : "No");
    TRACE_INFO("Service Port: %d\n", pCommand->servicePort);
}

// Returns information for the specified LU
SFBDCTLLIB_API BOOL GetLUInfo(HANDLE hDevice, PWCHAR pLUID, SFBDCommand* pResponseCommand, BOOL fDisplayInfo)
{
    SFBDCommand cmdGetInfoForLun;
    memset(&cmdGetInfoForLun, 0, sizeof(cmdGetInfoForLun));
    cmdGetInfoForLun.command = SFBDCommandCode::GetInfoForLu;
    wcscpy_s(cmdGetInfoForLun.luInfo.wszLUID, _countof(cmdGetInfoForLun.luInfo.wszLUID), pLUID);;

    SFBDCommand responseCommand;
    if (pResponseCommand == NULL)
    {
        pResponseCommand = &responseCommand;
    }

    memset(pResponseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &cmdGetInfoForLun, pResponseCommand);
    if (!fRetVal)
    {
        if (fDisplayInfo)
        {
            TRACE_ERROR("Unable to get information for LU!\n");
            DisplayErrorMessage(pResponseCommand);
        }
    }
    else
    {
        if (fDisplayInfo)
        {
            DisplayLUInfo(pResponseCommand);
        }
    }

    return fRetVal;
}

// Deletes the specified LU
SFBDCTLLIB_API BOOL DeleteLU(HANDLE hDevice, PWCHAR pLUID)
{
    SFBDCommand cmdDeleteLu;
    memset(&cmdDeleteLu, 0, sizeof(cmdDeleteLu));
    cmdDeleteLu.command = SFBDCommandCode::DeleteLu;
    wcscpy_s(cmdDeleteLu.luInfo.wszLUID, _countof(cmdDeleteLu.luInfo.wszLUID), pLUID);

    SFBDCommand responseCommand;
    memset(&responseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &cmdDeleteLu, &responseCommand);
    TRACE_INFO("LU was%s deleted.\n", (fRetVal == TRUE) ? "" : " not");
    if (!fRetVal)
    {
        DisplayErrorMessage(&responseCommand);
    }

    return fRetVal;
}

// Unmounts/mounts the specified LU
SFBDCTLLIB_API BOOL MountUnmountLU(HANDLE hDevice, PWCHAR pLUID, SFBDCommandCode command)
{
    SFBDCommand cmdUnmountLu;
    memset(&cmdUnmountLu, 0, sizeof(cmdUnmountLu));
    cmdUnmountLu.command = command;
    wcscpy_s(cmdUnmountLu.luInfo.wszLUID, _countof(cmdUnmountLu.luInfo.wszLUID), pLUID);

    SFBDCommand responseCommand;
    memset(&responseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &cmdUnmountLu, &responseCommand);

    if (command == SFBDCommandCode::UnmountLu)
    {
        TRACE_INFO("LU was%s unmounted.\n", (fRetVal == TRUE) ? "" : " not");
    }
    else
    {
        TRACE_INFO("LU was%s mounted.\n", (fRetVal == TRUE) ? "" : " not");
    }

    if (!fRetVal)
    {
        DisplayErrorMessage(&responseCommand);
    }

    return fRetVal;
}

SFBDCTLLIB_API BOOL ListAllLU(HANDLE hDevice)
{
    // Send the command to get the LU count
    SFBDCommand cmdListAllLu;

    memset(&cmdListAllLu, 0, sizeof(cmdListAllLu));
    cmdListAllLu.command = SFBDCommandCode::GetLuCount;

    SFBDCommand responseCommand;
    memset(&responseCommand, 0, sizeof(SFBDCommand));

    BOOL fRetVal = SendCommandToDriver(hDevice, &cmdListAllLu, &responseCommand);
    if (fRetVal)
    {
        // We have the count of LUs - now, iterate over them
        ULONG iNum = responseCommand.numLu;
        TRACE_INFO("%d LU managed by device adapter.\n", iNum);
        if (iNum > 0)
        {
            TRACE_INFO("Listing valid LUs : \n\n");

            for (ULONG index = 0; index < iNum; index++)
            {
                memset(&cmdListAllLu, 0, sizeof(cmdListAllLu));
                cmdListAllLu.command = SFBDCommandCode::GetInfoForLuIndex;
                cmdListAllLu.luIndex = index;

                memset(&responseCommand, 0, sizeof(SFBDCommand));

                BOOL fGotInfo = SendCommandToDriver(hDevice, &cmdListAllLu, &responseCommand);
                // List only LUs that are enabled.
                if (fGotInfo)
                {
                    if (responseCommand.luInfo.Enabled)
                    {
                        TRACE_INFO("LU #%d\n", index);
                        DisplayLUInfo(&responseCommand);
                        TRACE_INFO("\n");
                    }
                }
                else
                {
                    TRACE_ERROR("Unable to get details for LU at index #%d\n", index);
                    DisplayErrorMessage(&responseCommand);
                    TRACE_ERROR("\n");
                }
            }
        }
    }
    else
    {
        TRACE_ERROR("Unable to get the LU count.\n");
        DisplayErrorMessage(&responseCommand);
    }

    return fRetVal;
}
