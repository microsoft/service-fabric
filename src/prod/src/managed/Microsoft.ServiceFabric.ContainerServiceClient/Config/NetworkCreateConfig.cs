// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class IPAMConfig
    {
        [DataMember(Name = "Subnet", EmitDefaultValue = false)]
        public string Subnet { get; set; }

        [DataMember(Name = "IPRange", EmitDefaultValue = false)]
        public string IPRange { get; set; }

        [DataMember(Name = "Gateway", EmitDefaultValue = false)]
        public string Gateway { get; set; }

        [DataMember(Name = "AuxiliaryAddresses", EmitDefaultValue = false)]
        public IDictionary<string, string> AuxAddress { get; set; }
    }

    [DataContract]
    internal class IPAM
    {
        [DataMember(Name = "Driver", EmitDefaultValue = false)]
        public string Driver { get; set; }

        [DataMember(Name = "Options", EmitDefaultValue = false)]
        public IDictionary<string, string> Options { get; set; }

        [DataMember(Name = "Config", EmitDefaultValue = false)]
        public IList<IPAMConfig> Config { get; set; }
    }

    [DataContract]
    internal class NetworkCreateConfig
    {
        [DataMember(Name = "CheckDuplicate", EmitDefaultValue = false)]
        public bool CheckDuplicate { get; set; }

        [DataMember(Name = "Driver", EmitDefaultValue = false)]
        public string Driver { get; set; }

        [DataMember(Name = "EnableIPv6", EmitDefaultValue = false)]
        public bool EnableIPv6 { get; set; }

        [DataMember(Name = "IPAM", EmitDefaultValue = false)]
        public IPAM IPAM { get; set; }

        [DataMember(Name = "Internal", EmitDefaultValue = false)]
        public bool Internal { get; set; }

        [DataMember(Name = "Attachable", EmitDefaultValue = false)]
        public bool Attachable { get; set; }

        [DataMember(Name = "Options", EmitDefaultValue = false)]
        public IDictionary<string, string> Options { get; set; }

        [DataMember(Name = "Labels", EmitDefaultValue = false)]
        public IDictionary<string, string> Labels { get; set; }

        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }
    }
}