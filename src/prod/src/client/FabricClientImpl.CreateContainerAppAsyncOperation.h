// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::CreateContainerAppAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
    public:
        CreateContainerAppAsyncOperation(
            __in FabricClientImpl &client,
            std::wstring const &deploymentName,
            std::wstring const &composeFilePath,
            std::wstring const &overridesFilepath,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool isPasswordEncrypted,
            Common::ActivityId const& activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation);

    protected:
        void OnStartOperation(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void OnCreateContainerAppComplete(Common::AsyncOperationSPtr const& thisSPtr, bool);
        Common::ErrorCode ReadFileContents(Common::File &composeFile, Common::File &overridesFile);

        std::wstring deploymentName_;
        std::wstring composeFilePath_;
        std::wstring overridesFilePath_;
        std::wstring repositoryUserName_;
        std::wstring repositoryPassword_;
        bool isPasswordEncrypted_;
        Common::ByteBuffer composeFileContents_;
        Common::ByteBuffer overridesFileContents_;
    };
}
