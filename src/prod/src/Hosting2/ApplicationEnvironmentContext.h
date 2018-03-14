// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationEnvironmentContext : 
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationEnvironmentContext)

    public:
        explicit ApplicationEnvironmentContext(ServiceModel::ApplicationIdentifier const & applicationId);
        ~ApplicationEnvironmentContext();

        __declspec(property(get=get_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return this->applicationId_; }

        __declspec(property(get=get_PrincipalsContext)) PrincipalsProviderContextUPtr const & PrincipalsContext;
        PrincipalsProviderContextUPtr const & get_PrincipalsContext() const { return this->principalsContext_; }
        
        __declspec(property(get=get_LogCollectionPath, put=set_LogCollectionPath)) std::wstring LogCollectionPath;
        std::wstring const & get_LogCollectionPath() const { return this->logCollectionPath_; }
        void set_LogCollectionPath(std::wstring const & path) { this->logCollectionPath_ = path; }

        __declspec(property(get=get_DefaultRunAsUserId)) std::wstring const & DefaultRunAsUserId;
        std::wstring const & get_DefaultRunAsUser() const {return this->defaultRunAsUserId_; }

        Common::ErrorCode TryGetUserId(std::wstring const & name, __out std::wstring & userId) const;

        inline void SetPrincipalsProviderContext(PrincipalsProviderContextUPtr && context)
        {
            this->principalsContext_ = move(context);
        }

        Common::ErrorCode SetDefaultRunAsUser(std::wstring const & defaultRunAsUserName);

    private:
        ServiceModel::ApplicationIdentifier const applicationId_;
        
        PrincipalsProviderContextUPtr principalsContext_;

        // The path where all folders configured for log collection
        // are located
        std::wstring logCollectionPath_;
        
        // Default run as user, used when a service package 
        // doesn't specify any run as policy.
        std::wstring defaultRunAsUserId_;
    };
}
