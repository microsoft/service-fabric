// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Hosting
{
    using System;
    using System.Fabric;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class DefaultServiceFactoryTest
    {
        private const string DefaultName = "foo";
        private static readonly Uri DefaultUri = new Uri("http://localhost");
        private static readonly Guid DefaultPartitionId = Guid.NewGuid();
        private const long DefaultInstanceId = 213;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DefaultServiceFactory_CreatingServiceFactoryWithInvalidTypeFails()
        {
            TestUtility.ExpectException<ArgumentException>(() => new DefaultServiceFactory(typeof(object)));
        }

        private IStatelessServiceInstance InvokeCreateInstance(DefaultServiceFactory factory)
        {
            return factory.CreateInstance(
                DefaultServiceFactoryTest.DefaultName,
                DefaultServiceFactoryTest.DefaultUri,
                new byte[0],
                DefaultServiceFactoryTest.DefaultPartitionId,
                DefaultServiceFactoryTest.DefaultInstanceId);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DefaultServiceFactory_StatelessServiceInstanceIsCreated()
        {
            var factory = new DefaultServiceFactory(typeof(Stubs.StatelessServiceStubBase));

            Assert.IsNotNull(this.InvokeCreateInstance(factory));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DefaultServiceFactory_StatefulServiceReplicaFactoryThrowsExceptionIfCreateInstanceIsCalled()
        {
            var factory = new DefaultServiceFactory(typeof(Stubs.StatefulServiceReplicaStubBase));

            TestUtility.ExpectException<InvalidOperationException>(() => this.InvokeCreateInstance(factory));
        }

        private IStatefulServiceReplica InvokeCreateReplica(DefaultServiceFactory factory)
        {
            return factory.CreateReplica(
                DefaultServiceFactoryTest.DefaultName,
                DefaultServiceFactoryTest.DefaultUri,
                new byte[0],
                DefaultServiceFactoryTest.DefaultPartitionId,
                DefaultServiceFactoryTest.DefaultInstanceId);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DefaultServiceFactory_StatefulServiceInstanceIsCreated()
        {
            var factory = new DefaultServiceFactory(typeof(Stubs.StatefulServiceReplicaStubBase));

            Assert.IsNotNull(this.InvokeCreateReplica(factory));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DefaultServiceFactory_StatelessFactoryThrowsExceptionIfCreateReplicaIsCalled()
        {
            var factory = new DefaultServiceFactory(typeof(Stubs.StatelessServiceStubBase));

            TestUtility.ExpectException<InvalidOperationException>(() => this.InvokeCreateReplica(factory));
        }
    }
}