// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Health;

    internal interface IHealthReport
    {
        HealthReportKind Kind { get; }

        HealthInformation HealthInformation { get; }
    }
}