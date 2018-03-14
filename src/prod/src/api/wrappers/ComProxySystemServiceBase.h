// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
	template<class TComInterface>
    class ComProxySystemServiceBase : public Common::ComponentRoot
    {
        DENY_COPY(ComProxySystemServiceBase<TComInterface>);

    protected:
		ComProxySystemServiceBase(Common::ComPointer<TComInterface> const & comImpl);

		virtual ~ComProxySystemServiceBase();

        // SystemServiceCall
        Common::AsyncOperationSPtr BeginCallSystemServiceInternal(
            std::wstring const & action,
            std::wstring const & inputBlob,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCallSystemServiceInternal(
            Common::AsyncOperationSPtr const & asyncOperation,
            __inout std::wstring & outputBlob);

    private:
        class CallSystemServiceOperation;

	protected:
        Common::ComPointer<TComInterface> const comImpl_;
    };
}

#include "ComProxySystemServiceBase.cpp"
