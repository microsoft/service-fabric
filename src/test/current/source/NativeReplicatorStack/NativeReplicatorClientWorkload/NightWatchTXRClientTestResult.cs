// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NativeReplicatorClientWorkload
{
    [Serializable]
    public class NightWatchTXRClientTestResult : NightWatchTXRClientBaseResult
    {
        public long NumberOfOperations { get; set; }
        public Int64 DurationSeconds { get; set; }
        public double OperationsPerSecond { get; set; }
        public double OperationsPerMillisecond { get; set; }
    }
}