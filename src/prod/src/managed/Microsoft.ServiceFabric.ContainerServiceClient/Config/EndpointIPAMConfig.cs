// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class EndpointIPAMConfig
    {
        [DataMember(Name = "IPv4Address", EmitDefaultValue = false)]
        public string IPv4Address { get; set; }

        [DataMember(Name = "IPv6Address", EmitDefaultValue = false)]
        public string IPv6Address { get; set; }

        [DataMember(Name = "LinkLocalIPs", EmitDefaultValue = false)]
        public IList<string> LinkLocalIPs { get; set; }
    }
}