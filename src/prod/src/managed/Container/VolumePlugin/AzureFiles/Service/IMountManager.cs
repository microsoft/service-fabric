//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;

    interface IMountManager
    {
        MountParams GetMountExeAndArgs(string name, VolumeEntry volumeEntry);
        UnmountParams GetUnmountExeAndArgs(VolumeEntry volumeEntry);
    }
}
