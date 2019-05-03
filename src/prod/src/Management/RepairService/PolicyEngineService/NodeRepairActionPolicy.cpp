// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

const std::wstring NodeRepairActionPolicy::ProbationToFailingWaitDurationPostRepairInSecondsName = L"ProbationToFailingWaitDurationPostRepairInSeconds";
const std::wstring NodeRepairActionPolicy::ProbationToHealthyWaitDurationPostRepairInSecondsName = L"ProbationToHealthyWaitDurationPostRepairInSeconds";
const std::wstring NodeRepairActionPolicy::MaxOutstandingRepairTasksName = L"MaxOutstandingRepairTasks";
const std::wstring NodeRepairActionPolicy::PolicyActionTimeInSecondsName = L"PolicyActionTimeInSeconds";
const std::wstring NodeRepairActionPolicy::IsEnabledName = L"IsEnabled";

NodeRepairActionPolicy::NodeRepairActionPolicy(const std::wstring repairName, const std::wstring repairPolicySection) :
	probationToFailingAfterRepairDelay_(TimeSpan::FromMinutes(120)),
	probationToHealthyAfterRepairDelay_(TimeSpan::FromMinutes(15)),
	maxOutstandingRepairTasks_(30),
    policyActionTime_(TimeSpan::FromSeconds(7200)),
    repairName_(repairName),
    repairPolicyNameSection_(repairPolicySection),
	isEnabled_(true)
{
	Trace.WriteNoise(ComponentName, "NodeRepairPolicyConfiguration::NodeRepairActionPolicy::NodeRepairActionPolicy()");
}

void NodeRepairActionPolicy::TraceConfigurationValues()
{
	Trace.WriteNoise(ComponentName, "NodeRepairPolicyConfiguration::NodeRepairActionPolicy::TraceConfigurationValues()");
	Trace.WriteNoise(ComponentName, "Configuration values used:");
	Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", "repairName_", repairName_);
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ProbationToFailingWaitDurationPostRepairInSecondsName, probationToFailingAfterRepairDelay_.TotalSeconds());
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ProbationToHealthyWaitDurationPostRepairInSecondsName, probationToHealthyAfterRepairDelay_.TotalSeconds());
	Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", MaxOutstandingRepairTasksName, maxOutstandingRepairTasks_);
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", PolicyActionTimeInSecondsName, policyActionTime_);
	Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", IsEnabledName, isEnabled_);
}

ErrorCode NodeRepairActionPolicy::Load(ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr)
{
	bool success = true;

	try
    {
		Trace.WriteNoise(ComponentName, "Loading configuration from section '{0}'...", repairPolicyNameSection_);
		
		const FABRIC_CONFIGURATION_SECTION *fabricConfigurationSection;

        if (FAILED(fabricConfigurationPackageCPtr->GetSection(repairPolicyNameSection_.c_str(), &fabricConfigurationSection)) ||
            fabricConfigurationSection == nullptr)
        {
            Trace.WriteInfo(ComponentName, "Configuration section '{0}' not found. Default configuration values will be used.", repairPolicyNameSection_);
			TraceConfigurationValues();
            return ErrorCode(ErrorCodeValue::Success);
        }

        if (fabricConfigurationSection->Parameters == nullptr)
        {
            Trace.WriteError(ComponentName, "FABRIC_CONFIGURATION_SECTION.Parameters was null. Default configuration values will be used.");
			TraceConfigurationValues();
			return ErrorCode(ErrorCodeValue::InvalidConfiguration);
        }

        for (ULONG i = 0; i < fabricConfigurationSection->Parameters->Count; i++)
        {
			// Add to vector the name-value of this parameter
			const FABRIC_CONFIGURATION_PARAMETER& parameter = fabricConfigurationSection->Parameters->Items[i];
            if (parameter.Name == nullptr || parameter.Value == nullptr)
            {
                Trace.WriteError(ComponentName, "Configuration parameter or value was null. Some default configuration values will be used.");
                success = false;
                continue;
            }

			if (StringUtility::AreEqualCaseInsensitive(MaxOutstandingRepairTasksName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<uint, uint>(parameter.Name, parameter.Value, maxOutstandingRepairTasks_);
			}
			else if (StringUtility::AreEqualCaseInsensitive(IsEnabledName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<bool, bool>(parameter.Name, parameter.Value, isEnabled_);
			}
			else if (StringUtility::AreEqualCaseInsensitive(ProbationToFailingWaitDurationPostRepairInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, probationToFailingAfterRepairDelay_);
			}
			else if (StringUtility::AreEqualCaseInsensitive(ProbationToHealthyWaitDurationPostRepairInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, probationToHealthyAfterRepairDelay_);
			}
            else if (StringUtility::AreEqualCaseInsensitive(PolicyActionTimeInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, policyActionTime_);
			}
			else
			{
				Trace.WriteWarning(ComponentName, "Repair action policy with name '{0}' is not supported. Skipping it.",parameter.Name);
			}
		}

		TraceConfigurationValues();
	}
	catch (exception const& e)
    {
		Trace.WriteError(ComponentName, "Loading configuration section '{0}' failed. Default configuration values will be used. Exception:{1}", repairPolicyNameSection_, e.what());
        success = false;
    }

	return success ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::InvalidConfiguration);
}
