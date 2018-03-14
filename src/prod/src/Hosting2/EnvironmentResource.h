// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class EnvironmentResource
    {
        DENY_COPY(EnvironmentResource)

    public:
        struct ResourceAccess
        {
            SecurityPrincipalInformationSPtr PrincipalInfo;
            ServiceModel::GrantAccessType::Enum AccessType;
        };
        
    public:
        EnvironmentResource();
        virtual ~EnvironmentResource();

        __declspec(property(get=get_AccessEntries)) std::vector<ResourceAccess> const & AccessEntries;
        std::vector<ResourceAccess> const & get_AccessEntries() const { return resourceAccessEntries_; }

        void AddSecurityAccessPolicy(
            SecurityPrincipalInformationSPtr const & principalInfo,
            ServiceModel::GrantAccessType::Enum accessType);

        Common::ErrorCode GetDefaultSecurityAccess(
            __out ResourceAccess & securityAccess) const;
            
    private:
        std::vector<ResourceAccess> resourceAccessEntries_;
   };
}
