// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace SystemServices;
using namespace ClientServerTransport;

GlobalWString FaultAnalysisServiceTcpMessage::StartPartitionDataLossAction = make_global<wstring>(L"StartPartitionDataLossAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetPartitionDataLossProgressAction = make_global<wstring>(L"GetPartitionDataLossProgressAction");
GlobalWString FaultAnalysisServiceTcpMessage::StartPartitionQuorumLossAction = make_global<wstring>(L"StartPartitionQuorumLossAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetPartitionQuorumLossProgressAction = make_global<wstring>(L"GetPartitionQuorumLossProgressAction");
GlobalWString FaultAnalysisServiceTcpMessage::StartPartitionRestartAction = make_global<wstring>(L"StartPartitionRestartAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetPartitionRestartProgressAction = make_global<wstring>(L"GetPartitionRestartProgressAction");
GlobalWString FaultAnalysisServiceTcpMessage::CancelTestCommandAction = make_global<wstring>(L"CancelTestCommandAction");
GlobalWString FaultAnalysisServiceTcpMessage::StartChaosAction = make_global<wstring>(L"StartChaosAction");
GlobalWString FaultAnalysisServiceTcpMessage::StopChaosAction = make_global<wstring>(L"StopChaosAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetChaosAction = make_global<wstring>(L"GetChaosAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetChaosReportAction = make_global<wstring>(L"GetChaosReportAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetChaosEventsAction = make_global<wstring>(L"GetChaosEventsAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetChaosScheduleAction = make_global<wstring>(L"GetChaosScheduleAction");
GlobalWString FaultAnalysisServiceTcpMessage::PostChaosScheduleAction = make_global<wstring>(L"PostChaosScheduleAction");
GlobalWString FaultAnalysisServiceTcpMessage::StartNodeTransitionAction = make_global<wstring>(L"StartNodeTransitionAction");
GlobalWString FaultAnalysisServiceTcpMessage::GetNodeTransitionProgressAction = make_global<wstring>(L"GetNodeTransitionProgressAction");

void FaultAnalysisServiceTcpMessage::WrapForFaultAnalysisService()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::FaultAnalysisServicePackageName),
            SystemServiceApplicationNameHelper::FaultAnalysisServiceType));
}

const Actor::Enum FaultAnalysisServiceTcpMessage::actor_ = Actor::FAS;
