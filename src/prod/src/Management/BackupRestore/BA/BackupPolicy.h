// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupPolicy : public Serialization::FabricSerializable
        {
        public:
            BackupPolicy();
            ~BackupPolicy();

            __declspec(property(get = get_Name, put = set_Name)) wstring Name;
            wstring get_Name() const { return name_; }
            void set_Name(wstring value) { name_ = value; }

            __declspec(property(get = get_PolicyId, put = set_PolicyId)) GUID PolicyId;
            GUID get_PolicyId() const { return policyId_; }
            void set_PolicyId(GUID value) { policyId_ = value; }

            __declspec(property(get = get_PolicyType, put = set_PolicyType)) BackupPolicyType::Enum PolicyType;
            BackupPolicyType::Enum get_PolicyType() const { return policyType_; }
            void set_PolicyType(BackupPolicyType::Enum value) { policyType_ = value; }

            __declspec(property(get = get_MaxIncrementalBackups, put = set_MaxIncrementalBackups)) uint MaxIncrementalBackups;
            uint get_MaxIncrementalBackups() const { return maxIncrementalBackups_; }
            void set_MaxIncrementalBackups(uint value) { maxIncrementalBackups_ = value; }

            __declspec(property(get = get_BackupStoreInfo, put = set_BackupStoreInfo)) BackupRestoreAgentComponent::BackupStoreInfo BackupStoreInfo;
            BackupRestoreAgentComponent::BackupStoreInfo get_BackupStoreInfo() const { return backupStoreInfo_; }
            void set_BackupStoreInfo(BackupRestoreAgentComponent::BackupStoreInfo value) { backupStoreInfo_ = value; }

            __declspec(property(get = get_BackupSchedule, put = set_BackupSchedule)) BackupRestoreAgentComponent::BackupSchedule BackupSchedule;
            BackupRestoreAgentComponent::BackupSchedule get_BackupSchedule() const { return backupSchedule_; }
            void set_BackupSchedule(BackupRestoreAgentComponent::BackupSchedule value) { backupSchedule_ = value; }

            __declspec(property(get = get_BackupFrequency, put = set_BackupFrequency)) BackupRestoreAgentComponent::BackupFrequency BackupFrequency;
            BackupRestoreAgentComponent::BackupFrequency get_BackupFrequency() const { return backupFrequency_; }
            void set_BackupFrequency(BackupRestoreAgentComponent::BackupFrequency value) { backupFrequency_ = value; }

            FABRIC_FIELDS_07(
                name_,
                policyId_,
                policyType_,
                maxIncrementalBackups_,
                backupStoreInfo_,
                backupSchedule_,
                backupFrequency_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_POLICY & fabricBackupPolicy) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_POLICY const & fabricBackupPolicy);

        private:
            wstring name_;
            GUID policyId_;
            BackupPolicyType::Enum policyType_;
            
            uint maxIncrementalBackups_;
            BackupRestoreAgentComponent::BackupStoreInfo backupStoreInfo_;

            // Schedule based policy.
            BackupRestoreAgentComponent::BackupSchedule backupSchedule_;

            // Frequency based policy fields.
            BackupRestoreAgentComponent::BackupFrequency backupFrequency_;
        };
    }
}
