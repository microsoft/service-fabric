// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;

    public interface ITelemetryWriter : IDisposable
    {
        // interface to push telemetry to an endpoint. This method is called to send telemetry
        void PushTelemetry(TelemetryCollection telemetryCollection);
    }
}