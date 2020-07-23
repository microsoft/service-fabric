// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin
{
    static class Constants
    {
        internal const string ConfigurationPackageName = "Config";
        internal const string SectionName = "ServiceFabricVolumeDriver";
        internal const string ParameterName = "OperatingSystem";
        internal const string TraceSource = "ServiceFabricVolumeDriver";

        // Block Store Service Partition ID, used to get Block Store Service port and connect to that port
        internal const string ServicePartitionID = "servicePartitionId";
        
        // Disk information which will be used in Block Store Service
        // Example: LUID="Disk1", SizeDisk="1GB", FileSystem="NTFS"
        internal const string LUID = "LUID";
        internal const string SizeDisk = "sizeDisk";
        internal const string FileSystem = "fileSystem";

        internal const int sizeTypeLength = 2; // "MB" or "GB" or "TB"
        internal const int maxWCharSizeLUID = 50;

        internal const int MaxKestralCloseTimeoutSeconds = 300;
        internal const int KestralCloseSleepIntervalSeconds = 10;

        // NOTE: Please ensure the constants below are in sync with their peer definitions in native code.
        internal struct GatewayPayloadConst
        {
            public static ulong blockSizeDisk = 16384;
            public static ulong blockSizeManagementRequest = 16384;
            static int maxLengthDeviceID = 50 * 2; // Since we are dealing with wchar_t
            static int nativePayloadStructSize = 48;
            static int endOfPayloadMarkerSize = 4;
            public static int payloadHeaderSize = nativePayloadStructSize + maxLengthDeviceID + endOfPayloadMarkerSize;
            public static int BlockOperationCompleted = 0x827;
            public static int BlockOperationFailed = 0xeee;
            public static uint EndOfPayloadMarker = 0xd0d0d0d0;
            public static int maxLURegistrations = 100;
            public static int maxNamedPipePayloadSize = 64 * 1024;
        }

        public enum BlockMode
        {
            Read = 1,
            Write,
            RegisterLU,
            UnregisterLU,
            FetchRegisteredLUList,
            UnmountLU,
            MountLU,
            OperationCompleted = 0x827,
            OperationFailed = 0xeee,
            InvalidPayload = 0xfee,
            ServerInvalidPayload = 0xfef
        };
    }
}
