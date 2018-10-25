// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class LogConfig
    {
        [DataMember(Name = "Type", EmitDefaultValue = false)]
        public string Type { get; set; }

        [DataMember(Name = "Config", EmitDefaultValue = false)]
        public IDictionary<string, string> Config { get; set; }
    }
}