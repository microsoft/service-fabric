// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    internal interface IListElement<T>
    {
        long Id { get; }

        T Value { get; }

        /// <summary>The succeeding listElement in the list towards the tail</summary>
        IListElement<T> Next { get; }

        /// <summary>The preceding listElement in the list towards the head</summary> 
        IListElement<T> Previous { get; }

        ListElementState State { get; }

        ITransaction DequeueCommittingTransaction { get; }

        void StartEnqueue();

        void AbortEnqueue();

        void ApplyEnqueue();

        void UndoEnqueue();

        void StartDequeue();

        void AbortDequeue();

        void ApplyDequeue(ITransaction committingTx);

        void UndoDequeue();
    }
}