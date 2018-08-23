// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;

    internal sealed class ServiceFactoryBroker : NativeRuntime.IFabricStatefulServiceFactory, NativeRuntime.IFabricStatelessServiceFactory
    {
        private static readonly InteropApi CreateReplicaOrInstanceApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};

        private readonly CodePackageActivationContext codePackageActivationContext;
        private readonly object serviceFactory;
        
        public ServiceFactoryBroker(object serviceFactory, CodePackageActivationContext codePackageActivationContext)
        {
            Requires.Argument<object>("serviceFactory", serviceFactory).NotNull();
            this.serviceFactory = serviceFactory;            
            this.codePackageActivationContext = codePackageActivationContext;

            if (!(serviceFactory is IStatelessServiceFactory) && !(serviceFactory is IStatefulServiceFactory))
            {
                AppTrace.TraceSource.WriteError("ServiceFactoryBroker.ServiceFactoryBroker", "Tried to construct a service factory with an invalid type. this indicates a coding error, {0}", serviceFactory.GetType().FullName);
                throw new ArgumentException(StringResources.Error_ServiceFactoryBroker_Coding_Error_Invalid_Type);
            }
        }

        // for access via unit tests
        internal CodePackageActivationContext CodePackageActivationContext
        {
            get
            {
                return this.codePackageActivationContext;
            }
        }

        // for access via unit tests
        internal object ServiceFactory
        {
            get
            {
                return this.serviceFactory;
            }
        }

        public NativeRuntime.IFabricStatefulServiceReplica CreateReplica(
            IntPtr nativeServiceType, 
            IntPtr nativeServiceName, 
            uint initializationDataLength, 
            IntPtr nativeInitializationData,
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId)
        {
            Func<IStatefulServiceFactory, ServiceInitializationParameters, IStatefulServiceReplica> creationFunc = (factory, initParams) =>
            {
                return factory.CreateReplica(initParams.ServiceTypeName, initParams.ServiceName, initParams.InitializationData, partitionId, replicaId);                
            };

            ServiceInitializationParameters initializationParameters = new StatefulServiceInitializationParameters(this.codePackageActivationContext) { ReplicaId = replicaId };
            IStatefulServiceReplica statefulService = this.CreateHelper<IStatefulServiceFactory, IStatefulServiceReplica>(
                nativeServiceType,
                nativeServiceName,
                initializationDataLength,
                nativeInitializationData,
                partitionId,
                creationFunc,
                (inst, initParams) => inst.Initialize(initParams as StatefulServiceInitializationParameters),
                initializationParameters);

            return new StatefulServiceReplicaBroker(statefulService, initializationParameters, replicaId);
        }

        public NativeRuntime.IFabricStatelessServiceInstance CreateInstance(
            IntPtr nativeServiceType, 
            IntPtr nativeServiceName, 
            uint initializationDataLength, 
            IntPtr nativeInitializationData,
            Guid partitionId,
            long instanceId)
        {
            Func<IStatelessServiceFactory, ServiceInitializationParameters, IStatelessServiceInstance> creationFunc = (factory, initParams) =>
            {
                return factory.CreateInstance(initParams.ServiceTypeName, initParams.ServiceName, initParams.InitializationData, partitionId, instanceId);
            };

            ServiceInitializationParameters initializationParameters = new StatelessServiceInitializationParameters(this.codePackageActivationContext) { InstanceId = instanceId }; 
            IStatelessServiceInstance statelessService = this.CreateHelper<IStatelessServiceFactory, IStatelessServiceInstance>(
                nativeServiceType,
                nativeServiceName,
                initializationDataLength,
                nativeInitializationData,
                partitionId,
                creationFunc,
                (inst, initParams) => inst.Initialize(initParams as StatelessServiceInitializationParameters),
                initializationParameters);
            
            return new StatelessServiceBroker(statelessService, initializationParameters, instanceId);            
        }

        private static void ValidatePublicApiArguments(IntPtr serviceType, IntPtr serviceName)
        {
            if (serviceType == IntPtr.Zero)
            {
                AppTrace.TraceSource.WriteError("ServiceFactory.ValidateArguments", "ServiceType was null");
                throw new ArgumentNullException("serviceType");
            }

            if (serviceName == IntPtr.Zero)
            {
                AppTrace.TraceSource.WriteError("ServiceFactory.ValidateArguments", "ServiceName was null");
                throw new ArgumentNullException("serviceName");
            }
        }

        private static byte[] ParseInitializationData(uint initializationDataLength, IntPtr nativeInitializationData)
        {
            return NativeTypes.FromNativeBytes(nativeInitializationData, initializationDataLength);
        }

        private TReturnValue CreateHelper<TFactory, TReturnValue>(
            IntPtr nativeServiceType, 
            IntPtr nativeServiceName, 
            uint initializationDataLength, 
            IntPtr nativeInitializationData,
            Guid partitionId,
            Func<TFactory, ServiceInitializationParameters,  TReturnValue> creationFunc, 
            Action<TReturnValue, ServiceInitializationParameters> initializationFunc, 
            ServiceInitializationParameters initializationParameters) 
            where TReturnValue : class
            where TFactory : class
        {
            ServiceFactoryBroker.ValidatePublicApiArguments(nativeServiceType, nativeServiceName);

            string managedServiceType = NativeTypes.FromNativeString(nativeServiceType);
            string managedServiceName = NativeTypes.FromNativeString(nativeServiceName);
            byte[] initializationData = ServiceFactoryBroker.ParseInitializationData(initializationDataLength, nativeInitializationData);

            Uri serviceNameUri = null;

            if (managedServiceName.StartsWith("fabric:", StringComparison.Ordinal))
            {
                serviceNameUri = new Uri(managedServiceName);

                AppTrace.TraceSource.WriteNoise(
                    "ServiceFactoryBroker.CreateInstance", 
                    "Creating Instance for {0} Uri {1} partitionId {2}", 
                    managedServiceType, 
                    serviceNameUri.OriginalString, 
                    partitionId);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "ServiceFactoryBroker.CreateInstance", 
                    "Creating Instance for {0} System Service {1} partitionId {2}", 
                    managedServiceType, 
                    managedServiceName,
                    partitionId);
            }

            TFactory factory = this.serviceFactory as TFactory;
            if (factory == null)
            {
                AppTrace.TraceSource.WriteError("ServiceFactoryBroker.CreateInstanceHelper", "ServiceFactory type is incorrect for creation call {0}", this.serviceFactory.GetType().FullName);
                throw new InvalidOperationException(StringResources.Error_ServiceFactoryBroker_Invalid_ServiceFactoryType);
            }

            initializationParameters.InitializationData = initializationData;
            initializationParameters.ServiceTypeName = managedServiceType;
            initializationParameters.ServiceName = serviceNameUri;
            initializationParameters.PartitionId = partitionId;
            
            TReturnValue instance;

            try
            {
                instance = creationFunc(factory, initializationParameters);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsWarning("ServiceFactoryBroker.CreateInstanceHelper", e, "the passed in servicefactory threw exception");
                CreateReplicaOrInstanceApi.HandleException(e);
                throw;
            }

            if (instance == null)
            {
                AppTrace.TraceSource.WriteError("ServiceFactoryBroker.CreateInstance", "ServiceFactory returned null {0}", this.serviceFactory.GetType().FullName);
                throw new InvalidOperationException(StringResources.Error_ServiceFactoryBroker_Null_Return);
            }

            try
            {
                initializationFunc(instance, initializationParameters);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsWarning("ServiceFactoryBroker.CreateInstanceHelper", e, "the service threw an exception in Initialize");
                CreateReplicaOrInstanceApi.HandleException(e);
                throw;
            }

            return instance as TReturnValue;
        }
    }
}