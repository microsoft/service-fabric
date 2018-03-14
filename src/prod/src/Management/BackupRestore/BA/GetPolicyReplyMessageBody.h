// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class GetPolicyReplyMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            GetPolicyReplyMessageBody();
            GetPolicyReplyMessageBody(BackupPolicy policy);
            ~GetPolicyReplyMessageBody();

            __declspec(property(get = get_Policy, put = set_Policy)) BackupPolicy Policy;
            BackupPolicy get_Policy() const { return policy_; }
            void set_Policy(BackupPolicy value) { policy_ = value; }

            FABRIC_FIELDS_01(
                policy_);
        private:
            BackupPolicy policy_;
        };
    }
}

