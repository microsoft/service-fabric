// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    class InvalidServiceStub : IStatelessServiceInstance, IStatefulServiceReplica
    {
        #region IStatelessService Members


        public Threading.Tasks.Task CloseAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public void Abort()
        {
            throw new NotImplementedException();
        }

        public Collections.Generic.IEnumerable<Uri> ListenerPrefixes
        {
            get { throw new NotImplementedException(); }
        }

        public Threading.Tasks.Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region IStatefulServiceReplica Members

        public ReplicatorSettings ReplicatorSettings
        {
            get { throw new NotImplementedException(); }
        }

        public long CurrentProgress
        {
            get { throw new NotImplementedException(); }
        }

        public IOperationDataStream GetCopyContext()
        {
            throw new NotImplementedException();
        }

        public IOperationDataStream GetCopyState(long sequenceNumber, IOperationDataStream copyContext)
        {
            throw new NotImplementedException();
        }

        public Threading.Tasks.Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Threading.Tasks.Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public ReplicaRole ReplicaRole
        {
            get { throw new NotImplementedException(); }
        }

        public Threading.Tasks.Task<string> ReplicaRoleChangeAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        #endregion

        public virtual void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            throw new NotImplementedException();
        }

        public virtual void Initialize(StatelessServiceInitializationParameters initializationParameters)
        {
            throw new NotImplementedException();
        }

        public virtual Threading.Tasks.Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }
}