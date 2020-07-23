// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Health;

    internal sealed class MockHealthClient : IHealthClient
    {
        // no action for now
        public void ReportHealth(HealthReport healthReport)
        {            
        }
    }
}