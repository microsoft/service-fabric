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
    public class ApplicationUnitTests
    {
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ApplicationClientConstructorThrowsOnNullFabricClient()
        {
            var client = new ApplicationClient(null);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ApplicationCommandProcessorConstructorThrowsOnNullFabricClientWrapper()
        {
            var command = new ApplicationCommandProcessor(null);
        }

        [TestMethod]
        public void ApplicationCommandProcessorProcessFaultsOnNull()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
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
        public void ApplicationCommandProcessorProcessReturnsEmptyStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            var descriptions = new List<IOperationDescription>();
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.ProcessAsync(descriptions, context);
            Assert.IsFalse(task.Result.Any(), "Processing empty list should return no results.");
        }

        [TestMethod]
        public void ApplicationCommandProcessorProcessReturnsOnlyNonNullResults()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(10));
            var descriptions = new List<IOperationDescription>()
            {
                new ApplicationOperationDescription()
                {
                    OperationType = OperationType.CreateOrUpdate,
                    OperationSequenceNumber = 0,
                    ResourceId = "/app1"
                },
                new ApplicationOperationDescription()
                {
                    OperationType = OperationType.Delete,
                    OperationSequenceNumber = 1,
                    ResourceId = "/app1"
                }
            };

            var task = command.ProcessAsync(descriptions, context);
            Assert.AreEqual(task.Result.Count, 1);
        }

        [TestMethod]
        public void ApplicationCommandProcessorCreateAppIfNotExist()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new ApplicationOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/app1"
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
        public void ApplicationCommandProcessorUpdateAppIfExists()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult()
            {
                OperationStatus = new ApplicationOperationStatus(),
                QueryResult = new ApplicationFabricQueryResult(null)
            };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.UpdateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IFabricOperationResult>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new ApplicationOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/app1"
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
        public void ApplicationCommandProcessorNoActionForOperationInProgress()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new ApplicationOperationDescription()
            {
                OperationType = OperationType.CreateOrUpdate,
                OperationSequenceNumber = 0,
                ResourceId = "/app1"
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
                    c => c.CreateAsync(description, It.IsAny<IOperationContext>()),
                    Times.Once());
            mockClient
                .Verify(
                    c => c.UpdateAsync(description, It.IsAny<IFabricOperationResult>(), It.IsAny<IOperationContext>()),
                    Times.Never);
        }

        [TestMethod]
        public void ApplicationCommandProcessorExceptionOnCreateReturnsFailedStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(null));
            mockClient
                .Setup(c => c.CreateAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var description = new ApplicationOperationDescription("appResource1")
            {
                OperationType = OperationType.CreateOrUpdate
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var status = command.CreateOperationStatusAsync(description, context).Result;
            Assert.IsInstanceOfType(status, typeof(ApplicationOperationStatus));
            Assert.AreEqual(status.ResourceId, description.ResourceId);
            Assert.AreEqual(status.Status, ResultStatus.Failed);
        }

        [TestMethod]
        public void ApplicationCommandProcessorDeleteWithNonNullStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult() { OperationStatus = new ApplicationOperationStatus() };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult(0));
            var description = new ApplicationOperationDescription()
            {
                OperationType = OperationType.Delete,
                OperationSequenceNumber = 0,
                ResourceId = "/app1"
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
        public void ApplicationCommandProcessorExceptionOnDeleteReturnsFailedStatus()
        {
            var mockClient = new Mock<IFabricClientApplicationWrapper>();
            var command = new ApplicationCommandProcessor(mockClient.Object);
            var result = new FabricOperationResult() { OperationStatus = new ApplicationOperationStatus() };
            mockClient
                .Setup(c => c.GetAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Returns(Task.FromResult<IFabricOperationResult>(result));
            mockClient
                .Setup(c => c.DeleteAsync(It.IsAny<IOperationDescription>(), It.IsAny<IOperationContext>()))
                .Throws(new Exception());
            var description = new ApplicationOperationDescription("appResource1")
            {
                OperationType = OperationType.Delete
            };
            var context = new OperationContext(CancellationToken.None, TimeSpan.FromMinutes(1));

            var task = command.CreateOperationStatusAsync(description, context).Result;
            Assert.IsInstanceOfType(task, typeof(ApplicationOperationStatus));
            Assert.AreEqual(task.ResourceId, description.ResourceId);
            Assert.AreEqual(task.Status, ResultStatus.Failed);
        }
    }
}