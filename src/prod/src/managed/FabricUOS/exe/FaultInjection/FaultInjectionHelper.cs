// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal static class FaultInjectionHelper
    {
        public static FaultInjectionConfig TryGetFaultInjectionConfig(List<SettingsSectionDescription> fabricSettings)
        {
            if (fabricSettings == null)
            {
                return null;
            }

            SettingsSectionDescription section = fabricSettings.FirstOrDefault(sec => sec.Name == StringConstants.SectionName.UpgradeOrchestrationService);
            if (section == null)
            {
                return null;
            }

            SettingsParameterDescription faultStepParameter = section.Parameters.FirstOrDefault(p => p.Name == StringConstants.ParameterName.FaultStep);
            if (faultStepParameter == null)
            {
                return null;
            }

            int faultStep;
            UpgradeFlow faultFlow;

            if (!int.TryParse(faultStepParameter.Value, out faultStep) || faultStep < 0)
            {
                return null;
            }

            SettingsParameterDescription faultFlowParameter = section.Parameters.FirstOrDefault(p => p.Name == StringConstants.ParameterName.FaultFlow);
            if (faultFlowParameter == null)
            {
                return null;
            }

            if (!Enum.TryParse<UpgradeFlow>(faultFlowParameter.Value, out faultFlow))
            {
                return null;
            }

            return new FaultInjectionConfig(faultFlow, faultStep);
        }

        public static bool ShouldReportUnhealthy(ClusterUpgradeStateBase upgradeState, UpgradeFlow currentUpgradeFlow, FaultInjectionConfig faultInjectionConfig)
        {
            MultiphaseClusterUpgradeState multiPhaseUpgradeState = upgradeState as MultiphaseClusterUpgradeState;
            int currentStep = 0;
            if (multiPhaseUpgradeState != null)
            {
                currentStep = multiPhaseUpgradeState.CurrentListIndex;
            }

            return faultInjectionConfig.Equals(new FaultInjectionConfig(currentUpgradeFlow, currentStep));
        }

        public static UpgradeFlow GetUpgradeFlow(ClusterUpgradeStateBase upgradeState)
        {
            MultiphaseClusterUpgradeState multiPhaseUpgradeState = upgradeState as MultiphaseClusterUpgradeState;
            if (multiPhaseUpgradeState != null && multiPhaseUpgradeState.UpgradeUnsuccessful)
            {
                return UpgradeFlow.RollingBack;
            }
            else
            {
                return UpgradeFlow.RollingForward;
            }
        }
    }
}