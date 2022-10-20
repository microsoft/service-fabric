// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management::CentralSecretService;
using namespace Management::ImageModel;

StringLiteral TraceType_HostingHelper = "HostingHelper";

// ********************************************************************************************************************
// HostingHelper.GetSecretStoreRefValuesAsyncOperation Implementation
//
// Queries SecretStore to retrieve SecretRefValues
class HostingHelper::GetSecretStoreRefValuesAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(GetSecretStoreRefValuesAsyncOperation)

public:
    GetSecretStoreRefValuesAsyncOperation(
        _In_ HostingSubsystem & hosting,
        vector<wstring> const & secretStoreRefs,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : hosting_(hosting),
        secretStoreRefs_(secretStoreRefs),
        decryptedSecretStoreRefMap_(),
        AsyncOperation(callback, parent)
    {
    }

    virtual ~GetSecretStoreRefValuesAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        _Out_ map<wstring, wstring> & decryptedSecretStoreRef)
    {
        auto thisPtr = AsyncOperation::End<GetSecretStoreRefValuesAsyncOperation>(operation);
        decryptedSecretStoreRef = move(thisPtr->decryptedSecretStoreRefMap_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!CentralSecretServiceConfig::IsCentralSecretServiceEnabled())
        {
            ErrorCode err(ErrorCodeValue::InvalidOperation, StringResource::Get(IDS_HOSTING_SecretStoreIsUnavailable));
            TryComplete(thisSPtr, move(err));
            return;
        }

        GetSecretsDescription getSecretsDesc;
        ErrorCode error = hosting_.LocalSecretServiceManagerObj->ParseSecretStoreRef(
            secretStoreRefs_,
            getSecretsDesc);

        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Hosting,
                TraceType_HostingHelper,
                "Error while parsing SecretRefs LocalSecretServiceManagerObj->ParseSecretStoreRef {0}, SecretStoreRef: {1}",
                error,
                secretStoreRefs_);

            TryComplete(thisSPtr, move(error));
            return;
        }

        auto operation = hosting_.LocalSecretServiceManagerObj->BeginGetSecrets(
            getSecretsDesc,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnBeginGetSecretsCompleted(operation, false);
        },
            thisSPtr);
        this->OnBeginGetSecretsCompleted(operation, true);
    }

private:
    void OnBeginGetSecretsCompleted(
        AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        SecretsDescription secretsDesc;
        ErrorCode error = hosting_.LocalSecretServiceManagerObj->EndGetSecrets(operation, secretsDesc);
        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Hosting,
                TraceType_HostingHelper,
                "Error while obtaining SecretStoreRef values LocalSecretServiceManagerObj->EndGetSecrets {0}, SecretStoreRef: {1}",
                error,
                secretStoreRefs_);

            TryComplete(operation->Parent, move(error));
            return;
        }

        std::vector<Secret> secrets = secretsDesc.Secrets;
        for (auto secret : secrets)
        {
            wstring key = wformatString("{0}:{1}", secret.Name, secret.Version);
            decryptedSecretStoreRefMap_.insert(make_pair(move(key), move(secret.Value)));
        }

        TryComplete(operation->Parent, move(error));
    }

private:
    vector<wstring> const secretStoreRefs_;
    map<wstring, wstring> decryptedSecretStoreRefMap_;
    HostingSubsystem & hosting_;
};

ErrorCode HostingHelper::GenerateAllConfigSettings(
    wstring const& applicationId,
    wstring const& servicePackageName,
    RunLayoutSpecification const& runLayout,
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

            return ErrorCode(ErrorCodeValue::FileNotFound,
                wformatString(StringResource::Get(IDS_HOSTING_FailedToLoadConfigSettingsFile), settingsFile));
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

ErrorCode HostingHelper::DecryptAndGetSecretStoreRefForConfigSetting(
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

AsyncOperationSPtr HostingHelper::BeginGetSecretStoreRef(
    _In_ HostingSubsystem & hosting,
    vector<wstring> const & secretStoreRefs,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetSecretStoreRefValuesAsyncOperation>(
        hosting,
        secretStoreRefs,
        callback,
        parent);
}

ErrorCode HostingHelper::EndGetSecretStoreRef(
    AsyncOperationSPtr const & operation,
    _Out_ map<wstring, wstring> & decryptedSecretStoreRef)
{
    return GetSecretStoreRefValuesAsyncOperation::End(operation, decryptedSecretStoreRef);
}