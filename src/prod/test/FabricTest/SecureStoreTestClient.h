// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;
    class SecureStoreTestClient
    {
        DENY_COPY(SecureStoreTestClient);

        typedef std::function<bool(Common::StringCollection const &)> CommandHandler;

    public:
        // ctors
        SecureStoreTestClient(FabricTestDispatcher & dispatcher);
        virtual ~SecureStoreTestClient();

        // helpers
        bool IsSecureStoreClientCommand(std::wstring const & command);
        bool ExecuteCommand(std::wstring const & command, Common::StringCollection const & params);

        // API/command handling
        bool PutSecret(Common::StringCollection const & parameters);
        bool PutVersionedPlaintextSecret(Common::StringCollection const & parameters);
        bool GetSecret(Common::StringCollection const & parameters);
        bool GetVersionedSecretValue(Common::StringCollection const & parameters);
        bool ListSecretVersions(Common::StringCollection const & parameters);
        bool ListSecrets(Common::StringCollection const & parameters);
        bool RemoveSecret(Common::StringCollection const & parameters);
        bool RemoveSecretVersion(Common::StringCollection const & parameters);
        bool ClearAllSecrets(Common::StringCollection const & parameters);

        // parameters
        static struct SecretCommandParameters
        {
            static std::wstring const Name;
            static std::wstring const Version;
            static std::wstring const Value;
            static std::wstring const Kind;
            static std::wstring const ContentType;
            static std::wstring const Description;
            static std::wstring const ExpectedValue;
            static std::wstring const ExpectedValues;
            static std::wstring const ExpectedFailure;
        }   SecretCommandParameters;

    private:
        // helper methods
        void SetCommandHandlers();
        bool TryParseParameters(
            Common::StringCollection const & parameters, 
            std::vector<std::wstring> expectedParameters, 
            std::map<std::wstring, std::wstring> & parsedParameters, 
            std::wstring & firstError);
        void InstantiateInnerSecureStoreClient(__in FabricTestFederation & testFederation);

        void OnSetSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretsDescription & results, bool expectedSynchronousCompletion);
        void OnGetSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretsDescription & results, bool expectedSynchronousCompletion);
        void OnRemoveSecretsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretReferencesDescription & results, bool expectedSynchronousCompletion);
        void OnGetSecretVersionsComplete(AsyncOperationSPtr operation, Management::CentralSecretService::SecretReferencesDescription & results, bool expectedSynchronousCompletion);

        static std::shared_ptr<Management::CentralSecretService::Secret> InstantiateSecretFromParameters(std::map<std::wstring, std::wstring> parameterMap);
        static std::shared_ptr<Management::CentralSecretService::SecretReference> InstantiateSecretReferenceFromParameters(std::map<std::wstring, std::wstring> parameterMap);

        // SecureStore API

        // members

        // keeping the Dispatcher manageable, we'll track S3 commands in the SecureStore class.
        std::map<std::wstring, CommandHandler> commandHandlers_;

        // behavior control
        bool expectedFailureIsSet_;
        bool expectedFailure_;
        bool expectedValuesAreSet_;
        StringCollection expectedValues_;
        bool expectedValueIsSet_;
        std::wstring expectedValue_;

        // parent reference
        FabricTestDispatcher & dispatcher_;

        // proper secure store client instance
        Api::ISecretStoreClientPtr secureStoreClient_;
    };
}