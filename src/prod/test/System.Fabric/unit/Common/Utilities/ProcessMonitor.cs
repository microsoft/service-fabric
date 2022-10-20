// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    public class ProcessMonitor : IDisposable
    {
        private const int monitorIntervalMs = 500;

        private readonly IEnumerable<string> processNames;
        private readonly Thread monitorThread;
        private List<Tuple<string, int, DateTime>> previousProcesses = new List<Tuple<string, int, DateTime>>();
        private bool isStopped = false;

        public ProcessMonitor(IEnumerable<string> processNames)
        {
            this.processNames = processNames.Select(e => e.ToLower());

            this.monitorThread = new Thread(MonitorProc);
            this.monitorThread.Start();
        }

        private void MonitorProc(object o)
        {
            while (!this.isStopped)
            {
                var processes = Process.GetProcesses();

                try
                {
                    var currentProcesses = new List<Tuple<string, int, DateTime>>();

                    foreach (var process in processes)
                    {
                        if (this.processNames.FirstOrDefault(e => e.ToLower() == process.ProcessName.ToLower()) == null)
                        {
                            continue;
                        }

                        DateTime startTime;
                        try
                        {
                            startTime = process.StartTime;
                        }
                        catch (InvalidOperationException)
                        {
                            // the process has exited
                            continue;
                        }

                        currentProcesses.Add(Tuple.Create(process.ProcessName.ToLower(), process.Id, startTime));
                    }

                    var newProcesses = Enumerable.Except(currentProcesses, this.previousProcesses);
                    foreach (var item in newProcesses)
                    {
                        LogHelper.Log("ProcessMonitor - launch detected for {0}. PID {1}. StartTime {2}", item.Item1, item.Item2, item.Item3);
                    }

                    var killedProcesses = Enumerable.Except(this.previousProcesses, currentProcesses);
                    foreach (var item in killedProcesses)
                    {
                        LogHelper.Log("ProcessMonitor - exit detected for {0}. PID {1}. StartTime {2}", item.Item1, item.Item2, item.Item3);
                    }

                    this.previousProcesses = currentProcesses;
                }
                finally
                {
                    foreach (var item in processes)
                    {
                        item.Dispose();
                    }
                }

                Thread.Sleep(ProcessMonitor.monitorIntervalMs);
            }
        }

        public void Dispose()
        {
            if (this.isStopped)
            {
                return;
            }

            this.isStopped = true;
            Assert.IsTrue(this.monitorThread.Join(TimeSpan.FromSeconds(120)));
        }
    }

}