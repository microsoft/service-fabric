// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System.Collections.Generic;
    using System.Fabric.Testability.Common;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class CompositeStrategy : IStrategy
    {
        private readonly IStrategy[] strategies;

        public CompositeStrategy(params IStrategy[] strategies)
            : this((IEnumerable<IStrategy>)strategies)
        {
        }

        public CompositeStrategy(IEnumerable<IStrategy> strategies)
        {
            ThrowIf.Null(strategies, "strategies");
            this.strategies = strategies.ToArray();
        }

        public async Task PerformAsync(IRetryContext context, CancellationToken cancellationToken)
        {
            ThrowIf.Null(context, "context");

            foreach (IStrategy strategy in this.strategies)
            {
                await strategy.PerformAsync(context, cancellationToken);
            }
        }
    }
}


#pragma warning restore 1591