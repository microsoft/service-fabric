// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Globalization;
    using RD.Fabric.PolicyAgent;

    /// <summary>
    /// Helper class that provides repair actions from strings that match a <see cref="RepairActionTypeEnum"/>.
    /// </summary>
    internal sealed class RepairActionProvider
    {
        /// <summary>
        /// The abstract reboot maintenance action string that gets mapped to either HostReboot or Reboot (VM reboot).
        /// </summary>
        private const string AbstractRebootMaintenanceAction = "Reboot";

        /// <summary>
        /// The abstract re-image maintenance action string that gets mapped to either HostRepaveData or RepaveData.
        /// </summary>
        private const string AbstractFullReimageMaintenanceAction = "FullReimage";

        /// <summary>
        /// The format for passing in a command for a system action. E.g. System.Azure.Heal, System.Azure.HostReboot etc.
        /// </summary>
        private const string SystemActionFormat = "System.{0}";

        /// <summary>
        /// Indicates that the action is towards Azure. E.g. MR related actions etc.
        /// </summary>
        private const string ActionNamespaceAzure = "Azure";

        private readonly Dictionary<string, RepairActionTypeEnum> repairActionMap =
            new Dictionary<string, RepairActionTypeEnum>();

        private readonly IConfigSection configSection;

        public RepairActionProvider(IConfigSection configSection)
        {
            this.configSection = configSection.Validate("configSection");

            BuildRepairActionMap();
        }

        /// <summary>
        /// Gets the repair action from a string matching an <see cref="RepairActionTypeEnum"/>.        
        /// </summary>
        /// <param name="action">The action.</param>
        /// <returns>
        /// A <see cref="RepairActionTypeEnum"/> object or null if the action didn't match any of the allowed actions.
        /// </returns>
        public RepairActionTypeEnum? GetRepairAction(string action)
        {
            action.Validate("action");

            RepairActionTypeEnum repairAction;

            if (this.repairActionMap.TryGetValue(action, out repairAction))
            {
                return repairAction;
            }

            return null;
        }

        private void BuildRepairActionMap()
        {
            // if users specify a generic command. E.g. Reboot, then apply the config setting policy and determine
            // whether to apply it on the VM or on the Host
            RepairActionTypeEnum reboot = configSection.ReadConfigValue(Constants.ConfigKeys.RebootMaintenanceAction, RepairActionTypeEnum.Reboot);
            RepairActionTypeEnum repaveData = configSection.ReadConfigValue(Constants.ConfigKeys.FullReimageMaintenanceAction, RepairActionTypeEnum.RepaveData);

            AddRepairAction(reboot, AbstractRebootMaintenanceAction);
            AddRepairAction(repaveData, AbstractFullReimageMaintenanceAction);

            // There is no Host/VM target for ReimageOS currently. It is just applicable to VMs
            AddRepairAction(RepairActionTypeEnum.ReimageOS);

            // if users specify the command explicitly. E.g. System.Azure.Reboot, then apply the foll. irrespective of the 
            // *MaintenanceAction config setting
            // Note: Unlike ReimageOS above, we haven't added a generic 'Heal' action. Instead users have to specify this explicitly
            AddRepairActionWithNamespace(RepairActionTypeEnum.Reboot);
            AddRepairActionWithNamespace(RepairActionTypeEnum.ReimageOS);
            AddRepairActionWithNamespace(RepairActionTypeEnum.RepaveData);
            AddRepairActionWithNamespace(RepairActionTypeEnum.HostReboot);
            AddRepairActionWithNamespace(RepairActionTypeEnum.HostRepaveData);
            AddRepairActionWithNamespace(RepairActionTypeEnum.Heal);
        }

        /// <summary>
        /// This method is used to create an explicit maintenance action. E.g. System.Azure.HostReboot.
        /// </summary>
        /// <param name="action">The action.</param>
        private void AddRepairActionWithNamespace(RepairActionTypeEnum action)
        {
            string command = string.Format(CultureInfo.InvariantCulture, "{0}.{1}", ActionNamespaceAzure, action);

            AddRepairAction(action, command);
        }

        /// <summary>
        /// This method adds a generic maintenance action to a map of known actions. 
        /// This allows users to specify a simpler maintenance action string e.g. 'Reboot' which then gets mapped to either
        /// System.Azure.Reboot or System.Azure.HostReboot based on configuration settings.
        /// </summary>
        /// <param name="action">The action.</param>
        /// <param name="command">The command.</param>
        private void AddRepairAction(RepairActionTypeEnum action, string command = null)
        {
            string key = string.Format(CultureInfo.InvariantCulture, SystemActionFormat, command ?? action.ToString());
            repairActionMap.Add(key, action);

            Constants.TraceType.WriteInfo("Enabled execution of repair command: {0} with action: {1}", key, action);
        }
    }
}