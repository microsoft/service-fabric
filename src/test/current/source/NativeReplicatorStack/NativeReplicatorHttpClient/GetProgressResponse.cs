// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using Newtonsoft.Json;
    using System;
    using System.Collections.Generic;

    class GetProgressResponse
    {
        [JsonProperty(PropertyName = "CustomProgress")]
        public IList<IComparable> CustomProgress { get; set; }

        [JsonProperty(PropertyName = "LastCommittedSequenceNumber")]
        public long LastCommittedSequenceNumber { get; set; }

        [JsonProperty(PropertyName = "ConfigurationNumber")]
        public long ConfigurationNumber { get; set; }

        [JsonProperty(PropertyName = "DataLossNumber")]
        public long DataLossNumber { get; set; }
    }
}