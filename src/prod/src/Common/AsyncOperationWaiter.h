// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncOperationWaiter : public Common::ManualResetEvent
    {
        DENY_COPY(AsyncOperationWaiter)

    public:
        AsyncOperationWaiter()
            : Common::ManualResetEvent(false)
            , error_(ErrorCodeValue::Success)
        {
        }

        virtual ~AsyncOperationWaiter() { }

        inline ErrorCode & GetError() { return error_; }
        inline void SetError(ErrorCode& error) { error_ = error; }
        inline void SetError(ErrorCode&& error) { error_ = std::move(error); }

    private:
        ErrorCode error_;
    };

    typedef std::shared_ptr<AsyncOperationWaiter> AsyncOperationWaiterSPtr;
}
