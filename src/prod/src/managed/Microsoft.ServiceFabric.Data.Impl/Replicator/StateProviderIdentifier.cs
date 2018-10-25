// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;

    /// <summary>
    /// State  provider identifier.
    /// </summary>
    internal class StateProviderIdentifier
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StateProviderIdentifier"/> class.
        /// </summary>
        internal StateProviderIdentifier(Uri name, long stateProviderId)
        {
            this.Name = name;
            this.StateProviderId = stateProviderId;
        }

        /// <summary>
        /// Gets or sets state provider name.
        /// </summary>
        internal Uri Name { get; private set; }

        /// <summary>
        /// Gets or sets state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }
    }
}