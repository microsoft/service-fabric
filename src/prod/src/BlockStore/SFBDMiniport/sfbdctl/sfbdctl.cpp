// This is the control utility for interfacing with Service Fabric Block Device Driver.
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <stdlib.h>
#include <malloc.h>
#include "sfbdctl.h"

SFBDCommands commandArray[] = {
    { SFBDCommandCode::GetAdapterInfo, L"GetAdapterInfo", 0 },
    { SFBDCommandCode::CreateLU, L"CreateLu", 4 },
    { SFBDCommandCode::ProvisionLu, L"ProvisionLu", 4 },
    { SFBDCommandCode::GetInfoForLu, L"GetLuInfo", 1},
    { SFBDCommandCode::DeleteLu, L"DeleteLu", 1 },
    { SFBDCommandCode::UnmountLu, L"UnmountLu", 1 },
    { SFBDCommandCode::MountLu, L"MountLu", 1 },
    { SFBDCommandCode::ListAllLu, L"ListallLu", 0 },
    { SFBDCommandCode::InstallDevice, L"InstallDevice", 1 },
    { SFBDCommandCode::UninstallDevice, L"UninstallDevice", 1 }
#if defined(_DEBUG)
    ,{ SFBDCommandCode::ShutdownServiceVolumes, L"ShutdownVolumes", 0 }
#endif // _DEBUG
};

void DisplayHeader()
{
    printf("Service Fabric Block Device Control Application\n");
    printf("Copyright (c) Microsoft Corporation\n\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
    int iRetVal = 0;
    DWORD dwServicePort = DEFAULT_SERVICE_PORT;
    PWCHAR pMountPoint = NULL;

    DisplayHeader();

    int iCurrentArgumentIndex = 1;
    int commandCount = sizeof(commandArray) / sizeof(commandArray[0]);

    if (argc < 2)
    {
        DisplayCommandLineHelp();
        return -1;
    }

    // Get the service port if specified in environment
    char *pValue;
    errno_t err = _dupenv_s(&pValue, NULL, "SF_BLOCKSTORE_SERVICE_TARGET_PORT");
    if ((err == 0) && (pValue != NULL))
    {
        dwServicePort = atoi(pValue);
        free(pValue);
    }

    DWORD dwError = ERROR_SUCCESS;
    {
        // Loop through the arguments to process the commands
        int iCommandIndex = 0;
        for (iCommandIndex = 0; iCommandIndex < commandCount; iCommandIndex++)
        {
            if (_wcsnicmp(commandArray[iCommandIndex].szCommand, argv[iCurrentArgumentIndex], wcslen(commandArray[iCommandIndex].szCommand)) == 0)
            {
                if (argc < (commandArray[iCommandIndex].commandArgs + 2))
                {
                    wprintf_s(L"Invalid arguments specified for %s command!\n\n", commandArray[iCommandIndex].szCommand);
                    iCommandIndex = -1;
                }

                // Break out of the loop.
                break;
            }
        }

        // Did we find a command to process?
        if ((iCommandIndex >=0) && (iCommandIndex < commandCount))
        {
            HANDLE hDevice = INVALID_HANDLE_VALUE;

            // Subtract 2 to account for the appname and command.
            int iRemainingArgumentCount = argc - 2;

            // Connect to the device for non-install/uninstall commands.
            if (commandArray[iCommandIndex].commandCode < SFBDCommandCode::InstallDevice)
            {
                hDevice = ConnectToDevice(&dwError);
                if (hDevice == INVALID_HANDLE_VALUE)
                {
                    printf("Unable to connect to the device due to error: %08X!\n", dwError);
                    iRetVal = -1;
                }
            }

            if (iRetVal == 0)
            {
                iCurrentArgumentIndex++;

                BOOL fProvisionedOrCreated = FALSE;

                // Yes we did - process it.
                switch (commandArray[iCommandIndex].commandCode)
                {
                case SFBDCommandCode::GetAdapterInfo:
                    // GetBasicInfo
                    DisplayDeviceInfo(hDevice);
                    break;
                case SFBDCommandCode::CreateLU:
                case SFBDCommandCode::ProvisionLu:
                    // Create LUN
                    //
                    // Determine if the optional mount folder is specified as the last argument.
                    if (iRemainingArgumentCount > commandArray[iCommandIndex].commandArgs)
                    {
                        int iNextOptionalArgumentIndex = commandArray[iCommandIndex].commandArgs + 2;
                        pMountPoint = argv[iNextOptionalArgumentIndex];
                    }

                    printf("Service Fabric BlockStore Service Port: %d\n", dwServicePort);
                    fProvisionedOrCreated = FALSE;

                    if (commandArray[iCommandIndex].commandCode == SFBDCommandCode::CreateLU)
                    {
                        fProvisionedOrCreated = CreateLUN(hDevice, argv[iCurrentArgumentIndex], argv[iCurrentArgumentIndex + 1], argv[iCurrentArgumentIndex + 2], argv[iCurrentArgumentIndex + 3], pMountPoint, dwServicePort);
                    }
                    else
                    {
                        fProvisionedOrCreated = ProvisionLUN(hDevice, argv[iCurrentArgumentIndex], argv[iCurrentArgumentIndex + 1], argv[iCurrentArgumentIndex + 2], argv[iCurrentArgumentIndex + 3], pMountPoint, dwServicePort);
                    }

                    if (!fProvisionedOrCreated)
                    {
                        iRetVal = -1;
                    }
                    break;
                case SFBDCommandCode::GetInfoForLu:
                    // Get information for LU using Path/Target/Lun
                    if (!GetLUInfo(hDevice, argv[iCurrentArgumentIndex], NULL, TRUE))
                    {
                        iRetVal = -1;
                    }
                    break;
                case SFBDCommandCode::DeleteLu:
                    // Delete the specified LU
                    if (!DeleteLU(hDevice, argv[iCurrentArgumentIndex]))
                    {
                        iRetVal = -1;
                    }
                    break;
                case SFBDCommandCode::UnmountLu:
                case SFBDCommandCode::MountLu:
                    // Unmount the specified LU
                    if (!MountUnmountLU(hDevice, argv[iCurrentArgumentIndex], commandArray[iCommandIndex].commandCode))
                    {
                        iRetVal = -1;
                    }
                    break;
                case SFBDCommandCode::ListAllLu:
                    if (!ListAllLU(hDevice))
                    {
                        iRetVal = -1;
                    }
                    break;
                case SFBDCommandCode::InstallDevice:
                {
                    BOOL fRestartSystem;
                    if (!InstallSFBDDevice(argv[iCurrentArgumentIndex], fRestartSystem))
                    {
                        iRetVal = -1;
                    }
                    break;
                }
                case SFBDCommandCode::UninstallDevice:
                {
                    BOOL fRestartSystem;
                    if (UninstallErrorCodes::UnInstalledSucessfully != UninstallSFBDDevice(argv[iCurrentArgumentIndex], fRestartSystem, false))
                    {
                        iRetVal = -1;
                    }
                    break;
                }
#if defined(_DEBUG)
                case SFBDCommandCode::ShutdownServiceVolumes:
                    ShutdownVolumesForService(dwServicePort);
                    break;
#endif // _DEBUG
                default:
                    // Unsupported command
                    DisplayCommandLineHelp();
                    iRetVal = -1;
                    break;
                }
                CloseHandle(hDevice);
            }
        }
        else
        {
            if (iCommandIndex >= 0)
            {
                printf("Invalid command line specified!\n\n");
            }

            DisplayCommandLineHelp();
            iRetVal = -1;
        }
    }

    return iRetVal;
}

