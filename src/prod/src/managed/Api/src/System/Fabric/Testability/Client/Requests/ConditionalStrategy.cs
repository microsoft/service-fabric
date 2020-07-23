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
    using System.Threading;
    using System.Threading.Tasks;

    internal class ConditionalStrategy : IStrategy
    {
        private readonly IStrategy strategy;
        private readonly Predicate<IRetryContext> condition;

        public ConditionalStrategy(IStrategy strategy, Predicate<IRetryContext> condition)
        {
            ThrowIf.Null(strategy, "strategy");
            ThrowIf.Null(condition, "condition");

            this.strategy = strategy;
            this.condition = condition;
        }

        public async Task PerformAsync(IRetryContext context, CancellationToken cancellationToken)
        {
            ThrowIf.Null(context, "context");

            if (this.condition(context))
            {
                await this.strategy.PerformAsync(context, cancellationToken);
            }
        }
    }
}


#pragma warning restore 1591