// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System;

    internal class ExternalOperationTime
    {
        // The beginning tick count for async operation
        private readonly long[] beginTime;

        internal ExternalOperationTime(int concurrencyCount)
        {
            this.beginTime = new long[concurrencyCount];
            this.Reset();
        }

        internal enum ExternalOperationType
        {
            TableUpload,
            TableQuery,
            TableDeletion,
            BlobCopy,
            BlobQuery,
            BlobDeletion,
            FileShareCopy,
            FileShareDeletion
        }

        // Counter to count the number of async operations that we timed
        internal ulong OperationCount { get; private set; }

        // Cumulative time for async operations
        internal double CumulativeTimeSeconds { get; private set; }

        internal void Reset()
        {
            this.OperationCount = 0;
            this.CumulativeTimeSeconds = 0;
            for (int i = 0; i < this.beginTime.Length; i++)
            {
                this.beginTime[i] = 0;
            }
        }

        internal void OperationBegin(int index)
        {
            this.beginTime[index] = DateTime.Now.Ticks;
        }

        internal void OperationEnd(int index)
        {
            long operationTimeTicks = DateTime.Now.Ticks - this.beginTime[index];
            this.CumulativeTimeSeconds += ((double)operationTimeTicks) / (10 * 1000 * 1000);
            this.OperationCount++;
            this.beginTime[index] = 0;
        }
    }
}    