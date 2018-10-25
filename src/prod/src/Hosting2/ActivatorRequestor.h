// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    namespace ActivatorRequestorType
    {
        enum Enum : ULONG
        {
            FabricNode = 0,
            ContainerActivatorService = 1
        };
    }

    class ActivatorRequestor :
        public Common::FabricComponent,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>

    {
    public:
        ActivatorRequestor(
            ProcessActivationManagerHolder const & activatorHolder,
            FabricHostClientProcessTerminationCallback const & terminationCallback,
            ActivatorRequestorType::Enum requestorType,
            std::wstring const & requestorId,
            std::wstring const & callbackAddress,
            DWORD processId,
            BOOL obtainToken,
            uint64 nodeInstanceId = 0);

        ~ActivatorRequestor();

        __declspec(property(get=get_RequestorId)) std::wstring const& RequestorId;
        std::wstring const& get_RequestorId() const { return requestorId_; }

        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() const { return processId_; }

        __declspec(property(get=get_CallbackAddress)) std::wstring const& CallbackAddress;
        std::wstring const& get_CallbackAddress() const { return callbackAddress_; }

        Common::ErrorCode GetDefaultSecurityUser(__out SecurityUserSPtr & secUser, bool isWinFabAdminGroupMemeber);

#if defined(PLATFORM_UNIX)
        Common::ErrorCode GetDefaultAppSecurityUser(__out SecurityUserSPtr & secUser);
#endif

        __declspec(property(get=get_ProcessActivationManager)) ProcessActivationManager & ActivationManager;
        ProcessActivationManager & get_ProcessActivationManager() { return this->activatorHolder_.RootedObject; }

        __declspec(property(get = get_NodeInstanceId)) uint64 NodeInstanceId;
        uint64 get_NodeInstanceId() const { return nodeInstanceId_; }

    protected:
        // ****************************************************
        // FabricComponent specific methods
        // ****************************************************
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();
    private:
        Common::ErrorCode GetRequestorUserToken(
            Common::HandleUPtr const & appHostProcessHandle,
            __out Common::AccessTokenSPtr & requestorTokenRestricted,
            __out Common::AccessTokenSPtr & requestorTokenUnrestricted);

    private:
        std::wstring requestorId_;
        DWORD processId_;
        BOOL obtainToken_; 
        Common::ProcessWaitSPtr processMonitor_;
        std::wstring callbackAddress_;
        Common::AccessTokenSPtr accessTokenRestricted_;
        Common::AccessTokenSPtr accessTokenUnrestricted_;
        ProcessActivationManagerHolder const activatorHolder_;
        FabricHostClientProcessTerminationCallback terminationCallback_;
        ActivatorRequestorType::Enum requestorType_;
        uint64 nodeInstanceId_;
    };

}
