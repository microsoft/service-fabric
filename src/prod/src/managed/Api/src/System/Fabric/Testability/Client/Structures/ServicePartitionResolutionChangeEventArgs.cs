// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Fabric;

    internal class ServicePartitionResolutionChangeEventArgs : EventArgs
    {
        public ServicePartitionResolutionChangeEventArgs(ResolvedServicePartition partition, Exception exception, long callbackId)
        {
            if (partition == null && exception == null)
            {
                throw new ArgumentNullException("partition", "Both partition and exception cannot be null at the same time");
            }

            this.Partition = partition;
            this.Exception = exception;
            this.CallbackId = callbackId;
        }

        public Exception Exception
        {
            get;
            private set;
        }

        public ResolvedServicePartition Partition
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