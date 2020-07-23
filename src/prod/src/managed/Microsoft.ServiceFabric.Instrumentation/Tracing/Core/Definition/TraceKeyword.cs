// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    /// <summary>
    /// Keywords used in SF Tracing.
    /// </summary>
    public enum TraceKeyword : ulong
    {
        Default = 0x01,
        AppInstance = 0x02,
        ForQuery = 0x04,
        CustomerInfo = 0x08,
        DataMessaging = 0x10,
        DataMessagingAll = 0x20,
    }
}
