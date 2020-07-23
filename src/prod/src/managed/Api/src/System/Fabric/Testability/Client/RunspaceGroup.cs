// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    using System;
    using System.Collections.Concurrent;
    using System.Fabric.Common;
    using System.Fabric.Testability.Common;
    using System.Management.Automation.Runspaces;
    using System.Threading;

    using ThrowIf = System.Fabric.Common.ThrowIf;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    // Represents an bounded Runspace pool where Runspaces can be 'initialized' before being taken.
    // The RunspacePool type itself cannot be used due to this initialization requirement.
    // If a request comes in when no runspace available the call is blocked waiting 
    // for a runspace to be returned
    internal class RunspaceGroup
    {
        private readonly ConcurrentStack<Runspace> runspaces;

        private const int GroupSizeMax = 5;
        private static readonly TimeSpan MaxRunspaceWaitTime = TimeSpan.FromMinutes(10);

        private static readonly RunspaceGroup SingletonInstance = new RunspaceGroup();

        private readonly object thisLock;
        private readonly AutoResetEvent runSpaceAvailableEvent;
        private int currentStackSize;

        private RunspaceGroup()
        {
            this.runspaces = new ConcurrentStack<Runspace>();
            this.thisLock = new object();
            this.currentStackSize = 0;
            this.runSpaceAvailableEvent = new AutoResetEvent(false);
        }

        public static RunspaceGroup Instance
        {
            get { return SingletonInstance; }
        }

        public RunspaceGroup.Token Take()
        {
            Runspace runspace = null;
            bool createNew = false;
            lock (this.thisLock)
            {
                if (this.runspaces.IsEmpty && this.currentStackSize < GroupSizeMax)
                {
                    createNew = true;
                    this.currentStackSize++;
                }
            }

            if (createNew)
            {
                runspace = this.CreateRunspace();
            }

            TimeoutHelper helper = new TimeoutHelper(MaxRunspaceWaitTime);
            while (runspace == null)
            {
                if (!this.runspaces.TryPop(out runspace))
                {
                    this.runSpaceAvailableEvent.WaitOne(TimeSpan.FromSeconds(5));
                }

                ThrowIf.IsTrue(helper.GetRemainingTime() == TimeSpan.Zero, "Could not get a RunSpace in {0} minutes. Check if something is stuck", MaxRunspaceWaitTime);
            }

            return new Token(this, runspace);
        }

        private void Return(Runspace runspace)
        {
            this.runspaces.Push(runspace);
            this.runSpaceAvailableEvent.Set();
        }

        private Runspace CreateRunspace()
        {
            return Disposable.Guard(
                () => RunspaceFactory.CreateRunspace(InitialSessionState.CreateDefault()),
                delegate(Runspace runspace)
                {
                    runspace.Open();

                    RunspaceGroup.RefreshModules(runspace);

                    TestabilityTrace.TraceSource.WriteNoise("RunspaceGroup", "PowerShellClient Runspace: Created");
                });
        }

        private static object refreshLock = new object();
        public static void RefreshModules(Runspace runspace)
        {
            var timer = new System.Diagnostics.Stopwatch();
            timer.Start();
            lock (refreshLock)
            {
                timer.Stop();
                if (timer.Elapsed > TimeSpan.FromSeconds(1))
                {
                    TestabilityTrace.TraceSource.WriteWarning("RunspaceGroup", "PS pipeline RefreshModules waited for lock {0}", timer.Elapsed.ToString());
                }

                // Workaround for getting it to force load winfab module
                // RDBug 3780938:Automation tests fail with CommandNotFoundException (Remove when fixed)
                Pipeline pipeline = runspace.CreatePipeline();
                pipeline.Commands.AddScript("Get-Module –ListAvailable -Refresh");
                var results = pipeline.Invoke();
            }
        }

        public struct Token : IDisposable
        {
            private readonly RunspaceGroup parent;
            private readonly Runspace runspace;

            internal Token(RunspaceGroup parent, Runspace runspace)
            {
                this.parent = parent;
                this.runspace = runspace;
            }

            public Runspace Runspace
            {
                get { return this.runspace; }
            }

            public void Dispose()
            {
                this.parent.Return(this.Runspace);
            }
        }
    }
}


#pragma warning restore 1591