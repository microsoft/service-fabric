//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System.Collections.Generic;

    public class VolumeEntry
    {
        public Dictionary<string, string> VolumeOptions;

        public string Mountpoint;

        public HashSet<string> MountIDs;

        public VolumeEntry()
        {
            this.VolumeOptions = new Dictionary<string, string>();
            this.MountIDs = new HashSet<string>();
        }
    }
}
