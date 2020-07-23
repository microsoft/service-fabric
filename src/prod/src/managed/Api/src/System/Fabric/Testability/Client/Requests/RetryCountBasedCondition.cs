// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System.Fabric.Testability.Common;

    internal class RetryCountBasedCondition : ICondition
    {
        private readonly int maxRetryCount;

        public RetryCountBasedCondition(int maxRetryCount)
        {
            ThrowIf.OutOfRange(maxRetryCount, 0, int.MaxValue, "maxRetryCount");
            this.maxRetryCount = maxRetryCount;
        }

        public string ConditionType
        {
            get { return "count"; }
        }

        public bool IsSatisfied(IRetryContext context)
        {
            ThrowIf.Null(context, "context");

            return context.ShouldRetry && context.Iteration < this.maxRetryCount;
        }
    }
}


#pragma warning restore 1591