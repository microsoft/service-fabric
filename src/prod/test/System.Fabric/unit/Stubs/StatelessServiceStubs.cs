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

    public class StatelessServiceStubBase : IStatelessServiceInstance
    {
        #region IStatelessService Members

        public virtual Task CloseAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual void Abort()
        {
            throw new NotImplementedException();
        }

        public virtual void Initialize(StatelessServiceInitializationParameters initializationParameters)
        {
            throw new NotImplementedException();
        }

        public virtual IEnumerable<Uri> ListenerPrefixes
        {
            get { throw new NotImplementedException(); }
        }

        public virtual Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region IProcessHttpRequest Members

        public virtual Task<HttpStatusCode> ProcessRequestAsync(HttpListenerRequest request, Task<Net.HttpListenerResponse> responseContinuationTask, object asyncState)
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