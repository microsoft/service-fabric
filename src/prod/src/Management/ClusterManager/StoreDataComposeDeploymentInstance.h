// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataComposeDeploymentInstance : public Store::StoreData
        {
        public:
            StoreDataComposeDeploymentInstance();

            StoreDataComposeDeploymentInstance(
                std::wstring const &,
                ServiceModelVersion const &);

            StoreDataComposeDeploymentInstance(StoreDataComposeDeploymentInstance &&);

            StoreDataComposeDeploymentInstance(
                std::wstring const &,
                ServiceModelVersion const &,
                std::vector<Common::ByteBuffer> && composeFiles,
                std::vector<Common::ByteBuffer> && sfSettingsFiles,
                std::wstring const &repoUserName,
                std::wstring const &repoPassword,
                bool isPasswordEncrypted);

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode UpdateMergedComposeFile(Store::StoreTransaction &, std::wstring const&);

            std::wstring GetDeploymentNameKeyPrefix() const;

            __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get=get_Version)) ServiceModelVersion const & Version;
            ServiceModelVersion const & get_Version() const { return version_; }

            __declspec(property(get=get_ComposeFiles)) std::vector<Common::ByteBufferSPtr> const & ComposeFiles;
            std::vector<Common::ByteBufferSPtr> const & get_ComposeFiles() const { return composeFiles_; }

            __declspec(property(get=get_repoUserName)) std::wstring const & RepositoryUserName;
            std::wstring const & get_repoUserName() const { return repositoryUserName_; }

            __declspec(property(get=get_repoPassword)) std::wstring const & RepositoryPassword;
            std::wstring const & get_repoPassword() const { return repositoryUserPassword_; }

            __declspec(property(get = get_PasswordEncrypted)) bool IsPasswordEncrypted;
            bool get_PasswordEncrypted() const { return isPasswordEncrypted_; }

            __declspec(property(get=get_MergedComposeFile)) std::wstring const & MergedComposeFile;
            std::wstring const & get_MergedComposeFile() const { return mergedComposeFile_; }

            FABRIC_FIELDS_08(
                deploymentName_, 
                version_, 
                composeFiles_, 
                sfSettingsFiles_, 
                repositoryUserName_,
                repositoryUserPassword_,
                isPasswordEncrypted_,
                mergedComposeFile_);

        protected:
            virtual std::wstring ConstructKey() const override;
        private:

            std::wstring deploymentName_;
            ServiceModelVersion version_;
            std::vector<Common::ByteBufferSPtr> composeFiles_;
            std::vector<Common::ByteBufferSPtr> sfSettingsFiles_;
            std::wstring repositoryUserName_;
            std::wstring repositoryUserPassword_;
            bool isPasswordEncrypted_;
            std::wstring mergedComposeFile_;
        };
    }
}
