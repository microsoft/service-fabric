// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Net;
    using System.Fabric.Common.Tracing;

    public class Program
    {
        private static readonly TraceType TraceType = new TraceType("Host");
        private static readonly UpgradeSystemServiceFactory factory = new UpgradeSystemServiceFactory();

        public static int Main(string[] args)
        {
            TraceConfig.InitializeFromConfigStore();

            Console.CancelKeyPress += Console_CancelKeyPress;
            Trace.ConsoleWriteLine(TraceType, "Microsoft Azure Service Fabric Upgrade Service Host");
            Trace.ConsoleWriteLine(TraceType, "Copyright (c) Microsoft Corporation");
            Console.WriteLine();
            Trace.ConsoleWriteLine(TraceType, "Registering Upgrade service...");

            #if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
            #endif

            using (var runtime = FabricRuntime.Create())
            {
                factory.RegisterUpgradeSystemService(runtime);
                Trace.ConsoleWriteLine(TraceType, "Upgrade Service host ready. Press [Enter] to exit ...");
                Console.ReadLine();
            }

            return 0;
        }

        // TODO: Ctrl+C handler is not supported on CoreCLR Linux. Need to work to find alternate.
        private static void Console_CancelKeyPress(object sender, ConsoleCancelEventArgs e)
        {
            Trace.WriteInfo(TraceType, "Received {0}, exiting", e.SpecialKey);
        }
    }
}