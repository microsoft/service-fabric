// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class UpdateRepairTaskHealthPolicyAsyncOperation : public ClientRequestAsyncOperation
        {
        public:
            UpdateRepairTaskHealthPolicyAsyncOperation(
                __in RepairManagerServiceReplica &,
                Transport::MessageUPtr &&,
                Transport::IpcReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            UpdateRepairTaskHealthPolicyAsyncOperation(
                __in RepairManagerServiceReplica &,
                UpdateRepairTaskHealthPolicyMessageBody &&,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

        protected:
            Common::AsyncOperationSPtr BeginAcceptRequest(
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) override;
            Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &) override;

        private:
            UpdateRepairTaskHealthPolicyMessageBody body_;
        };
    }
}
