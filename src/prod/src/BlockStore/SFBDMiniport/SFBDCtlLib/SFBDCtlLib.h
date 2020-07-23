// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !defined(DRIVER_BUILD)

#include <SetupAPI.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <initguid.h>
#include <vds.h>
#include <newdev.h>

#endif // !defined(DRIVER_BUILD)

#include "..\sfbdctl\SFBDCommands.h"

typedef enum
{
    Success = 0,
    GeneralFailure,
    DeviceNotAvailable,
    DeviceNotFound,
    InvalidArguments,
    FailedToRegisterLU,
    FailedToUnregisterLU,
    FailedToUpdateDeviceList,
    LUNotMounted,
    FailedToUnmountLU,
    LUAlreadyMounted,
    FailedToMountLU,
    InvalidLuIndex,
    UnableToFetchSrbMdl,
    UnableToMapDataBuffer,
    DataBufferLengthMismatch,
    DeviceListInitializedFailed,
    UnableToFetchLUList,
    UnsupportedCommand

} SFBDCommandStatus;

typedef enum
{
    MB,
    GB,
    TB
} DiskSizeUnit;


typedef enum
{
    UnInstalledSucessfully = 0,
    NotFound = 1,
    UnInstallFailed = 2
} UninstallErrorCodes;

// Describes the basic information for a HBA.
typedef struct _BasicInfo
{
    // Total # of Buses per Adapter
    UCHAR           BusCount;

    // Total # of Target controllers/devices per Bus
    UCHAR           TargetCount;

    // Total # of LU per Target controller/device
    UCHAR           LUCount;

} BasicInfo, *PBasicInfo;


// Describes the information returned for a SRB;
typedef struct __SRBInfo
{
    // Reference to the SRB for which details need to be fetched.
    PVOID SRBAddress;

    // Address of the SRB's R/W buffer.
    PVOID UserModeBuffer;

    // Length of the SRB dataBuffer
    ULONG DataBufferLength;

} SRBInfo, *PSRBInfo;

#define MAX_WCHAR_SIZE_LUID 50

// Describes the LU to be created.
typedef struct _LUType
{
    // Unique ID for LU.
    WCHAR wszLUID[MAX_WCHAR_SIZE_LUID];

    // Disk size type
    DiskSizeUnit sizeUnit;

    // Disk size
    ULONG diskSize;

    // Fields describing where the LU was created
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    BOOLEAN  Enabled;
    BOOLEAN  Mounted;
} LUType, *PLUType;

// Describes the command to be issues to the driver with buffers to get data from.
typedef struct __SFBDCommand
{
    // Command to be issued
    SFBDCommandCode command;

    // Completion status of the command.
    SFBDCommandStatus cmdStatus;

    // Used when GetBasicInfo command.
    //
    // Contains basic adapater information
    BasicInfo       basicInfo;

    // Used when requesting SRBInformation
    //
    // Contains SRB details from the driver.
    SRBInfo         srbInfo;

    // Used with CreateLU command.
    //
    // Specifies the details of the LU to be created
    // and returns with the details of where it was created.
    LUType          luInfo;

    // Number of LUs managed by the device.
    ULONG           numLu;

    // Zero-based LU Index to fetch information for.
    // Must be < NumLU.
    ULONG           luIndex;

    // Port of the BlockStore service
    ULONG           servicePort;
} SFBDCommand, *PSFBDCommand;

// GPT data partition type (from https://msdn.microsoft.com/en-us/library/windows/desktop/aa365449(v=vs.85).aspx).
#define PARTITION_BASIC_DATA_GUID {0xebd0a0a2, 0xb9e5, 0x4433, 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7}

#define _SafeRelease(x) {if (NULL != x) { x->Release(); x = NULL; } }

#define SFBD_ADAPTOR_FRIENDLY_NAME L"Service Fabric BlockDevice Virtual Miniport Driver"

#define DEFAULT_VOLUME_LABEL L"SFBDVolume"

// Define IOCTL codes for reading/writing data between user and kernel mode
#define FUNCTION_READ       2079
#define FUNCTION_WRITE      2080

// The DeviceType must be IOCTL_SCSI_BASE for the IOCTL to be delivered to us.
#define IOCTL_SFBD_READ_FROM_USERMODE       CTL_CODE(IOCTL_SCSI_BASE, FUNCTION_READ, METHOD_IN_DIRECT, FILE_READ_ACCESS)
#define IOCTL_SFBD_READ_FROM_SRB            CTL_CODE(IOCTL_SCSI_BASE, FUNCTION_WRITE, METHOD_OUT_DIRECT, FILE_WRITE_ACCESS)

#if !defined(DRIVER_BUILD)

#define SFBD_HW_ID L"ROOT\\SFBDMINIPORT"
#define MAX_CLASS_NAME_LEN 32
#define DELAY_BETWEEN_DEV_NODE_CREATE_AND_UPDATE_DRIVER_IN_MS  50

// Maximum number of times we will attempt to query storage stack for the LU
// we just created.
#define MAX_LU_REFRESH_RETRY_COUNT 5
#define DELAY_BETWEEN_LU_REFRESH_RETRY_IN_MS 1000

// These commands are invoked by the controller app
SFBDCTLLIB_API void DisplayCommandLineHelp();
SFBDCTLLIB_API HANDLE ConnectToDevice(PDWORD pLastError);
SFBDCTLLIB_API BOOL DisplayDeviceInfo(HANDLE hDevice);
SFBDCTLLIB_API BOOL CreateLUN(HANDLE hDevice, PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, LPWSTR FileSystem, PWCHAR pMountPoint, DWORD dwServicePort);
SFBDCTLLIB_API BOOL ProvisionLUN(HANDLE hDevice, PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, LPWSTR FileSystem, PWCHAR pMountPoint, DWORD dwServicePort);
SFBDCTLLIB_API BOOL GetLUInfo(HANDLE hDevice, PWCHAR pLUID, SFBDCommand* pResponseCommand, BOOL fDisplayInfo);
SFBDCTLLIB_API BOOL DeleteLU(HANDLE hDevice, PWCHAR pLUID);
SFBDCTLLIB_API BOOL MountUnmountLU(HANDLE hDevice, PWCHAR pLUID, SFBDCommandCode command);
SFBDCTLLIB_API BOOL ListAllLU(HANDLE hDevice);
SFBDCTLLIB_API UninstallErrorCodes UninstallSFBDDevice(LPCWSTR pPathToINF, BOOL& fRestartSystem, BOOL fDuringInstallation);
SFBDCTLLIB_API BOOL InstallSFBDDevice(LPCWSTR pPathToINF, BOOL& fRestartSystem);

// These commands are invoked by the service
SFBDCTLLIB_API BOOL ReadFromUserBuffer(PVOID pSrb, PVOID pBuffer, ULONG Length, PDWORD pError);
SFBDCTLLIB_API BOOL ReadFromSRBBuffer(PVOID pSrb, PVOID pBuffer, ULONG Length, PDWORD pError);
SFBDCTLLIB_API BOOL RefreshServiceLUList(DWORD servicePort);
SFBDCTLLIB_API BOOL ShutdownVolumesForService(DWORD dwServicePort);

// Helpers to work with VDS
HRESULT InitVDSService(IVdsService **ppVDSService);
VOID ShutdownVDSService(IVdsService *pVDSService);
BOOL InitializeSFBDDisk(IVdsService *pVDSService, VDS_PARTITION_STYLE partitionStyle, UCHAR PathId, UCHAR TargetId, UCHAR Lun, VDS_FILE_SYSTEM_TYPE fsType, PWCHAR VolumeLabel, PWCHAR pMountPoint);
BOOL VdsUpdateMountpointForVolume(UCHAR PathId, UCHAR TargetId, UCHAR Lun, PWCHAR pMountPoint);

#endif // !defined(DRIVER_BUILD)
