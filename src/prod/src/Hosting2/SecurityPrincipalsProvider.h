// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SecurityPrincipalsProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(SecurityPrincipalsProvider)

    public:
        explicit SecurityPrincipalsProvider(Common::ComponentRoot const & root);
        virtual ~SecurityPrincipalsProvider();

        Common::ErrorCode Setup(
            ConfigureSecurityPrincipalRequest && request,
            __inout std::vector<SecurityPrincipalInformationSPtr> & principalsInformation);

        Common::ErrorCode Cleanup(
            std::wstring const & nodeId,
            std::wstring const & applicationId,
            bool const cleanupForNode);

        Common::ErrorCode Cleanup(
            std::wstring const & nodeId);

        Common::ErrorCode GetSecurityUser(
            std::wstring const & userId,
           __out SecurityUserSPtr & secUser);

       Common::ErrorCode GetSecurityUser(
            std::wstring const & nodeId,
            std::wstring const & applicationId,
            std::wstring const & name,
           __out SecurityUserSPtr & secUser);

        Common::ErrorCode GetSecurityGroup(
            std::wstring const & groupId,
            __out SecurityGroupSPtr & secGroup);

        Common::ErrorCode GetSecurityGroup(
            std::wstring const & nodeId,
            std::wstring const & applicationId,
            std::wstring const & name,
            __out SecurityGroupSPtr & secGroup);

    protected:
        // ****************************************************
        // FabricComponent specific methods
        // ****************************************************
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

        Common::ErrorCode AddGetNodeEnvManager(
            std::wstring const & nodeId,
            __out NodeEnvironmentManagerSPtr & envMgr);


    private:
        SecurityIdentityMap<NodeEnvironmentManagerSPtr> envManagerMap_;
        Common::RwLock lock_;
    };
}
