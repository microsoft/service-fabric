// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    template <class TSpecification>
    class UpgradeRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        UpgradeRequestMessageBody()
            : isRollback_(false)
        {
        }

        UpgradeRequestMessageBody(
            TSpecification && specification,
            Common::TimeSpan upgradeReplicaSetCheckTimeout,
            std::vector<std::wstring> && verifiedUpgradeDomains,
            int64 sequenceNumber,
            bool isRollback)
            : specification_(std::move(specification)),
              upgradeReplicaSetCheckTimeout_(upgradeReplicaSetCheckTimeout),
              verifiedUpgradeDomains_(std::move(verifiedUpgradeDomains)),
              sequenceNumber_(sequenceNumber),
              isRollback_(isRollback)
        {
        }

        __declspec (property(get=get_Specification)) TSpecification const & Specification;
        TSpecification const & get_Specification() const { return specification_; }

        __declspec (property(get=get_MutableSpecification)) TSpecification & MutableSpecification;
        TSpecification & get_MutableSpecification() { return specification_; }

        __declspec (property(get=get_UpgradeReplicaSetCheckTimeout)) Common::TimeSpan const & UpgradeReplicaSetCheckTimeout;
        Common::TimeSpan const & get_UpgradeReplicaSetCheckTimeout() const { return upgradeReplicaSetCheckTimeout_; }
        void SetReplicaSetCheckTimeout(Common::TimeSpan value) { upgradeReplicaSetCheckTimeout_ = value; }

        __declspec (property(get=get_VerifiedUpgradeDomains)) std::vector<std::wstring> const & VerifiedUpgradeDomains;
        std::vector<std::wstring> const & get_VerifiedUpgradeDomains() const { return verifiedUpgradeDomains_; }

        __declspec (property(get = get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }
        void SetSequenceNumber(int64 value) { sequenceNumber_ = value; }

        __declspec(property(get = get_IsRollback)) bool IsRollback;
        bool get_IsRollback() const { return isRollback_; }

        void UpdateVerifiedUpgradeDomains(std::vector<std::wstring> && upgradeDomains) 
        { 
            verifiedUpgradeDomains_ = std::move(upgradeDomains); 
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}, checkTimeout={1}, verified domains={2}, SeqNum={3}, IsRollback={4}",
                specification_, upgradeReplicaSetCheckTimeout_, verifiedUpgradeDomains_, sequenceNumber_, isRollback_);
        }

        FABRIC_FIELDS_05(
            specification_,
            upgradeReplicaSetCheckTimeout_,
            verifiedUpgradeDomains_,
            sequenceNumber_,
            isRollback_);

    private:
        TSpecification specification_;
        Common::TimeSpan upgradeReplicaSetCheckTimeout_;
        std::vector<std::wstring> verifiedUpgradeDomains_;
        int64 sequenceNumber_;
        bool isRollback_;
    };

    typedef UpgradeRequestMessageBody<ServiceModel::ApplicationUpgradeSpecification> UpgradeApplicationRequestMessageBody;
    typedef UpgradeRequestMessageBody<ServiceModel::FabricUpgradeSpecification> UpgradeFabricRequestMessageBody;
}
