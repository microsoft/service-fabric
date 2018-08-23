// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class InProcessApplicationHostCodePackageActivator :
        public ApplicationHostCodePackageActivator
    {
    public:
        InProcessApplicationHostCodePackageActivator(
            Common::ComponentRoot const & root,
            InProcessApplicationHost & appHost);

    protected:
        Common::AsyncOperationSPtr BeginSendRequest(
            ApplicationHostCodePackageOperationRequest && message,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndSendRequest(
            Common::AsyncOperationSPtr const & operation,
            __out ApplicationHostCodePackageOperationReply & reply) override;

        CodePackageContext GetCodePackageContext() override;
        ApplicationHostContext GetHostContext() override;

    private:
        __declspec(property(get = get_AppHostMgr)) ApplicationHostManagerUPtr const & AppHostMgr;
        inline ApplicationHostManagerUPtr const & get_AppHostMgr() const
        { 
            return appHost_.Hosting.ApplicationHostManagerObj;
        }

    private:
        InProcessApplicationHost & appHost_;
    };
}
