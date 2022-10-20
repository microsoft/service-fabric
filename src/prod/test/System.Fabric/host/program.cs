// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Host
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Linq;
    using System.Diagnostics;
    using System.Threading;
    using System.IO;
    using System.IO.Pipes;
    using System.Threading.Tasks;

    public class Program
    {
        public static int Main(string[] args)
        {
            Program.WaitForDebugger(args);

            if (args.Length == 0)
            {
                return -1;
            }

            bool isAdHoc = args[0] == "adhoc";
            args = args.Skip(1).ToArray();

            if (isAdHoc)
            {
                AdHocHost.Run(args);
            }
            else
            {
                ManagedHost.Setup(args);

                ManagedHost.Run(args);
            }

            return 0;
        }

        private static void WaitForDebugger(string[] args)
        {
            if (!args.Contains("-debug"))
            {
                return;
            }

            DateTime start = DateTime.Now;
            while (DateTime.Now - start < TimeSpan.FromSeconds(30))
            {
                if (Debugger.IsAttached)
                {
                    Debugger.Break();
                    return;
                }

                Thread.Sleep(1000);
            }
        }


    }
}