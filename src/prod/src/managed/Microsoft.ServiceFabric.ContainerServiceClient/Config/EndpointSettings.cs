// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class EndpointSettings
    {
        [DataMember(Name = "IPAMConfig", EmitDefaultValue = false)]
        public EndpointIPAMConfig IPAMConfig { get; set; }

        [DataMember(Name = "Links", EmitDefaultValue = false)]
        public IList<string> Links { get; set; }

        [DataMember(Name = "Aliases", EmitDefaultValue = false)]
        public IList<string> Aliases { get; set; }

        [DataMember(Name = "NetworkID", EmitDefaultValue = false)]
        public string NetworkID { get; set; }

        [DataMember(Name = "EndpointID", EmitDefaultValue = false)]
        public string EndpointID { get; set; }

        [DataMember(Name = "Gateway", EmitDefaultValue = false)]
        public string Gateway { get; set; }

        [DataMember(Name = "IPAddress", EmitDefaultValue = false)]
        public string IPAddress { get; set; }

        [DataMember(Name = "IPPrefixLen", EmitDefaultValue = false)]
        public long IPPrefixLen { get; set; }

        [DataMember(Name = "IPv6Gateway", EmitDefaultValue = false)]
        public string IPv6Gateway { get; set; }

        [DataMember(Name = "GlobalIPv6Address", EmitDefaultValue = false)]
        public string GlobalIPv6Address { get; set; }

        [DataMember(Name = "GlobalIPv6PrefixLen", EmitDefaultValue = false)]
        public long GlobalIPv6PrefixLen { get; set; }

        [DataMember(Name = "MacAddress", EmitDefaultValue = false)]
        public string MacAddress { get; set; }
    }
}