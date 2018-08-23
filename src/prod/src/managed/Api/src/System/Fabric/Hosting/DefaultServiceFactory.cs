// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Strings;

    internal sealed class DefaultServiceFactory : IStatelessServiceFactory, IStatefulServiceFactory
    {
        private readonly Type serviceImplementationType;

        public DefaultServiceFactory(Type serviceImplementationType)
        {
            Requires.Argument<Type>("serviceImplemetnationType", serviceImplementationType).NotNull();

            this.serviceImplementationType = serviceImplementationType;

            if (!Helpers.IsStatefulService(serviceImplementationType).HasValue)
            {
                AppTrace.TraceSource.WriteError("DefaultServiceFactory.DefaultServiceFactory", "DefaultServiceFactory constructor passed an invalid type: {0}", serviceImplementationType.FullName);
                throw new ArgumentException(StringResources.Error_DefaultServiceFactory_Invalid_Type_In_Constructor);
            }
        }

        // for access via Unit tests
        internal Type ServiceImplementationType
        {
            get
            {
                return this.serviceImplementationType;
            }
        }

        public IStatefulServiceReplica CreateReplica(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
        {
            return this.CreateHelper<IStatefulServiceReplica>(serviceType, serviceName, initializationData, partitionId, instanceId, true);
        }

        public IStatelessServiceInstance CreateInstance(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
        {
            return this.CreateHelper<IStatelessServiceInstance>(serviceType, serviceName, initializationData, partitionId, instanceId, false);
        }

        private T CreateHelper<T>(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId, bool isStatefulCreate) where T : class
        {
            if (Helpers.IsStatefulService(this.serviceImplementationType).Value != isStatefulCreate)
            {
                AppTrace.TraceSource.WriteError("DefaultServiceFactory.CreateReplica", "Mismatch between service implementation type (Stateless/Stateful) and create call (CreateInstance/CreateReplica) {0}", this.serviceImplementationType.FullName);
                throw new InvalidOperationException(StringResources.Error_DefaultServiceFactory_Service_Implementation_Type_Mismatch);
            }

            try
            {
                return Activator.CreateInstance(this.serviceImplementationType) as T;
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteExceptionAsError("DefaultServiceFactory.CreateInstanceHelper", ex, "Failed while creating user object");
                throw;
            }
        }
    }
}