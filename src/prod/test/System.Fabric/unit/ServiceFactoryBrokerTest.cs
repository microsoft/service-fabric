// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceFactoryBrokerTest
    {
        private const string DefaultServiceTypeName = "SomeService";
        private const string DefaultServiceUri = "fabric:/SomeService";
        private static readonly Guid DefaultPartitionId = Guid.NewGuid();
        private const long DefaultInstanceId = 12;
        private static readonly byte[] DefaultInitializationData = new byte[] { 0x01, 0x02, 0x03 };

        private static readonly CodePackageActivationContext DefaultCodePackageActivationContext = new CodePackageActivationContext(new CodePackageActivationContextStub());

        private static Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> StatefulServiceCreationFunc = (sf, i1, i2, i3, i4, i5, i6) =>
        {
            return (sf.CreateReplica(i1, i2, i3, i4, i5, i6) as StatefulServiceReplicaBroker).Service;
        };

        private static Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> StatelessServiceCreationFunc = (sf, i1, i2, i3, i4, i5, i6) =>
        {
            return (sf.CreateInstance(i1, i2, i3, i4, i5, i6) as StatelessServiceBroker).Service;
        };

        private static IntPtr PtrDefaultServiceType;
        private static IntPtr PtrDefaultServiceUri;

        private static PinBlittable PinServiceType;
        private static PinBlittable PinServiceUri;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            ServiceFactoryBrokerTest.PinServiceType = new PinBlittable(ServiceFactoryBrokerTest.DefaultServiceTypeName);
            PtrDefaultServiceType = ServiceFactoryBrokerTest.PinServiceType.AddrOfPinnedObject();

            ServiceFactoryBrokerTest.PinServiceUri = new PinBlittable(ServiceFactoryBrokerTest.DefaultServiceUri);
            PtrDefaultServiceUri = ServiceFactoryBrokerTest.PinServiceUri.AddrOfPinnedObject();
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            ServiceFactoryBrokerTest.PinServiceType.Dispose();
            ServiceFactoryBrokerTest.PinServiceUri.Dispose();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_ConstructorFailsIfIncorrectTypeIsSpecified()
        {
            TestUtility.ExpectException<ArgumentException>(() => new ServiceFactoryBroker(new object(), null));
        }

        #region Parameter Validation

        private void ParameterValidationTestHelper<TStub>(
            IntPtr serviceType,
            IntPtr serviceUri,
            Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> creationFunc) where TStub : class, new()
        {
            try
            {
                var factory = CreateServiceFactory<TStub>(null);
                creationFunc(factory, serviceType, serviceUri, 0, IntPtr.Zero, ServiceFactoryBrokerTest.DefaultPartitionId, ServiceFactoryBrokerTest.DefaultInstanceId);
                Assert.Fail("Expected exception");
            }
            catch (ArgumentNullException)
            {
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactory_Stateless_NullServiceTypeThrowsArgumentNullException()
        {
            this.ParameterValidationTestHelper<StatelessServiceFactoryStubBase>(
                IntPtr.Zero,
                ServiceFactoryBrokerTest.PtrDefaultServiceUri,
                ServiceFactoryBrokerTest.StatelessServiceCreationFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactory_Stateful_NullServiceTypeThrowsArgumentNullException()
        {
            this.ParameterValidationTestHelper<StatefulServiceFactoryStubBase>(
                IntPtr.Zero,
                ServiceFactoryBrokerTest.PtrDefaultServiceUri,
                ServiceFactoryBrokerTest.StatefulServiceCreationFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactory_Stateless_NullServiceUriThrowsArgumentNullException()
        {
            this.ParameterValidationTestHelper<StatelessServiceFactoryStubBase>(
                ServiceFactoryBrokerTest.PtrDefaultServiceType,
                IntPtr.Zero,
                ServiceFactoryBrokerTest.StatelessServiceCreationFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactory_Stateful_NullServiceUriThrowsArgumentNullException()
        {
            this.ParameterValidationTestHelper<StatefulServiceFactoryStubBase>(
                ServiceFactoryBrokerTest.PtrDefaultServiceType,
                IntPtr.Zero,
                ServiceFactoryBrokerTest.StatefulServiceCreationFunc);
        }

        #endregion

        private void ServiceCreationAndInstanceTypeMatchTestHelper<T>(
            Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> func) where T : class, new()
        {
            var broker = ServiceFactoryBrokerTest.CreateServiceFactory<T>();

            byte[] initializationData = ServiceFactoryBrokerTest.DefaultInitializationData;
            using (var pin = new PinBlittable(initializationData))
            {
                TestUtility.ExpectException<InvalidOperationException>(() =>
                {
                    func(
                        broker,
                        ServiceFactoryBrokerTest.PtrDefaultServiceType,
                        ServiceFactoryBrokerTest.PtrDefaultServiceUri,
                        (uint)initializationData.Length,
                        pin.AddrOfPinnedObject(),
                        ServiceFactoryBrokerTest.DefaultPartitionId,
                        ServiceFactoryBrokerTest.DefaultInstanceId);
                });
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_CreationFailsIfStatelessServiceIsCreatedWithStatefulFactory()
        {
            this.ServiceCreationAndInstanceTypeMatchTestHelper<StatefulServiceFactoryStubBase>(ServiceFactoryBrokerTest.StatelessServiceCreationFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_CreationFailsIfStatefulServiceIsCreatedWithStatelessFactory()
        {
            this.ServiceCreationAndInstanceTypeMatchTestHelper<StatelessServiceFactoryStubBase>(ServiceFactoryBrokerTest.StatefulServiceCreationFunc);
        }

        private void InvalidOperationExceptionIsThrownIfServiceFactoryReturnsNullTestHelper(
            Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> func)
        {
            var broker = ServiceFactoryBrokerTest.CreateServiceFactory<NullReturningStub>();

            byte[] initializationData = ServiceFactoryBrokerTest.DefaultInitializationData;
            using (var pin = new PinBlittable(initializationData))
            {
                TestUtility.ExpectException<InvalidOperationException>(() =>
                {
                    func(
                        broker,
                        ServiceFactoryBrokerTest.PtrDefaultServiceType,
                        ServiceFactoryBrokerTest.PtrDefaultServiceUri,
                        (uint)initializationData.Length,
                        pin.AddrOfPinnedObject(),
                        ServiceFactoryBrokerTest.DefaultPartitionId,
                        ServiceFactoryBrokerTest.DefaultInstanceId);
                });
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_InvalidOPerationExceptionIsThrownIfStatelessFactoryReturnsNull()
        {
            this.InvalidOperationExceptionIsThrownIfServiceFactoryReturnsNullTestHelper(ServiceFactoryBrokerTest.StatelessServiceCreationFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_InvalidOPerationExceptionIsThrownIfStatefulFactoryReturnsNull()
        {
            this.InvalidOperationExceptionIsThrownIfServiceFactoryReturnsNullTestHelper(ServiceFactoryBrokerTest.StatefulServiceCreationFunc);
        }

        #region Parameter translation

        private void ParametersArePassedToServiceFactoryTestHelper(
            byte[] initializationData,
            object returnValue,
            CodePackageActivationContext context,
            Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> factoryFunc,
            Func<object, ServiceInitializationParameters> initParamGetterFunc,
            Func<object, long> idGetterFunc)
        {
            ParameterSavingStub stub = new ParameterSavingStub
            {
                InstanceToReturn = returnValue
            };

            var broker = new ServiceFactoryBroker(stub, context);

            // the service factory returns byte[0]
            byte[] initializationDataExpected = initializationData == null ? new byte[0] : initializationData;

            using (var pin = new PinBlittable(initializationData))
            {
                var instance = factoryFunc(
                        broker,
                        ServiceFactoryBrokerTest.PtrDefaultServiceType,
                        ServiceFactoryBrokerTest.PtrDefaultServiceUri,
                        initializationData == null ? 0 : (uint)initializationData.Length,
                        pin.AddrOfPinnedObject(),
                        ServiceFactoryBrokerTest.DefaultPartitionId,
                        ServiceFactoryBrokerTest.DefaultInstanceId);

                // the correct values must be passed into the service factory
                Assert.AreEqual<long>(ServiceFactoryBrokerTest.DefaultInstanceId, stub.InstanceId);
                Assert.AreEqual(ServiceFactoryBrokerTest.DefaultServiceTypeName, stub.ServiceType);
                Assert.AreEqual(new Uri(ServiceFactoryBrokerTest.DefaultServiceUri).ToString().ToLower(), stub.ServiceName.ToString().ToLower());

                MiscUtility.CompareEnumerables(initializationDataExpected, stub.InitializationData);

                Assert.AreSame(returnValue, instance);

                // the values must be set on the initialization parameters (initialize must have been called)
                ServiceInitializationParameters actualInitParams = initParamGetterFunc(instance);
                long actualId = idGetterFunc(instance);
                Assert.AreEqual<long>(ServiceFactoryBrokerTest.DefaultInstanceId, actualId);
                Assert.AreEqual(ServiceFactoryBrokerTest.DefaultServiceTypeName, actualInitParams.ServiceTypeName);
                Assert.AreEqual(new Uri(ServiceFactoryBrokerTest.DefaultServiceUri).ToString().ToLower(), actualInitParams.ServiceName.ToString().ToLower());
                Assert.AreEqual(ServiceFactoryBrokerTest.DefaultPartitionId, actualInitParams.PartitionId);

                if (context != null)
                {
                    Assert.AreSame(context, actualInitParams.CodePackageActivationContext);
                }

                MiscUtility.CompareEnumerables(initializationDataExpected, actualInitParams.InitializationData);
            }
        }

        private void ParametersArePassedToServiceFactoryTestHelper(
            object service,
            Func<ServiceFactoryBroker, IntPtr, IntPtr, uint, IntPtr, Guid, long, object> factoryFunc,
            Func<object, ServiceInitializationParameters> initParamGetterFunc,
            Func<object, long> idGetterFunc)
        {
            LogHelper.Log("Testing empty byte arr, null cp");
            this.ParametersArePassedToServiceFactoryTestHelper(new byte[0], service, null, factoryFunc, initParamGetterFunc, idGetterFunc);
            this.ParametersArePassedToServiceFactoryTestHelper(new byte[0], service, ServiceFactoryBrokerTest.DefaultCodePackageActivationContext, factoryFunc, initParamGetterFunc, idGetterFunc);
            this.ParametersArePassedToServiceFactoryTestHelper(null, service, null, factoryFunc, initParamGetterFunc, idGetterFunc);
            this.ParametersArePassedToServiceFactoryTestHelper(new byte[0], service, ServiceFactoryBrokerTest.DefaultCodePackageActivationContext, factoryFunc, initParamGetterFunc, idGetterFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_Stateless_ParametersAreAssignedCorrectly()
        {
            this.ParametersArePassedToServiceFactoryTestHelper(
                new StatelessStubImplementingInitialize(),
                ServiceFactoryBrokerTest.StatelessServiceCreationFunc,
                (obj) => (obj as StatelessStubImplementingInitialize).InitializationParameters,
                (obj) => (obj as StatelessStubImplementingInitialize).InitializationParameters.InstanceId);
        }

        class StatelessStubImplementingInitialize : Stubs.StatelessServiceStubBase
        {
            public StatelessServiceInitializationParameters InitializationParameters { get; set; }

            public override void Initialize(StatelessServiceInitializationParameters initializationParameters)
            {
                this.InitializationParameters = initializationParameters;
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceFactoryBroker_Stateful_ParametersAreAssignedCorrectly()
        {
            this.ParametersArePassedToServiceFactoryTestHelper(
                new StatefulStubImplementingInitialize(),
                ServiceFactoryBrokerTest.StatefulServiceCreationFunc,
                (obj) => (obj as StatefulStubImplementingInitialize).InitializationParameters,
                (obj) => (obj as StatefulStubImplementingInitialize).InitializationParameters.ReplicaId);
        }


        class StatefulStubImplementingInitialize : Stubs.StatefulServiceReplicaStubBase
        {
            public StatefulServiceInitializationParameters InitializationParameters { get; set; }
            public Guid PartitionId { get; set; }
            public long ReplicaId { get; set; }

            public override void Initialize(StatefulServiceInitializationParameters initializationParameters)
            {
                this.InitializationParameters = initializationParameters;
            }
        }

        #endregion

        private static ServiceFactoryBroker CreateServiceFactory<T>(T factory = null, CodePackageActivationContext context = null) where T : class, new()
        {
            if (context == null)
            {
                context = ServiceFactoryBrokerTest.DefaultCodePackageActivationContext;
            }

            if (factory == null)
            {
                factory = new T();
            }

            return new ServiceFactoryBroker(factory, context);
        }

        class ParameterSavingStub : IStatelessServiceFactory, IStatefulServiceFactory
        {
            public string ServiceType { get; set; }
            public Uri ServiceName { get; set; }
            public byte[] InitializationData { get; set; }
            public Guid PartitionId { get; set; }
            public long InstanceId { get; set; }
            public object InstanceToReturn { get; set; }

            public IStatefulServiceReplica CreateReplica(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                this.ServiceType = serviceType;
                this.ServiceName = serviceName;
                this.InitializationData = initializationData;
                this.PartitionId = partitionId;
                this.InstanceId = instanceId;
                return this.InstanceToReturn as IStatefulServiceReplica;
            }

            public IStatelessServiceInstance CreateInstance(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                this.ServiceType = serviceType;
                this.ServiceName = serviceName;
                this.InitializationData = initializationData;
                this.PartitionId = partitionId;
                this.InstanceId = instanceId;
                return this.InstanceToReturn as IStatelessServiceInstance;
            }
        }

        class NullReturningStub : IStatelessServiceFactory, IStatefulServiceFactory
        {
            public IStatefulServiceReplica CreateReplica(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                return null;
            }

            public IStatelessServiceInstance CreateInstance(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                return null;
            }
        }
    }
}