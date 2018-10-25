// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal class LoggingReplicatorCopyContext : IOperationDataStream
    {
        private readonly OperationData copyData;

        private bool isDone;

        public LoggingReplicatorCopyContext(
            long replicaId,
            ProgressVector progressVector,
            IndexingLogRecord logHeadRecord,
            LogicalSequenceNumber logTailLsn,
            long latestrecoveredAtomicRedoOperationLsn)
        {
            using (var stream = new MemoryStream())
            {
                using (var bw = new BinaryWriter(stream))
                {
                    bw.Write(replicaId);
                    progressVector.Write(bw);
                    bw.Write(logHeadRecord.CurrentEpoch.DataLossNumber);
                    bw.Write(logHeadRecord.CurrentEpoch.ConfigurationNumber);
                    bw.Write(logHeadRecord.Lsn.LSN);
                    bw.Write(logTailLsn.LSN);
                    bw.Write(latestrecoveredAtomicRedoOperationLsn);
                    this.copyData = new OperationData(stream.ToArray());
                    this.isDone = false;
                }
            }
        }

        public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            var data = (this.isDone == false) ? this.copyData : null;
            this.isDone = true;
            return Task.FromResult(data);
        }
    }
}