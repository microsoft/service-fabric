// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Threading.Tasks;
    using System.Threading;
    using System.Fabric.UpgradeService;
    using System.Fabric.WRP.Model;
    using System.Linq;

    [TestClass]
    [ExcludeFromCodeCoverage]
    public class ServiceUnitTests
    {
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ServiceClientConstructorThrowsOnNullFabricClient()
        {
            var client = new ServiceClient(null);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ServiceCommandProcessorConstructorThrowsOnNullFabricClientWrapper()
        {
            var command = new ServiceCommandProcessor(null);
        }

        [TestMethod]
        public void ServiceCommandProcessorProcessFaultsOnNull()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            try
            {
                command.ProcessAsync(null, null).Wait();
                Assert.Fail($"Null {nameof(IOperationDescription)} did not throw an exception");
            }
            catch (AggregateException ae)
            {
                Assert.IsTrue(ae.InnerException != null);
                Assert.IsInstanceOfType(ae.InnerException, typeof(ArgumentNullException));
            }
        }

        [TestMethod]
        public void ServiceCommandProcessorProcessReturnsEmptyStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            var descriptions = new List<IOperationDescription>();
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.ProcessAsync(descriptions, context);
            Assert.IsFalse(task.Result.Any(), "Processing empty list should return no results.");
        }

        [TestMethod]
        public void ServiceCommandProcessorProcessReturnsOnlyNonNullResults()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));
            var descriptions = new List<IOperationDescription>()
            {
                new StatelessServiceOperationDescription()
                {
                    OperationType = OperationType.CreateOrUpdate,
                    OperationSequenceNumber = 0,
                    ResourceId = "/svc1"
                },
                new StatefulServiceOperationDescription()
                {
                    OperationType = OperationType.Delete,
                    OperationSequenceNumber = 1,
                    ResourceId = "/svc1"
                }
            };

            var task = command.ProcessAsync(descriptions, context);
            Assert.AreEqual(task.Result.Count, 1);
        }

        [TestMethod]
        public void ServiceCommandProcessorCreateAppIfNotExist()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new StatelessServiceOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/svc1"
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.CreateOperationStatusAsync(description, context);
            Assert.AreEqual(task.Result, null);
            mockClient
                .Verify(
                    c => c.GetAsync(description, context),
                    Times.Exactly(2));
            mockClient
                .Verify(
                    c => c.CreateAsync(description, It.IsAny<IOperationContext>() /*context is changed in the method*/),
                    Times.Once());
            mockClient
                .Verify(
                    c => c.UpdateAsync(description, It.IsAny<IFabricOperationResult>(), It.IsAny<IOperationContext>()),
                    Times.Never);
        }

        [TestMethod]
        public void ServiceCommandProcessorUpdateAppIfExists()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult()
            {
                OperationStatus = new ServiceOperationStatus(),
                QueryResult = new ServiceClientFabricQueryResult(null)
            };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.UpdateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IFabricOperationResult>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new StatelessServiceOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/svc1"
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.CreateOperationStatusAsync(description, context);
            Assert.AreNotEqual(task.Result, null);
            mockClient
                .Verify(
                    c => c.GetAsync(description, context),
                    Times.Exactly(2));
            mockClient
                .Verify(
                    c => c.CreateAsync(description, It.IsAny<IOperationContext>()),
                    Times.Never);
            mockClient
                .Verify(
                    c => c.UpdateAsync(description, result, It.IsAny<IOperationContext>()),
                    Times.Once());
        }

        [TestMethod]
        public void ServiceCommandProcessorNoActionForOperationInProgress()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new StatelessServiceOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/svc1"
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            command.CreateOperationStatusAsync(description, context).Wait();
            command.CreateOperationStatusAsync(description, context).Wait();
            mockClient
                .Verify(
                    c => c.GetAsync(description, context),
                    Times.Exactly(3));
            mockClient
                .Verify(
                    c => c.CreateAsync(description, It.IsAny<IOperationContext>() /*context is changed in the method*/),
                    Times.Once());
            mockClient
                .Verify(
                    c => c.UpdateAsync(description, It.IsAny<IFabricOperationResult>(), It.IsAny<IOperationContext>()),
                    Times.Never);
        }

        [TestMethod]
        public void ServiceCommandProcessorExceptionOnCreateReturnsFailedStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var description = new StatelessServiceOperationDescription("statelessServiceResource1")
            {
                OperationType = OperationType.CreateOrUpdate
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var status = command.CreateOperationStatusAsync(description, context).Result;
            Assert.IsInstanceOfType(status, typeof(ServiceOperationStatus));
            Assert.AreEqual(status.ResourceId, description.ResourceId);
            Assert.AreEqual(status.Status, ResultStatus.Failed);
        }

        [TestMethod]
        public void ServiceCommandProcessorTransientExceptionReturnsNoStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new System.Fabric.FabricTransientException());
            var description = new StatelessServiceOperationDescription("statelessServiceResource1")
            {
                OperationType = OperationType.CreateOrUpdate
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var status = command.CreateOperationStatusAsync(description, context).Result;            
            Assert.AreEqual(status, null);
        }

        [TestMethod]
        public void ServiceCommandProcessorDeleteWithNonNullStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult() { OperationStatus = new ServiceOperationStatus() };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new StatefulServiceOperationDescription()
            {
                OperationType = OperationType.Delete,
                OperationSequenceNumber = 0,
                ResourceId = "/svc1"
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.CreateOperationStatusAsync(description, context);
            Assert.AreNotEqual(task.Result, null);
            mockClient
                .Verify(
                    c => c.GetAsync(description, context),
                    Times.Exactly(2));
            mockClient
                .Verify(
                    c => c.DeleteAsync(description, It.IsAny<IOperationContext>()),
                    Times.Once());
        }

        [TestMethod]
        public void ServiceCommandProcessorExceptionOnDeleteReturnsFailedStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ServiceCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult() { OperationStatus = new ServiceOperationStatus() };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var description = new StatefulServiceOperationDescription("statefulServiceResource1")
            {
                OperationType = OperationType.Delete
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.CreateOperationStatusAsync(description, context).Result;
            Assert.IsInstanceOfType(task, typeof(ServiceOperationStatus));
            Assert.AreEqual(task.ResourceId, description.ResourceId);
            Assert.AreEqual(task.Status, ResultStatus.Failed);
        }
    }
}