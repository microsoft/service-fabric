// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents a service that performs infrastructure coordination operations.
    /// </summary>
    /// <remarks>Abstraction for testability</remarks>
    internal interface IInfrastructureCoordinator : IInfrastructureService
    {
        /// <summary>
        /// Starts the coordinator.
        /// </summary>
        /// <param name="primaryEpoch">
        /// Primary epoch number, used as the leading bits of the instance ID
        /// in interactions with the Cluster Manager.
        /// </param>
        /// <param name="token">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A long-running task that represents the asynchronous execution of the coordinator.
        /// The task runs until the coordinator has finished shutting down in response to a cancellation request.
        /// </returns>
        Task RunAsync(int primaryEpoch, CancellationToken token);
    }
}