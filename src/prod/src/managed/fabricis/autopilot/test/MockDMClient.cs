// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot.Test
{
    using System.Threading.Tasks;
    using Microsoft.Search.Autopilot;

    internal sealed class MockDMClient : IAutopilotDMClient
    {
        public Func<DMResponse> GetDevicesPendingMaintenanceHandler = null;
        public Func<DMResponse> GetRepairListHandler = null;
        public Func<DMResponse> GetMachineInfoHandler = null;
        public Func<string, TimeSpan, DMResponse> UpdateMaintenanceDelayHandler = null;

        public Task<DMResponse> GetDevicesPendingMaintenanceAsync()
        {
            return Task.FromResult(this.GetDevicesPendingMaintenanceHandler());
        }

        public Task<DMResponse> GetRepairListAsync()
        {
            return Task.FromResult(this.GetRepairListHandler());
        }

        public Task<DMResponse> GetMachineInfoAsync()
        {
            return Task.FromResult(this.GetMachineInfoHandler());
        }

        public Task<DMResponse> UpdateMaintenanceDelayAsync(string machineName, TimeSpan newDelay)
        {
            return Task.FromResult(this.UpdateMaintenanceDelayHandler(machineName, newDelay));
        }
    }
}