// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;

    internal class DockerComposeConstants
    {
        public const string SemiColonDelimiter = ":";
        public const string ForwardSlashDelimiter = "/";
        public static readonly char [] EqualsDelimiter = new char [] { '=' };
        public const string HttpProtocol = "http";
        public const string SupportedDockerComposeVersion3 = "3";
        public const string SupportedDockerComposeVersion2 = "2";
        public const string DefaultApplicationTypeName = "__GeneratedApplicationTypeName";

        // root keys
        public const string ServicesKey = "services";
        public const string VersionKey = "version";
        public const string VolumesKey = "volumes";
        public const string NetworksKey = "networks";
        public const string LabelsKey = "labels";

        // services keys
        public const string PortsKey = "ports";
        public const string ImageKey = "image";
        public const string EntryPointKey = "entrypoint";
        public const string CommandKey = "command";
        public const string LoggingKey = "logging";
        public const string DriverKey = "driver";
        public const string LoggingDriverOptionsKey = "options";
        public const string DriverOptionsKey = "driver_opts";
        public const string EnvironmentKey = "environment";
        public const string IsolationKey = "isolation";
        public const string IsolationValueDefault = "default";
        public const string IsolationValueProcess = "process";
        public const string IsolationValueHyperv = "hyperv";

        // services->deploy keys
        public const string DeployKey = "deploy";
        public const string ReplicasKey = "replicas";
        public const string PlacementKey = "placement";
        public const string ConstraintsKey = "constraints";
        public const string ResourcesKey = "resources";
        public const string ResourceLimitsKey = "limits";
        public const string ResourceReservationsKey = "reservations";
        public const string CpuSharesKey = "cpu_shares";
        public const string CpusKey = "cpus";
        public const string MemoryKey = "memory";
        public const string MemorySwapKey = "memory-swap";
        public const string DeploymentModeKey = "mode";
        public const string DeploymentModeReplicatedValue = "replicated";

        // top level networks keys
        public const string ExternalNetworkDriver = "external";
        public const string DefaultNetwork = "default";
        public const string OpenExternalNetworkValue = "open";
        public const string NatExternalNetworkValue = "nat";
        public const string OpenExternalNetworkServiceManifestValue = "Open";
        public const string NatExternalNetworkServiceManifestValue = "Other";

        // Service Fabric labels
        public const string SfApplicationTypeNameKey = "sf.applicationtype.name";
        public const string SfApplicationTypeVersionKey = "sf.applicationtype.version";

        // misc
        public const string ReadWrite = "rw";
        public const string ReadOnly = "ro";

        public static readonly HashSet<string> UnsupportedKeys = new HashSet<string>() {"devices"};

        public static readonly HashSet<string> IgnoredKeys = new HashSet<string>()
        {
            "build",
            "container_name",
            "depends_on",
            "dns_search",
            "tmpfs"
        };
    }
}