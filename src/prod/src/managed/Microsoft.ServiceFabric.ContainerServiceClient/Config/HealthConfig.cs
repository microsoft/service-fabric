// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class HealthConfig // (container.HealthConfig)
    {
        [DataMember(Name = "Test", EmitDefaultValue = false)]
        public IList<string> Test { get; set; }

        [DataMember(Name = "Interval", EmitDefaultValue = false)]
        public long IntervalNanoSec { get; set; }

        [DataMember(Name = "Timeout", EmitDefaultValue = false)]
        public long TimeoutSec { get; set; }

        [DataMember(Name = "Retries", EmitDefaultValue = false)]
        public long Retries { get; set; }
    }
}