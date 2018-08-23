// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Operaion context along with the state provider identifier
    /// </summary>
    internal class NamedOperationContext
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NamedOperationContext"/> class.
        /// </summary>
        internal NamedOperationContext(object operationContext, long stateProviderId)
        {
            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "Stateprovider id cannot be empty in named operation context.");

            this.OperationContext = operationContext;
            this.StateProviderId = stateProviderId;
        }

        /// <summary>
        /// Gets or sets the operation context.
        /// </summary>
        internal object OperationContext { get; private set; }

        /// <summary>
        /// Gets or sets the state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }
    }
}