// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    public static class FabricRequestExtensions
    {
        private const double DefaultRetryDelaySeconds = 5.0;
        private static readonly TimeSpan DelayTime = TimeSpan.FromSeconds(DefaultRetryDelaySeconds);

        public static Task ExecuteAsync(this FabricRequest request, TimeSpan maxTimeout)
        {
            return request.ExecuteAsync(maxTimeout, CancellationToken.None);
        }

        public static Task ExecuteAsync(this FabricRequest request, TimeSpan maxTimeout, CancellationToken cancellationToken)
        {
            TimeBasedCondition condition = new TimeBasedCondition(maxTimeout);
            return ExecuteAsync(request, condition, cancellationToken);
        }

        public static Task ExecuteAsync(this FabricRequest request, TimeSpan delayBetweenRetries, TimeSpan maxTimeout, CancellationToken cancellationToken)
        {
            TimeBasedCondition condition = new TimeBasedCondition(maxTimeout);            
            return ExecuteAsync(request, condition, delayBetweenRetries, x => x, cancellationToken);
        }

        public static Task<OperationResult<TOutput>> ExecuteAsync<TOutput>(this FabricRequest request, TimeSpan maxTimeout)
        {
            return request.ExecuteAsync<TOutput>(maxTimeout, CancellationToken.None);
        }

        public static Task<OperationResult<TOutput>> ExecuteAsync<TOutput>(this FabricRequest request, TimeSpan maxTimeout, CancellationToken cancellationToken)
        {
            TimeBasedCondition condition = new TimeBasedCondition(maxTimeout);
            return ExecuteAsync<TOutput>(request, condition, cancellationToken);
        }

        public static Task ExecuteAsync(this FabricRequest request, int retryCount)
        {
            return request.ExecuteAsync(retryCount, CancellationToken.None);
        }

        public static Task<OperationResult<TOutput>> ExecuteAsync<TOutput>(this FabricRequest request, int retryCount)
        {
            return request.ExecuteAsync<TOutput>(retryCount, CancellationToken.None);
        }

        public static Task ExecuteAsync(this FabricRequest request, int retryCount, CancellationToken cancellationToken)
        {
            RetryCountBasedCondition condition = new RetryCountBasedCondition(retryCount);
            return ExecuteAsync(request, condition, cancellationToken);
        }

        public static Task<OperationResult<TOutput>> ExecuteAsync<TOutput>(this FabricRequest request, int retryCount, CancellationToken cancellationToken)
        {
            RetryCountBasedCondition condition = new RetryCountBasedCondition(retryCount);
            return ExecuteAsync<TOutput>(request, condition, cancellationToken);
        }

        private static Task ExecuteAsync(FabricRequest request, ICondition condition, CancellationToken cancellationToken)
        {
            return ExecuteAsync(request, condition, DelayTime, x => x, cancellationToken);
        }

        private static Task ExecuteAsync(FabricRequest request, ICondition condition, TimeSpan delayTime, Func<TimeSpan, TimeSpan> getNextDelayTime, CancellationToken cancellationToken)
        {
            RequestPerformer performer = new RequestPerformer();
            DelayStrategy delay = new DelayStrategy(delayTime, getNextDelayTime);
            ConditionalStrategy conditionalStrategy = new ConditionalStrategy(delay, r => r.ShouldRetry);
            CompositeStrategy compositeStrategy = new CompositeStrategy(performer, conditionalStrategy);

            RetryLoop loop = new RetryLoop(compositeStrategy, condition);

            return loop.PerformAsync(request, cancellationToken);
        }

        private static async Task<OperationResult<TOutput>> ExecuteAsync<TOutput>(FabricRequest request, ICondition condition, CancellationToken cancellationToken)
        {
            await ExecuteAsync(request, condition, cancellationToken);

            return (OperationResult<TOutput>)request.OperationResult;
        }
    }
}


#pragma warning restore 1591