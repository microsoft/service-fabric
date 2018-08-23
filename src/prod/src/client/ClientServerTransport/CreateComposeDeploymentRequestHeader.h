// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class CreateComposeDeploymentRequestHeader :
        public Transport::MessageHeader<Transport::MessageHeaderId::CreateComposeDeploymentRequest>,
        public Serialization::FabricSerializable
    {
    public:
        CreateComposeDeploymentRequestHeader()
            : deploymentName_()
            , applicationName_()
            , repositoryUserName_()
            , repositoryPassword_()
            , isPasswordEncrypted_(false)
            , composeFileSizes_()
            , sfSettingsFileSizes_()
        {
        }

        CreateComposeDeploymentRequestHeader(
            std::wstring const &deploymentName,
            std::wstring const &applicationName,
            std::vector<size_t> &&composeFileSizes,
            std::vector<size_t> &&sfSettingsFileSizes,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool isPasswordEncrypted)
            : deploymentName_(deploymentName)
            , applicationName_(applicationName)
            , composeFileSizes_(move(composeFileSizes))
            , sfSettingsFileSizes_(move(sfSettingsFileSizes))
            , repositoryUserName_(repositoryUserName)
            , repositoryPassword_(repositoryPassword)
            , isPasswordEncrypted_(isPasswordEncrypted)
        {
        }

        __declspec(property(get=get_ApplicationName)) std::wstring const &ApplicationName;
        inline std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_DeploymentName)) std::wstring const &DeploymentName;
        inline std::wstring const& get_DeploymentName() const { return deploymentName_; }

        __declspec(property(get=get_ComposeFileSizes)) std::vector<size_t> const &ComposeFileSizes;
        inline std::vector<size_t> const& get_ComposeFileSizes() const { return composeFileSizes_; }

        __declspec(property(get=get_SfSettingsFileSizes)) std::vector<size_t> const &SFSettingsFileSizes;
        inline std::vector<size_t> const& get_SfSettingsFileSizes() const { return sfSettingsFileSizes_; }
        
        __declspec(property(get=get_RepositoryUserName)) std::wstring const &RepositoryUserName;
        inline std::wstring const& get_RepositoryUserName() const { return repositoryUserName_; }

        __declspec(property(get=get_RepositoryPassword)) std::wstring const &RepositoryPassword;
        inline std::wstring const& get_RepositoryPassword() const { return repositoryPassword_; }

        __declspec(property(get=get_IsPasswordEncrypted)) bool IsPasswordEncrypted;
        bool get_IsPasswordEncrypted() { return isPasswordEncrypted_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("CreateComposeDeploymentRequestHeader{ ");
            w.Write("DeploymentName : {0},", deploymentName_);
            w.Write("ApplicationName : {0},", applicationName_);
            w.Write("# ComposeFiles : {0},", composeFileSizes_.size());
            w.Write("# Sf settings files : {0}", sfSettingsFileSizes_.size());
            w.Write("UserName : {0}", repositoryUserName_);
            w.Write(" }");
        }

        FABRIC_FIELDS_07(deploymentName_,
                         applicationName_,
                         composeFileSizes_,
                         sfSettingsFileSizes_,
                         repositoryUserName_,
                         repositoryPassword_,
                         isPasswordEncrypted_);

    private:
        std::wstring deploymentName_;
        std::wstring applicationName_;
        std::vector<size_t> composeFileSizes_;
        std::vector<size_t> sfSettingsFileSizes_;
        std::wstring repositoryUserName_;
        std::wstring repositoryPassword_;
        bool isPasswordEncrypted_;
    };
}
