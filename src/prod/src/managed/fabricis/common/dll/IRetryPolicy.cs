// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Interface for a retry policy to handle re-tryable exceptions like transient exceptions, timeout exceptions etc.    
    /// </summary>
    /// <remarks>Concepts are similar to Azure storage SDK's retry policy.</remarks>
    internal interface IRetryPolicy
    {
        /// <summary>
        /// Gets or sets a user supplied method that determines whether the operation should be retried.
        /// </summary>
        Func<Exception, bool> ShouldRetry { get; set; }

        /// <summary>
        /// Executes the specified user supplied method and retries accordingly.
        /// </summary>
        /// <typeparam name="T">The return type of the user supplied method.</typeparam>
        /// <param name="func">The user supplied method to execute and retry in case of retry-able exceptions.</param>
        /// <param name="funcName">The name of the user supplied method (just for additional context while tracing/logging).</param>
        /// <returns>The return value of the user supplied method or a default value in case of failure.</returns>
        T Execute<T>(Func<T> func, string funcName);

        /// <summary>
        /// Executes the specified user method in async way and retries accordingly.
        /// </summary>
        /// <typeparam name="T">The return type of the user supplied method.</typeparam>
        /// <param name="func">The user supplied method to execute and retry in case of retry-able exceptions.</param>
        /// <param name="funcName">The name of the user supplied method (just for additional context while tracing/logging).</param>
        /// <returns>Returns the task thas represent the completion of the user supplied method. Task will return value of the user supplied method or a default value in case of failure.</returns>
        Task<T> ExecuteAsync<T>(Func<CancellationToken, Task<T>> func, string funcName, CancellationToken token);
    }
}