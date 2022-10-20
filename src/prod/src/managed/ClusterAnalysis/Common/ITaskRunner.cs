// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Task Runner contract
    /// </summary>
    /// <remarks>
    /// Today's the contract is very basic, but I have many plans for this. Basically, it can support
    /// features like
    ///     a. Retry Task automatically X times.
    ///     b. Handle a given set of exceptions.
    ///     c. Notify caller when Task fails/finishes etc through a callback.
    /// In future, I expect that there would a Policy class which this runner abstraction would accept.
    /// </remarks>
    public interface ITaskRunner
    {
        /// <summary>
        /// Run a CPU bound operation on a new Task from the queue.
        /// </summary>
        /// <param name="taskName">Task's Friendly Name</param>
        /// <param name="workItem">Compute operation to be executed</param>
        /// <param name="token">Cancellation Task</param>
        /// <returns></returns>
        Task Run(string taskName, Func<Task> workItem, CancellationToken token);

        /// <summary>
        /// Default Continuation. This is a way hosting environment can provide the hosted services
        /// with a default continuations that is suitable for the hosting conditions.
        /// </summary>
        /// <remarks>
        /// It can be used if someone is running the task themselves but want to use a system wide continuation.
        /// In future, I expect it to change as well. Basically, there should be a way to chain continuation and the user
        /// can chain together their own logic as well as the hosting environment continuation and use that.
        /// </remarks>
        Action<Task> GetDefaultContinuation(string taskName, CancellationToken token);
    }
}