// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class VolumeConfig
    {
        [DataMember(Name = "Name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "Driver", EmitDefaultValue = false)]
        public string Driver { get; set; }

        [DataMember(Name = "DriverOpts", EmitDefaultValue = false)]
        public IDictionary<string, string> DriverOpts { get; set; }

        [DataMember(Name = "Labels", EmitDefaultValue = false)]
        public IDictionary<string, string> Labels { get; set; }
    }
}