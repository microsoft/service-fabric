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

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                std::forward<Arg0>(arg0),
                std::forward<Arg1>(arg1),
                std::forward<Arg2>(arg2)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2, class Arg3>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                std::forward<Arg0>(arg0),
                std::forward<Arg1>(arg1),
                std::forward<Arg2>(arg2),
                std::forward<Arg3>(arg3)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                std::forward<Arg0>(arg0),
                std::forward<Arg1>(arg1),
                std::forward<Arg2>(arg2),
                std::forward<Arg3>(arg3),
                std::forward<Arg4>(arg4)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                std::forward<Arg0>(arg0),
                std::forward<Arg1>(arg1),
                std::forward<Arg2>(arg2),
                std::forward<Arg3>(arg3),
                std::forward<Arg4>(arg4),
                std::forward<Arg5>(arg5)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                std::forward<Arg0>(arg0),
                std::forward<Arg1>(arg1),
                std::forward<Arg2>(arg2),
                std::forward<Arg3>(arg3),
                std::forward<Arg4>(arg4),
                std::forward<Arg5>(arg5),
                std::forward<Arg6>(arg6)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

        template<typename TAsyncOperation, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
        static AsyncOperationSPtr CreateAndStart(KAllocator &allocator, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
        {
            AsyncOperationSPtr asyncOperation = std::shared_ptr<TAsyncOperation>(
                new(allocator)TAsyncOperation(
                    std::forward<Arg0>(arg0),
                    std::forward<Arg1>(arg1),
                    std::forward<Arg2>(arg2),
                    std::forward<Arg3>(arg3),
                    std::forward<Arg4>(arg4),
                    std::forward<Arg5>(arg5),
                    std::forward<Arg6>(arg6),
                    std::forward<Arg7>(arg7)));
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }
    };
}
