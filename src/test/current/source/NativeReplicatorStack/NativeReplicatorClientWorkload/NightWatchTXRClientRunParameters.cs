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
    public class NightWatchTXRClientRunParameters
    {
        [JsonProperty(PropertyName = "NumberOfOperations")]
        public long NumberOfOperations { get; set; }

        [JsonProperty(PropertyName = "MaxOutstandingOperations")]
        public long MaxOutstandingOperations { get; set; }

        [JsonProperty(PropertyName = "OperationSize")]
        public long OperationSize { get; set; }


        [JsonProperty(PropertyName = "Action")]
        [JsonConverter(typeof(StringEnumConverter))]
        public NightWatchTXRClientTestAction Action { get; set; }
    }
}