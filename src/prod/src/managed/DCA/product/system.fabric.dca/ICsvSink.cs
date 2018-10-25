// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    internal interface ICsvSink
    {
        // Method invoked when the producer makes a set of raw ETW events available
        void OnCsvEventsAvailable(IEnumerable<CsvEvent> csvEvents);
    }
}