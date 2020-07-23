// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using System.Threading.Tasks;
    using Microsoft.Search.Autopilot;

    // TODO IDisposable?
    internal interface IAutopilotDMClient
    {
        Task<DMResponse> GetDevicesPendingMaintenanceAsync();
        Task<DMResponse> GetRepairListAsync();
        Task<DMResponse> GetMachineInfoAsync();
        Task<DMResponse> UpdateMaintenanceDelayAsync(string machineName, TimeSpan newDelay);
    }
}