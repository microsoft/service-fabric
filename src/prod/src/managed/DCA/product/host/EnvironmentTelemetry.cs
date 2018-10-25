// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Runtime.InteropServices;
    using Newtonsoft.Json;
    using TelemetryAggregation;

    public class EnvironmentTelemetry
    {
        private const string LogSourceId = "EnvironmentTelemetry";
        private const string TaskNameStr = "FabricDCA";
        private const string EventNameStr = "Environment";
        private const string ServiceFabricVersionStr = "ServiceFabricVersion";
        private const string ClusterTypeStr = "ClusterType";
        private const string ClusterDevTypeStr = "ClusterDevType";
        private const string ProcessorCountStr = "ProcessorCount";
        private const string PhysicalMemoryInKBStr = "PhysicalMemoryInKB";
        private const string OperatingSystemStr = "OS";
        private const string OperatingSystemVersionStr = "OSVersion";
        private const string DisksStr = "Disks";
        private const string DrivesStr = "Drives";

        private static FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

        public EnvironmentTelemetry(
            string clusterType,
            string serviceFabricVersion,
            string clusterDevType)
        {
            // getting hardware information
            int processorCount;
            long physicalMemoryInKB;
            EnvironmentTelemetry.GetSystemInfo(out processorCount, out physicalMemoryInKB);

            // getting disk and drive info
            string disks = "[]";
            string drives = "[]";
            try
            {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                disks = JsonConvert.SerializeObject(DiskInfoTelemetry.GetAllDisks());
                drives = JsonConvert.SerializeObject(DriveInfoTelemetry.GetAllDrives());
#endif
            }
            catch (Exception e)
            {
                // catching any exception for safety in case it fails for some reason
                // otherwise it will crash DCA everytime and prevent other telemetry to be sent
                EnvironmentTelemetry.traceSource.WriteError(LogSourceId, "Failed trying to get information about disks and drives. {0}", e);
            }

            // getting OS information
            string operatingSystem;
            string operatingSystemVersion;
            EnvironmentTelemetry.GetOperatingSystemInfo(out operatingSystem, out operatingSystemVersion);

            // populating the properties list with arguments
            this.Properties = new Dictionary<string, string>();
            this.Properties.Add(ClusterTypeStr, clusterType);
            this.Properties.Add(ServiceFabricVersionStr, serviceFabricVersion);
            this.Properties.Add(ClusterDevTypeStr, clusterDevType);
            this.Properties.Add(DisksStr, disks);
            this.Properties.Add(DrivesStr, drives);
            this.Properties.Add(OperatingSystemStr, operatingSystem);
            this.Properties.Add(OperatingSystemVersionStr, operatingSystemVersion);

            // populating the metrics list with arguments
            this.Metrics = new Dictionary<string, double>();
            this.Metrics.Add(ProcessorCountStr, (double)processorCount);
            this.Metrics.Add(PhysicalMemoryInKBStr, (double)physicalMemoryInKB);

            this.TraceAggregationCfg = TraceAggregationConfig.CreateSnapshotTraceAggregatorConfig(TaskNameStr, EventNameStr, string.Empty, this.Properties.Keys, this.Metrics.Keys);
        }

        public TraceAggregationConfig TraceAggregationCfg { get; }

        public Dictionary<string, string> Properties { get; }

        public Dictionary<string, double> Metrics { get; }

        private static void GetSystemInfo(out int processorCount, out long physicalMemoryInKB)
        {
            // processor count for windows and linux
            processorCount = Environment.ProcessorCount;
            try
            {
                // getting physical memory from kernel32.dll native method
                if (!EnvironmentTelemetry.GetPhysicallyInstalledSystemMemory(out physicalMemoryInKB))
                {
                    physicalMemoryInKB = -1;
                }
            }
            catch (Exception e)
            {
                EnvironmentTelemetry.traceSource.WriteExceptionAsError(
                    LogSourceId,
                    e,
                    "Failed to read amount of physical memory");
                physicalMemoryInKB = -1;
            }
        }

        private static void GetOperatingSystemInfo(out string operatingSystem, out string operatingSystemVersion)
        {
#if !DotNetCoreClrLinux
            operatingSystem = "windows";
            operatingSystemVersion = Environment.OSVersion.ToString();
#else
            operatingSystem = "linux";
            operatingSystemVersion = RuntimeInformation.OSDescription;
#endif
        }

#if !DotNetCoreClrLinux
        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetPhysicallyInstalledSystemMemory(out long totalMemoryInKilobytes);
#else
        private static bool GetPhysicallyInstalledSystemMemory(out long totalMemoryInKilobytes)
        {
            int pageSize = EnvironmentTelemetry.getpagesize();
            long physPages = EnvironmentTelemetry.get_phys_pages();
            totalMemoryInKilobytes = physPages * pageSize / 1024;

            return true;
        }

        // unix native methods
        [DllImport("c")]
        private static extern int getpagesize();
        [DllImport("c")]
        private static extern long get_phys_pages();
#endif
    }
}