// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System.Threading;
    using System.Threading.Tasks;
    using FabricMonSvc;
    using Moq;

    /// <summary>
    /// This static class contains extension helper methods on mock objects.
    /// </summary>
    internal static class MockExtensions
    {
        internal static void Cancel_AfterFirstReadPass(this Mock<HealthDataService> serviceMock, CancellationTokenSource tokenSource, CancellationToken cancellationToken)
        {
            serviceMock
                .Setup(mock => mock.DelayNextReadPass(It.Is<CancellationToken>(token => token.Equals(cancellationToken))))
                .Callback(() => tokenSource.Cancel())
                .Returns(() => Task.FromResult(false));
        }
    }
}