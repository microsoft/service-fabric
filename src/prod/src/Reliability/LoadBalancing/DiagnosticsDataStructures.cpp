// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DiagnosticsDataStructures.h"
#include "PLBConfig.h"
#include "Node.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;
using namespace ServiceModel;

void DecisionToken::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->DecisionToken1(contextSequenceId,
                                                        decisionId_,
                                                        reasonToken_,
                                                        CheckToken(SchedulerStage::Stage::ClientAPICall),
                                                        CheckToken(SchedulerStage::Stage::Skip),
                                                        CheckToken(SchedulerStage::Stage::NoneExpired),
                                                        CheckToken(SchedulerStage::Stage::Placement),
                                                        CheckToken(SchedulerStage::Stage::Balancing),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck));

    PlacementAndLoadBalancing::PLBTrace->DecisionToken2(contextSequenceId,
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ReplicaExclusionStatic),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ReplicaExclusionDynamic),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::PlacementConstraint),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::NodeCapacity),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::Affinity),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::FaultDomain),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::UpgradeDomain),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::PreferredLocation),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ScaleoutCount),
                                                        CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ApplicationCapacity));
}

std::wstring DecisionToken::ToString() const
{
    std::wstring flagString;
    StringWriter writer(flagString);

    std::wstring abbrevCons[10] = {L"RxS", L"Rxd", L"PC", L"NCap", L"Aff", L"FD", L"UD", L"PLoc", L"Scl", L"ACap"};

    writer.Write("{0} | ", decisionId_);

    auto s = GetStage();

    switch(s)
    {
        case SchedulerStage::Stage::ClientAPICall:
            writer.Write("API");
            break;
        case SchedulerStage::Stage::Skip:
            writer.Write("Skip");
            break;
        case SchedulerStage::Stage::NoneExpired:
            writer.Write("NExp");
            break;
        case SchedulerStage::Stage::Placement:
            writer.Write("Place");
            break;
        case SchedulerStage::Stage::Balancing:
            writer.Write("Bal");
            break;
        case SchedulerStage::Stage::ConstraintCheck:
            writer.Write("Cons");

            for (int c = 0; c < 10; ++c)
            {
                if (CheckToken(SchedulerStage::Stage::ConstraintCheck, static_cast<IConstraint::Enum>(c)))
                {
                    writer.Write(" | {0}", abbrevCons[c]);
                }
            }

            break;
        default:
        Common::Assert::TestAssert("Invalid value for scheduler stage {0}", static_cast<int64>(s));
    }

    return flagString;
};

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & w, DecisionToken const & val)
{
    w << wformatString("DecisionId: {0}\r\nTokenValue: {1}\r\nClientAPICall {2} \r\nSkip {3}\r\nNoneExpired {4}\r\nPlacement {5}\r\nBalancing {6}\r\nConstraintCheck {7}",
        val.DecisionId, val.ReasonToken, val.CheckToken(SchedulerStage::Stage::ClientAPICall),
        val.CheckToken(SchedulerStage::Stage::Skip), val.CheckToken(SchedulerStage::Stage::NoneExpired),
        val.CheckToken(SchedulerStage::Stage::Placement), val.CheckToken(SchedulerStage::Stage::Balancing), val.CheckToken(SchedulerStage::Stage::ConstraintCheck));

    w << wformatString("\r\nReplicaExclusionStatic {0}\r\nReplicaExclusionDynamic {1}\r\nPlacementConstraint {2}\r\nNodeCapacity {3}\r\nAffinity {4}\r\nFaultDomain {5}\r\nUpgradeDomain {6}\r\nPreferredLocation {7}\r\nScaleoutCount {8}\r\nApplicationCapacity {9}",
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ReplicaExclusionStatic),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ReplicaExclusionDynamic),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::PlacementConstraint),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::NodeCapacity),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::Affinity),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::FaultDomain),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::UpgradeDomain),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::PreferredLocation),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ScaleoutCount),
        val.CheckToken(SchedulerStage::Stage::ConstraintCheck, IConstraint::ApplicationCapacity));
}

void DecisionToken::SetToken(Common::Guid const & id, SchedulerStage::Stage stage, std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & data)
{
    decisionId_ = id;
    reasonToken_ = 0;

    switch(stage)
    {
        case SchedulerStage::Stage::ClientAPICall:
            reasonToken_ = 1 << 0;
            break;
        case SchedulerStage::Stage::Skip:
            reasonToken_ = 1 << 1;
            break;
        case SchedulerStage::Stage::NoneExpired:
            reasonToken_ = 1 << 2;
            break;
        case SchedulerStage::Stage::Placement:
            reasonToken_ = 1 << 3;
            break;
        case SchedulerStage::Stage::Balancing:
            reasonToken_ = 1 << 4;
            break;
        case SchedulerStage::Stage::ConstraintCheck:
            reasonToken_ = 1 << 9;

            for (auto d : data)
            {
                reasonToken_ |= (1 << ( 10 + d->constraintType_));
            }

            break;
        default:
            Common::Assert::TestAssert("Invalid value for scheduler stage {0}", static_cast<int64>(stage));
    }
}

bool DecisionToken::CheckToken(SchedulerStage::Stage stage, IConstraint::Enum constraintType) const
{
    TESTASSERT_IF((SchedulerStage::Stage::ConstraintCheck != stage), "Stage and Constraint Type Mismatch");

    bool anyConstraintValid = ((reasonToken_ & (1 << 9)) !=0);
    bool theConstraintValid = ( (reasonToken_ & (1 << (10 + constraintType))) != 0);

    TESTASSERT_IF(theConstraintValid && !anyConstraintValid, "Constraint Validity Mismatch");

    return (anyConstraintValid && theConstraintValid);
}

bool DecisionToken::CheckToken(SchedulerStage::Stage stage) const
{
    switch(stage)
    {
        case SchedulerStage::Stage::ClientAPICall:
            return ((reasonToken_ & (1 << 0)) !=0);
            break;
        case SchedulerStage::Stage::Skip:
            return ((reasonToken_ & (1 << 1)) !=0);
            break;
        case SchedulerStage::Stage::NoneExpired:
            return ((reasonToken_ & (1 << 2)) !=0);
            break;
        case SchedulerStage::Stage::Placement:
            return ((reasonToken_ & (1 << 3)) !=0);
            break;
        case SchedulerStage::Stage::Balancing:
            return ((reasonToken_ & (1 << 4)) !=0);
            break;
        case SchedulerStage::Stage::ConstraintCheck:
            return ((reasonToken_ & (1 << 9)) !=0);
            break;
        default:
        Common::Assert::TestAssert("Invalid value for scheduler stage {0}", static_cast<int64>(stage));
        return false;
    }
}

SchedulerStage::Stage DecisionToken::GetStage() const
{
    auto stageNums = {0, 1, 2, 3, 4, 9};

    for (int s : stageNums)
    {
        if ((reasonToken_ & (1 << s)) != 0)
        {
            switch(s)
            {
                case 0:
                    return SchedulerStage::Stage::ClientAPICall;
                    break;
                case 1:
                    return SchedulerStage::Stage::Skip;
                    break;
                case 2:
                    return SchedulerStage::Stage::NoneExpired;
                    break;
                case 3:
                    return  SchedulerStage::Stage::Placement;
                    break;
                case 4:
                    return SchedulerStage::Stage::Balancing;
                    break;
                case 9:
                    return SchedulerStage::Stage::ConstraintCheck;
                    break;
                default:
                    Common::Assert::TestAssert("Invalid Value of stage {0}", s);
                    return SchedulerStage::Stage::Skip;
            }
        }
    }

    return SchedulerStage::Stage::Skip;
}


BalancingDiagnosticsData::BalancingDiagnosticsData():
    changed_(false),
    metricDiagnostics()
{
}

BalancingDiagnosticsData::BalancingDiagnosticsData(BalancingDiagnosticsData && other):
    changed_(move(other.changed_)),
    metricDiagnostics(move(other.metricDiagnostics))
{

}


BalancingDiagnosticsData & BalancingDiagnosticsData::operator = (BalancingDiagnosticsData && other)
{
    if (this != &other)
    {
        changed_ = std::move(other.changed_);
        metricDiagnostics = std::move(other.metricDiagnostics);
    }

    return *this;
}

MetricCapacityDiagnosticInfo::MetricCapacityDiagnosticInfo():
    metricName_(),
    nodeId_(),
    nodeName_(),
    load_(),
    capacity_(),
    applicationName_()
{
}

MetricCapacityDiagnosticInfo::MetricCapacityDiagnosticInfo(
    std::wstring const & metricName,
    Federation::NodeId const & nodeId,
    std::wstring const & nodeName,
    int64 load,
    int64 capacity,
    std::wstring const & applicationName /* = L"" */):
        metricName_(metricName),
        nodeId_(nodeId),
        nodeName_(nodeName),
        load_(load),
        capacity_(capacity),
        applicationName_(applicationName)
{
}

MetricCapacityDiagnosticInfo::MetricCapacityDiagnosticInfo(MetricCapacityDiagnosticInfo && other):
    metricName_(move(other.metricName_)),
    nodeId_(move(other.nodeId_)),
    nodeName_(move(other.nodeName_)),
    load_(move(other.load_)),
    capacity_(move(other.capacity_)),
    applicationName_(move(other.applicationName_))
{
}


MetricCapacityDiagnosticInfo & MetricCapacityDiagnosticInfo::operator = (MetricCapacityDiagnosticInfo && other)
{
    if (this != &other)
    {
        metricName_= std::move(other.metricName_);
        nodeId_ = std::move(other.nodeId_);
        nodeName_ = std::move(other.nodeName_);
        load_ = std::move(other.load_);
        capacity_ = std::move(other.capacity_);
        applicationName_ = std::move(other.applicationName_);
    }

    return *this;
}

BasicDiagnosticInfo::BasicDiagnosticInfo():
    serviceName_(),
    partitionIds_(),
    nodes_(),
    miscellanious_()
{
}

BasicDiagnosticInfo::BasicDiagnosticInfo(BasicDiagnosticInfo && other):
    serviceName_(move(other.serviceName_)),
    partitionIds_(move(other.partitionIds_)),
    nodes_(move(other.nodes_)),
    miscellanious_(move(other.miscellanious_))
{
}

BasicDiagnosticInfo & BasicDiagnosticInfo::operator = (BasicDiagnosticInfo && other)
{
    if (this != &other)
    {
        serviceName_= std::move(other.serviceName_);
        partitionIds_ = std::move(other.partitionIds_);
        nodes_ = std::move(other.nodes_);
        miscellanious_ = std::move(other.miscellanious_);
    }

    return *this;
}

void BasicDiagnosticInfo::AddPartition(Common::Guid const & partitionId)
{
    if (partitionIds_.size() < PLBConfig::GetConfig().DetailedPartitionListLimit)
    {
        partitionIds_.push_back(partitionId);
    }
}

void BasicDiagnosticInfo::AddNode(Federation::NodeId const & nodeId, std::wstring const & nodeName)
{
    if (nodes_.size() < PLBConfig::GetConfig().DetailedNodeListLimit)
    {
        nodes_.push_back(std::pair<Federation::NodeId, std::wstring>(nodeId, nodeName));
    }
}

std::shared_ptr<IConstraintDiagnosticsData> IConstraintDiagnosticsData::MakeSharedChildOfType(IConstraint::Enum childType)
{
    switch(childType)
    {
        case IConstraint::ReplicaExclusionStatic: return std::make_shared<ReplicaExclusionStaticConstraintDiagnosticsData>();
        case IConstraint::ReplicaExclusionDynamic: return std::make_shared<ReplicaExclusionDynamicConstraintDiagnosticsData>();
        case IConstraint::PlacementConstraint: return std::make_shared<PlacementConstraintDiagnosticsData>();
        case IConstraint::NodeCapacity: return std::make_shared<NodeCapacityConstraintDiagnosticsData>();
        case IConstraint::Affinity: return std::make_shared<AffinityConstraintDiagnosticsData>();
        case IConstraint::FaultDomain: return std::make_shared<FaultDomainConstraintDiagnosticsData>();
        case IConstraint::UpgradeDomain: return std::make_shared<UpgradeDomainConstraintDiagnosticsData>();
        case IConstraint::PreferredLocation: return std::make_shared<PreferredLocationConstraintDiagnosticsData>();
        case IConstraint::ScaleoutCount: return std::make_shared<ScaleoutCountConstraintDiagnosticsData>();
        case IConstraint::ApplicationCapacity: return std::make_shared<ApplicationCapacityConstraintDiagnosticsData>();
        case IConstraint::Throttling: return std::make_shared<ThrottlingConstraintDiagnosticsData>();
        default:
        TESTASSERT_IF(childType != IConstraint::ApplicationCapacity, "Invalid Value of IConstraint::Enum {0}", static_cast<int>(childType));
        return nullptr;
    }
}

std::wstring IConstraintDiagnosticsData::RelevantNodes()
{
    std::wstring nodeString;
    StringWriter writer(nodeString);


    for (auto const & i : basicDiagnosticsInfo_)
    {
        for (auto const & j : i.Nodes)
        {
            writer.Write("Node {0}, {1}, ", j.first, j.second);
        }
    }

    return nodeString;
}

void IConstraintDiagnosticsData::AddBasicDiagnosticsInfo(BasicDiagnosticInfo && info)
{
    if (basicDiagnosticsInfo_.size() < PLBConfig::GetConfig().DetailedDiagnosticsInfoListLimit)
    {
        basicDiagnosticsInfo_.push_back(move(info));
    }
}

void NodeCapacityConstraintDiagnosticsData::AddMetricDiagnosticInfo(MetricCapacityDiagnosticInfo && info)
{
    if (metricDiagnosticsInfo_.size() < PLBConfig::GetConfig().DetailedDiagnosticsInfoListLimit)
    {
        metricDiagnosticsInfo_.push_back(move(info));
    }
}

void ApplicationCapacityConstraintDiagnosticsData::AddMetricDiagnosticInfo(MetricCapacityDiagnosticInfo && info)
{
    if (metricDiagnosticsInfo_.size() < PLBConfig::GetConfig().DetailedDiagnosticsInfoListLimit)
    {
        metricDiagnosticsInfo_.push_back(move(info));
    }
}

bool ServiceDomainSchedulerStageDiagnostics::IsTracingNeeded()
{
    return PLBConfig::GetConfig().TraceCRMReasons && (decisionScheduled_ || !isbalanced_ || !constraintsDiagnosticsData_.empty());
}

bool ServiceDomainSchedulerStageDiagnostics::WhyNot(Common::TextWriter& writer, SchedulerStage::Stage stage) const
{
    bool isValid = false;

    writer.Write("\r\n\t");

    if (stage ==  SchedulerStage::Stage::Placement)
    {
        //Why Placement was not Scheduled
        writer.Write("Placement was not scheduled because: \r\n\t\t");

        if (!CreationActionNeeded())
        {
            writer.Write("There were no new replicas, upgrades, or extra replicas that needed to be dropped. \r\n\t");
        }
        else if (!placementExpired_)
        {
            writer.Write("Enough time hasn't passed since the previous round of Placement.\r\n\t");
        }
        else
        {
            isValid = true;
            TESTASSERT_IF(stage_ == SchedulerStage::Stage::Placement , "WhyNot can't be run on a state when the state matches: {0}", static_cast<int>(stage));
            if (decisionScheduled_)
            {
                writer.Write("Placement was not the most overdue phase needing scheduling.\r\n\t");
            }
        }
    }
    else if (stage ==  SchedulerStage::Stage::ConstraintCheck)
    {
        //Why Constraint Checking was not Scheduled
        writer.Write("Constraint Fixing was not scheduled because: \r\n\t\t");

        if (!constraintCheckEnabledConfig_)
        {
            writer.Write("The config to check for constraint violations was not enabled.\r\n\t");
        }
        else if (constraintCheckPaused_)
        {
            writer.Write("Upgrades temporarily paused checking for constraint violations.\r\n\t");
        }
        else if (!constraintCheckExpired_)
        {
            writer.Write("Enough time hasn't passed since the previous round of Checking for Constraint Violations.\r\n\t");
        }
        else if (!hasMovableReplica_)
        {
            writer.Write("There are no replicas that can be relocated to fix potential Constraint Violations.\r\n\t");
        }
        else if (wasConstraintCheckRun_)
        {
            if (constraintsDiagnosticsData_.empty())
            {
                writer.Write("Constraint Fixing was not scheduled because there were no detected constraint violations.\r\n\t");
            }
            else if (isConstraintCheckNodeChangeThrottled_)
            {
                 writer.Write("Constraint Fixing was not scheduled because one or more nodes recently came up or went down and the only violations found were FaultDomain/UpgradeDomain.\r\n\t");
            }
            else
            {
                Common::Assert::TestAssert("Invalid Diagnostics WhyNot Scenario for ConstraintCheck");
            }
        }
        else
        {
            isValid = true;
            TESTASSERT_IF(stage_ == SchedulerStage::Stage::ConstraintCheck , "WhyNot can't be run on a state when the state matches: {0}", static_cast<int>(stage));

            if (decisionScheduled_)
            {
                writer.Write("Constraint Violation Fixing was not the most overdue phase needing scheduling.\r\n\t");
            }
        }
    }
    else if (stage ==  SchedulerStage::Stage::Balancing)
    {
        //Why Balancing Checking Was not Scheduled
        writer.Write("Balancing was not scheduled because: \r\n\t\t");

        if (!balancingEnabledConfig_)
        {
            writer.Write("The config for balancing load was not enabled.\r\n\t");
        }
        else if (balancingPaused_)
        {
            writer.Write("Upgrades temporarily paused balancing.\r\n\t");
        }
        else if (!balancingExpired_)
        {
            writer.Write("Enough time has not passed since the previous round of Balance Checking.\r\n\t");
        }
        else if (!hasMovableReplica_)
        {
            writer.Write("There are no replicas that can be relocated to fix potential imbalances of load.\r\n\t");
        }
        else if (isbalanced_ || balancingDiagnosticsData_ && balancingDiagnosticsData_->changed_ && balancingDiagnosticsData_->metricDiagnostics.empty())
        {
            writer.Write("Balancing was not scheduled because there were no metrics that violated Metric Activity and Balancing Thresholds.\r\n\t");
        }
        else if (isBalancingSmallChangeThrottled_)
        {
            writer.Write("There haven't been significant changes since the last round of balancing.\r\n\t");
            writer.Write("\r\nImbalanced Metric Information: \r\n\t{0} \r\n", *balancingDiagnosticsData_);
        }
        else if (isBalancingNodeChangeThrottled_)
        {
            writer.Write("One or more nodes recently came up or went down.\r\n\t");
            writer.Write("\r\nImbalanced Metric Information: \r\n\t{0} \r\n", *balancingDiagnosticsData_);
        }
        else
        {
            isValid = true;
            TESTASSERT_IF(stage_ == SchedulerStage::Stage::Balancing , "WhyNot can't be run on a state when the state matches: {0}", static_cast<int>(stage));
            if (decisionScheduled_)
            {
                writer.Write("Balancing was not the most overdue phase needing scheduling.\r\n\t");
            }
        }
    }
    else
    {
        //Just need to assert so using a dummy inequality to make it happen
        TESTASSERT_IF(stage != SchedulerStage::Stage::Placement , "WhyNot is not supported for this parameter: {0}", static_cast<int>(stage));
    }

    return isValid;
}

void ServiceDomainSchedulerStageDiagnostics::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

void ServiceDomainSchedulerStageDiagnostics::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("\r\n\t");

    if (skipRefreshSchedule_)
    {
        TESTASSERT_IF(stage_ != SchedulerStage::Stage::Skip , "Scheduler Stage Diagnostics Mismatch: {0}", static_cast<int>(stage_));
        writer.Write("Scheduling new actions was skipped because there are already pending planned moves.\r\n");
    }
    else
    {
        if (stage_ == SchedulerStage::Stage::Placement)
        {
            //Why Placement Happened
            writer.Write("Placement was scheduled because: \r\n\t\t");

            if (newReplicaNeeded_)
            {
                writer.Write("New replicas needed to be placed.\r\n\t\t");
            }

            if (upgradeNeeded_)
            {
                writer.Write("An upgrade was going through the system, and hence primary replica needed to be swapped to different nodes.\r\n\t\t");
            }

            if (dropNeeded_)
            {
                writer.Write("Extra replicas were found and needed to be removed.\r\n\t\t");
            }

            if (placementExpired_)
            {
                writer.Write("Enough time has passed since the previous round of Placement.\r\n\t");
            }

            //Why the others didn't happen
            if (WhyNot(writer, SchedulerStage::Stage::ConstraintCheck) | WhyNot(writer, SchedulerStage::Stage::Balancing))
            {
                writer.Write("\r\n\tBetween Placement, Constraint Violation Fixing, and Balancing, it was both necessary and the longest overdue.\r\n\t\t");
            }
        }
        else if (stage_ == SchedulerStage::Stage::ConstraintCheck)
        {
            //Why Constraint Violation Fixing Happened
            writer.Write("Constraint Violation Checking was scheduled because: \r\n\t\t");

            if (constraintCheckEnabledConfig_)
            {
                writer.Write("The config to check for constraint violations was enabled.\r\n\t\t");
            }

            if (!constraintCheckPaused_)
            {
                writer.Write("Upgrades did not temporarily pause checking for constraint violations.\r\n\t\t");
            }

            if (constraintCheckExpired_)
            {
                writer.Write("Enough time has passed since the previous round of Checking for Constraint Violations.\r\n\t\t");
            }

            if (hasMovableReplica_)
            {
                writer.Write("There are replicas that can be moved to fix Potential Constraint Violations.\r\n\t\t");
            }

            if (!isConstraintCheckNodeChangeThrottled_)
            {
                writer.Write("No nodes recently came up or went down.\r\n\t");
            }
            else
            {
                writer.Write("Nodes recently came up or went down, but there were non FaultDomain/UpgradeDomain violations that needed to be fixed.\r\n\t");
            }

            //Why the others didn't happen
            if (WhyNot(writer, SchedulerStage::Stage::Placement) | WhyNot(writer, SchedulerStage::Stage::Balancing))
            {
                writer.Write("\r\n\tBetween Placement, Constraint Violation Fixing, and Balancing, it was both necessary and the longest overdue.\r\n");
            }

            if (decisionScheduled_ || !constraintsDiagnosticsData_.empty())
            {
                writer.Write("\r\nConstraint Violations: \r\n\t ");

                for (auto i : constraintsDiagnosticsData_)
                {
                    writer.Write("{0}\r\n\t ", *i);
                }
            }
            else
            {
                writer.Write("Constraint Fixing was not scheduled because there were no detected constraint violations.\r\n\t\t");
            }
        }
        else if (stage_ == SchedulerStage::Stage::Balancing)
        {
            //Why Balance Checking Happened
            writer.Write("Balance Checking was scheduled because: \r\n\t\t");

            if (balancingEnabledConfig_)
            {
                writer.Write("The config for balancing load was enabled.\r\n\t\t");
            }

            if (!balancingPaused_)
            {
                writer.Write("Upgrades did not temporarily pause balancing.\r\n\t\t");
            }

            if (balancingExpired_)
            {
                writer.Write("Enough time has passed since the previous round of Balancing.\r\n\t\t");
            }

            if (hasMovableReplica_)
            {
                writer.Write("There are replicas that can be moved to fix potential imbalances of load.\r\n\t\t");
            }

            if (!isBalancingSmallChangeThrottled_)
            {
                writer.Write("There have been significant changes since the last round of balancing.\r\n\t\t");
            }

            if (!isBalancingNodeChangeThrottled_)
            {
                writer.Write("No nodes recently came up or went down.\r\n\t\t");
            }

            //Why the others didn't happen
            if (WhyNot(writer, SchedulerStage::Stage::Placement) | WhyNot(writer, SchedulerStage::Stage::ConstraintCheck))
            {
                writer.Write("\r\n\tBetween Placement, Constraint Violation Fixing, and Balancing, it was both necessary and the longest overdue.\r\n\t\t");
            }

            if (decisionScheduled_)
            {
                writer.Write("\r\nImbalanced Metric Information: \r\n\t{0} \r\n", *balancingDiagnosticsData_);
            }
        }
        else if (stage_ == SchedulerStage::Stage::Skip)
        {
            TESTASSERT_IF(decisionScheduled_, " Mismatch Between Decision Being Scheduled and SchedulerStage being Skip.");

            WhyNot(writer, SchedulerStage::Stage::Placement);
            WhyNot(writer, SchedulerStage::Stage::ConstraintCheck);
            WhyNot(writer, SchedulerStage::Stage::Balancing);
        }
        else if (stage_ == SchedulerStage::Stage::NoneExpired)
        {
            TESTASSERT_IF(decisionScheduled_, " Mismatch Between Decision Being Scheduled and SchedulerStage being NoneExpired.");
            writer.Write("Not enough time has passed since the last rounds of Placement, Constraint Checking, and Balancing.\r\n\t\t");
        }
        else
        {
            TESTASSERT_IF(stage_ != SchedulerStage::Stage::NoneExpired , " Undefined Scheduler Stage Diagnostics: {0}", static_cast<int>(stage_));
        }
    }
}

bool ServiceDomainSchedulerStageDiagnostics::AllViolationsSkippable()
{
    /*
    If the only violations are FD or UD violations, then if a node has recently gone down or come up, that means we can skip the constraint check altogether.
    AllviolationsSkippable just returns whether every violation is either an FD violation or a UD violation.
    if not, then both dynamic pointer casts return null and therefore the & also returns a null
    */

    bool allViolationsSkippable = true;

    for (auto i : constraintsDiagnosticsData_)
    {
        bool isFDorUD =
            dynamic_pointer_cast<FaultDomainConstraintDiagnosticsData>(i)
            ||
            dynamic_pointer_cast<UpgradeDomainConstraintDiagnosticsData>(i);

        allViolationsSkippable &= isFDorUD;
    }

    return allViolationsSkippable;
}

std::wstring ServiceDomainSchedulerStageDiagnostics::DiagnosticsMessage()
{
    std::wstring genericSchedulerInfo1 =
    wformatString("skipRefreshSchedule_: {0} \r\n stage_: {1} \r\n newReplicaNeeded_: {2} \r\n upgradeNeeded_: {3} \r\n dropNeeded_: {4} \r\n constraintCheckEnabledConfig_: {5} \r\n constraintCheckPaused_: {6} \r\n balancingEnabledConfig_: {7} \r\n balancingPaused_: {8} \r\n hasMovableReplica_: {9} \r\n",
            skipRefreshSchedule_,
            static_cast<int>(stage_),
            newReplicaNeeded_,
            upgradeNeeded_,
            dropNeeded_,
            constraintCheckEnabledConfig_,
            constraintCheckPaused_,
            balancingEnabledConfig_,
            balancingPaused_,
            hasMovableReplica_
        );

    std::wstring genericSchedulerInfo2 =
    wformatString("placementExpired_: {0} \r\n constraintCheckExpired_: {1} \r\n balancingExpired_: {2} \r\n noExpiredTimerActions_: {3} \r\n creationWithMoveEnabled_: {4} \r\n decisionScheduled_: {5} \r\n",
    placementExpired_,
    constraintCheckExpired_,
    balancingExpired_,
    noExpiredTimerActions_,
    creationWithMoveEnabled_,
    decisionScheduled_);

    std::wstring constraintsDiagnosticsInfo;

    for (auto i : constraintsDiagnosticsData_)
    {
        constraintsDiagnosticsInfo += i->DiagnosticsMessage() + Environment::NewLine;
    }
    std::wstring balancingDiagnosticsInfo = balancingDiagnosticsData_->DiagnosticsMessage();

    return L"Decision:" + StringUtility::ToWString(DecisionGuid) + Environment::NewLine + genericSchedulerInfo1 + Environment::NewLine + genericSchedulerInfo2 + Environment::NewLine + constraintsDiagnosticsInfo + Environment::NewLine + balancingDiagnosticsInfo;
};

ServiceDomainSchedulerStageDiagnostics::ServiceDomainSchedulerStageDiagnostics()
: decisionGuid_(Common::Guid::NewGuid()),
stage_(SchedulerStage::Stage::NoneExpired),
skipRefreshSchedule_(false),
newReplicaNeeded_(false),
upgradeNeeded_(false),
dropNeeded_(false),
constraintCheckEnabledConfig_(false),
constraintCheckPaused_(false),
balancingEnabledConfig_(false),
balancingPaused_(false),
hasMovableReplica_(false),
placementExpired_(false),
constraintCheckExpired_(false),
balancingExpired_(false),
noExpiredTimerActions_(false),
creationWithMoveEnabled_(false),
constraintsDiagnosticsData_(),
decisionScheduled_(false),
wasConstraintCheckRun_(false),
isConstraintCheckNodeChangeThrottled_(false),
isbalanced_(false),
isBalancingNodeChangeThrottled_(false),
isBalancingSmallChangeThrottled_(false),
balancingDiagnosticsData_(),
metricString_(),
metricProperty_()
{
}

ServiceDomainSchedulerStageDiagnostics::ServiceDomainSchedulerStageDiagnostics(ServiceDomainSchedulerStageDiagnostics && other)
: skipRefreshSchedule_(std::move(other.skipRefreshSchedule_)),
stage_(std::move(other.stage_)),
newReplicaNeeded_(std::move(other.newReplicaNeeded_)),
upgradeNeeded_(std::move(other.upgradeNeeded_)),
dropNeeded_(std::move(other.dropNeeded_)),
constraintCheckEnabledConfig_(std::move(other.constraintCheckEnabledConfig_)),
constraintCheckPaused_(std::move(other.constraintCheckPaused_)),
balancingEnabledConfig_(std::move(other.balancingEnabledConfig_)),
balancingPaused_(std::move(other.balancingPaused_)),
hasMovableReplica_(std::move(other.hasMovableReplica_)),
placementExpired_(std::move(other.placementExpired_)),
constraintCheckExpired_(std::move(other.constraintCheckExpired_)),
balancingExpired_(std::move(other.balancingExpired_)),
creationWithMoveEnabled_(std::move(other.creationWithMoveEnabled_)),
constraintsDiagnosticsData_(std::move(other.constraintsDiagnosticsData_)),
decisionScheduled_(std::move(other.decisionScheduled_)),
wasConstraintCheckRun_(std::move(other.wasConstraintCheckRun_)),
isConstraintCheckNodeChangeThrottled_(std::move(other.isConstraintCheckNodeChangeThrottled_)),
isbalanced_(std::move(other.isbalanced_)),
isBalancingNodeChangeThrottled_(std::move(other.isBalancingNodeChangeThrottled_)),
isBalancingSmallChangeThrottled_(std::move(other.isBalancingSmallChangeThrottled_)),
balancingDiagnosticsData_(std::move(other.balancingDiagnosticsData_)),
metricString_(std::move(other.metricString_)),
metricProperty_(std::move(other.metricProperty_))
{
}

ServiceDomainSchedulerStageDiagnostics & ServiceDomainSchedulerStageDiagnostics::operator = (ServiceDomainSchedulerStageDiagnostics && other)
{
    if (this != &other)
    {
        skipRefreshSchedule_ = std::move(other.skipRefreshSchedule_);
        stage_ = std::move(other.stage_);
        newReplicaNeeded_ = std::move(other.newReplicaNeeded_);
        upgradeNeeded_ = std::move(other.upgradeNeeded_);
        dropNeeded_ = std::move(other.dropNeeded_);
        constraintCheckEnabledConfig_ = std::move(other.constraintCheckEnabledConfig_);
        constraintCheckPaused_ = std::move(other.constraintCheckPaused_);
        balancingEnabledConfig_ = std::move(other.balancingEnabledConfig_);
        balancingPaused_ = std::move(other.balancingPaused_);
        hasMovableReplica_ = std::move(other.hasMovableReplica_);
        placementExpired_ = std::move(other.placementExpired_);
        constraintCheckExpired_ = std::move(other.constraintCheckExpired_);
        balancingExpired_ = std::move(other.balancingExpired_);
        creationWithMoveEnabled_ = std::move(other.creationWithMoveEnabled_);
        constraintsDiagnosticsData_ = std::move(other.constraintsDiagnosticsData_);
        decisionScheduled_ = std::move(other.decisionScheduled_);
        wasConstraintCheckRun_ = std::move(other.wasConstraintCheckRun_);
        isConstraintCheckNodeChangeThrottled_ = std::move(other.isConstraintCheckNodeChangeThrottled_);
        isbalanced_ = std::move(other.isbalanced_);
        isBalancingNodeChangeThrottled_ = std::move(other.isBalancingNodeChangeThrottled_);
        isBalancingSmallChangeThrottled_ = std::move(other.isBalancingSmallChangeThrottled_);
        balancingDiagnosticsData_ = std::move(other.balancingDiagnosticsData_);
        metricString_ = std::move(other.metricString_);
        metricProperty_ = std::move(other.metricProperty_);
    }

    return *this;
}

SchedulingDiagnostics::SchedulingDiagnostics():
    keepHistory_(false),
    schedulerDiagnosticsMap_(),
    sdBalancingDiagnosticsMap_()
{
}

void SchedulingDiagnostics::CleanupStageDiagnostics()
{
    if (keepHistory_)
    {
        //Add Cleanup Method here when adding snapshothistory functionality
    }
    else
    {
        schedulerDiagnosticsMap_.clear();
    }
}

void SchedulingDiagnostics::AddStageDiagnostics(std::wstring const & serviceDomainId, ServiceDomainSchedulerStageDiagnosticsSPtr const & stageDiagnostics)
{

    stageDiagnostics->balancingDiagnosticsData_ = AddOrGetSDBalancingDiagnostics(serviceDomainId, false);

    if (schedulerDiagnosticsMap_.count(serviceDomainId))
    {
        if (keepHistory_)
        {
            schedulerDiagnosticsMap_.at(serviceDomainId).push_back(stageDiagnostics);
        }
        else
        {
            schedulerDiagnosticsMap_.at(serviceDomainId).front() = stageDiagnostics;
        }
    }
    else
    {
        std::vector<ServiceDomainSchedulerStageDiagnosticsSPtr> temp;
        temp.push_back(stageDiagnostics);
        schedulerDiagnosticsMap_.insert(std::pair<std::wstring, std::vector<ServiceDomainSchedulerStageDiagnosticsSPtr>>(serviceDomainId, move(temp)));
    }
}

BalancingDiagnosticsDataSPtr SchedulingDiagnostics::AddOrGetSDBalancingDiagnostics(std::wstring const & serviceDomainId, bool createIfNull)
{
    if (sdBalancingDiagnosticsMap_.count(serviceDomainId))
    {
        BalancingDiagnosticsDataSPtr retVal = sdBalancingDiagnosticsMap_.at(serviceDomainId);

        if (createIfNull)
        {
            retVal = std::make_shared<BalancingDiagnosticsData>();
        }

        sdBalancingDiagnosticsMap_[serviceDomainId] = retVal;

        return retVal;
    }
    else
    {
        BalancingDiagnosticsDataSPtr  retVal = nullptr;

        if (createIfNull)
        {
            retVal = std::make_shared<BalancingDiagnosticsData>();
        }

        sdBalancingDiagnosticsMap_.insert(std::pair<std::wstring, BalancingDiagnosticsDataSPtr>(serviceDomainId, retVal));

        return retVal;
    }
}

SchedulingDiagnostics::ServiceDomainSchedulerStageDiagnosticsSPtr SchedulingDiagnostics::GetLatestStageDiagnostics(std::wstring const & serviceDomainId)
{
    if (schedulerDiagnosticsMap_.count(serviceDomainId))
    {
        TESTASSERT_IF(schedulerDiagnosticsMap_.at(serviceDomainId).empty(), "SD Diagnostics Vector should not be empty for sd {0}.", serviceDomainId);
        return (!schedulerDiagnosticsMap_.at(serviceDomainId).empty()) ? schedulerDiagnosticsMap_.at(serviceDomainId).back() : nullptr;

    }
    else
    {
        return nullptr;
    }
}

NodeCapacityViolationDiagnosticsData::NodeCapacityViolationDiagnosticsData(
    std::wstring && metricName,
    int64 replicaLoad,
    int64 currentLoad,
    int64 disappearingLoad,
    int64 appearingLoad,
    int64 reservedLoad,
    int64 capacity,
    bool diagnosticsSucceeded)
    : metricName_(std::move(metricName)),
    replicaLoad_(replicaLoad),
    currentLoad_(currentLoad),
    disappearingLoad_(disappearingLoad),
    appearingLoad_(appearingLoad),
    reservedLoad_(reservedLoad),
    capacity_(capacity),
    diagnosticsSucceeded_(diagnosticsSucceeded)
{
    changed_ = false;
}

NodeCapacityViolationDiagnosticsData::NodeCapacityViolationDiagnosticsData(NodeCapacityViolationDiagnosticsData && other)
    : metricName_(std::move(other.metricName_)),
    replicaLoad_(std::move(other.replicaLoad_)),
    currentLoad_(std::move(other.currentLoad_)),
    disappearingLoad_(std::move(other.disappearingLoad_)),
    appearingLoad_(std::move(other.appearingLoad_)),
    reservedLoad_(std::move(other.reservedLoad_)),
    capacity_(std::move(other.capacity_)),
    diagnosticsSucceeded_(std::move(other.diagnosticsSucceeded_))
{
    changed_ = std::move(other.changed_);
}

NodeCapacityViolationDiagnosticsData & NodeCapacityViolationDiagnosticsData::operator = (NodeCapacityViolationDiagnosticsData && other)
{
    if (this != &other)
    {
        metricName_ = std::move(other.metricName_);
        replicaLoad_ = std::move(other.replicaLoad_);
        currentLoad_ = std::move(other.currentLoad_);
        disappearingLoad_ = std::move(other.disappearingLoad_);
        appearingLoad_ = std::move(other.appearingLoad_);
        reservedLoad_ = std::move(other.reservedLoad_);
        capacity_ = std::move(other.capacity_);
        diagnosticsSucceeded_ = std::move(other.diagnosticsSucceeded_);
        changed_ = std::move(other.changed_);
    }

    return *this;
}
