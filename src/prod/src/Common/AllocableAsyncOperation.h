// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AllocableAsyncOperation : public AsyncOperation, public AllocableObject
    {
        DENY_COPY(AllocableAsyncOperation)
    public:
        AllocableAsyncOperation(
            AsyncCallback const& callback,
            AsyncOperationSPtr const& parent)
            : AsyncOperation(callback, parent)
        {
        }

		template<typename TAsyncOperation, class ...Args>
		static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Args&& ...args)
		{
			AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(new(allocator)TAsyncOperation(std::forward<Args>(args)...));
			asyncOperation->Start(asyncOperation);
			
			return asyncOperation;
		}
    };
}
