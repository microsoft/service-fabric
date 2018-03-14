// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupScheduleRuntimeList : public Serialization::FabricSerializable
        {
        public:
            BackupScheduleRuntimeList();
            BackupScheduleRuntimeList(vector<int64> runTimes);
            ~BackupScheduleRuntimeList();

            __declspec(property(get = get_RunTimes, put = set_RunTimes)) vector<int64> RunTimes;
            vector<int64> get_RunTimes() const { return runTimes_; }
            void set_RunTimes(vector<int64> value) { runTimes_ = value; }

            FABRIC_FIELDS_01(runTimes_);
        private:
            vector<int64> runTimes_;
        };
    }
}
