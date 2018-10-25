// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;

    [DataContract]
    internal class PeerInfo
    {
        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "IP", EmitDefaultValue = false)]
        public string IP { get; set; }
    }

    [DataContract]
    internal class EndpointResource
    {
        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "EndpointID", EmitDefaultValue = false)]
        public string EndpointID { get; set; }

        [DataMember(Name = "MacAddress", EmitDefaultValue = false)]
        public string MacAddress { get; set; }

        [DataMember(Name = "IPv4Address", EmitDefaultValue = false)]
        public string IPv4Address { get; set; }

        [DataMember(Name = "IPv6Address", EmitDefaultValue = false)]
        public string IPv6Address { get; set; }
    }

    [DataContract]
    internal class NetworkListResponse
    {
        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "Id", EmitDefaultValue = false)]
        public string ID { get; set; }

        [DataMember(Name = "Created", EmitDefaultValue = false)]
        public DateTime Created { get; set; }

        [DataMember(Name = "Scope", EmitDefaultValue = false)]
        public string Scope { get; set; }

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

        [DataMember(Name = "Containers", EmitDefaultValue = false)]
        public IDictionary<string, EndpointResource> Containers { get; set; }

        [DataMember(Name = "Options", EmitDefaultValue = false)]
        public IDictionary<string, string> Options { get; set; }

        [DataMember(Name = "Labels", EmitDefaultValue = false)]
        public IDictionary<string, string> Labels { get; set; }

        [DataMember(Name = "Peers", EmitDefaultValue = false)]
        public IList<PeerInfo> Peers { get; set; }
    }
}