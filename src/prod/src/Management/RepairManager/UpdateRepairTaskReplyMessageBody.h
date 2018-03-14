// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace RepairManager
    {
        class UpdateRepairTaskReplyMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateRepairTaskReplyMessageBody() : commitVersion_(0) { }

            UpdateRepairTaskReplyMessageBody(int64 commitVersion) : commitVersion_(commitVersion) { }

            __declspec(property(get=get_CommitVersion)) int64 CommitVersion;

            int64 get_CommitVersion() { return commitVersion_; }

            FABRIC_FIELDS_01(commitVersion_);

        private:
            int64 commitVersion_;
        };
    }
}
