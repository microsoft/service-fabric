// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Threading;

    public class ServiceFabricAutopilotAgentHost
    {
        private readonly ServiceFabricAutopilotAgent serviceFabricAutopilotAgent;
        private readonly EventWaitHandle exitEvent;

        private int exitCode;

        public ServiceFabricAutopilotAgentHost()
        {
            this.serviceFabricAutopilotAgent = new ServiceFabricAutopilotAgent(this.ExitProcess);
            this.exitEvent = new ManualResetEvent(false);
            this.exitCode = 0;
        }

        public static int Main(string[] args)
        {
            if (args.Length >= 2 || (args.Length == 1 && string.Compare(args[0], "/TestMode", StringComparison.OrdinalIgnoreCase) != 0))
            {
                Console.WriteLine("Usage: ServiceFabricAutopilotAgent [/TestMode]");

                return 1;
            }

            /* 
             * As an AP application service, the service host should clean up and exit after receiving Ctrl + Break. 
             * ServiceFabricAutopilotAgent would subscribe to the event and signal service host for exit.
            */
            ServiceFabricAutopilotAgentHost host = new ServiceFabricAutopilotAgentHost();

            host.StartAgent(args.Length == 1);

            host.WaitForExit();

            return host.exitCode;
        }

        private void StartAgent(bool useTestMode = false)
        {
            this.serviceFabricAutopilotAgent.Start(useTestMode);
        }

        private void ExitProcess(bool succeeded)
        {
            this.exitCode = succeeded ? 0 : 1;

            this.exitEvent.Set();
        }

        private void WaitForExit()
        {
            this.exitEvent.WaitOne();
        }
    }
}