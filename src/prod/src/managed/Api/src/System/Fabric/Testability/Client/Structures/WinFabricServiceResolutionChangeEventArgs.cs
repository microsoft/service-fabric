// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    public class WinFabricServiceResolutionChangeEventArgs : EventArgs
    {
        public WinFabricServiceResolutionChangeEventArgs(TestServicePartitionInfo[] partition, uint errorCode, long callbackId)
        {
            this.ErrorCode = errorCode;
            this.Partition = partition;
            this.CallbackId = callbackId;
        }

        public IList<TestServicePartitionInfo> Partition
        {
            get;
            private set;
        }

        public uint ErrorCode
        {
            get;
            private set;
        }

        public long CallbackId
        {
            get;
            private set;
        }
    }
}


#pragma warning restore 1591