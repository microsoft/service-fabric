// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NativeReplicatorClientWorkload
{
    [Serializable]
    public enum NightWatchTXRClientTestAction
    {
        [JsonProperty(PropertyName = "Invalid")]
        Invalid,
        [JsonProperty(PropertyName = "Run")]
        Run,
        [JsonProperty(PropertyName = "Status")]
        Status,
    }
}