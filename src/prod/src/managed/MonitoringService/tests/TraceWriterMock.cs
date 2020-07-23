// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using FabricMonSvc;
    using Moq;
    using VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Contains helper methods to help create mock objects.
    /// </summary>
    internal class TraceWriterMock
    {
        internal static Mock<TraceWriterWrapper> Create_TraceWriterMock_ThrowsOnWarningOrError()
        {
            Exception traceEx = null;
            string traceError = null;
            var traceMock = new Mock<TraceWriterWrapper>() { CallBase = false };
            traceMock.Setup(mock => mock.WriteInfo(It.IsAny<string>())).Verifiable();
            traceMock
                .Setup(mock => mock.WriteError(It.IsAny<string>(), It.IsAny<object[]>()))
                .Callback((string format, object[] args) =>
                {
                    traceError = string.Format(format, args);
                })
                .Throws(new AssertFailedException(traceError));
            traceMock
                .Setup(mock => mock.Exception(It.IsAny<Exception>(), It.IsAny<string>()))
                .Callback((Exception ex, string callerName) => traceEx = ex)
                .Throws(traceEx);
            return traceMock;
        }

        internal static Mock<TraceWriterWrapper> Create_Verifible()
        {
            var traceMock = new Mock<TraceWriterWrapper>() { CallBase = false };
            traceMock
                .Setup(mock => mock.WriteInfo(It.IsAny<string>()))
                .Verifiable();
            traceMock
                .Setup(mock => mock.WriteError(It.IsAny<string>(), It.IsAny<object[]>()))
                .Verifiable();
            traceMock
                .Setup(mock => mock.Exception(It.IsAny<Exception>(), It.IsAny<string>()))
                .Verifiable();
            return traceMock;
        }
    }
}