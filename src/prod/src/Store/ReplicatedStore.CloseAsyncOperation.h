// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::CloseAsyncOperation : public Common::AsyncOperation
    {
    public:
        CloseAsyncOperation(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
        {
            return Common::AsyncOperation::End<CloseAsyncOperation>(operation)->Error;
        }

    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const &)
        {
            // intentional no-op: externally completed by ReplicatedStore on state change
        }

    };
}
