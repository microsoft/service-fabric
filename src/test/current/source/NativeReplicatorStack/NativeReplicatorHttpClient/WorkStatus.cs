// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using Newtonsoft.Json;
    using System;

    [Serializable]
    public enum WorkStatus
    {
        [JsonProperty(PropertyName = "Unknown")]
        Unknown,
        [JsonProperty(PropertyName = "NotStarted")]
        NotStarted,
        [JsonProperty(PropertyName = "Faulted")]
        Faulted,
        [JsonProperty(PropertyName = "InProgress")]
        InProgress,
        [JsonProperty(PropertyName = "Done")]
        Done,
    }
}