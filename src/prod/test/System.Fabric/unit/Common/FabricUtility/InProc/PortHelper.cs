// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility.InProc
{
    using System;

    public class TestPortHelper
    {
        private const string EnvVarName_WinFabTestPorts = "WinFabTestPorts";
        private const int DefaultStartPort = 22000;
        private const int DefaultEndPort = 22979;
        private const int DefaultEndLeasePort = 22999;

        static int StartPort = 0;
        static int EndPort = 0;
        static int EndLeasePort = 0;
        static int CurrentIndex = 0;
        static int LeaseIndex = 0;

        static object portLock = new object();

        public static int GetNextLeasePort()
        {
            int nextPort = EndPort + 1 + LeaseIndex;
            if (nextPort == EndLeasePort)
            {
                LeaseIndex = 0;
            }
            else
            {
                ++LeaseIndex;
            }

            return nextPort;
        }

        public static void GetPorts(int numberOfPorts, out int startPort)
        {
            lock (portLock)
            {
                if (StartPort == 0 && EndPort == 0)
                {
                    string ports = Environment.GetEnvironmentVariable(EnvVarName_WinFabTestPorts);
                    if (ports != null)
                    {
                        string[] portsArray = ports.Split(',');
                        if (portsArray.Length == 2)
                        {
                            StartPort = int.Parse(portsArray[0]);
                            EndLeasePort = int.Parse(portsArray[1]);
                            if (EndLeasePort - StartPort < 500)
                            {
                                throw new InvalidOperationException("Not enough ports supplied to TestPortHelpers");
                            }

                            EndPort = EndLeasePort - 20;
                        }
                        else
                        {
                            StartPort = TestPortHelper.DefaultStartPort;
                            EndPort = TestPortHelper.DefaultEndPort;
                            EndLeasePort = TestPortHelper.DefaultEndLeasePort;
                        }
                    }
                    else
                    {
                        StartPort = TestPortHelper.DefaultStartPort;
                        EndPort = TestPortHelper.DefaultEndPort;
                        EndLeasePort = TestPortHelper.DefaultEndLeasePort;
                    }

                    CurrentIndex = StartPort;
                }

                if (numberOfPorts > (EndPort - StartPort) + 1)
                {
                    throw new InvalidOperationException("Not enough ports to assign");
                }

                if ((CurrentIndex + numberOfPorts - 1) <= EndPort)
                {
                    startPort = CurrentIndex;
                    CurrentIndex = CurrentIndex + numberOfPorts;
                }
                else
                {
                    startPort = StartPort;
                    CurrentIndex = StartPort;
                }
            }
        }
    }
}