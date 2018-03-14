// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // Provides a way to share runtime within fabric for various Fabric services
    class IRuntimeSharingHelper : public Common::AsyncFabricComponent
    {
        DENY_COPY(IRuntimeSharingHelper)

    public:
        IRuntimeSharingHelper() {}
        virtual ~IRuntimeSharingHelper() {}

        virtual Common::ComPointer<IFabricRuntime> GetRuntime() = 0;

        virtual Common::AsyncOperationSPtr BeginOpen(
            std::wstring const & runtimeServiceAddress,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        
        virtual Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation) = 0;
    };

    struct RuntimeSharingHelperConstructorParameters
    {
        Common::ComponentRoot const * Root;
        KtlLogger::KtlLoggerNodeSPtr KtlLogger;
    };

    typedef IRuntimeSharingHelperUPtr RuntimeSharingHelperFactoryFunctionPtr(RuntimeSharingHelperConstructorParameters &);

    RuntimeSharingHelperFactoryFunctionPtr RuntimeSharingHelperFactory;
}
