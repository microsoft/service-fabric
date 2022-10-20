// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using Newtonsoft.Json;
    using System;

    [Serializable]
    public enum HttpClientAction
    {
        [JsonProperty(PropertyName = "DoWork")]
        DoWork,
        [JsonProperty(PropertyName = "CheckWorkStatus")]
        CheckWorkStatus,
        [JsonProperty(PropertyName = "GetProgress")]
        GetProgress,
    }
}