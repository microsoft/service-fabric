// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NodeEnvironmentManager : 
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(NodeEnvironmentManager)

    public:
        explicit NodeEnvironmentManager(
            Common::ComponentRoot const & root,
            std::wstring const & nodeId);
        virtual ~NodeEnvironmentManager();

        Common::ErrorCode SetupSecurityPrincipals(
            ConfigureSecurityPrincipalRequest && request,
            __inout std::vector<SecurityPrincipalInformationSPtr> & principalsInformation);

        Common::ErrorCode Cleanup();
        Common::ErrorCode Cleanup(bool removePrincipals);
        Common::ErrorCode Cleanup(std::wstring const & applicationId, bool const cleanupForNode);

        Common::ErrorCode GetSecurityUser(
            std::wstring const & applicationId,
            std::wstring const & name,
           __out SecurityUserSPtr & secUser);


        Common::ErrorCode GetSecurityGroup(
            std::wstring const & applicationId,
            std::wstring const & name,
            __out SecurityGroupSPtr & secGroup);


    private:
        Common::RwLock lock_;
        std::wstring const nodeId_;

        using ApplicationPrincipalsMap = std::map<std::wstring, ApplicationPrincipalsSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>>;
        ApplicationPrincipalsMap applicationPrincipalsMap_;
        ApplicationPrincipalsMap nodePrincipalsMap_;
    };
}
