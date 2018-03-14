// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class PartitionInfoMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            PartitionInfoMessageBody();
            PartitionInfoMessageBody(wstring serviceName, Common::Guid partitionId);
            ~PartitionInfoMessageBody();

            __declspec(property(get = get_ServiceName, put = set_ServiceName)) wstring ServiceName;
            wstring get_ServiceName() const { return serviceName_; }
            void set_ServiceName(wstring value) { serviceName_ = value; }

            __declspec(property(get = get_PartitionId, put = set_PartitionId)) Common::Guid PartitionId;
            Common::Guid get_PartitionId() const { return partitionId_; }
            void set_PartitionId(Common::Guid value) { partitionId_ = value; }

            FABRIC_FIELDS_02(
                serviceName_,
                partitionId_);
        private:
            wstring serviceName_;
            Common::Guid partitionId_;
        };
    }
}

