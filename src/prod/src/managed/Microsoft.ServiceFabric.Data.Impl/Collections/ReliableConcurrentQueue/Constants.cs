// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System.Collections.Generic;
    using System.Fabric.Common;

    enum RCQMode : ushort
    {
        NonBlocking = 0,
        Blocking = 1
    }

    internal static class Constants
    {
        public const RCQMode QueueMode = RCQMode.NonBlocking;

        public const short SerializedVersion = 1;

        public const int DefaultQueueMaximumSize = int.MaxValue; // todo?: change default once this can be configured

        /// <summary>
        /// ReadOnly dictionary to get check what ListElementState is considered as persisted
        /// True : Persisted state (Would be written to disk during a Checkpoint)
        /// False : This state is not persisted.
        /// </summary>
        public static readonly ReadOnlyDictionary<ListElementState, bool> PersistentStateDictionary =
            new ReadOnlyDictionary<ListElementState, bool>(
                new Dictionary<ListElementState, bool>
                    {
                        { ListElementState.Invalid, false },
                        { ListElementState.EnqueueInFlight, false },
                        { ListElementState.EnqueueApplied, true },
                        { ListElementState.EnqueueUndone, false },
                        { ListElementState.DequeueInFlight, true },
                        { ListElementState.DequeueApplied, false }
                    });
    }
}