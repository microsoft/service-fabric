// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class SecretsResourceHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(SecretsResourceHandler)

    public:
        SecretsResourceHandler(HttpGatewayImpl &server);
        virtual ~SecretsResourceHandler();

        Common::ErrorCode Initialize();

    private:
        void HandleError(HandlerAsyncOperation* const & handlerOperation, Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);

        /**
         * <summary>Get the secrets.</summary>
         * */
        void GetSecrets(Common::AsyncOperationSPtr const &thisSPtr);
        void OnGetSecretsComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Get the secret.</summary>
         * */
        void GetSecret(Common::AsyncOperationSPtr const &thisSPtr);
        void OnGetSecretComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Create the secret.</summary>
         * */
        void PutSecret(Common::AsyncOperationSPtr const &thisSPtr);
        void OnPutSecretComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Remove the secret.</summary>
         * */
        void DeleteSecret(Common::AsyncOperationSPtr const &thisSPtr);
        void OnDeleteSecretComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Get the all versions of the secret.</summary>
         * */
        void GetSecretVersions(Common::AsyncOperationSPtr const &thisSPtr);
        void OnGetSecretVersionsComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Get a specified version of the secret.</summary>
         * */
        void GetSecretVersion(Common::AsyncOperationSPtr const &thisSPtr);
        void OnGetSecretVersionComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Get the value of the specified version of the secret.</summary>
         * */
        void GetVersionedSecretValue(Common::AsyncOperationSPtr const &thisSPtr);
        void OnGetVersionedSecretValueComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Create the specific version of the secret.</summary>
         * */
        void PutVersionedSecretValue(Common::AsyncOperationSPtr const &thisSPtr);
        void OnPutVersionedSecretValueComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        /**
         * <summary>Create the specific version of the secret.</summary>
         * */
        void DeleteSecretVersion(Common::AsyncOperationSPtr const &thisSPtr);
        void OnDeleteSecretVersionComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);
    };
}
