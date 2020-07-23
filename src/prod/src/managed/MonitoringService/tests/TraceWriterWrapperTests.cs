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

    [TestClass]
    public class TraceWriterWrapperTests
    {
        [TestMethod]
        public void TraceException_Traces_ExceptionChain_ToEventSource()
        {
            var traceWriterMock = new Mock<TraceWriterWrapper>() { CallBase = true };

            string errorTrace = null;
            traceWriterMock
                .Setup(mock => mock.WriteError(It.IsAny<string>(), It.IsAny<object[]>()))
                .Callback<string, object[]>((format, args) => errorTrace = string.Format(format, args));

            try
            {
                var innerEx = new Exception("Inner_Exception");
                var exception = new Exception("Test", innerEx);
                throw exception;
            }
            catch (Exception ex)
            {
                traceWriterMock.Object.Exception(ex);
            }

            Assert.IsTrue(errorTrace.Contains("Test"));
            Assert.IsTrue(errorTrace.Contains("Inner_Exception"));
        }
    }
}