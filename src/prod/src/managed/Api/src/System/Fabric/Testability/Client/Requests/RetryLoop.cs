// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RetryLoop
    {
        private readonly ICondition condition;
        private readonly IStrategy strategy;

        public RetryLoop(IStrategy strategy, ICondition condition)
        {
            ThrowIf.Null(strategy, "strategy");
            ThrowIf.Null(condition, "condition");

            this.strategy = strategy;
            this.condition = condition;
        }

        public async Task<IRetryContext> PerformAsync(IRequest request, CancellationToken cancellationToken)
        {
            ThrowIf.Null(request, "request");

            Guid operationId = Guid.NewGuid();
            RetryContext context = new RetryContext(request, operationId);

            TestabilityTrace.TraceSource.WriteNoise("RetryLoop", "{0}: Retry Loop started for request: '{1}'.", operationId, request.ToString());

            do
            {
                context.Iteration++;
                if (context.Iteration > 0)
                {
                    TestabilityTrace.TraceSource.WriteInfo("RetryLoop", "Request '{0}' is being retried. Iteration:{1}", context.Request.ToString(), context.Iteration);
                }

                await this.strategy.PerformAsync(context, cancellationToken);
                TestabilityTrace.TraceSource.WriteNoise(
                    "RetryLoop", 
                    "{0}: Retry Loop iteration completed. Current ContextInfo: Succeeded: '{1}' ShouldRetry: '{2}' Iteration: '{3}' ElapsedTime: '{4}'", 
                    operationId, 
                    context.Succeeded, 
                    context.ShouldRetry, 
                    context.Iteration, 
                    context.ElapsedTime.ToString());
            }
            while (this.condition.IsSatisfied(context) && !cancellationToken.IsCancellationRequested);

            if (cancellationToken.IsCancellationRequested)
            {
                TestabilityTrace.TraceSource.WriteWarning("RetryLoop", "{0}: Cancellation was requested for request", operationId);
                throw new OperationCanceledException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Cancellation requested for operation '{0}'",
                        context.Request.ToString()));
            }

            if (!context.Succeeded)
            {
                TestabilityTrace.TraceSource.WriteWarning("RetryLoop", "{0}: Retry '{1}' has been exhausted for request", operationId, this.condition.ConditionType);
                throw new TimeoutException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Retry {0} has been exhausted for operation '{1}'",
                        this.condition.ConditionType,
                        context.Request.ToString()));
            }
            else
            {
                return context;
            }
        }

        private sealed class RetryContext : IRetryContext
        {
            private readonly Stopwatch stopWatch;

            public RetryContext(IRequest request, Guid activityId)
            {
                ThrowIf.Null(request, "request");

                this.Request = request;
                this.ActivityId = activityId;
                this.Iteration = -1;
                this.stopWatch = Stopwatch.StartNew();
            }

            public IRequest Request
            {
                get;
                private set;
            }

            public Guid ActivityId
            {
                get;
                private set;
            }

            public bool Succeeded
            {
                get;
                set;
            }

            public bool ShouldRetry
            {
                get;
                set;
            }

            public int Iteration
            {
                get;
                set;
            }

            public TimeSpan ElapsedTime
            {
                get
                {
                    return this.stopWatch.Elapsed;
                }
            }
        }
    }
}


#pragma warning restore 1591