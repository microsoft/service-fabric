// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using System.Threading.Tasks;
    using Diagnostics;
    using Microsoft.Search.Autopilot;

    internal sealed class AutopilotDMClient : IAutopilotDMClient
    {
        private static readonly TraceType traceType = new TraceType("APDMClient");
        private readonly ManagedDMClient dmClient;

        public AutopilotDMClient()
        {
            dmClient = new ManagedDMClient();
        }

        public Task<DMResponse> GetDevicesPendingMaintenanceAsync()
        {
            return SendRequestAsync("GetDevicesPendingMaintenance", readOnly: true);
        }

        public Task<DMResponse> GetRepairListAsync()
        {
            return SendRequestAsync("GetRepairList", readOnly: true);
        }

        public Task<DMResponse> GetMachineInfoAsync()
        {
            return SendRequestAsync("GetMachineInfo", readOnly: true, logResponseContent: false);
        }

        public Task<DMResponse> UpdateMaintenanceDelayAsync(string machineName, TimeSpan newDelay)
        {
            string command = "UpdateMaintenanceDelay -s {0} -timeDelayInSec {1}".ToString(
                machineName,
                newDelay.TotalSeconds);

            return SendRequestAsync(command, false);
        }

        private async Task<DMResponse> SendRequestAsync(string command, bool readOnly, bool logResponseContent = true)
        {
            traceType.WriteInfo("Sending {0} command: {1}", readOnly ? "read" : "write", command);

            var watch = Stopwatch.StartNew();

            DMResponse response = await dmClient.SendRequestAsync(command, null, readOnly);

            string message = string.Format(
                "DM response to command '{0}': HTTP {1}, status {2}, elapsed {3} msec\n{4}",
                command,
                response.HttpResponseCode,
                response.Status,
                watch.ElapsedMilliseconds,
                logResponseContent ? response.Response : $"(response content omitted; length = {response.Response?.Length})");

            if (response.Status == DMServerResponseCode.Ok)
            {
                traceType.WriteInfo("{0}", message);
            }
            else
            {
                traceType.WriteWarning("{0}", message);
            }

            return response;
        }
    }

}