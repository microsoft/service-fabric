//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    class ISecretStoreClient
    {
    public:
        virtual ~ISecretStoreClient() {};

#pragma region GetSecrets()

        virtual Common::AsyncOperationSPtr BeginGetSecrets(
            Management::CentralSecretService::GetSecretsDescription & getSecretsDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetSecrets(
            Common::AsyncOperationSPtr const &,
            __out Management::CentralSecretService::SecretsDescription & result) = 0;

#pragma endregion

#pragma region SetSecrets()

        virtual Common::AsyncOperationSPtr BeginSetSecrets(
            Management::CentralSecretService::SecretsDescription & secrets,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndSetSecrets(
            Common::AsyncOperationSPtr const &,
            __out Management::CentralSecretService::SecretsDescription & result) = 0;

#pragma endregion

#pragma region RemoveSecrets()

        virtual Common::AsyncOperationSPtr BeginRemoveSecrets(
            Management::CentralSecretService::SecretReferencesDescription & secretReferences,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRemoveSecrets(
            Common::AsyncOperationSPtr const &,
            __out Management::CentralSecretService::SecretReferencesDescription & result) = 0;

#pragma endregion

#pragma region GetSecretVersions()

        virtual Common::AsyncOperationSPtr BeginGetSecretVersions(
            Management::CentralSecretService::SecretReferencesDescription & secretReferences,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetSecretVersions(
            Common::AsyncOperationSPtr const &,
            __out Management::CentralSecretService::SecretReferencesDescription & result) = 0;

#pragma endregion
    };
}
