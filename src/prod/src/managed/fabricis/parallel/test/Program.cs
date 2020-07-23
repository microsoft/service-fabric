// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using System.Fabric.Common.Tracing;
    
    public static class Program
    {
        public static int Main(string[] args)
        {
            TraceConfig.InitializeFromConfigStore();

            int status = new ManualTester().Execute(args);
            return status;
        }
    }
}