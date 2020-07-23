// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin
{
    using System.Collections.Generic;

    public class VolumeEntry
    {
        public Dictionary<string, string> VolumeOptions;

        public HashSet<string> MountIDs;

        public int NumberOfMounts;
        
        public string ServicePartitionID;
        public string LUID;
        public string SizeDisk;
        public string FileSystem;
        public string Mountpoint;

        public VolumeEntry()
        {
            this.VolumeOptions = new Dictionary<string, string>();
            this.MountIDs = new HashSet<string>();
            this.NumberOfMounts = 0;
        }
    }
}
