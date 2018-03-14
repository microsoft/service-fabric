// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class NodeDescription
        {
            DENY_COPY_ASSIGNMENT(NodeDescription);

        public:
            typedef std::vector<std::wstring> DomainId;
            static Common::GlobalWString const FormatHeader;
            NodeDescription(
                Federation::NodeInstance nodeInstance,
                bool isUp,
                Reliability::NodeDeactivationIntent::Enum deactivationIntent,
                Reliability::NodeDeactivationStatus::Enum deactivationStatus,
                std::map<std::wstring, std::wstring> && nodeProperties,
                DomainId && faultDomainId,
                std::wstring && upgradeDomainId,
                std::map<std::wstring, uint> && capacityRatios, 
                std::map<std::wstring, uint> && capacities);

            NodeDescription(NodeDescription && other);

            NodeDescription(NodeDescription const & other);

            NodeDescription & operator = (NodeDescription && other);

            __declspec (property(get=get_NodeInstance)) Federation::NodeInstance NodeInstance;
            Federation::NodeInstance get_NodeInstance() const { return nodeInstance_; }

            __declspec (property(get = get_NodeName)) std::wstring const NodeName;
            std::wstring const get_NodeName() const { return nodeProperties_.count(L"NodeName") ? nodeProperties_.find(L"NodeName")->second : Federation::NodeIdGenerator::GenerateNodeName(NodeInstance.Id); }

            __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
            Federation::NodeId get_NodeId() const { return nodeInstance_.Id; }

            __declspec (property(get=get_IsUpAndActivated)) bool IsUpAndActivated;
            bool get_IsUpAndActivated() const { return isUp_ && deactivationIntent_ == NodeDeactivationIntent::None; }

            __declspec (property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const { return isUp_; }

            __declspec (property(get = get_IsPaused)) bool IsPaused;
            bool get_IsPaused() const { return isUp_ && deactivationIntent_ == NodeDeactivationIntent::Pause; }

            __declspec (property(get = get_IsRestart)) bool IsRestart;
            bool get_IsRestart() const { return isUp_ && deactivationIntent_ == NodeDeactivationIntent::Restart; }

            __declspec (property(get = get_IsPause)) bool IsPause;
            bool get_IsPause() const { return isUp_ && deactivationIntent_ == NodeDeactivationIntent::Pause; }

            __declspec (property(get = get_IsRemove)) bool IsRemove;
            bool get_IsRemove() const { return isUp_ && deactivationIntent_ == NodeDeactivationIntent::RemoveData || deactivationIntent_ == NodeDeactivationIntent::RemoveNode; }

            __declspec (property(get = get_AreSafetyChecksInProgress)) bool AreSafetyChecksInProgress;
            bool get_AreSafetyChecksInProgress() const { return deactivationStatus_ == NodeDeactivationStatus::DeactivationSafetyCheckInProgress; }

            __declspec (property(get = get_AreSafetyChecksCompleted)) bool AreSafetyChecksCompleted;
            bool get_AreSafetyChecksCompleted() const { return deactivationStatus_ == NodeDeactivationStatus::DeactivationSafetyCheckComplete; }

            __declspec (property(get = get_IsCapacityAvailable)) bool IsCapacityAvailable;
            bool get_IsCapacityAvailable() const
            {
                return isUp_ && !(deactivationStatus_ == NodeDeactivationStatus::DeactivationComplete
                    && (deactivationIntent_ == NodeDeactivationIntent::RemoveData || deactivationIntent_ == NodeDeactivationIntent::RemoveNode));
            }

            __declspec (property(get = get_IsDeactivated)) bool IsDeactivated;
            bool get_IsDeactivated() const { return deactivationIntent_ != NodeDeactivationIntent::None; }

            __declspec (property(get = get_IsRestartOrPauseCheckInProgress)) bool IsRestartOrPauseCheckInProgress;
            bool get_IsRestartOrPauseCheckInProgress() const { return AreSafetyChecksInProgress && (IsRestart || IsPause); }

            __declspec (property(get = get_IsRestartOrPauseCheckCompleted)) bool IsRestartOrPauseCheckCompleted;
            bool get_IsRestartOrPauseCheckCompleted() const { return AreSafetyChecksCompleted && (IsRestart || IsPause); }

            __declspec (property(get = get_DeactivationIntent)) Reliability::NodeDeactivationIntent::Enum DeactivationIntent;
            Reliability::NodeDeactivationIntent::Enum get_DeactivationIntent() const { return deactivationIntent_; }

            __declspec (property(get = get_DeactivationStatus)) Reliability::NodeDeactivationStatus::Enum DeactivationStatus;
            Reliability::NodeDeactivationStatus::Enum get_DeactivationStatus() const { return deactivationStatus_; }

            __declspec (property(get=get_Properties)) std::map<std::wstring, std::wstring> const& NodeProperties;
            std::map<std::wstring, std::wstring> const& get_Properties() const { return nodeProperties_; }

            __declspec (property(get=get_FaultDomainId)) DomainId const& FaultDomainId;
            DomainId const& get_FaultDomainId() const { return faultDomainId_; }

            __declspec (property(get=get_UpgradeDomainId)) std::wstring const& UpgradeDomainId;
            std::wstring const& get_UpgradeDomainId() const { return upgradeDomainId_; }

            __declspec (property(get=get_CapacityRatios)) std::map<std::wstring, uint> const& CapacityRatios;
            std::map<std::wstring, uint> const& get_CapacityRatios() const { return capacityRatios_; }

            __declspec (property(get=get_Capacities)) std::map<std::wstring, uint> const& Capacities;
            std::map<std::wstring, uint> const& get_Capacities() const { return capacities_; }

            __declspec(property(get = get_NodeIndex, put = put_NodeIndex)) uint64 NodeIndex;
            uint64 get_NodeIndex() const { return nodeIndex_; }
            void put_NodeIndex(uint64 nodeIndex) { nodeIndex_ = nodeIndex; }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

            void WriteToEtw(uint16 contextSequenceId) const;

            void CorrectCpuCapacity();

        private:
            Federation::NodeInstance nodeInstance_;
            bool isUp_;
            Reliability::NodeDeactivationIntent::Enum deactivationIntent_;
            Reliability::NodeDeactivationStatus::Enum deactivationStatus_;
            std::map<std::wstring, std::wstring> nodeProperties_;
            DomainId faultDomainId_;
            std::wstring upgradeDomainId_;
            std::map<std::wstring, uint> capacityRatios_;
            std::map<std::wstring, uint> capacities_;
            uint64 nodeIndex_;
        };
    }
}
