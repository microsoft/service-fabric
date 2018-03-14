// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class PrincipalsProviderContext : protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(PrincipalsProviderContext)

    public:
        explicit PrincipalsProviderContext();

        ~PrincipalsProviderContext();

        __declspec(property(get=get_PrincipalsInformation)) std::vector<SecurityPrincipalInformationSPtr> const & PrincipalsInformation;
        std::vector<SecurityPrincipalInformationSPtr> const & get_PrincipalsInformation() const { return securityPrincipalInformation_; }

        void AddSecurityPrincipals(std::vector<SecurityPrincipalInformation> const & principalsInfo);

        Common::ErrorCode TryGetPrincipalId(std::wstring const & name, __out std::wstring & principalId) const;

        Common::ErrorCode TryGetPrincipalInfo(std::wstring const & name, __out SecurityPrincipalInformationSPtr & principalInfo);

        std::vector<Common::SidSPtr> const & GetPrincipalSids();

        Common::ErrorCode InitializeSecurityPrincipalSids();

    private:
        mutable Common::RwLock lock_; 
        std::vector<SecurityPrincipalInformationSPtr> securityPrincipalInformation_;
        std::vector<Common::SidSPtr> principalSids_;
    };
}
