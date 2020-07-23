// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Common.Tracing;
    using System.Threading.Tasks;

    public class Program
    {
        private static readonly TraceType TraceType = new TraceType("Host");

        public static int Main(string[] args)
        {
            // To enable breakForDebugger, add the <Arguments> tag in servicemanifest.xml as shown
            // <EntryPoint>
            //   <ExeHost>
            //     <Program>FabricIS.exe</Program>
            //     <Arguments>-debug</Arguments>
            //   </ExeHost>
            // </EntryPoint>

            bool breakForDebugger = false;
            bool appMode = false;

            for (int i = 0; i < args.Length; ++i)
            {
                string command = args[i].TrimStart('-', '/').ToLowerInvariant();

                switch (command)
                {
                    case "debug":
                        breakForDebugger = true;
                        break;

                    case "app":
                        appMode = true;
                        break;

                    default:
                        TraceType.ConsoleWriteLine("WARNING: Ignoring unknown argument: {0}", args[i]);
                        break;
                }
            }

            if (breakForDebugger)
            {
                TraceType.ConsoleWriteLine("Waiting for debugger. Press [Enter] to continue execution ...");
                Console.ReadLine();
            }

            TraceConfig.InitializeFromConfigStore();

            Console.CancelKeyPress += Console_CancelKeyPress;

            TraceType.ConsoleWriteLine(
                "Service Fabric Infrastructure Service starting up in {0} mode",
                appMode ? "user application" : "system service");

            if (appMode)
            {
                using (var runtime = FabricRuntime.Create())
                {
                    runtime.RegisterStatefulServiceFactory("InfrastructureServiceType", ServiceFactory.CreateAppServiceFactory());
                    int status = WaitForProgramClosure();
                    return status;
                }
            }
            else
            {
                ServiceFactory.CreateAndRegister();
                int status = WaitForProgramClosure();
                return status;
            }
        }

        /// <summary>
        /// Just waits till the program exits either by the user pressing the Enter key (not likely)
        /// or by a known failure from which we cannot continue further.
        /// </summary>
        /// <returns>The exit code that is actually returned from Main.</returns>
        internal static int WaitForProgramClosure()
        {
            Trace.ConsoleWriteLine(TraceType, "Infrastructure Service host ready. Press [Enter] to exit ...");

            Task<int> t1 = Task.Run(() =>
            {
                Console.ReadLine();
                Trace.ConsoleWriteLine(TraceType, "Exiting due to console input");
                return 0;
            });

            Task<int> t2 = Task.Run(() =>
            {
                ProcessCloser.ExitEvent.WaitOne();
                Trace.ConsoleWriteLine(TraceType, "Exiting due to signalled exit event");
                return 0;
            });

            Task<int> t = Task.WhenAny(t1, t2).GetAwaiter().GetResult();
            int status = t.GetAwaiter().GetResult();

            Trace.ConsoleWriteLine(TraceType, "Infrastructure Service host exiting with exit code: {0}", status);

            return status;
        }

        private static void Console_CancelKeyPress(object sender, ConsoleCancelEventArgs e)
        {
            Trace.WriteInfo(TraceType, "Received {0}, exiting", e.SpecialKey);
        }
    }
}