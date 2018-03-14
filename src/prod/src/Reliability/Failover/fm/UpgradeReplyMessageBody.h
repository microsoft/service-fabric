// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpgradeReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        UpgradeReplyMessageBody()
            : errorCodeValue_(Common::ErrorCodeValue::Success)
        {
        }

        __declspec (property(get=get_ErrorCodeValue, put=set_ErrorCodeValue)) Common::ErrorCodeValue::Enum ErrorCodeValue;
        Common::ErrorCodeValue::Enum get_ErrorCodeValue() const { return errorCodeValue_; }
        void set_ErrorCodeValue(Common::ErrorCodeValue::Enum error) { errorCodeValue_ = error; }

        __declspec (property(get=get_CompletedUpgradeDomains)) std::vector<std::wstring> & CompletedUpgradeDomains;
        std::vector<std::wstring> & get_CompletedUpgradeDomains() { return completedUpgradeDomains_; }

        __declspec (property(get=get_PendingUpgradeDomains)) std::vector<std::wstring> & PendingUpgradeDomains;
        std::vector<std::wstring> & get_PendingUpgradeDomains() { return pendingUpgradeDomains_; }

        __declspec (property(get = get_SkippedUpgradeDomains)) std::vector<std::wstring> & SkippedUpgradeDomains;
        std::vector<std::wstring> & get_SkippedUpgradeDomains() { return skippedUpgradeDomains_; }

        __declspec (property(get = get_UpgradeDomainProgress)) UpgradeDomainProgress CurrentUpgradeDomainProgress;
        UpgradeDomainProgress & get_UpgradeDomainProgress() { return currentUpgradeDomainProgress_; }

        void SetCompletedUpgradeDomains(std::vector<std::wstring> && completedUpgradeDomains)
        {
            completedUpgradeDomains_ = std::move(completedUpgradeDomains);
        }

        void SetPendingUpgradeDomain(std::vector<std::wstring> && pendingUpgradeDomains)
        {
            pendingUpgradeDomains_ = std::move(pendingUpgradeDomains);
        }

        void SetSkippedUpgradeDomains(std::vector<std::wstring> const& skippedUpgradeDomains)
        {
            skippedUpgradeDomains_ = skippedUpgradeDomains;
        }

        void SetUpgradeProgressDetails(UpgradeDomainProgress && currentUpgradeDomainProgress)
        {
            currentUpgradeDomainProgress_ = std::move(currentUpgradeDomainProgress);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("Error = {0}", errorCodeValue_);

            w.Write("Completed = [ ");
            for (size_t i = 0; i < completedUpgradeDomains_.size(); i++)
            {
                w << completedUpgradeDomains_[i] << ' ';
            }
            w.WriteLine("] ");

            w.Write("Pending = [ ");
            for (size_t i = 0; i < pendingUpgradeDomains_.size(); i++)
            {
                w << pendingUpgradeDomains_[i] << ' ';
            }
            w.WriteLine("] ");

            w.Write("Skipped = [ ");
            for (size_t i = 0; i < skippedUpgradeDomains_.size(); i++)
            {
                w << skippedUpgradeDomains_[i] << ' ';
            }
            w.WriteLine("] ");

            w.WriteLine(currentUpgradeDomainProgress_);
        }

        FABRIC_FIELDS_05(completedUpgradeDomains_, pendingUpgradeDomains_, errorCodeValue_, currentUpgradeDomainProgress_, skippedUpgradeDomains_);

    private:
        std::vector<std::wstring> completedUpgradeDomains_;
        std::vector<std::wstring> pendingUpgradeDomains_;
        std::vector<std::wstring> skippedUpgradeDomains_;
        Common::ErrorCodeValue::Enum errorCodeValue_;

        UpgradeDomainProgress currentUpgradeDomainProgress_;
    };
}
