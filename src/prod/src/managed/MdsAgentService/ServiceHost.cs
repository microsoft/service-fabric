// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Fabric;

    internal sealed class ServiceHost
    {
        private static void Main(string[] args)
        {
            FabricRuntime fabricRuntime = null;
            try
            {
                fabricRuntime = FabricRuntime.Create();
                if (fabricRuntime != null)
                {
                    fabricRuntime.RegisterServiceType(Constants.ServiceTypeName, typeof(Service));
                    Console.WriteLine("[Press enter to exit]");
                    Console.ReadLine();
                }
            }
            finally
            {
                if (fabricRuntime != null)
                {
                    fabricRuntime.Dispose();
                }

                fabricRuntime = null;
            }
        }
    }
}