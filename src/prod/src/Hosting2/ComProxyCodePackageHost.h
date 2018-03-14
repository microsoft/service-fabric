// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComProxyCodePackageHost
    {
        DENY_COPY(ComProxyCodePackageHost);

    public:
        ComProxyCodePackageHost(Common::ComPointer<IFabricCodePackageHost> const & codePackageHost);
        ~ComProxyCodePackageHost();

        Common::AsyncOperationSPtr BeginActivate(
            std::wstring const & codePackageId,
            Common::ComPointer<IFabricCodePackageActivationContext> const & activationContext,
            Common::ComPointer<IFabricRuntime> const & fabricRuntime,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginDeactivate(
            std::wstring const & codePackageId,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivate(Common::AsyncOperationSPtr const & asyncOperation);

        _declspec(property(get=get_CodePackageHost)) Common::ComPointer<IFabricCodePackageHost> const & CodePackageHost;
        inline Common::ComPointer<IFabricCodePackageHost> const & get_CodePackageHost() const { return codePackageHost_; };

    private:
        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;

    private:
        Common::ComPointer<IFabricCodePackageHost> codePackageHost_;
    };
}
