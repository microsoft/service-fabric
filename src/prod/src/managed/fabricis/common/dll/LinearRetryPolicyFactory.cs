// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;

    internal class LinearRetryPolicyFactory : IRetryPolicyFactory
    {
        private readonly int backoffPeriodInMilliseconds;
        private readonly int maxRetryAttempts;
        private readonly TraceType traceType;
        private readonly Func<Exception, bool> shouldRetry;

        public LinearRetryPolicyFactory(
            TraceType traceType, 
            int backoffPeriodInMilliseconds, 
            int maxRetryAttempts,
            Func<Exception, bool> shouldRetry)
        {
            traceType.ThrowIfNull("traceType");
            shouldRetry.ThrowIfNull("shouldRetry");

            this.traceType = traceType;
            this.backoffPeriodInMilliseconds = backoffPeriodInMilliseconds;
            this.maxRetryAttempts = maxRetryAttempts;
            this.shouldRetry = shouldRetry;
        }

        public IRetryPolicy Create()
        {
            var policy = new LinearRetryPolicy(
                traceType,
                TimeSpan.FromMilliseconds(backoffPeriodInMilliseconds),
                maxRetryAttempts)
            {
                ShouldRetry = shouldRetry
            };

            return policy;
        }
    }
}