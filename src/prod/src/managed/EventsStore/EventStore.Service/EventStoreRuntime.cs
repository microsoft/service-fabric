// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    using EventStore.Service.DataReader;

    /// <summary>
    /// Encapsulates information relevant to the runtime
    /// </summary>
    public sealed class EventStoreRuntime
    {
        internal EventStoreRuntime()
        {
        }

        internal EventStoreRuntime(EventStoreReader eventReader) : this(eventReader, false)
        {
        }

        internal EventStoreRuntime(EventStoreReader eventReader, bool testMode)
        {
            Assert.IsNotEmptyOrNull("eventReader", "eventReader != null");
            this.EventStoreReader = eventReader;
            this.TestMode = testMode;
        }

        internal EventStoreReader EventStoreReader { get; private set; }

        internal bool TestMode { get; private set; }
    }
}