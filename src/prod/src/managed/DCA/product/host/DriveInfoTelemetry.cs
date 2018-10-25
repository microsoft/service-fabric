// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System.Collections.Generic;
    using System.IO;
    using Newtonsoft.Json;

    public sealed class DriveInfoTelemetry
    {
        private DriveInfoTelemetry(DriveInfo driveInfo)
        {
            this.Name = driveInfo.Name;
            this.Format = driveInfo.DriveFormat;
            this.Type = driveInfo.DriveType.ToString();
            this.TotalFreeSpace = driveInfo.TotalFreeSpace;
            this.TotalSize = driveInfo.TotalSize;
            this.AvailableFreeSpace = driveInfo.AvailableFreeSpace;
        }

        // hash of C:, D:, ...
        [JsonProperty("name")]
        public string Name { get; }

        // NTFS, FAT, ...
        [JsonProperty("format")]
        public string Format { get; }

        // CD-ROM, removable, fixed, network, ...
        [JsonProperty("type")]
        public string Type { get; }

        // amount of space available in bytes
        [JsonProperty("totalFreeSpace")]
        public long TotalFreeSpace { get; }

        // total size of storage
        [JsonProperty("totalSize")]
        public long TotalSize { get; }

        // amount of available free space (accounts for quotas)
        [JsonProperty("availableFreeSpace")]
        public long AvailableFreeSpace { get; }

        public static DriveInfoTelemetry[] GetAllDrives()
        {
            DriveInfo[] allDrives = DriveInfo.GetDrives();
            List<DriveInfoTelemetry> allDrivesInfoTelemetry = new List<DriveInfoTelemetry>();

            foreach (var drive in allDrives)
            {
                if (drive.IsReady)
                {
                    allDrivesInfoTelemetry.Add(new DriveInfoTelemetry(drive));
                }
            }

            return allDrivesInfoTelemetry.ToArray();
        }
    }
}