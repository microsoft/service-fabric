// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Common;

    class WorkloadList
    {
        private IDictionary<string, bool> workloadScriptStatus;

        public WorkloadList(IEnumerable<string> workloadScriptBaseNames)
        {
            this.workloadScriptStatus = new Dictionary<string, bool>();
            if (workloadScriptBaseNames != null)
            {
                foreach (string workloadName in workloadScriptBaseNames)
                {
                    this.workloadScriptStatus.Add(workloadName, false);
                }
            }   
        }

        public IEnumerable<string> Workloads
        {
            get { return workloadScriptStatus.Keys; }
        }

        public bool GetWorkLoadState(string workloadName)
        {
            ReleaseAssert.AssertIfNot(this.workloadScriptStatus.ContainsKey(workloadName), "Workload name {0} is not known", workloadName);
            return this.workloadScriptStatus[workloadName];
        }

        public void FlipWorkloadState(string workloadName)
        {
            ReleaseAssert.AssertIfNot(this.workloadScriptStatus.ContainsKey(workloadName), "Workload name {0} is not known", workloadName);
            this.workloadScriptStatus[workloadName] = !this.workloadScriptStatus[workloadName];
        }
    }
}