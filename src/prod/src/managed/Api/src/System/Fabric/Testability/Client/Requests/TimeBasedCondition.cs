// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Common;

    internal class TimeBasedCondition : ICondition
    {
        private readonly TimeSpan maxRetryTime;

        public TimeBasedCondition(TimeSpan maxRetryTime)
        {
            this.maxRetryTime = maxRetryTime;          
        }

        public string ConditionType
        {
            get { return "time"; }
        }

        public bool IsSatisfied(IRetryContext context)
        {
            ThrowIf.Null(context, "context");

            return context.ShouldRetry && context.ElapsedTime < this.maxRetryTime;
        }
    }
}


#pragma warning restore 1591