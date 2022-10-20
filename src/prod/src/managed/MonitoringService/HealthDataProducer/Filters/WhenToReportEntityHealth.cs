// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Filters
{
    internal enum WhenToReportEntityHealth
    {
        Always = 0,
        OnWarningOrError,
        Never
    }
}