// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class CreateContainerExecResponse
    {
        [DataMember(Name = "Id", EmitDefaultValue = false)]
        public string ID { get; set; }
    }
}