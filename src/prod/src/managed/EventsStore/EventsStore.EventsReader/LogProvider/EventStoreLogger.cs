// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.LogProvider
{
    using ClusterAnalysis.Common.Log;

    internal static class EventStoreLogger
    {
        public static ILogger Logger = EventStoreLogProvider.LogProvider.CreateLoggerInstance("EventStoreExe");
    }
}
