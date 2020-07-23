// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace RepairPolicyEngine
{

const std::wstring NodeRepairPolicyConfiguration::ProbationToHealthyWaitDurationInSecondsName = L"ProbationToHealthyWaitDurationInSeconds";
const std::wstring NodeRepairPolicyConfiguration::ProbationToFailingWaitDurationInSecondsName = L"ProbationToFailingWaitDurationInSeconds";
const std::wstring NodeRepairPolicyConfiguration::MinimumHealthyDurationInSecondsName = L"MinimumHealthyDurationInSeconds";
const std::wstring NodeRepairPolicyConfiguration::IsEnabledName = L"IsEnabled";

NodeRepairPolicyConfiguration::NodeRepairPolicyConfiguration() :
	probationToHealthyDelay_(TimeSpan::FromMinutes(15)),
    probationToFailingDelay_(TimeSpan::FromMinutes(30)),
    minimumHealthyDuration_(TimeSpan::FromMinutes(5)),
	isEnabled_(true)
{
	Trace.WriteNoise(ComponentName, "NodeRepairPolicyConfiguration::NodeRepairPolicyConfiguration()");
}

void NodeRepairPolicyConfiguration::TraceConfigurationValues()
{
	Trace.WriteNoise(ComponentName, "NodeRepairPolicyConfiguration::TraceConfigurationValues()");
	Trace.WriteNoise(ComponentName, "Configuration values used:");
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ProbationToHealthyWaitDurationInSecondsName, probationToHealthyDelay_.TotalSeconds());
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ProbationToFailingWaitDurationInSecondsName, probationToFailingDelay_.TotalSeconds());
	Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", MinimumHealthyDurationInSecondsName, minimumHealthyDuration_.TotalSeconds());
	Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", IsEnabledName, isEnabled_);
}

ErrorCode NodeRepairPolicyConfiguration::Load(ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr)
{
	bool success = true;
	const std::wstring configurationSectionName = L"NodeRepairPolicy";

	try
    {
        Trace.WriteNoise(ComponentName, "Loading Node Repair Policy configuration from section '{0}'...", configurationSectionName);
		
		const FABRIC_CONFIGURATION_SECTION *fabricConfigurationSection = nullptr;

        if (FAILED(fabricConfigurationPackageCPtr->GetSection(configurationSectionName.c_str(), &fabricConfigurationSection)) ||
            fabricConfigurationSection == nullptr)
        {
            Trace.WriteInfo(ComponentName, "Configuration section '{0}' not found. Default configuration values will be used.", configurationSectionName);
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
			const FABRIC_CONFIGURATION_PARAMETER& parameter = fabricConfigurationSection->Parameters->Items[i];
            if (parameter.Name == nullptr || parameter.Value == nullptr)
            {
                Trace.WriteError(ComponentName, "Configuration parameter or value was null. Some default configuration values will be used.");
                success = false;
                continue;
            }

			if(StringUtility::AreEqualCaseInsensitive(ProbationToHealthyWaitDurationInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, probationToHealthyDelay_);
			}
			else if(StringUtility::AreEqualCaseInsensitive(ProbationToFailingWaitDurationInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, probationToFailingDelay_);
			}
			else if(StringUtility::AreEqualCaseInsensitive(MinimumHealthyDurationInSecondsName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, minimumHealthyDuration_);
			}
			else if (StringUtility::AreEqualCaseInsensitive(IsEnabledName, parameter.Name))
			{
				success &= RepairPolicyEngineConfigurationHelper::TryParse<bool, bool>(parameter.Name, parameter.Value, isEnabled_);
			}
			else
			{
				Trace.WriteWarning(ComponentName, "Configuration parameter '{0}' is not supported. Skipping it.",parameter.Name);
			}
		}
	}
	catch (exception const& e)
    {
        Trace.WriteError(ComponentName, "Loading configuration failed. Default configuration values will be used. Exception:{0}", e.what());
        success = false;
    }

	TraceConfigurationValues();
	return success ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::InvalidConfiguration);
}


ErrorCode NodeRepairPolicyConfiguration::LoadActionList(ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr)
{
	bool success = true;
	const std::wstring repairActionListSectionName = L"NodeRepairActionList";

	try
    {
		Trace.WriteNoise(ComponentName, "Loading configuration from section '{0}'...", repairActionListSectionName);
		
		const FABRIC_CONFIGURATION_SECTION *fabricConfigurationSection = nullptr;

        if (FAILED(fabricConfigurationPackageCPtr->GetSection(repairActionListSectionName.c_str(), &fabricConfigurationSection)) ||
            fabricConfigurationSection == nullptr)
        {
            Trace.WriteInfo(ComponentName, "Configuration section '{0}' not found. Default configuration values will be used.", repairActionListSectionName);
            return ErrorCode(ErrorCodeValue::Success);
        }

        if (fabricConfigurationSection->Parameters == nullptr)
        {
            Trace.WriteError(ComponentName, "FABRIC_CONFIGURATION_SECTION.Parameters was null. Default configuration values will be used.");
            return ErrorCode(ErrorCodeValue::InvalidConfiguration);
        }

        for (ULONG i = 0; i < fabricConfigurationSection->Parameters->Count; i++)
        {
			const FABRIC_CONFIGURATION_PARAMETER& parameter = fabricConfigurationSection->Parameters->Items[i];
            if (parameter.Name == nullptr || parameter.Value == nullptr)
            {
                Trace.WriteError(ComponentName, "Configuration parameter or value was null. Some default configuration values will be used.");
                success = false;
                continue;
            }

			std::wstring repairType = std::wstring(parameter.Name);
            auto policy = std::find_if(NodeRepairActionMap_.begin(), NodeRepairActionMap_.end(), [repairType](NodeRepairActionPolicy p) 
                { return StringUtility::AreEqualCaseInsensitive(p.get_RepairName(), repairType); });
			
            if (policy != NodeRepairActionMap_.end())
			{
				Trace.WriteWarning(ComponentName, "Repair action with name '{0}' already exists. Skipping it.",parameter.Name);
				success = false;
				continue;
			}
            auto actionPolicy = NodeRepairActionPolicy(parameter.Name, parameter.Value);
            bool isSuccess = actionPolicy.Load(fabricConfigurationPackageCPtr).IsSuccess();
            if (isSuccess)
            {
                NodeRepairActionMap_.push_back(actionPolicy);
            }
            success &= isSuccess;
		}
	}
	catch (exception const& e)
    {
		Trace.WriteError(ComponentName, "Loading configuration section '{0}' failed. Default configuration values will be used. Exception:{1}", repairActionListSectionName, e.what());
        success = false;
    }

	return success ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::InvalidConfiguration);
}

}
