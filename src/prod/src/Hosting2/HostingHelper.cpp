// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral TraceType_HostingHelper = "HostingHelper";


ErrorCode HostingHelper::GenerateAllConfigSettings(
    wstring const& applicationId,
    wstring const& servicePackageName,
    Management::ImageModel::RunLayoutSpecification const& runLayout,
    vector<DigestedConfigPackageDescription> const& digestedConfigPackages,
    _Out_ map<wstring, ConfigSettings> & configSettingsMapResult)
{
    for (auto digestedConfigPackage : digestedConfigPackages)
    {
        wstring configPackageFolder = runLayout.GetConfigPackageFolder(
            applicationId,
            servicePackageName,
            digestedConfigPackage.ConfigPackage.Name,
            digestedConfigPackage.ConfigPackage.Version,
            digestedConfigPackage.IsShared);

        wstring settingsFile = runLayout.GetSettingsFile(configPackageFolder);

		if (!File::Exists(settingsFile))
		{
			TraceError(
				TraceTaskCodes::Hosting,
				TraceType_HostingHelper,
				"Couldn't find file Settings.xml {0}",
				settingsFile);
			return ErrorCodeValue::FileNotFound;
		}

        ConfigSettings configSettings;
        auto error = Parser::ParseConfigSettings(settingsFile, configSettings);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (!digestedConfigPackage.ConfigOverrides.ConfigPackageName.empty())
        {
            ASSERT_IFNOT(
                StringUtility::AreEqualCaseInsensitive(digestedConfigPackage.ConfigPackage.Name, digestedConfigPackage.ConfigOverrides.ConfigPackageName),
                "Error in DigestedConfigPackage element. The ConfigPackage Name '{0}' does not match the ConfigOverrides Name '{1}'.",
                digestedConfigPackage.ConfigPackage.Name,
                digestedConfigPackage.ConfigOverrides.ConfigPackageName);

            configSettings.ApplyOverrides(digestedConfigPackage.ConfigOverrides.SettingsOverride);
        }

        configSettingsMapResult[digestedConfigPackage.ConfigPackage.Name] = move(configSettings);

    }

    return ErrorCodeValue::Success;
}

ErrorCode HostingHelper::DecryptAndGetSecretStoreRef(
    _Inout_ map<wstring, ConfigSettings> & configSettingsMapResult,
    _Out_ vector<wstring> & secretStoreRef)
{
    for (auto & sections : configSettingsMapResult)
    {
        for (auto & section : sections.second.Sections)
        {
            if (section.second.Parameters.size() > 0)
            {
                for (auto & parameter : section.second.Parameters)
                {
                    wstring value;

                    if (StringUtility::AreEqualCaseInsensitive(parameter.second.Type, Constants::Encrypted))
                    {
                        auto error = ContainerConfigHelper::DecryptValue(parameter.second.Value, value);
                        if (!error.IsSuccess())
                        {
                            return error;
                        }

                        parameter.second.Value = value;
                        continue;
                    }
                    else if (StringUtility::AreEqualCaseInsensitive(parameter.second.Type, Constants::SecretsStoreRef))
                    {
                        secretStoreRef.push_back(parameter.second.Value);
                    }
                }
            }
        }
    }

    return ErrorCodeValue::Success;
}


void HostingHelper::ReplaceSecretStoreRef(_Inout_ map<wstring, ConfigSettings> & configSettingsMapResult, map<wstring, wstring> const& secrteStoreRef)
{
    if (secrteStoreRef.empty())
    {
        return;
    }

    for (auto & sections : configSettingsMapResult)
    {
        for (auto & section : sections.second.Sections)
        {
            if (section.second.Parameters.size() > 0)
            {
                for (auto & parameter : section.second.Parameters)
                {
                    if (StringUtility::AreEqualCaseInsensitive(parameter.second.Type, Constants::SecretsStoreRef))
                    {
                        auto it = secrteStoreRef.find(parameter.second.Value);
                        if (it != secrteStoreRef.end())
                        {
                            parameter.second.Value = it->second;
                        }
                        else
                        {
                            Assert::CodingError("Can't find secret value for given secret {0} in secret store", parameter.second.Value);
                        }
                    }
                }
            }
        }
    }
}

ErrorCode HostingHelper::WriteSettingsToFile(wstring const& settingName, wstring const& settingValue, wstring const& fileName)
{
    ErrorCode error;
    try
    {
        FileWriter fileWriter;
        error = fileWriter.TryOpen(fileName);
        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Hosting,
                TraceType_HostingHelper,
                "Couldn't open file {0} to write setting with name {1}, error {2}",
                fileName,
                settingName,
                error);
            return error;
        }

        std::string result;
        StringUtility::UnicodeToAnsi(settingValue, result);
        fileWriter << result;
        fileWriter.Close();

        TraceNoise(
            TraceTaskCodes::Hosting,
            TraceType_HostingHelper,
            "WriteSettingsToFile FileName = {0}. SettingName = {1}.", fileName, settingName);
    }
    catch (std::exception const& e)
    {
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_HostingHelper,
            "WriteSettingsToFile failed : FileName = {0} SettingName {1}.Failed with error = {2}.", fileName, settingName, e.what());
        return ErrorCodeValue::OperationFailed;
    }

    return error;
}
