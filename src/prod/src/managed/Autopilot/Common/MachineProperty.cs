// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using Microsoft.Search.Autopilot;
    using Microsoft.Search.Autopilot.DMCommands;

    internal static class MachineProperty
    {
        private static bool useTestMode = false;
        private static ManagedDMClient dmClient = null;
        private static DMCommands dmCommands = null;

        public static void Initialize(bool testMode = false)
        {
            useTestMode = testMode;

            if (!useTestMode)
            {
                dmClient = new ManagedDMClient();
                dmCommands = new DMCommands(dmClient);
            }
        }

        public static bool ReportOK(string property, string value)
        {
            return ReportMachineProperty(MachinePropertyLevel.Ok, property, value);
        }

        public static bool ReportWarning(string property, string value)
        {
            return ReportMachineProperty(MachinePropertyLevel.Warning, property, value);
        }

        public static bool ReportError(string property, string value)
        {
            return ReportMachineProperty(MachinePropertyLevel.Error, property, value);
        }

        public static bool ReportMachineProperty(MachinePropertyLevel propertyLevel, string property, string value)
        {
            return ReportMachineProperty(useTestMode ? Environment.MachineName : APRuntime.MachineName, propertyLevel, property, value);
        }

        public static bool ReportMachineProperty(string machineName, MachinePropertyLevel propertyLevel, string property, string value)
        {
            if (useTestMode)
            {
                TextLogger.LogInfo("Test mode machine property report => machine : {0}, level : {1}, property : {2}, value : {3}.", machineName, propertyLevel, property, value);

                return true;
            }
            else
            {
                try
                {
                    UpdateMachinePropertyRequest request = new UpdateMachinePropertyRequest
                    {
                        StatusFlag = "always",
                        Environment = APRuntime.EnvironmentName,
                    };

                    request.PropertyList.Add(
                        new DMMachineProperty
                        {
                            MachineName = machineName,
                            Level = propertyLevel,
                            Property = property,
                            Value = value
                        });

                    DMServerResponseCode responseCode = dmCommands.UpdateMachineProperty(request);
                    if (responseCode != DMServerResponseCode.Ok)
                    {
                        TextLogger.LogError("Failed to report {0} level machine property {1} against machine {2} with value {3}. Response code : {4}.", propertyLevel, property, machineName, value, responseCode);

                        return false;
                    }

                    return true;
                }
                catch (Exception e)
                {
                    TextLogger.LogError("Failed to report {0} level machine property {1} against machine {2} with value {3} : {4}.", propertyLevel, property, machineName, value, e);

                    return false;
                }
            }
        }
    }
}