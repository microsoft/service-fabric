// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::ChangeRoleAsyncOperation : public Common::AsyncOperation
    {
    public:
        ChangeRoleAsyncOperation(
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
            , newRole_(newRole)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &, __out ::FABRIC_REPLICA_ROLE &);

    protected:
        void OnStart(Common::AsyncOperationSPtr const &)
        {
            // intentional no-op: externally completed by ReplicatedStore on state change
        }

    private:
        ::FABRIC_REPLICA_ROLE newRole_;
    };
}
