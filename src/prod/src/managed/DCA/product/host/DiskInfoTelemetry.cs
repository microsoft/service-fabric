// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Management;
    using Newtonsoft.Json;

    public sealed class DiskInfoTelemetry
    {
        private DiskInfoTelemetry(ManagementObject drive)
        {
            string deviceId;
            DiskInfoTelemetry.TryGetString(drive, "DeviceID", out deviceId);
            this.DeviceIdHash = ((uint)deviceId.GetHashCode()).ToString();

            // MediaType
            ushort mediaTypeId;
            if (DiskInfoTelemetry.TryGetUShort(drive, "MediaType", out mediaTypeId))
            {
                this.MediaType = DiskInfoTelemetry.GetDiskMediaTypeName(mediaTypeId);
            }
            else
            {
                this.MediaType = "undefined-MediaType";
            }

            // Size
            ulong size;
            DiskInfoTelemetry.TryGetULong(drive, "Size", out size);
            this.Size = size;

            // Allocated size
            ulong allocatedSize;
            DiskInfoTelemetry.TryGetULong(drive, "AllocatedSize", out allocatedSize);
            this.AllocatedSize = allocatedSize;

            // Spindle Speed
            uint spindleSpeed;
            DiskInfoTelemetry.TryGetUInt(drive, "SpindleSpeed", out spindleSpeed);
            this.SpindleSpeed = spindleSpeed;
        }

        // Device Unique Identifier Hash
        [JsonProperty("deviceIdHash")]
        public string DeviceIdHash { get; }

        // SSD, HDD, SCM, Unspecified
        [JsonProperty("mediaType")]
        public string MediaType { get; }

        // Total physical storage size of the disk, in bytes.
        [JsonProperty("size")]
        public ulong Size { get; }

        // The total amount of used space on this physical disk
        [JsonProperty("allocatedSize")]
        public ulong AllocatedSize { get; }

        // The rotational speed of spindle-based physical disks. For non-rotation should be 0
        [JsonProperty("spindleSpeed")]
        public uint SpindleSpeed { get; }

        public static DiskInfoTelemetry[] GetAllDisks()
        {
            List<DiskInfoTelemetry> disksInfo = new List<DiskInfoTelemetry>();
            ManagementObjectSearcher s = new ManagementObjectSearcher(
            "\\root\\Microsoft\\Windows\\Storage",
            "SELECT * FROM MSFT_PhysicalDisk",
            new EnumerationOptions());

            foreach (ManagementObject drive in s.Get())
            {
                DiskInfoTelemetry disk = new DiskInfoTelemetry(drive);
                disksInfo.Add(disk);
            }

            return disksInfo.ToArray();
        }

        private static bool TryGetUShort(ManagementObject drive, string fieldName, out ushort value, ushort defaultValue = 0)
        {
            value = defaultValue;

            if (drive[fieldName] == null)
            {
                return false;
            }

            return ushort.TryParse(drive[fieldName].ToString(), out value);
        }

        private static bool TryGetULong(ManagementObject drive, string fieldName, out ulong value, ulong defaultValue = 0)
        {
            value = defaultValue;

            if (drive[fieldName] == null)
            {
                return false;
            }

            return ulong.TryParse(drive[fieldName].ToString(), out value);
        }

        private static bool TryGetUInt(ManagementObject drive, string fieldName, out uint value, uint defaultValue = 0)
        {
            value = defaultValue;

            if (drive[fieldName] == null)
            {
                return false;
            }

            return uint.TryParse(drive[fieldName].ToString(), out value);
        }

        private static bool TryGetString(ManagementObject drive, string fieldName, out string value, string defaultValue = "")
        {
            value = defaultValue;

            if (drive[fieldName] == null)
            {
                return false;
            }

            value = drive[fieldName].ToString();

            return true;
        }

        private static string GetDiskMediaTypeName(ushort mediaTypeId)
        {
            string mediaType;

            // from (https://msdn.microsoft.com/en-us/library/windows/desktop/hh830532(v=vs.85).aspx)
            switch (mediaTypeId)
            {
                case 0:
                    mediaType = "Unspecified";
                    break;
                case 3:
                    mediaType = "HDD";
                    break;
                case 4:
                    mediaType = "SSD";
                    break;
                case 5:
                    mediaType = "SCM";
                    break;
                default:
                    // documentation does not say if this can occur
                    // adding this here to differentiate to the case where drive["MediaType"] does not exist
                    mediaType = "undefined";
                    break;
            }

            return mediaType;
        }
    }
}