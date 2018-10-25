// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Runtime.Serialization;

    [DataContract]
    internal class NetworkConnectConfig
    {
        [DataMember(Name = "Container", EmitDefaultValue = false)]
        public string Container { get; set; }

        [DataMember(Name = "EndpointConfig", EmitDefaultValue = false)]
        public EndpointSettings EndpointConfig { get; set; }
    }
}