// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.Collections.Generic;
    using Tools.EtlReader;

    public interface IEtwSink
    {
        // Whether or not ETW events should be delivered in their decoded form
        bool NeedsDecodedEvent { get; }

        // Method invoked when the producer makes the manifest cache available
        void SetEtwManifestCache(ManifestCache manifestCache);

        // Method invoked when the producer makes a set of raw ETW events available
        void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents);

        // Method invoked when the producer makes a set of decoded ETW events available
        void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents);
    }
}