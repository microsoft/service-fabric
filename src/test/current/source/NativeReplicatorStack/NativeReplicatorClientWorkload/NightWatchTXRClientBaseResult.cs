// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NativeReplicatorClientWorkload
{
    [Serializable]
    public class NightWatchTXRClientBaseResult
    {
        public string Msg { get; set; }

        [JsonConverter(typeof(StringEnumConverter))]
        public NightWatchTXRClientTestStatus Status { get; set; }
    }
}