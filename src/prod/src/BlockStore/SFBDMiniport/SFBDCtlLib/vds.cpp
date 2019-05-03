// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SFBDCtlLib.h"

// Initializes the Virtual Disk Service
HRESULT InitVDSService(IVdsService **ppVDSService)
{
    HRESULT hr;
    IVdsService *pService = NULL;
    IVdsServiceLoader *pLoader = NULL;

    // Avoid to call CoInitializeSecurity twice which will return RPC_E_TOO_LATE
    static bool fInitializedCOMSecurity = false;

    // Init the outgoing args
    *ppVDSService = NULL;

    // Initialize COM - Proceed only if we succeed initializing it (or someone already initialized for us)
    hr = CoInitialize(NULL);
    if (hr ==  RPC_E_CHANGED_MODE)
    {
        TRACE_INFO("COM is already initialized.\n");
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (!fInitializedCOMSecurity)
        {
            hr = CoInitializeSecurity(
                NULL,
                -1,
                NULL,
                NULL,
                RPC_C_AUTHN_LEVEL_CONNECT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                0,
                NULL
            );
            if (hr == RPC_E_TOO_LATE)
            {
                TRACE_INFO("COMSecurity is already initialized.\n");
                hr = S_OK;
            }
        }

        if (SUCCEEDED(hr))
        {
            fInitializedCOMSecurity = true;
            // Get reference to the VDSLoader
            hr = CoCreateInstance(CLSID_VdsLoader,
                NULL,
                CLSCTX_LOCAL_SERVER,
                IID_IVdsServiceLoader,
                (void **)&pLoader);

            if (SUCCEEDED(hr))
            {
                // Fetch the reference to VDSService
                hr = pLoader->LoadService(NULL, &pService);
            }

            //
            // Release the loader since it is no longer required
            //
            _SafeRelease(pLoader);

            if (SUCCEEDED(hr))
            {
                hr = pService->WaitForServiceReady();
                if (SUCCEEDED(hr))
                {
                    // Service is initialized
                    *ppVDSService = pService;
                }
                else
                {
                    _SafeRelease(pService);
                }
            }
        }
    }

    return hr;
}

// Shuts down the VDS service.
VOID ShutdownVDSService(IVdsService *pVDSService)
{
    _SafeRelease(pVDSService);
}

// Helper release COM allocated strings
void FreeCOMString(LPWSTR pStrToFree)
{
    if (pStrToFree != NULL)
        CoTaskMemFree(pStrToFree);
}

// Enumerates the Software providers (registered with VDS) for the one that 
// allows single disk to be in a VdsPack.
HRESULT GetPackForSingleDisk(IVdsService *pVDSService, IVdsPack **ppVdsPack)
{
    IEnumVdsObject *pEnumVdsObject = NULL;
    IVdsProvider *pVDSProvider = NULL;
    IVdsSwProvider *pVDSSwProvider = NULL;
    IUnknown *pIUnknown = NULL;

    // Init the outgoing arguments
    *ppVdsPack = NULL;

    HRESULT hr = pVDSService->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS, &pEnumVdsObject);
    if (SUCCEEDED(hr))
    {
        DWORD dwFetchedCount = 0;
        BOOL fDone = FALSE;

        while (!fDone)
        {
            hr = pEnumVdsObject->Next(1, &pIUnknown, &dwFetchedCount);
            if (hr == S_OK)
            {
                // QI for IVdsProvider interface
                hr = pIUnknown->QueryInterface(IID_IVdsProvider, (void **)&pVDSProvider);
                if (SUCCEEDED(hr))
                {
                    // Fetch the provider properties
                    VDS_PROVIDER_PROP props;
                    memset(&props, 0, sizeof(props));

                    hr = pVDSProvider->GetProperties(&props);
                    if (SUCCEEDED(hr))
                    {
                        FreeCOMString(props.pwszName);

                        FreeCOMString(props.pwszVersion);

                        // Is this the provider we are looking for?
                        if (props.ulFlags & VDS_PF_ONE_DISK_ONLY_PER_PACK)
                        {
                            // Yes, this is the one.
                            hr = pVDSProvider->QueryInterface(IID_IVdsSwProvider, (void **)&pVDSSwProvider);
                            if (SUCCEEDED(hr))
                            {
                                // Create a pack
                                hr = pVDSSwProvider->CreatePack(ppVdsPack);
                            }

                            break;
                        }
                    }
                }
            }
            else
            {
                fDone = TRUE;
            }
        }
    }

    _SafeRelease(pVDSSwProvider);
    _SafeRelease(pVDSProvider);
    _SafeRelease(pIUnknown);
    _SafeRelease(pEnumVdsObject);

    return hr;
}

// Loops over the drive letters to find the one that is available.
HRESULT GetNextAvailableDriveLetter(IVdsService *pVDSService, PWCHAR pAvailableDriveLetter)
{
    HRESULT hr = S_OK;
    VDS_DRIVE_LETTER_PROP driveLetterList[26];

    *pAvailableDriveLetter = NULL;
    memset(&driveLetterList, 0, sizeof(driveLetterList));

    hr = pVDSService->QueryDriveLetters(L'A', 26, driveLetterList);
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < 26; i++)
        {
            if (!driveLetterList[i].bUsed)
            {
                *pAvailableDriveLetter = driveLetterList[i].wcLetter;
                break;
            }
        }
    }

    if (*pAvailableDriveLetter == NULL)
    {
        hr = VDS_E_INVALID_DRIVE_LETTER;
    }

    return hr;
}

// Create a partition of the specified type
HRESULT CreatePartition(IVdsDisk *pVDSDisk, VDS_PARTITION_STYLE partitionStyle, BOOLEAN fBootablePartition, BYTE partitionType, PULONGLONG pOffSet, ULONGLONG partitionSize, VDS_OBJECT_ID* pVolumeID)
{
    HRESULT hr = S_OK;
    IVdsAsync *pVDSAsync = NULL;
    IVdsAdvancedDisk *pVDSAdvancedDisk = NULL;

    hr = pVDSDisk->QueryInterface(IID_IVdsAdvancedDisk, (void **)&pVDSAdvancedDisk);
    if (SUCCEEDED(hr))
    {
        // Create the Partition
        CREATE_PARTITION_PARAMETERS params;
        VDS_ASYNC_OUTPUT output;
        
        memset(&output, 0, sizeof(output));
        memset(&params, 0, sizeof(params));
        
        params.style = partitionStyle;
        if (partitionStyle == VDS_PST_MBR)
        {
            params.MbrPartInfo.bootIndicator = fBootablePartition;
            params.MbrPartInfo.partitionType = partitionType;
        }
        else
        {
            // Basic data partition
            params.GptPartInfo.partitionType = PARTITION_BASIC_DATA_GUID;

            // Create and assign a unique GUID
            params.GptPartInfo.partitionId = GUID_NULL;

            params.GptPartInfo.attributes = 0;

            // No partition name
            params.GptPartInfo.name[0] = 0;
        }

        hr = pVDSAdvancedDisk->CreatePartition(*pOffSet, partitionSize, &params, &pVDSAsync);
        if (SUCCEEDED(hr))
        {
            HRESULT hrCreatePart = pVDSAsync->Wait(&hrCreatePart, &output);
            if (SUCCEEDED(hr))
            {
                hr = hrCreatePart;
            }
       }

        // Assign the drive letter if partition was created successfully
        if (SUCCEEDED(hr))
        {
            // Fetch the updated offset, and the volume ID corresponding to the partition, from the AsyncOutput
            *pOffSet = output.cp.ullOffset;
            *pVolumeID = output.cp.volumeId;
        }
    }

    _SafeRelease(pVDSAsync);
    _SafeRelease(pVDSAdvancedDisk);

    return hr;
}

// Formats the volume specified by the VolumeID.
HRESULT FormatVolume(IVdsService *pVDSService, VDS_OBJECT_ID volumeID, VDS_FILE_SYSTEM_TYPE FilesystemType, PWCHAR VolumeLabel, BOOL ShouldQuickFormat, BOOL ShouldEnableCompression, WCHAR driveLetter, PWCHAR pMountPoint)
{
    HRESULT hr = S_OK;
    IUnknown *pIUnknown = NULL;
    IVdsVolume *pVDSVolume = NULL;
    IVdsVolumeMF *pVDSVolumeMF = NULL;
    IVdsAsync *pVDSAsync = NULL;

    // Get the IUnknown for Volume
    hr = pVDSService->GetObjectW(volumeID, VDS_OT_VOLUME, &pIUnknown);
    if (SUCCEEDED(hr))
    {
        // Ask for the IVdsVolume interface
        hr = pIUnknown->QueryInterface(IID_IVdsVolume, (void **)&pVDSVolume);
        if (SUCCEEDED(hr))
        {
            // Ask for the IVdsVolumeMF interface
            hr = pVDSVolume->QueryInterface(IID_IVdsVolumeMF, (void **)&pVDSVolumeMF);
            if (SUCCEEDED(hr))
            {
                // Attempt to format the disk
                hr = pVDSVolumeMF->Format(FilesystemType, VolumeLabel, 0, TRUE, ShouldQuickFormat, ShouldEnableCompression, &pVDSAsync);
                if (SUCCEEDED(hr))
                {
                    HRESULT hrWait = S_OK;
                    VDS_ASYNC_OUTPUT output;

                    memset(&output, 0, sizeof(output));

                    hr = pVDSAsync->Wait(&hrWait, &output);
                    if (SUCCEEDED(hr))
                    {
                        hr = hrWait;
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Assign the drive letter to the volume
                        WCHAR accessPath[MAX_PATH] = L"X:\\";
                        PWCHAR pAccessPathForVolume = accessPath;

                        if (pMountPoint)
                        {
                            pAccessPathForVolume = pMountPoint;
                        }
                        else
                        {
                            // Set the drive letter
                            accessPath[0] = driveLetter;
                        }

                        TRACE_INFO(L"Mounting volume at %s\n", pAccessPathForVolume);

                        hr = pVDSVolumeMF->AddAccessPath(pAccessPathForVolume);
                    }
                }
            }
        }
    }

    _SafeRelease(pVDSAsync);
    _SafeRelease(pVDSVolumeMF);
    _SafeRelease(pVDSVolume);
    _SafeRelease(pIUnknown);

    return hr;
}

// Initializes an unallocated disk, including bringing it online, after adding it to a VdsPack
// so that it can be treated like a VdsVolume.
HRESULT InitDisk(IVdsService *pVDSService, VDS_OBJECT_ID diskID, VDS_PARTITION_STYLE partitionStyle)
{
    HRESULT hr = S_OK;
    IVdsPack *pVDSPack = NULL;

    // Get VdsPack for single disk
    hr = GetPackForSingleDisk(pVDSService, &pVDSPack);
    if (SUCCEEDED(hr))
    {
        // Insert the disk to the pack
        hr = pVDSPack->AddDisk(diskID, partitionStyle, FALSE);
    }

    _SafeRelease(pVDSPack);

    return hr;
}

// Gets the maximum size for the volume and the offset for the partition
HRESULT GetMaximumVolumeSize(IVdsDisk *pVDSDisk, PULONGLONG pOffset, PULONGLONG pMaxVolSize)
{
    HRESULT hr = S_FALSE;
    IVdsDisk3 *pVDSDisk3 = NULL;

    *pMaxVolSize = 0;

    hr = pVDSDisk->QueryInterface(IID_IVdsDisk3, (void **)&pVDSDisk3);
    if (SUCCEEDED(hr))
    {
        // Query for the free extents
        VDS_DISK_FREE_EXTENT *pFreeExtentList = NULL;
        LONG numFreeExtents = 0;

        hr = pVDSDisk3->QueryFreeExtents(0, &pFreeExtentList, &numFreeExtents);
        if (hr == S_OK)
        {
            // Add up the various free extents
            for (int i = 0; i < numFreeExtents; i++)
            {
                // Get alignment only for the first extent
                if (i == 0)
                {
                    *pOffset = pFreeExtentList[i].ullOffset;
                }

                // Sum up the sizes of all extents for volume size.
                *pMaxVolSize += (pFreeExtentList+i)->ullSize;
            }

            CoTaskMemFree(pFreeExtentList);
        }
    }

    _SafeRelease(pVDSDisk3);

    return hr;
}

HRESULT BringDiskOnline(IVdsDisk* pVDSDisk)
{
    HRESULT hr = E_FAIL;

    IVdsDiskOnline * pDiskOnline  = NULL;
    hr = pVDSDisk->QueryInterface( IID_IVdsDiskOnline, (void **)&pDiskOnline );
    if ( SUCCEEDED(hr) )
    {
        hr = pDiskOnline->Online();
    }

    _SafeRelease(pDiskOnline);

    return hr;
}

BOOL ends_with(LPWSTR searchWithin, LPWSTR searchFor)
{
    // If the string to search for is longer than the string
    // in which we have to search, then we will never find it.
    if (wcslen(searchFor) > wcslen(searchWithin))
        return FALSE;

    LPWSTR pSearchFrom = searchWithin + (wcslen(searchWithin) - wcslen(searchFor));
    if (wcsncmp(pSearchFrom, searchFor, wcslen(searchFor)) == 0)
        return TRUE;
    else
        return FALSE;
}

// Returns a boolean indicating if the disk properties represent a SFBD disk at the specified
// SCSI path.
BOOL IsSFBDDisk(VDS_DISK_PROP* pDiskProp, UCHAR PathId, UCHAR TargetId, UCHAR Lun)
{
    BOOL fIsMatch = FALSE;

    if (wcsncmp(pDiskProp->pwszAdaptorName, SFBD_ADAPTOR_FRIENDLY_NAME, wcslen(SFBD_ADAPTOR_FRIENDLY_NAME)) == 0)
    {
        // Form the path we are looking for
        WCHAR diskScsiPath[50];
        memset(diskScsiPath, 0, sizeof(diskScsiPath));

        swprintf_s(diskScsiPath, _countof(diskScsiPath), L"Path%dTarget%dLun%d", PathId, TargetId, Lun);
        fIsMatch = ends_with(pDiskProp->pwszDiskAddress, diskScsiPath);
    }

    return fIsMatch;
}

// Returns the VolumeId corresponding to the first partition on the specified disk.
// For Service Fabric BlockStore based volumes, we only ever have a single partition.
BOOL GetVolumeIdForSFBDDisk(IVdsDisk* pVdsDisk, VDS_OBJECT_ID* pFoundDiskId)
{
    BOOL fGotVolumeIdForDisk = FALSE;
    VDS_DISK_EXTENT* pArrayDiskExtents = NULL;
    LONG numDiskExtents = 0;

    HRESULT hr = pVdsDisk->QueryExtents(&pArrayDiskExtents, &numDiskExtents);
    if (SUCCEEDED(hr))
    {
        for(LONG i = 0; i < numDiskExtents; i++)
        {
            // Skip extents that are not associated with a volume.
            if (pArrayDiskExtents[i].volumeId == GUID_NULL)
                continue;

            *pFoundDiskId = pArrayDiskExtents[i].volumeId;
            fGotVolumeIdForDisk = TRUE;
            break;
        }
        
        if (numDiskExtents == 0)
        {
            TRACE_INFO("Unexpected number of extents for Service Fabric Volume: %d\n", numDiskExtents);
        }
    }
    else
    {
        TRACE_ERROR("Failed to get extents for Service Fabric Volume due to HRESULT: %08X\n", numDiskExtents);
    }

    return fGotVolumeIdForDisk;
}

// Updates the mount point for the specified volume object
BOOL UpdateDiskMountpoint(IVdsService* pVdsService, PWCHAR pMountPoint, VDS_OBJECT_ID volumeId)
{
    IUnknown *pIUnknown = NULL;
    IVdsVolume *pVDSVolume = NULL;
    IVdsVolumeMF *pVDSVolumeMF = NULL;
    
    // Get the IUnknown for Volume
    HRESULT hr = pVdsService->GetObjectW(volumeId, VDS_OT_VOLUME, &pIUnknown);
    if (SUCCEEDED(hr))
    {
        // Ask for the IVdsVolume interface
        hr = pIUnknown->QueryInterface(IID_IVdsVolume, (void **)&pVDSVolume);
        if (SUCCEEDED(hr))
        {
            // Ask for the IVdsVolumeMF interface
            hr = pVDSVolume->QueryInterface(IID_IVdsVolumeMF, (void **)&pVDSVolumeMF);
            if (SUCCEEDED(hr))
            {
                // Query existing access paths and remove them
                LPWSTR* pAccessPathArray = NULL;
                LONG numAccessPaths = 0;
                hr = pVDSVolumeMF->QueryAccessPaths(&pAccessPathArray, &numAccessPaths);
                if (SUCCEEDED(hr) && (numAccessPaths > 0))
                {
                    for(LONG index = 0; index < numAccessPaths; index++)
                    {
                        pVDSVolumeMF->DeleteAccessPath(pAccessPathArray[index], FALSE);
                        CoTaskMemFree(pAccessPathArray[index]);
                    }

                    // Free the array
                    CoTaskMemFree(pAccessPathArray);
                }

                // Add the new access path to be the mountpoint
                hr = pVDSVolumeMF->AddAccessPath(pMountPoint);
            }
        }
    }
    else
    {
        TRACE_ERROR("Failed to get volume object for VolumeID.\n");
    }

    _SafeRelease(pVDSVolumeMF);
    _SafeRelease(pVDSVolume);
    _SafeRelease(pIUnknown);

    if (FAILED(hr))
    {
        TRACE_ERROR("Unable to update mountpoint due to HRESULT: %08X\n", hr);
    }
    else
    {
        TRACE_INFO("Mountpoint successfully updated.\n");
    }

    return SUCCEEDED(hr)?TRUE:FALSE;
}

// Enumerates the disks in the specified VdsPack
HRESULT EnumDisks(IVdsPack* pVdsPack, UCHAR PathId, UCHAR TargetId, UCHAR Lun, PBOOL pFoundDisk, VDS_OBJECT_ID* pFoundDiskId)
{
    IEnumVdsObject *pEnumDisks = NULL;
    HRESULT hr = pVdsPack->QueryDisks(&pEnumDisks);
    if (hr == S_OK)
    {
        // Enumerate through each provider and process their packs
        while(true)
        {
            DWORD dwFetched = 0;
            IUnknown* pUnknown = NULL;
            IVdsDisk* pVdsDisk = NULL;
            
            hr = pEnumDisks->Next(1, &pUnknown, &dwFetched);
            if (SUCCEEDED(hr) && (dwFetched > 0))
            {
                // QI for the VdsPack interface
                HRESULT hrLocal = pUnknown->QueryInterface(IID_IVdsDisk, (void **)&pVdsDisk);
                if (SUCCEEDED(hrLocal))
                {
                    // Get the disk properties
                    VDS_DISK_PROP diskProp;
                    memset(&diskProp, 0, sizeof(diskProp));

                    if (SUCCEEDED(pVdsDisk->GetProperties(&diskProp)))
                    {
                        if (IsSFBDDisk(&diskProp, PathId, TargetId, Lun))
                        {
                            *pFoundDisk = TRUE;

                            TRACE_INFO("Bringing disk online...\n");
                            if (diskProp.status == VDS_DS_OFFLINE)
                            {
                                hr = BringDiskOnline(pVdsDisk);
                            }

                            if (SUCCEEDED(hr))
                            {
                                if (GetVolumeIdForSFBDDisk(pVdsDisk, pFoundDiskId))
                                {
                                    hr = S_OK;
                                    TRACE_INFO("Fetched VolumeId for Service Fabric volume at Bus: %d, Target: %d, Lun: %d\n", PathId, TargetId, Lun);
                                }
                                else
                                {
                                    hr = E_FAIL;
                                    TRACE_ERROR("Unable to fetch VolumeId for Service Fabric volume at Bus: %d, Target: %d, Lun: %d\n", PathId, TargetId, Lun);
                                }
                            }
                            else
                            {
                                TRACE_ERROR("Unable to bring disk online due to HRESULT: %X\n", hr);
                            }
                        }

                        // Free the references before looping
                        FreeCOMString(diskProp.pwszDiskAddress);
                        FreeCOMString(diskProp.pwszName);
                        FreeCOMString(diskProp.pwszFriendlyName);
                        FreeCOMString(diskProp.pwszAdaptorName);
                        FreeCOMString(diskProp.pwszDevicePath);
                    }
                }
            }

            _SafeRelease(pVdsDisk);
            _SafeRelease(pUnknown);

            if (FAILED(hr) || (dwFetched == 0) || (*pFoundDisk == TRUE))
            {
                break;
            }
        }

        _SafeRelease(pEnumDisks);
    }
    else
    {
        TRACE_ERROR("Failed to enumerate VdsDisks due to HRESULT: %X\n", hr);
    }

    return hr;
}

// Enumerates packs for the Software Provider
HRESULT EnumPacks(IVdsSwProvider* pSwProvider, UCHAR PathId, UCHAR TargetId, UCHAR Lun, PBOOL pFoundDisk, VDS_OBJECT_ID* pFoundDiskId)
{
    IEnumVdsObject *pEnumPacks = NULL;
    HRESULT hr = pSwProvider->QueryPacks(&pEnumPacks);
    if (hr == S_OK)
    {
        // Enumerate through each provider and process their packs
        while(true)
        {
            DWORD dwFetched = 0;
            IUnknown* pUnknown = NULL;
            IVdsPack* pVdsPack = NULL;

            hr = pEnumPacks->Next(1, &pUnknown, &dwFetched);
            if (SUCCEEDED(hr) && (dwFetched > 0))
            {
                // QI for the VdsPack interface
                HRESULT hrLocal = pUnknown->QueryInterface(IID_IVdsPack, (void **)&pVdsPack);
                if (SUCCEEDED(hrLocal))
                {
                    VDS_PACK_PROP packProp;
                    memset(&packProp, 0, sizeof(packProp));

                    if (SUCCEEDED(pVdsPack->GetProperties(&packProp)))
                    {
                        FreeCOMString(packProp.pwszName);
                        if (packProp.status == VDS_PS_ONLINE) 
                        {
                            hr = EnumDisks(pVdsPack, PathId, TargetId, Lun, pFoundDisk, pFoundDiskId);
                        }
                    }
                }
            }

            _SafeRelease(pVdsPack);
            _SafeRelease(pUnknown);

            if (FAILED(hr) || (dwFetched == 0) || (*pFoundDisk == TRUE))
            {
                break;
            }
        }

        _SafeRelease(pEnumPacks);
    }
    else
    {
        TRACE_ERROR("Failed to enumerate VdsPacks due to HRESULT: %X\n", hr);
    }

    return hr;
}

// Updates the mountpoint of the specified volume
BOOL VdsUpdateMountpointForVolume(UCHAR PathId, UCHAR TargetId, UCHAR Lun, PWCHAR pMountPoint)
{
    // Format the disk we just created.
    IVdsService *pVDSService = NULL;
    BOOL fUpdatedMountpoint = FALSE;

    TRACE_INFO("Initializing Virtual Disk Service...\n");
    HRESULT hrStatus = InitVDSService(&pVDSService);
    if (SUCCEEDED(hrStatus))
    {
        TRACE_INFO("Enumerating the VSD volumes...\n");

        BOOL fFoundDisk = FALSE;
        VDS_OBJECT_ID idFoundDisk = {0};

        hrStatus = pVDSService->Reenumerate();
        if (SUCCEEDED(hrStatus))
        {
            IEnumVdsObject *pEnumProviders = NULL;
            hrStatus = pVDSService->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS, &pEnumProviders);
            if ((hrStatus == S_OK) && (pEnumProviders != NULL))
            {
                // Enumerate through each provider and process their packs
                while(true)
                {
                    DWORD dwFetched = 0;
                    IUnknown* pUnknown = NULL;
                    IVdsSwProvider* pSwProvider = NULL;

                    hrStatus = pEnumProviders->Next(1, &pUnknown, &dwFetched);
                    if (SUCCEEDED(hrStatus) && (dwFetched > 0))
                    {
                        // QI for the Provider interface
                        HRESULT hrLocal = pUnknown->QueryInterface(IID_IVdsSwProvider, (void **)&pSwProvider);
                        if (SUCCEEDED(hrLocal))
                        {
                            // Process packs for the provider
                            hrStatus = EnumPacks(pSwProvider, PathId, TargetId, Lun, &fFoundDisk, &idFoundDisk);
                        }
                    }

                    _SafeRelease(pSwProvider);
                    _SafeRelease(pUnknown);

                    if (FAILED(hrStatus) || (dwFetched == 0) || fFoundDisk)
                    {
                        break;
                    }
                }

                _SafeRelease(pEnumProviders);
            }
            else
            {
                TRACE_ERROR("Failed to enumerate software providers due to HRESULT: %X\n", hrStatus);
            }
        }
        else
        {
            TRACE_ERROR("Volume enumeration failed due to HRESULT: %08X\n", hrStatus);
        }
        
        if (FAILED(hrStatus))
        {
            TRACE_ERROR("Failed to locate volume due to HRESULT: %X\n", hrStatus);
        }
        else if (!fFoundDisk)
        {
            // Retry refreshing the storage stack for the created LU.
            TRACE_ERROR("Unable to locate the volume.\n");
        }
        else
        {
            // Found the disk - now, update the mountpoint
            TRACE_INFO(L"Updating volume mount-point to %s.\n", pMountPoint);
            fUpdatedMountpoint = UpdateDiskMountpoint(pVDSService, pMountPoint, idFoundDisk);
        }

        ShutdownVDSService(pVDSService);
    }
    else
    {
        TRACE_ERROR("Unable to initialize Virtual Disk Service due to HRESULT: %08X\n", hrStatus);
    }

    return fUpdatedMountpoint;
}

// Initializes the disk created by SFBD's CreateLU command.
BOOL InitializeSFBDDisk(IVdsService *pVDSService, VDS_PARTITION_STYLE partitionStyle, UCHAR PathId, UCHAR TargetId, UCHAR Lun, VDS_FILE_SYSTEM_TYPE fsType, PWCHAR VolumeLabel, PWCHAR pMountPoint)
{
    IEnumVdsObject *pEnumVdsObject = NULL;
    IVdsDisk *pVDSDisk = NULL;
    IUnknown *pIUnknown = NULL;
    VDS_DISK_PROP diskProp;
    BOOL fFreeStrings = FALSE;
    DWORD dwEnumeratedDisks = 0;

    memset(&diskProp, 0, sizeof(diskProp));

    // Query for the unallocated disks since they are not claimed by any provider.
    HRESULT hr = pVDSService->QueryUnallocatedDisks(&pEnumVdsObject);
    if (SUCCEEDED(hr))
    {
        if (!pEnumVdsObject)
        {
            hr = E_UNEXPECTED;
        }
        else
        {
            // Enumerate through the unallocated disks
            BOOL fDone = FALSE;
           
            while (!fDone)
            {
                DWORD dwFetchedCount = 0;
                pIUnknown = NULL;
                
                hr = pEnumVdsObject->Next(1, &pIUnknown, &dwFetchedCount);
                if (hr == S_OK)
                {
                    // QI for IVDisk interface
                    hr = pIUnknown->QueryInterface(IID_IVdsDisk, (void **)&pVDSDisk);
                    if (SUCCEEDED(hr))
                    {
                        memset(&diskProp, 0, sizeof(diskProp));

                        // Fetch the disk properties
                        hr = pVDSDisk->GetProperties(&diskProp);
                        if (SUCCEEDED(hr))
                        {
                            // Does this correspond to a drive backed by SFBD?
                            if (IsSFBDDisk(&diskProp, PathId, TargetId, Lun))
                            {
                                // Found the disk we were looking for.
                                dwEnumeratedDisks++;

                                // Ensure the Read-Only flag is not set and
                                // we bring the disk online.
                                //
                                // It appears that these steps are required on RS3.
                                pVDSDisk->ClearFlags(VDS_DF_READ_ONLY);

                                TRACE_INFO("Bringing disk online...\n");
                                if (diskProp.status == VDS_DS_OFFLINE)
                                {
                                    hr = BringDiskOnline(pVDSDisk);
                                }

                                if (SUCCEEDED(hr))
                                {
                                    // If the disk is not having partition,
                                    // create one and format it.
                                    TRACE_INFO("Initializing disk...\n");
                                    if (diskProp.PartitionStyle == VDS_PST_UNKNOWN)
                                    {
                                        hr = InitDisk(pVDSService, diskProp.id, partitionStyle);
                                    }
                                }

                                if (SUCCEEDED(hr))
                                {
                                    // Get the maximum volume size for the disk
                                    ULONGLONG ullMaxVolSize = 0;
                                    ULONGLONG ullOffset = 0;

                                    TRACE_INFO("Fetching disk size...\n");
                                    hr = GetMaximumVolumeSize(pVDSDisk, &ullOffset, &ullMaxVolSize);
                                    if (hr == S_OK)
                                    {
                                        WCHAR driveLetter = NULL;
                                        if (!pMountPoint)
                                        {
                                            // Get drive letter we can use since no mountpoint has been specified.
                                            TRACE_INFO("Fetching next available drive letter...\n");
                                            hr = GetNextAvailableDriveLetter(pVDSService, &driveLetter);
                                        }

                                        if (SUCCEEDED(hr))
                                        {
                                            // Create partition - see https://microsoft.visualstudio.com/DefaultCollection/OS/_git/os?path=%2Fvm%2Ftest%2Ftools%2Fstorage%2Fdiskop%2FVdsCmds.cpp&version=GBofficial%2Frs3_release for CreatePartitionPrimary function.
                                            TRACE_INFO("Partitioning the disk...\n");

                                            VDS_OBJECT_ID idVolume;
                                            hr = CreatePartition(pVDSDisk, partitionStyle, FALSE, PARTITION_HUGE, &ullOffset, ullMaxVolSize, &idVolume);
                                            if (SUCCEEDED(hr))
                                            {
                                                // No need to format if the user requested RAW FS as that is the default when new partition is created.
                                                if (fsType != VDS_FST_RAW)
                                                {
                                                    // Format the volume
                                                    TRACE_INFO("Formatting the disk...\n");
                                                    hr = FormatVolume(pVDSService, idVolume, fsType, VolumeLabel, TRUE, FALSE, driveLetter, pMountPoint);
                                                }
                                            }
                                        }
                                    }
                                }

                                fFreeStrings = TRUE;

                                break;
                            }

                            // Free the references before looping
                            FreeCOMString(diskProp.pwszDiskAddress);
                            FreeCOMString(diskProp.pwszName);
                            FreeCOMString(diskProp.pwszFriendlyName);
                            FreeCOMString(diskProp.pwszAdaptorName);
                            FreeCOMString(diskProp.pwszDevicePath);
                        }
                    }
                }
                else
                {
                    fDone = TRUE;
                    hr = S_FALSE;
                }
            }
        }
    }

    if (fFreeStrings)
    {
        // Free the references before looping
        FreeCOMString(diskProp.pwszDiskAddress);
        FreeCOMString(diskProp.pwszName);
        FreeCOMString(diskProp.pwszFriendlyName);
        FreeCOMString(diskProp.pwszAdaptorName);
        FreeCOMString(diskProp.pwszDevicePath);
    }

    // Release the interface now that we are done with it.
    _SafeRelease(pVDSDisk);
    _SafeRelease(pIUnknown);
    _SafeRelease(pEnumVdsObject);

    // Did we find any disk that we created?
    hr = (dwEnumeratedDisks == 0) ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : hr;
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        TRACE_ERROR("Unable to locate the LU in the disk subsystem.\n");
    }

    TRACE_INFO("Disk initialization %s with HRESULT: %08X\n", SUCCEEDED(hr)?"completed":"failed", hr);

    return SUCCEEDED(hr)?TRUE:FALSE;
}