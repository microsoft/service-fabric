// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM
{
    public enum HealthState
    {
        Invalid = 0x0,
        Ok = 0x1,
        Warning = 0x2,
        Error = 0x3,
        Unknown = 0xffff,
    }
}
