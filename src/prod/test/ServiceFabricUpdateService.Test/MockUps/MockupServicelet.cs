// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Diagnostics.Tracing;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    internal sealed class MockUpTraceEventProvider : TraceEventProvider
    {
        public MockUpTraceEventProvider()
        {
        }

        internal override void InternalWrite(int eventId, EventLevel eventLevel, string content)
        {
        }
    }
}