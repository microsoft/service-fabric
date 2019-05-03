// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace ServiceModel;
using namespace Management;
using namespace CentralSecretService;

StringLiteral const TraceSource = "SecureStoreTestClient";

//
// Construction/instantiation.
//

SecureStoreTestClient::SecureStoreTestClient(FabricTestDispatcher & dispatcher)
    : dispatcher_(dispatcher)
{
    SetCommandHandlers();

    expectedFailureIsSet_ = false;
    expectedValueIsSet_ = false;
    expectedValuesAreSet_ = false;
}

SecureStoreTestClient::~SecureStoreTestClient()
{
}

bool SecureStoreTestClient::IsSecureStoreClientCommand(std::wstring const & command)
{

    auto cmdCopy = std::wstring(command.c_str()); 
    StringUtility::ToLower(cmdCopy);

    return commandHandlers_.end() != commandHandlers_.find(cmdCopy);
}

void SecureStoreTestClient::SetCommandHandlers()
{
    commandHandlers_[FabricTestCommands::SecretCommands::PutSecret] = [this](StringCollection const & paramCollection)
    {
        return PutSecret(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::AddVersionedPlaintextSecretValue] = [this](StringCollection const & paramCollection)
    {
        return PutVersionedPlaintextSecret(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::GetSecret] = [this](StringCollection const & paramCollection)
    {
        return GetSecret(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::GetVersionedSecretValue] = [this](StringCollection const & paramCollection)
    {
        return GetVersionedSecretValue(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::ListVersions] = [this](StringCollection const & paramCollection)
    {
        return ListSecretVersions(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::List] = [this](StringCollection const & paramCollection)
    {
        return ListSecrets(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::Remove] = [this](StringCollection const & paramCollection)
    {
        return RemoveSecret(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::RemoveVersion] = [this](StringCollection const & paramCollection)
    {
        return RemoveSecretVersion(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SecretCommands::Clear] = [this](StringCollection const & paramCollection)
    {
        return ClearAllSecrets(paramCollection);
    };
}

bool SecureStoreTestClient::ExecuteCommand(
    std::wstring const & command, 
    Common::StringCollection const & params)
{
    if (command.empty()
        || !IsSecureStoreClientCommand(command))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Unrecognized command: '{0}'",
            command);

        return false;
    }

    // ensure the inner client is instantiated
    InstantiateInnerSecureStoreClient(FABRICSESSION.FabricDispatcher.Federation);

    auto commandIter = commandHandlers_.find(command);
    return commandIter != commandHandlers_.end()
        && commandIter->second(params);
}

//
// Mock APIs - translate test script commands into corresponding SecretStoreClient API calls.
//
bool SecureStoreTestClient::PutSecret(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);
    expectedParameterNames.push_back(SecretCommandParameters::Kind);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for test command PutSecret: {0}",
            errorMsg);
    }

    // call secret store client here
    auto secretFromParameters = InstantiateSecretFromParameters(parsedParameters);
    std::vector<Management::CentralSecretService::Secret> secrets;
    secrets.push_back(*secretFromParameters.get());

    Common::AsyncOperationSPtr opPtr;

    Management::CentralSecretService::SecretsDescription input(secrets);
    Management::CentralSecretService::SecretsDescription result;

    auto operation = secureStoreClient_->BeginSetSecrets(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, result](AsyncOperationSPtr const & operation) { this->OnSetSecretsComplete(operation, (Management::CentralSecretService::SecretsDescription&)result, false); },
        opPtr);

    OnSetSecretsComplete(operation, result, true);

    return true;
}

bool SecureStoreTestClient::PutVersionedPlaintextSecret(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);
    expectedParameterNames.push_back(SecretCommandParameters::Version);
    expectedParameterNames.push_back(SecretCommandParameters::Value);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for test command PutVersionedPlaintextSecret: {0}",
            errorMsg);
    }

    // call secret store client here
    auto secretFromParameters = InstantiateSecretFromParameters(parsedParameters);
    std::vector<Management::CentralSecretService::Secret> secrets;
    secrets.push_back(*secretFromParameters.get());

    Common::AsyncOperationSPtr opPtr;
    Management::CentralSecretService::SecretsDescription input(secrets);
    Management::CentralSecretService::SecretsDescription result;

    auto context = secureStoreClient_.get()->BeginSetSecrets(
        input, 
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout, 
        [this, result](AsyncOperationSPtr const & operation) { OnSetSecretsComplete(operation, (Management::CentralSecretService::SecretsDescription&)result, false); },
        opPtr);

    OnSetSecretsComplete(context, result, true);

    return true;
}

bool SecureStoreTestClient::GetSecret(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for test command GetSecret: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;

    std::vector<SecretReference> references;
    references.push_back(*InstantiateSecretReferenceFromParameters(parsedParameters));
    GetSecretsDescription input(references, false);
    Management::CentralSecretService::SecretsDescription result;

    auto context = secureStoreClient_->BeginGetSecrets(
        input, 
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout, 
        [this, result](AsyncOperationSPtr const & operation) { OnGetSecretsComplete(operation, (Management::CentralSecretService::SecretsDescription&)result, false); },
        asyncOpPtr);

    OnGetSecretsComplete(context, result, true);

    return true;
}

bool SecureStoreTestClient::GetVersionedSecretValue(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);
    expectedParameterNames.push_back(SecretCommandParameters::Version);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation GetSecretVersion: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;
    std::vector<SecretReference> references;
    references.push_back(*InstantiateSecretReferenceFromParameters(parsedParameters));
    GetSecretsDescription input(references, true);
    Management::CentralSecretService::SecretsDescription result;

    auto context = secureStoreClient_->BeginGetSecrets(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, result](AsyncOperationSPtr const & operation) {OnGetSecretsComplete(operation, (Management::CentralSecretService::SecretsDescription&)result, false); },
        asyncOpPtr);

    OnGetSecretsComplete(context, result, true);

    return true;
}

bool SecureStoreTestClient::ListSecretVersions(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation ListSecretVersions: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;
    std::vector<SecretReference> references;
    references.push_back(*InstantiateSecretReferenceFromParameters(parsedParameters));
    SecretReferencesDescription input(references);
    Management::CentralSecretService::SecretReferencesDescription results;

    auto context = secureStoreClient_->BeginGetSecretVersions(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, results](AsyncOperationSPtr const & operation) { OnGetSecretVersionsComplete(operation, (Management::CentralSecretService::SecretReferencesDescription&)results, false); },
        asyncOpPtr);

    OnGetSecretVersionsComplete(context, results, true);
    
    return true;
}

bool SecureStoreTestClient::ListSecrets(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation ListSecrets: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;
    std::vector<SecretReference> references;
    GetSecretsDescription input(references, false);
    Management::CentralSecretService::SecretsDescription results;

    auto context = secureStoreClient_->BeginGetSecrets(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, results](AsyncOperationSPtr const & operation) { OnGetSecretsComplete(operation, (Management::CentralSecretService::SecretsDescription&)results, false); },
        asyncOpPtr);

    OnGetSecretsComplete(context, results, true);

    return true;
}

bool SecureStoreTestClient::RemoveSecret(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation RemoveSecret: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;
    std::vector<SecretReference> references;
    references.push_back(*InstantiateSecretReferenceFromParameters(parsedParameters));
    SecretReferencesDescription input(references);
    SecretReferencesDescription results;

    auto context = secureStoreClient_->BeginRemoveSecrets(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, results](AsyncOperationSPtr const & operation) { OnRemoveSecretsComplete(operation, (Management::CentralSecretService::SecretReferencesDescription&)results, false); },
        asyncOpPtr);

    OnRemoveSecretsComplete(context, results, true);

    return true;
}

bool SecureStoreTestClient::RemoveSecretVersion(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;
    expectedParameterNames.push_back(SecretCommandParameters::Name);
    expectedParameterNames.push_back(SecretCommandParameters::Version);

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation RemoveSecretVersion: {0}",
            errorMsg);
    }

    // call secret store client here
    Common::AsyncOperationSPtr asyncOpPtr;
    std::vector<SecretReference> references;
    references.push_back(*InstantiateSecretReferenceFromParameters(parsedParameters));
    SecretReferencesDescription input(references);
    SecretReferencesDescription results;

    auto context = secureStoreClient_->BeginRemoveSecrets(
        input,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [this, results](AsyncOperationSPtr const & operation) { OnRemoveSecretsComplete(operation, (Management::CentralSecretService::SecretReferencesDescription&)results, false); },
        asyncOpPtr);

    OnRemoveSecretsComplete(context, results, true);

    return true;
}

bool SecureStoreTestClient::ClearAllSecrets(Common::StringCollection const & parameters)
{
    std::vector<std::wstring> expectedParameterNames;

    std::map<std::wstring, std::wstring> parsedParameters;
    std::wstring errorMsg = L"";
    if (!TryParseParameters(parameters, expectedParameterNames, parsedParameters, errorMsg))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Incorrect parameters provided for operation ClearAllSecrets: {0}",
            errorMsg);
    }

    // call secret store client here

    return false;
}

void SecureStoreTestClient::OnGetSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretsDescription & result, bool expectedSynchronousCompletion)
{
    if (operation->CompletedSynchronously != expectedSynchronousCompletion)
    {
        return;
    }

    auto error = secureStoreClient_->EndGetSecrets(operation, result);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(
            TraceSource,
            "End(OnGetSecretsCompleted): ErrorCode={0}",
            error);
    }

    operation->TryComplete(operation->Parent, error);
}

void SecureStoreTestClient::OnRemoveSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretReferencesDescription & results, bool expectedSynchronousCompletion)
{
    if (operation->CompletedSynchronously != expectedSynchronousCompletion)
    {
        return;
    }

    auto error = secureStoreClient_->EndRemoveSecrets(operation, results);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(
            TraceSource,
            "End(OnRemoveSecretsCompleted): ErrorCode={0}",
            error);
    }

    operation->TryComplete(operation->Parent, error);
}

void SecureStoreTestClient::OnGetSecretVersionsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretReferencesDescription & results, bool expectedSynchronousCompletion)
{
    if (operation->CompletedSynchronously != expectedSynchronousCompletion)
    {
        return;
    }

    auto error = secureStoreClient_->EndGetSecretVersions(operation, results);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(
            TraceSource,
            "End(OnGetSecretVersionsCompleted): ErrorCode={0}: {1}",
            error,
            error.Message);
    }

    operation->TryComplete(operation->Parent, error);
}

void SecureStoreTestClient::OnSetSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretsDescription & result, bool expectedSynchronousCompletion)
{
    if (operation->CompletedSynchronously != expectedSynchronousCompletion)
    {
        return;
    }

    auto error = secureStoreClient_->EndSetSecrets(operation, result);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(
            TraceSource,
            "End(OnSetSecretsCompleted): ErrorCode={0}: {1}",
            error,
            error.Message);
    }

    operation->TryComplete(operation->Parent, error);
}

//
// Parameter handling.
//
bool SecureStoreTestClient::TryParseParameters(
    Common::StringCollection const & parameters, 
    std::vector<std::wstring> expectedParameters, 
    std::map<std::wstring, std::wstring> & parsedParameters, 
    std::wstring & firstError)
{
    if (expectedParameters.size() > parameters.size())
    {
        firstError = wformatString(L"the command expects {0} parameters, but only {1} parameters were provided.", expectedParameters.size(), parameters.size());

        return false;
    }

    auto validatedAllParams = true;
    CommandLineParser parser(parameters);
    std::wstring paramValue = L"";
    
    // parse required parameters
    for (auto const & paramKey : expectedParameters)
    {
        validatedAllParams &= parser.TryGetString(paramKey, paramValue);
        if (!validatedAllParams)
        {
            firstError = wformatString(L"expected parameter '{0}' was not specified.", paramKey);
            break;
        }

        parsedParameters[paramKey] = paramValue;
    }

    if (validatedAllParams)
    {
        // proceed with parsing of optional/behavioral parameters ('expected*')
        expectedFailureIsSet_ = parser.TryGetBool(SecureStoreTestClient::SecretCommandParameters::ExpectedFailure, expectedFailure_);

        expectedValuesAreSet_ = parser.GetVector(SecureStoreTestClient::SecretCommandParameters::ExpectedValues, expectedValues_);

        expectedValueIsSet_ = parser.TryGetString(SecureStoreTestClient::SecretCommandParameters::ExpectedValue, expectedValue_);
    }
    else
    {
        parsedParameters.clear();
    }

    return validatedAllParams;
}

void SecureStoreTestClient::InstantiateInnerSecureStoreClient(__in FabricTestFederation & testFederation)
{
    if (secureStoreClient_.get() == NULL)
    {
        auto factoryPtr = TestFabricClient::FabricCreateNativeClientFactory(testFederation);
        auto error = factoryPtr->CreateSecretStoreClient(secureStoreClient_);
        TestSession::FailTestIfNot(error.IsSuccess(), "InternalClientFactory does not support ISecretStoreClient");
    }
}

std::shared_ptr<Secret> SecureStoreTestClient::InstantiateSecretFromParameters(std::map<std::wstring, std::wstring> parameterMap)
{
    std::wstring secretName;
    auto it = parameterMap.find(SecretCommandParameters::Name);
    if (it != parameterMap.end())
    {
        secretName = it->second;
    }

    std::wstring secretVersionId;
    it = parameterMap.find(SecretCommandParameters::Version);
    if (it != parameterMap.end())
    {
        secretVersionId = it->second;
    }

    it = parameterMap.find(SecretCommandParameters::Value);
    std::wstring secretValue;
    if (it != parameterMap.end())
    {
        secretValue = it->second;
    }

    std::wstring kind = L"";
    it = parameterMap.find(SecretCommandParameters::Kind);
    if (it != parameterMap.end())
        kind = it->second;

    std::wstring contentType = L"";
    it = parameterMap.find(SecretCommandParameters::ContentType);
    if (it != parameterMap.end())
        contentType = it->second;

    std::wstring description = L"";
    it = parameterMap.find(SecretCommandParameters::Description);
    if (it != parameterMap.end())
        description = it->second;

    return std::make_shared<Secret>(secretName, secretVersionId, secretValue, kind, contentType, description);
}

std::shared_ptr<SecretReference> SecureStoreTestClient::InstantiateSecretReferenceFromParameters(std::map<std::wstring, std::wstring> parameterMap)
{

    std::wstring secretName;
    auto it = parameterMap.find(SecretCommandParameters::Name);
    if (it != parameterMap.end())
    {
        secretName = it->second;
    }

    std::wstring secretVersionId;
    it = parameterMap.find(SecretCommandParameters::Version);
    if (it != parameterMap.end())
    {
        secretVersionId = it->second;
    }

    return std::make_shared<SecretReference>(secretName, secretVersionId);
}


// parameter name constants
std::wstring const SecureStoreTestClient::SecretCommandParameters::Name = L"name";
std::wstring const SecureStoreTestClient::SecretCommandParameters::Version = L"version";
std::wstring const SecureStoreTestClient::SecretCommandParameters::Value = L"value";
std::wstring const SecureStoreTestClient::SecretCommandParameters::ExpectedValue = L"expectedvalue";
std::wstring const SecureStoreTestClient::SecretCommandParameters::ExpectedValues = L"expectedvalues";
std::wstring const SecureStoreTestClient::SecretCommandParameters::ExpectedFailure = L"expectedfailure";
std::wstring const SecureStoreTestClient::SecretCommandParameters::Kind = L"kind";
std::wstring const SecureStoreTestClient::SecretCommandParameters::ContentType = L"contentType";
std::wstring const SecureStoreTestClient::SecretCommandParameters::Description = L"description";

