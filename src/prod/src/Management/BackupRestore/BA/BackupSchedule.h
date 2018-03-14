// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupSchedule : public Serialization::FabricSerializable
        {
        public:
            BackupSchedule();
            ~BackupSchedule();

            __declspec(property(get = get_Type, put = set_Type)) BackupPolicyRunScheduleMode::Enum Type;
            BackupPolicyRunScheduleMode::Enum get_Type() const { return type_; }
            void set_Type(BackupPolicyRunScheduleMode::Enum value) { type_ = value; }

            __declspec(property(get = get_RunTimes, put = set_RunTimes)) vector<DWORD> RunTimes;
            vector<DWORD> get_RunTimes() const { return runTimes_; }
            void set_RunTimes(vector<DWORD> value) { runTimes_ = value; }

            __declspec(property(get = get_RunDays, put = set_RunDays)) byte RunDays;
            byte get_RunDays() const { return runDays_; }
            void set_RunDays(byte value) { runDays_ = value; }

            FABRIC_FIELDS_03(
                type_,
                runTimes_,
                runDays_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_SCHEDULE_BASED_BACKUP_POLICY & fabricBackupSchedule) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_SCHEDULE_BASED_BACKUP_POLICY const & fabricBackupSchedule);

        private:
            BackupPolicyRunScheduleMode::Enum type_;
            vector<DWORD> runTimes_;
            byte runDays_;
        };
    }
}
