// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Collections.Generic;
    using IO;
    using Microsoft.Search.Autopilot;
    using Threading.Tasks;

    internal sealed class DMClientException : Exception
    {
        public DMClientException()
            : base()
        {
        }

        public DMClientException(string message)
            : base(message)
        {
        }

        public DMClientException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        public DMClientException(DMResponse response)
            : base($"DM returned response with status {response.Validate("response").Status}")
        {
            this.Response = response;
        }

        public DMResponse Response { get; private set; }
    }

    internal sealed class AutopilotDMClientWrapper
    {
        private static readonly TraceType traceType = new TraceType("APDMClientWrapper");
        private readonly IAutopilotDMClient dmClient;

        public AutopilotDMClientWrapper(IAutopilotDMClient dmClient)
        {
            this.dmClient = dmClient.Validate("dmClient");
        }

        public async Task<IList<MachineMaintenanceRecord>> GetDevicesPendingMaintenanceAsync()
        {
            var response = await dmClient.GetDevicesPendingMaintenanceAsync();
            VerifyResponse(response);

            using (var csvReader = new CsvReader(new StringReader(response.Response)))
            {
                List<MachineMaintenanceRecord> records = new List<MachineMaintenanceRecord>();

                while (csvReader.Read())
                {
                    string machineName = csvReader["MACHINENAME"];
                    string repairType = csvReader["ACTION"];
                    string delayInSec = csvReader["DELAYINSEC"];

                    int delay;
                    if (!int.TryParse(delayInSec, out delay))
                    {
                        traceType.WriteWarning(
                            "Unable to parse current delay: MACHINENAME='{0}' ACTION='{1}' DELAYINSEC='{2}'",
                            machineName,
                            repairType,
                            delayInSec);

                        delay = 59; // set this to some small value, so that we assume it is already approved
                    }

                    TimeSpan delaySpan = TimeSpan.FromSeconds(delay);

                    var recordId = new MaintenanceRecordId(machineName, repairType);
                    var record = MachineMaintenanceRecord.FromPendingRepair(recordId, delaySpan);
                    records.Add(record);
                }

                return records;
            }
        }

        public async Task<IList<MachineMaintenanceRecord>> GetRepairListAsync()
        {
            var response = await dmClient.GetRepairListAsync();
            VerifyResponse(response);

            using (var csvReader = new CsvReader(new StringReader(response.Response)))
            {
                List<MachineMaintenanceRecord> records = new List<MachineMaintenanceRecord>();

                while (csvReader.Read())
                {
                    string machineName = csvReader["MACHINENAME"];
                    string repairType = csvReader["REQUESTTYPE"];

                    var recordId = new MaintenanceRecordId(machineName, repairType);
                    var record = MachineMaintenanceRecord.FromActiveRepair(recordId);
                    records.Add(record);
                }

                return records;
            }
        }

        public async Task<IList<MachineInfo>> GetMachineInfoAsync()
        {
            var response = await dmClient.GetMachineInfoAsync();
            VerifyResponse(response);

            using (var csvReader = new CsvReader(new StringReader(response.Response)))
            {
                List<MachineInfo> records = new List<MachineInfo>();

                while (csvReader.Read())
                {
                    string machineName = csvReader["MACHINE"];
                    string repairType = csvReader["REPAIR"];
                    string repairActionState = csvReader["REPAIRACTIONSTATE"];
                    string machineStatus = csvReader["STATUS"];

                    var recordId = new MaintenanceRecordId(machineName, repairType);
                    var record = new MachineInfo(recordId, machineStatus, repairActionState);
                    records.Add(record);
                }

                return records;
            }
        }

        public async Task UpdateMaintenanceDelayAsync(string machineName, TimeSpan newDelay)
        {
            var response = await this.dmClient.UpdateMaintenanceDelayAsync(machineName, newDelay);
            VerifyResponse(response);
        }

        private void VerifyResponse(DMResponse response)
        {
            if (response.Status != DMServerResponseCode.Ok)
            {
                throw new DMClientException(response);
            }
        }
    }
}