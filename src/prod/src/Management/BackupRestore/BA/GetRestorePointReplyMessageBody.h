// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class GetRestorePointReplyMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            GetRestorePointReplyMessageBody();
            GetRestorePointReplyMessageBody(RestorePointDetails restorePoint);
            ~GetRestorePointReplyMessageBody();

            __declspec(property(get = get_RestorePoint, put = set_RestorePoint)) RestorePointDetails RestorePoint;
            RestorePointDetails get_RestorePoint() const { return restorePoint_; }
            void set_RestorePoint(RestorePointDetails value) { restorePoint_ = value; }

            FABRIC_FIELDS_01(
                restorePoint_);
        private:
            RestorePointDetails restorePoint_;
        };
    }
}

