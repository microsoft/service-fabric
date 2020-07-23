// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    internal class MachineHealthContainer
    {
        private ConcurrentDictionary<string, bool> machinesHealthDic;

        private int totalMachineCount;

        public MachineHealthContainer(int maxPercentFailedNodes)
        {
            this.MaxPercentFailedNodes = maxPercentFailedNodes;
        }

        public MachineHealthContainer(
          IEnumerable<string> machineNames,
          int maxPercentUnhealthyNodes = 0)
        {
            this.machinesHealthDic = new ConcurrentDictionary<string, bool>(machineNames.ToDictionary(machine => machine, machine => true));
            this.totalMachineCount = machineNames.Count();
            this.MaxPercentFailedNodes = maxPercentUnhealthyNodes;
        }

        internal int MaxPercentFailedNodes
        {
            get;
            set;
        }

        internal IEnumerable<string> GetHealthyMachineNames()
        {
           return this.machinesHealthDic.Where(machine => machine.Value).Select(machine => machine.Key);
        }

        internal IEnumerable<string> GetAllMachineNames()
        {
            return this.machinesHealthDic.Select(machine => machine.Key);
        }

        internal void SetHealthyMachines(IEnumerable<string> machineNames)
        {
            this.machinesHealthDic = new ConcurrentDictionary<string, bool>(machineNames.ToDictionary(machine => machine, machine => true));
            this.totalMachineCount = machineNames.Count();
        }

        internal void MarkMachineAsUnhealthy(string machineName)
        {
            this.machinesHealthDic[machineName] = false;
        }

        internal bool IsMachineHealthy(string machineName)
        {
            return this.machinesHealthDic[machineName];
        }

        internal bool EnoughHealthyMachines()
        {
            int currentHealthyCount = this.machinesHealthDic.Where(machine => machine.Value).Count();
            if (this.MaxPercentFailedNodes == 0)
            {
                return currentHealthyCount == this.totalMachineCount;
            }

            int unhealthyCount = this.totalMachineCount - currentHealthyCount;
            int unhealthyPercentage = 100 * unhealthyCount / this.totalMachineCount;

            if (unhealthyPercentage > this.MaxPercentFailedNodes)
            {
                SFDeployerTrace.WriteError(StringResources.Error_NotEnoughHealthyMachines, unhealthyPercentage, unhealthyCount, this.totalMachineCount, this.MaxPercentFailedNodes);
                return false;
            }
            else if (unhealthyPercentage < 100)
            {
                SFDeployerTrace.WriteWarning(StringResources.Warning_SomeMachinesFailed, currentHealthyCount);
            }

            return true;
        }
    }
}