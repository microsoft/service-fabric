// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;
    using TelemetryAggregation;

    internal sealed class MockUpTelemetryUploader : ITelemetryWriter
    {
        public MockUpTelemetryUploader()
        {
        }

        public void PushTelemetry(TelemetryCollection telemetryCollection)
        {
            this.TelemetriesUploaded++;
        }

        public void Dispose()
        {
        }

        public int TelemetriesUploaded { get; private set; }
    }
}