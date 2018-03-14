// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CommonUpgradeContextData : public  Serialization::FabricSerializable
        {
            DENY_COPY(CommonUpgradeContextData)

        public:
            CommonUpgradeContextData();

            CommonUpgradeContextData(CommonUpgradeContextData &&);

            CommonUpgradeContextData & operator=(CommonUpgradeContextData &&);

            __declspec(property(get=get_IsPostUpgradeHealthCheckComplete,put=set_IsPostUpgradeHealthCheckComplete)) bool IsPostUpgradeHealthCheckComplete;
            __declspec(property(get=get_StartTime,put=set_StartTime)) Common::DateTime StartTime;
            __declspec(property(get=get_FailureTime,put=set_FailureTime)) Common::DateTime FailureTime;
            __declspec(property(get=get_FailureReason,put=set_FailureReason)) UpgradeFailureReason::Enum FailureReason;
            __declspec(property(get=get_UpgradeProgressAtFailure,put=set_UpgradeProgressAtFailure)) Reliability::UpgradeDomainProgress const & UpgradeProgressAtFailure;
            __declspec(property(get=get_UpgradeStatusDetails, put=set_UpgradeStatusDetails)) std::wstring const & UpgradeStatusDetails;
            __declspec(property(get=get_UpgradeStatusDetailsAtFailure, put=set_UpgradeStatusDetailsAtFailure)) std::wstring const & UpgradeStatusDetailsAtFailure;
            __declspec(property(get=get_IsRollbackAllowed, put=set_IsRollbackAllowed)) bool const IsRollbackAllowed;
            __declspec(property(get=get_IsSkipRollbackUD, put=set_IsSkipRollbackUD)) bool const IsSkipRollbackUD;

            bool get_IsPostUpgradeHealthCheckComplete() const { return isPostUpgradeHealthCheckComplete_; }
            Common::DateTime get_StartTime() const { return startTime_; }
            Common::DateTime get_FailureTime() const { return failureTime_; }
            UpgradeFailureReason::Enum get_FailureReason() const { return failureReason_; }
            Reliability::UpgradeDomainProgress const & get_UpgradeProgressAtFailure() const { return upgradeProgressAtFailure_; }
            std::wstring const & get_UpgradeStatusDetails() const { return upgradeStatusDetails_; }
            std::wstring const & get_UpgradeStatusDetailsAtFailure() const { return upgradeStatusDetailsAtFailure_; }
            bool const get_IsRollbackAllowed() const { return isRollbackAllowed_; }
            bool const get_IsSkipRollbackUD() const { return isSkipRollbackUD_; }

            void set_IsPostUpgradeHealthCheckComplete(bool value) { isPostUpgradeHealthCheckComplete_ = value; }
            void set_StartTime(Common::DateTime value) { startTime_ = value; }
            void set_FailureTime(Common::DateTime value) { failureTime_ = value; }
            void set_FailureReason(UpgradeFailureReason::Enum value) { failureReason_ = value; }
            void set_UpgradeProgressAtFailure(Reliability::UpgradeDomainProgress const & value) { upgradeProgressAtFailure_ = value; }
            void set_UpgradeStatusDetails(std::wstring const & value) { upgradeStatusDetails_ = value; }
            void set_UpgradeStatusDetailsAtFailure(std::wstring const & value) { upgradeStatusDetailsAtFailure_ = value; }
            void set_IsRollbackAllowed(bool const value) { isRollbackAllowed_ = value; }
            void set_IsSkipRollbackUD(bool const value) { isSkipRollbackUD_ = value; }

            void Reset();
            void ClearFailureData();

            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_09(
                isPostUpgradeHealthCheckComplete_, 
                startTime_, 
                failureTime_, 
                failureReason_, 
                upgradeProgressAtFailure_,
                upgradeStatusDetails_,
                upgradeStatusDetailsAtFailure_,
                isRollbackAllowed_,
                isSkipRollbackUD_);

        private:
            bool isPostUpgradeHealthCheckComplete_;
            Common::DateTime startTime_;
            Common::DateTime failureTime_;
            UpgradeFailureReason::Enum failureReason_;
            Reliability::UpgradeDomainProgress upgradeProgressAtFailure_;

            // Miscellaneous status details presented to user in upgrade status query. 
            // Concatenates snapshot of failure status details with dynamically 
            // changing details during upgrade.
            //
            std::wstring upgradeStatusDetails_;             

            // Snapshot of status details at the time of upgrade failure.
            // Preserves information about the original upgrade failure to be included
            // with dynamic status details.
            //
            std::wstring upgradeStatusDetailsAtFailure_;    

            bool isRollbackAllowed_;
            bool isSkipRollbackUD_; // flags that this rollback skips deleting default service and rollback UD and performs rollback updated default services directly;

        };
    }
}
