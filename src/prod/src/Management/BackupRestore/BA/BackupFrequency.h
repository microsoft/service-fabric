// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupFrequency : public Serialization::FabricSerializable
        {
        public:
            BackupFrequency();
            ~BackupFrequency();

            __declspec(property(get = get_Type, put = set_Type)) BackupPolicyRunFrequencyMode::Enum Type;
            BackupPolicyRunFrequencyMode::Enum get_Type() const { return type_; }
            void set_Type(BackupPolicyRunFrequencyMode::Enum value) { type_ = value; }

            __declspec(property(get = get_Value, put = set_Value)) uint16 Value;
            uint16 get_Value() const { return value_; }
            void set_value(uint16 value) { value_ = value; }

            FABRIC_FIELDS_02(
                type_,
                value_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_FREQUENCY_BASED_BACKUP_POLICY & fabricBackupFrequency) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_FREQUENCY_BASED_BACKUP_POLICY const & fabricBackupFrequency);

        private:
            BackupPolicyRunFrequencyMode::Enum type_;
            uint16 value_;
        };
    }
}
