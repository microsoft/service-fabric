// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SingleCodePackageApplicationHostCodePackageActivator :
        public ApplicationHostCodePackageActivator
    {
    public:
        SingleCodePackageApplicationHostCodePackageActivator(
            Common::ComponentRoot const & root,
            SingleCodePackageApplicationHost & appHost);

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
        SingleCodePackageApplicationHost & appHost_;
    };
}
