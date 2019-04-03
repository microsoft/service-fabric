// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;

    /// <summary>
    /// Apply context for operations on the state manager
    /// </summary>
    internal class StateManagerApplyOperationContext
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StateManagerApplyOperationContext"/> class.
        /// </summary>
        internal StateManagerApplyOperationContext(Uri name, StateManagerApplyType applyType)
        {
            Utility.Assert(name != null, "Name cannot be empty instate manager apply operation context.");

            this.Name = name;
            this.ApplyType = applyType;
        }

        /// <summary>
        /// operation apply type.
        /// </summary>
        internal StateManagerApplyType ApplyType { get; private set; }

        /// <summary>
        /// state manager name.
        /// </summary>
        internal Uri Name { get; private set; }
    }
}