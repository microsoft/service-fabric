// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    internal sealed class MockupServicelet : ServiceletBase
    {
        public MockupServicelet(ProgramConfig serviceConfig, TraceLogger logger)
            : base(serviceConfig, logger)
        {
        }
    }
}