// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class HostConfig
    {
        [DataMember(Name = "Binds", EmitDefaultValue = false)]
        public List<string> Binds { get; set; }

        [DataMember(Name = "ContainerIDFile", EmitDefaultValue = false)]
        public string ContainerIDFile { get; set; }

        [DataMember(Name = "LogConfig", EmitDefaultValue = false)]
        public LogConfig LogConfig { get; set; }

        [DataMember(Name = "NetworkMode", EmitDefaultValue = false)]
        public string NetworkMode { get; set; }

        [DataMember(Name = "PortBindings", EmitDefaultValue = false)]
        public IDictionary<string, IList<PortBinding>> PortBindings { get; set; }

        [DataMember(Name = "AutoRemove", EmitDefaultValue = false)]
        public bool AutoRemove { get; set; }

        [DataMember(Name = "VolumeDriver", EmitDefaultValue = false)]
        public string VolumeDriver { get; set; }

        [DataMember(Name = "VolumesFrom", EmitDefaultValue = false)]
        public IList<string> VolumesFrom { get; set; }

        [DataMember(Name = "CapAdd", EmitDefaultValue = false)]
        public IList<string> CapAdd { get; set; }

        [DataMember(Name = "CapDrop", EmitDefaultValue = false)]
        public IList<string> CapDrop { get; set; }

        [DataMember(Name = "Dns", EmitDefaultValue = false)]
        public IList<string> DNS { get; set; }

        [DataMember(Name = "DnsOptions", EmitDefaultValue = false)]
        public IList<string> DNSOptions { get; set; }

        [DataMember(Name = "DnsSearch", EmitDefaultValue = false)]
        public IList<string> DNSSearch { get; set; }

        [DataMember(Name = "ExtraHosts", EmitDefaultValue = false)]
        public IList<string> ExtraHosts { get; set; }

        [DataMember(Name = "GroupAdd", EmitDefaultValue = false)]
        public IList<string> GroupAdd { get; set; }

        [DataMember(Name = "IpcMode", EmitDefaultValue = false)]
        public string IpcMode { get; set; }

        [DataMember(Name = "Cgroup", EmitDefaultValue = false)]
        public string Cgroup { get; set; }

        [DataMember(Name = "Links", EmitDefaultValue = false)]
        public IList<string> Links { get; set; }

        [DataMember(Name = "OomScoreAdj", EmitDefaultValue = false)]
        public long OomScoreAdj { get; set; }

        [DataMember(Name = "PidMode", EmitDefaultValue = false)]
        public string PidMode { get; set; }

        [DataMember(Name = "Privileged", EmitDefaultValue = false)]
        public bool Privileged { get; set; }

        [DataMember(Name = "PublishAllPorts", EmitDefaultValue = false)]
        public bool PublishAllPorts { get; set; }

        [DataMember(Name = "ReadonlyRootfs", EmitDefaultValue = false)]
        public bool ReadonlyRootfs { get; set; }

        [DataMember(Name = "SecurityOpt", EmitDefaultValue = false)]
        public IList<string> SecurityOpt { get; set; }

        [DataMember(Name = "StorageOpt", EmitDefaultValue = false)]
        public IDictionary<string, string> StorageOpt { get; set; }

        [DataMember(Name = "Tmpfs", EmitDefaultValue = false)]
        public IDictionary<string, string> Tmpfs { get; set; }

        [DataMember(Name = "UTSMode", EmitDefaultValue = false)]
        public string UTSMode { get; set; }

        [DataMember(Name = "UsernsMode", EmitDefaultValue = false)]
        public string UsernsMode { get; set; }

        [DataMember(Name = "ShmSize", EmitDefaultValue = false)]
        public ulong ShmSize { get; set; }

        [DataMember(Name = "Sysctls", EmitDefaultValue = false)]
        public IDictionary<string, string> Sysctls { get; set; }

        [DataMember(Name = "Runtime", EmitDefaultValue = false)]
        public string Runtime { get; set; }

        [DataMember(Name = "ConsoleSize", EmitDefaultValue = false)]
        public ulong[] ConsoleSize { get; set; }

        [DataMember(Name = "Isolation", EmitDefaultValue = false)]
        public string Isolation { get; set; }

        [DataMember(Name = "CpuShares", EmitDefaultValue = false)]
        public long CPUShares { get; set; }

        [DataMember(Name = "Memory", EmitDefaultValue = false)]
        public ulong Memory { get; set; }

        [DataMember(Name = "NanoCpus", EmitDefaultValue = false)]
        public ulong NanoCPUs { get; set; }

        [DataMember(Name = "CgroupParent", EmitDefaultValue = false)]
        public string CgroupParent { get; set; }

        [DataMember(Name = "BlkioWeight", EmitDefaultValue = false)]
        public uint BlkioWeight { get; set; }

        [DataMember(Name = "CpuPeriod", EmitDefaultValue = false)]
        public long CPUPeriod { get; set; }

        [DataMember(Name = "CpuQuota", EmitDefaultValue = false)]
        public long CPUQuota { get; set; }

        [DataMember(Name = "CpuRealtimePeriod", EmitDefaultValue = false)]
        public long CPURealtimePeriod { get; set; }

        [DataMember(Name = "CpuRealtimeRuntime", EmitDefaultValue = false)]
        public long CPURealtimeRuntime { get; set; }

        [DataMember(Name = "CpusetCpus", EmitDefaultValue = false)]
        public string CpusetCpus { get; set; }

        [DataMember(Name = "CpusetMems", EmitDefaultValue = false)]
        public string CpusetMems { get; set; }

        [DataMember(Name = "DiskQuota", EmitDefaultValue = false)]
        public ulong DiskQuota { get; set; }

        [DataMember(Name = "KernelMemory", EmitDefaultValue = false)]
        public ulong KernelMemory { get; set; }

        [DataMember(Name = "MemoryReservation", EmitDefaultValue = false)]
        public ulong MemoryReservation { get; set; }

        [DataMember(Name = "MemorySwap", EmitDefaultValue = false)]
        public long MemorySwap { get; set; } //docker treats this as -1 when unlimited so make it signed int

        [DataMember(Name = "MemorySwappiness", EmitDefaultValue = false)]
        public long? MemorySwappiness { get; set; }

        [DataMember(Name = "OomKillDisable", EmitDefaultValue = false)]
        public bool? OomKillDisable { get; set; }

        [DataMember(Name = "PidsLimit", EmitDefaultValue = false)]
        public long PidsLimit { get; set; }

        [DataMember(Name = "CpuCount", EmitDefaultValue = false)]
        public long CPUCount { get; set; }

        [DataMember(Name = "CpuPercent", EmitDefaultValue = false)]
        public long CPUPercent { get; set; }

        [DataMember(Name = "IOMaximumIOps", EmitDefaultValue = false)]
        public ulong IOMaximumIOps { get; set; }

        [DataMember(Name = "IOMaximumBandwidth", EmitDefaultValue = false)]
        public ulong IOMaximumBandwidth { get; set; }

        [DataMember(Name = "Init", EmitDefaultValue = false)]
        public bool? Init { get; set; }

        [DataMember(Name = "InitPath", EmitDefaultValue = false)]
        public string InitPath { get; set; }
    }
}