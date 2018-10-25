// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using System.Runtime.Serialization;

    [DataContract]
    internal class NetworkCreateResponse
    {
        [DataMember(Name = "Id", EmitDefaultValue = false)]
        public string Id { get; set; }

        [DataMember(Name = "Warning", EmitDefaultValue = false)]
        public string Warning { get; set; }
    }
}