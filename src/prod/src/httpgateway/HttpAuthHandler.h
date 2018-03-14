// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpAuthHandler
        : public std::enable_shared_from_this<HttpAuthHandler>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(HttpAuthHandler)

    public:
        HttpAuthHandler(
            __in Transport::SecurityProvider::Enum handlerType)
            : handlerType_(handlerType)
            , isClientRoleInEffect_(false)
            , role_(Transport::RoleMask::None)
        {
        }

        template<typename handler>
        static Common::ErrorCode Create(
            __in Transport::SecuritySettings const& securitySettings,
            __in Transport::SecurityProvider::Enum handlerType,
            __in FabricClientWrapperSPtr &client,
            __out HttpAuthHandlerSPtr &handlerSPtr)
        {
            HttpAuthHandlerSPtr handlerBaseSPtr = std::make_shared<handler>(handlerType, client);

            auto error = handlerBaseSPtr->Initialize(securitySettings);
            if (error.IsSuccess())
            {
                handlerSPtr = move(handlerBaseSPtr);
            }

            return error;
        }

        Common::AsyncOperationSPtr BeginCheckAccess(
            HttpServer::IRequestMessageContext & request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCheckAccess(
            Common::AsyncOperationSPtr const& operation,
            __out USHORT &httpStatusOnError,
            __inout std::wstring & authHeaderName,
            __inout std::wstring & authHeaderValue,
            __out Transport::RoleMask::Enum& roleOnSuccess);

        Common::ErrorCode Initialize(Transport::SecuritySettings const& securitySettings)
        {
            Common::AcquireWriteLock writeLock(lock_);
            isClientRoleInEffect_ = securitySettings.IsClientRoleInEffect();
            return OnInitialize(securitySettings);
        }

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const& securitySettings)
        {
            Common::AcquireWriteLock writeLock(lock_);
            isClientRoleInEffect_ = securitySettings.IsClientRoleInEffect();
            return OnSetSecurity(securitySettings);
        }

    protected:

        // OnCheckAccess is synchronized with OnSetSecurity by the base class
        virtual void OnCheckAccess(Common::AsyncOperationSPtr const& thisSPtr) = 0;
        virtual Common::ErrorCode OnSetSecurity(Transport::SecuritySettings const&) = 0;
        virtual Common::ErrorCode OnInitialize(Transport::SecuritySettings const&) = 0;

        class AccessCheckAsyncOperation;

        Transport::SecurityProvider::Enum handlerType_;
        bool isClientRoleInEffect_;
        
        // protects access to the properties of the base and derived class
        mutable Common::RwLock lock_;
        Transport::RoleMask::Enum role_;
    };
}
