// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System.Fabric;

    class StatelessServiceFactoryStubBase : IStatelessServiceFactory
    {
        public virtual IStatelessServiceInstance CreateInstance(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
        {
            throw new NotImplementedException();
        }
    }

    class StatefulServiceFactoryStubBase : IStatefulServiceFactory
    {
        public IStatefulServiceReplica CreateReplica(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
        {
            throw new NotImplementedException();
        }
    }
}