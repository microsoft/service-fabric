// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Net;
    using System.Threading;
    using System.Threading.Tasks;

    class StatefulServiceReplicaStubBase : IStatefulServiceReplica, IStateProvider
    {
        #region IStatefulServiceReplica Members

        public virtual Task CloseAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual void Abort()
        {
            throw new NotImplementedException();
        }

        public virtual Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            throw new NotImplementedException();
        }

        public virtual long GetLastCommittedSequenceNumber()
        {
            throw new NotImplementedException(); 
        }

        public virtual IOperationDataStream GetCopyContext()
        {
            throw new NotImplementedException();
        }

        public virtual IOperationDataStream GetCopyState(long sequenceNumber, IOperationDataStream copyContext)
        {
            throw new NotImplementedException();
        }

        public virtual IEnumerable<Uri> ListenerPrefixes
        {
            get { throw new NotImplementedException(); }
        }

        public virtual Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual ReplicaRole ReplicaRole
        {
            get { throw new NotImplementedException(); }
        }

        public virtual Task<string> ReplicaRoleChangeAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual ReplicatorSettings ReplicatorSettings
        {
            get { return null; }
        }

        #endregion

        #region IProcessHttpRequest Members

        public virtual Task<HttpStatusCode> ProcessRequestAsync(HttpListenerRequest request, Task<HttpListenerResponse> responseContinuationTask, object asyncState)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region IProcessTcpRequest Members

        public virtual Task<Tuple<IDictionary<string, string>, Stream>> ProcessRequestAsync(IDictionary<string, string> requestHeaders, Stream requestBody, object asyncState)
        {
            throw new NotImplementedException();
        }

        #endregion

        
    }
}