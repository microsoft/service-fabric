// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using System;
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class HealthcheckResult
    {
        [DataMember(Name = "Start", EmitDefaultValue = false)]
        public DateTime Start { get; set; }

        [DataMember(Name = "End", EmitDefaultValue = false)]
        public DateTime End { get; set; }

        [DataMember(Name = "ExitCode", EmitDefaultValue = false)]
        public long ExitCode { get; set; }

        [DataMember(Name = "Output", EmitDefaultValue = false)]
        public string Output { get; set; }
    }

    [DataContract]
    internal class ContainerHealth
    {
        [DataMember(Name = "Status", EmitDefaultValue = false)]
        public string Status { get; set; }

        [DataMember(Name = "FailingStreak", EmitDefaultValue = false)]
        public long FailingStreak { get; set; }

        [DataMember(Name = "Log", EmitDefaultValue = false)]
        public IList<HealthcheckResult> Log { get; set; }
    }

    [DataContract]
    internal class ContainerState
    {
        [DataMember(Name = "Status", EmitDefaultValue = false)]
        public string Status { get; set; }

        [DataMember(Name = "Running", EmitDefaultValue = false)]
        public bool Running { get; set; }

        [DataMember(Name = "Paused", EmitDefaultValue = false)]
        public bool Paused { get; set; }

        [DataMember(Name = "Restarting", EmitDefaultValue = false)]
        public bool Restarting { get; set; }

        [DataMember(Name = "OOMKilled", EmitDefaultValue = false)]
        public bool OOMKilled { get; set; }

        [DataMember(Name = "Dead", EmitDefaultValue = false)]
        public bool Dead { get; set; }

        [DataMember(Name = "Pid", EmitDefaultValue = false)]
        public long Pid { get; set; }

        [DataMember(Name = "ExitCode", EmitDefaultValue = false)]
        public long ExitCode { get; set; }

        [DataMember(Name = "Error", EmitDefaultValue = false)]
        public string Error { get; set; }

        [DataMember(Name = "StartedAt", EmitDefaultValue = false)]
        public string StartedAt { get; set; }

        [DataMember(Name = "FinishedAt", EmitDefaultValue = false)]
        public string FinishedAt { get; set; }

        [DataMember(Name = "Health", EmitDefaultValue = false)]
        public ContainerHealth Health { get; set; }
    }

    [DataContract]
    internal class ContainerNode
    {
        [DataMember(Name = "ID", EmitDefaultValue = false)]
        public string ID { get; set; }

        [DataMember(Name = "IP", EmitDefaultValue = false)]
        public string IPAddress { get; set; }

        [DataMember(Name = "Addr", EmitDefaultValue = false)]
        public string Addr { get; set; }

        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "Cpus", EmitDefaultValue = false)]
        public long Cpus { get; set; }

        [DataMember(Name = "Memory", EmitDefaultValue = false)]
        public long Memory { get; set; }

        [DataMember(Name = "Labels", EmitDefaultValue = false)]
        public IDictionary<string, string> Labels { get; set; }
    }

    [DataContract]
    internal class GraphDriverData
    {
        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "Data", EmitDefaultValue = false)]
        public IDictionary<string, string> Data { get; set; }
    }

    [DataContract]
    internal class MountPoint
    {
        [DataMember(Name = "Type", EmitDefaultValue = false)]
        public string Type { get; set; }

        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "Source", EmitDefaultValue = false)]
        public string Source { get; set; }

        [DataMember(Name = "Destination", EmitDefaultValue = false)]
        public string Destination { get; set; }

        [DataMember(Name = "Driver", EmitDefaultValue = false)]
        public string Driver { get; set; }

        [DataMember(Name = "Mode", EmitDefaultValue = false)]
        public string Mode { get; set; }

        [DataMember(Name = "RW", EmitDefaultValue = false)]
        public bool RW { get; set; }

        [DataMember(Name = "Propagation", EmitDefaultValue = false)]
        public string Propagation { get; set; }
    }

    [DataContract]
    internal class Address
    {
        [DataMember(Name = "Addr", EmitDefaultValue = false)]
        public string Addr { get; set; }

        [DataMember(Name = "PrefixLen", EmitDefaultValue = false)]
        public long PrefixLen { get; set; }
    }

    [DataContract]
    internal class NetworkSettings
    {
        [DataMember(Name = "Bridge", EmitDefaultValue = false)]
        public string Bridge { get; set; }

        [DataMember(Name = "SandboxID", EmitDefaultValue = false)]
        public string SandboxID { get; set; }

        [DataMember(Name = "HairpinMode", EmitDefaultValue = false)]
        public bool HairpinMode { get; set; }

        [DataMember(Name = "LinkLocalIPv6Address", EmitDefaultValue = false)]
        public string LinkLocalIPv6Address { get; set; }

        [DataMember(Name = "LinkLocalIPv6PrefixLen", EmitDefaultValue = false)]
        public long LinkLocalIPv6PrefixLen { get; set; }

        [DataMember(Name = "Ports", EmitDefaultValue = false)]
        public IDictionary<string, IList<PortBinding>> Ports { get; set; }

        [DataMember(Name = "SandboxKey", EmitDefaultValue = false)]
        public string SandboxKey { get; set; }

        [DataMember(Name = "SecondaryIPAddresses", EmitDefaultValue = false)]
        public IList<Address> SecondaryIPAddresses { get; set; }

        [DataMember(Name = "SecondaryIPv6Addresses", EmitDefaultValue = false)]
        public IList<Address> SecondaryIPv6Addresses { get; set; }

        [DataMember(Name = "EndpointID", EmitDefaultValue = false)]
        public string EndpointID { get; set; }

        [DataMember(Name = "Gateway", EmitDefaultValue = false)]
        public string Gateway { get; set; }

        [DataMember(Name = "GlobalIPv6Address", EmitDefaultValue = false)]
        public string GlobalIPv6Address { get; set; }

        [DataMember(Name = "GlobalIPv6PrefixLen", EmitDefaultValue = false)]
        public long GlobalIPv6PrefixLen { get; set; }

        [DataMember(Name = "IPAddress", EmitDefaultValue = false)]
        public string IPAddress { get; set; }

        [DataMember(Name = "IPPrefixLen", EmitDefaultValue = false)]
        public long IPPrefixLen { get; set; }

        [DataMember(Name = "IPv6Gateway", EmitDefaultValue = false)]
        public string IPv6Gateway { get; set; }

        [DataMember(Name = "MacAddress", EmitDefaultValue = false)]
        public string MacAddress { get; set; }

        [DataMember(Name = "Networks", EmitDefaultValue = false)]
        public IDictionary<string, EndpointSettings> Networks { get; set; }
    }

    [DataContract]
    internal class ContainerInspectResponse
    {
        [DataMember(Name = "Id", EmitDefaultValue = false)]
        public string ContainerId { get; set; }

        [DataMember(Name = "Created", EmitDefaultValue = false)]
        public DateTime Created { get; set; }

        [DataMember(Name = "Path", EmitDefaultValue = false)]
        public string Path { get; set; }

        [DataMember(Name = "Args", EmitDefaultValue = false)]
        public IList<string> Args { get; set; }

        [DataMember(Name = "State", EmitDefaultValue = false)]
        public ContainerState State { get; set; }

        [DataMember(Name = "Image", EmitDefaultValue = false)]
        public string Image { get; set; }

        [DataMember(Name = "ResolvConfPath", EmitDefaultValue = false)]
        public string ResolvConfPath { get; set; }

        [DataMember(Name = "HostnamePath", EmitDefaultValue = false)]
        public string HostnamePath { get; set; }

        [DataMember(Name = "HostsPath", EmitDefaultValue = false)]
        public string HostsPath { get; set; }

        [DataMember(Name = "LogPath", EmitDefaultValue = false)]
        public string LogPath { get; set; }

        [DataMember(Name = "Node", EmitDefaultValue = false)]
        public ContainerNode Node { get; set; }

        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "RestartCount", EmitDefaultValue = false)]
        public long RestartCount { get; set; }

        [DataMember(Name = "Driver", EmitDefaultValue = false)]
        public string Driver { get; set; }

        [DataMember(Name = "MountLabel", EmitDefaultValue = false)]
        public string MountLabel { get; set; }

        [DataMember(Name = "ProcessLabel", EmitDefaultValue = false)]
        public string ProcessLabel { get; set; }

        [DataMember(Name = "AppArmorProfile", EmitDefaultValue = false)]
        public string AppArmorProfile { get; set; }

        [DataMember(Name = "ExecIDs", EmitDefaultValue = false)]
        public IList<string> ExecIDs { get; set; }

        [DataMember(Name = "HostConfig", EmitDefaultValue = false)]
        public HostConfig HostConfig { get; set; }

        [DataMember(Name = "GraphDriver", EmitDefaultValue = false)]
        public GraphDriverData GraphDriver { get; set; }

        [DataMember(Name = "SizeRw", EmitDefaultValue = false)]
        public long? SizeRw { get; set; }

        [DataMember(Name = "SizeRootFs", EmitDefaultValue = false)]
        public long? SizeRootFs { get; set; }

        [DataMember(Name = "Mounts", EmitDefaultValue = false)]
        public IList<MountPoint> Mounts { get; set; }

        [DataMember(Name = "Config", EmitDefaultValue = false)]
        public ContainerConfig Config { get; set; }

        [DataMember(Name = "NetworkSettings", EmitDefaultValue = false)]
        public NetworkSettings NetworkSettings { get; set; }
    }
}
