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
    public class NightWatchTXRClientTestParameters
    {
        public int NumberOfWorkloads { get; set; }
        public long NumberOfOperations { get; set; }
        public long MaxOutstandingOperations { get; set; }
        public long OperationSize { get; set; }
    }
}