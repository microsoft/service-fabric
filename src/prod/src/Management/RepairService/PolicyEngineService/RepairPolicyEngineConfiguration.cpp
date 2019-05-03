// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

const std::wstring RepairPolicyEngineConfiguration::HealthCheckIntervalInSecondsName = L"HealthCheckIntervalInSeconds";
const std::wstring RepairPolicyEngineConfiguration::ActionSchedulingIntervalInSecondsName = L"ActionSchedulingIntervalInSeconds";
const std::wstring RepairPolicyEngineConfiguration::MaxNumberOfRepairsToRememberName = L"MaxNumberOfRepairsToRemember";
const std::wstring RepairPolicyEngineConfiguration::ExpectedHealthRefreshIntervalInSecondsName = L"ExpectedHealthRefreshIntervalInSeconds";
const std::wstring RepairPolicyEngineConfiguration::WaitForHealthUpdatesAfterBecomingPrimaryInSecondsName = L"WaitForHealthUpdatesAfterBecomingPrimaryInSeconds";
const std::wstring RepairPolicyEngineConfiguration::WindowsFabricQueryTimeoutInSecondsName = L"WaitForHealthUpdatesAfterBecomingPrimaryInSeconds";

RepairPolicyEngineConfiguration::RepairPolicyEngineConfiguration() : 
    WindowsFabricQueryTimeoutInterval(TimeSpan::FromSeconds(30)),
    HealthCheckInterval(TimeSpan::FromSeconds(30)),
    RepairActionSchedulingInterval(TimeSpan::FromMinutes(5)),
    WaitForHealthStatesAfterBecomingPrimaryInterval(TimeSpan::FromSeconds(70)),
    MaxNumberOfRepairsToRemember(RepairPolicyEngineConfiguration::DefaultMaxNumberOfRepairsToRemember),
    ExpectedHealthRefreshInterval(TimeSpan::FromSeconds(RepairPolicyEngineConfiguration::DefaultExpectedHealthRefreshIntervalInSeconds))
{
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineConfiguration::RepairPolicyEngineConfiguration()");
}

void RepairPolicyEngineConfiguration::TraceConfigurationValues()
{
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineConfiguration::TraceConfigurationValues()");
    Trace.WriteNoise(ComponentName, "Configuration values used:");
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", HealthCheckIntervalInSecondsName, HealthCheckInterval.TotalSeconds());
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", WaitForHealthUpdatesAfterBecomingPrimaryInSecondsName, WaitForHealthStatesAfterBecomingPrimaryInterval.TotalSeconds());
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", WindowsFabricQueryTimeoutInSecondsName, WindowsFabricQueryTimeoutInterval.TotalSeconds());
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ActionSchedulingIntervalInSecondsName, RepairActionSchedulingInterval.TotalSeconds() );
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", MaxNumberOfRepairsToRememberName, MaxNumberOfRepairsToRemember);
    Trace.WriteNoise(ComponentName, "Parameter:{0} Value:'{1}'", ExpectedHealthRefreshIntervalInSecondsName, ExpectedHealthRefreshInterval.TotalSeconds());
}

ErrorCode RepairPolicyEngineConfiguration::Load(ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr)
{
    bool success = true;
    const std::wstring configurationPackageName = L"RepairPolicyEngineService.Config";

    try
    {
        Trace.WriteNoise(ComponentName, "Loading configuration package '{0}'...", configurationPackageName);

        ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr;
        if (FAILED(codePackageActivationContextCPtr->GetConfigurationPackage(configurationPackageName.c_str(), fabricConfigurationPackageCPtr.InitializationAddress())) ||
            fabricConfigurationPackageCPtr.GetRawPointer() == nullptr)
        {
            Trace.WriteError(ComponentName, "Configuration package '{0}' not found. Default configuration values will be used.", configurationPackageName);
            TraceConfigurationValues();
			nodeRepairPolicyConfiguration_.TraceConfigurationValues();
            return ErrorCode(ErrorCodeValue::ConfigurationPackageNotFound);
        }

		success &= LoadServiceSettings(fabricConfigurationPackageCPtr).IsSuccess();
		success &= nodeRepairPolicyConfiguration_.Load(fabricConfigurationPackageCPtr).IsSuccess();
		success &= nodeRepairPolicyConfiguration_.LoadActionList(fabricConfigurationPackageCPtr).IsSuccess();
    }
    catch (exception const& e)
    {
        Trace.WriteError(ComponentName, "Loading configuration failed. Default configuration values will be used. Exception:{0}", e.what());
        success = false;
    }

    return success ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::InvalidConfiguration);
}

ErrorCode RepairPolicyEngineConfiguration::LoadServiceSettings(ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr)
{
	bool success = true;
    const std::wstring configurationSectionName = L"RepairPolicyEngine";

	try
    {
        Trace.WriteNoise(ComponentName, "Loading Policy Engine configuration from section '{0}'...", configurationSectionName);

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
                Trace.WriteError(ComponentName, "Configuration parameter or value was null. Some default configuration values will be used.", configurationSectionName);
                success = false;
                continue;
            }

			if (StringUtility::AreEqualCaseInsensitive(HealthCheckIntervalInSecondsName, parameter.Name))
            { 
				success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, HealthCheckInterval);
            }
            else if(StringUtility::AreEqualCaseInsensitive(ExpectedHealthRefreshIntervalInSecondsName, parameter.Name))
            {
                success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, ExpectedHealthRefreshInterval);
            }
            else if(StringUtility::AreEqualCaseInsensitive(WaitForHealthUpdatesAfterBecomingPrimaryInSecondsName, parameter.Name))
            {
                success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, WaitForHealthStatesAfterBecomingPrimaryInterval);
            }
            else if(StringUtility::AreEqualCaseInsensitive(WindowsFabricQueryTimeoutInSecondsName, parameter.Name))
            {
                success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, WindowsFabricQueryTimeoutInterval);
            }
            else if(StringUtility::AreEqualCaseInsensitive(ActionSchedulingIntervalInSecondsName, parameter.Name))
            {
                success &= RepairPolicyEngineConfigurationHelper::TryParse<double, TimeSpan>(parameter.Name, parameter.Value, RepairActionSchedulingInterval);
            }
            else if(StringUtility::AreEqualCaseInsensitive(MaxNumberOfRepairsToRememberName, parameter.Name))
            {
                success &= RepairPolicyEngineConfigurationHelper::TryParse<uint, uint>(parameter.Name, parameter.Value, MaxNumberOfRepairsToRemember);
				if (MaxNumberOfRepairsToRemember < 1)
				{
					Trace.WriteError(ComponentName, "Configuration parameter '{0}' has value < 1. Default value '{1}' will be used.", parameter.Name, RepairPolicyEngineConfiguration::DefaultMaxNumberOfRepairsToRemember);
					MaxNumberOfRepairsToRemember = RepairPolicyEngineConfiguration::DefaultMaxNumberOfRepairsToRemember;
					success = false;
				}
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
