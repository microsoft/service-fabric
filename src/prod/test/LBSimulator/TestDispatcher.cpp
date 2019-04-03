// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <algorithm>    // std::shuffle
#include <array>        // std::array
#include <iomanip>      // std::setw
#include <iostream>
#include <sstream>      // std::stringstream
#include <string>       // stoi()
#include <random> 
#include "TestDispatcher.h"
#include "FM.h"
#include "DataParser.h"
#include "DataGenerator.h"
#include "Reliability\LoadBalancing\PlacementAndLoadBalancing.h"
#include "Reliability\LoadBalancing\FailoverUnitMovement.h"
#include "TestSession.h"
#include "TraceParser.h"
#include "Utility.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

wstring const TestDispatcher::VerifyCommand = L"verify";
wstring const TestDispatcher::CreateNodeCommand = L"+";
wstring const TestDispatcher::DeleteNodeCommand = L"-";
wstring const TestDispatcher::CreateServiceCommand = L"createservice";
wstring const TestDispatcher::DeleteServiceCommand = L"deleteservice";
wstring const TestDispatcher::ReportLoadCommand = L"reportload";
wstring const TestDispatcher::ReportBatchLoadCommand = L"reportbatchload";
wstring const TestDispatcher::ReportBatchMoveCostCommand = L"reportbatchmovecost";
wstring const TestDispatcher::LoadPlacementCommand = L"loadplacement";
wstring const TestDispatcher::LoadRandomPlacementCommand = L"loadrandomplacement";
wstring const TestDispatcher::LoadPlacementDumpCommand = L"loadplacementdump";
wstring const TestDispatcher::ExecutePLBCommand = L"executeplb";
wstring const TestDispatcher::ExecutePlacementDumpActionCommand = L"executeplacementdump";
wstring const TestDispatcher::ResetCommand = L"reset";
wstring const TestDispatcher::SetDefaultMoveCostCommand = L"setdefaultmovecost";
wstring const TestDispatcher::ShowAllCommand = L"all";
wstring const TestDispatcher::ShowServicesCommand = L"services";
wstring const TestDispatcher::ShowNodesCommand = L"nodes";
wstring const TestDispatcher::ShowGFUMCommand = L"gfum";
wstring const TestDispatcher::ShowFailoverUnitsCommand = L"fus";
wstring const TestDispatcher::ShowPlacementCommand = L"placement";
wstring const TestDispatcher::SetPLBConfigCommand = L"setplbconfig";
wstring const TestDispatcher::GetPLBConfigCommand = L"getplbconfig";
wstring const TestDispatcher::DeleteReplicaCommand = L"deletereplica";
wstring const TestDispatcher::PromoteReplicaCommand = L"promotereplica";
wstring const TestDispatcher::MoveReplicaCommand = L"movereplica";
wstring const TestDispatcher::LogMetricsCommand = L"logmetrics";
wstring const TestDispatcher::ForceRefreshCommand = L"forceexecuteplb";
wstring const TestDispatcher::PrintClusterStateCommand = L"printclusterstate";
wstring const TestDispatcher::PLBBatchRunsCommand = L"batchexecuteplb";
wstring const TestDispatcher::FlushTracesCommand = L"flushtraces";
wstring const TestDispatcher::ConvertNodeIdCommand = L"convertnodeid";
wstring const TestDispatcher::LoadPlacementFromTracesCommand = L"loadplacementfromtrace";
// If someone outside of PLB team wish to access the code and create his own function
// this is the place he could easily do it.
wstring const TestDispatcher::PlaceholderFunctionCommand = L"placeholderfunction";

TestDispatcher::TestDispatcher()
{
}

bool TestDispatcher::Open()
{
    fm_ = FM::Create(LBSimulatorConfigObj);
    traceFileVersion_ = 0;
    return true;
}

void TestDispatcher::Close()
{
    fm_ = nullptr;
    TestCommon::TestDispatcher::Close();
}

bool TestDispatcher::ExecuteCommand(wstring command)
{
    StringCollection paramCollection = Utility::Split(command, Utility::ParamDelimiter, Utility::EmptyCharacter);
    if (paramCollection.size() == 0)
    {
        return false;
    }

    if (StringUtility::StartsWithCaseInsensitive(command, CreateNodeCommand))
    {
        paramCollection[0] = paramCollection[0].substr(CreateNodeCommand.size());
        return CreateNode(paramCollection);
    }
    else if (StringUtility::StartsWithCaseInsensitive(command, DeleteNodeCommand))
    {
        paramCollection[0] = paramCollection[0].substr(DeleteNodeCommand.size());
        return DeleteNode(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], VerifyCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return Verify(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], CreateServiceCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return CreateServices(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], DeleteServiceCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return DeleteService(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ReportLoadCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ReportLoad(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ReportBatchLoadCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ReportBatchLoad(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ReportBatchMoveCostCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ReportBatchMoveCost(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], LoadPlacementCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LoadPlacement(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], LoadRandomPlacementCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LoadRandomPlacement(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], LoadPlacementDumpCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LoadPlacementDump(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ExecutePLBCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ExecutePLB(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ExecutePlacementDumpActionCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ExecutePlacementDumpAction(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ResetCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return Reset(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], SetDefaultMoveCostCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return SetDefaultMoveCost(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowAllCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ShowAll(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowServicesCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ShowServices(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowNodesCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ShowNodes(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowGFUMCommand) ||
             StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowFailoverUnitsCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ShowGFUM(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ShowPlacementCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ShowPlacement(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], SetPLBConfigCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return SetPLBConfig(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], GetPLBConfigCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return GetPLBConfig(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], DeleteReplicaCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return DeleteReplica(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], PromoteReplicaCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return PromoteReplica(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], MoveReplicaCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return MoveReplica(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ForceRefreshCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ForceRefresh(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], PrintClusterStateCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return PrintClusterState(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], LogMetricsCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LogMetrics(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], PLBBatchRunsCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return PLBBatchRuns(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], FlushTracesCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return FlushTraces(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], ConvertNodeIdCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return ConvertNodeId(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], LoadPlacementFromTracesCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LoadPlacementFromTraces(paramCollection);
    }
    else if (StringUtility::AreEqualCaseInsensitive(paramCollection[0], PlaceholderFunctionCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return PlaceholderFunction(paramCollection);
    }
    else
    {
        return false;
    }
}

wstring TestDispatcher::GetState(wstring const & param)
{
    param;
    return wstring();
}

bool TestDispatcher::Verify(StringCollection const & params)
{
    if (params.size() > 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect Verify parameters. Type \"!help verify\" for details.");
        return false;
    }

    TimeSpan timeout = LBSimulatorConfig::GetConfig().VerifyTimeout;
    TimeoutHelper timeoutHelper(timeout);

    DWORD interval = 1000;

    bool ret = false;

    do
    {
        ret = fm_->VerifyAll();
        Sleep(interval);
        interval = min<DWORD>(interval + 500, 3000);
    } while (!ret && !timeoutHelper.IsExpired);

    if (ret)
    {
        TestSession::WriteInfo(Utility::TraceSource, "All verification succeeded.");
    }
    else if (!LBSimulatorConfig::GetConfig().FailTestIfVerifyTimeout)
    {
        TestSession::WriteWarning(Utility::TraceSource,
            "Verification failed with a Timeout of {0} seconds. Check logs for details.", timeout.TotalSeconds());
    }
    else
    {
        TestSession::FailTest(
            "Verification failed with a Timeout of {0} seconds. Check logs for details.",
            timeout.TotalSeconds());
    }

    return true;
}

bool TestDispatcher::CreateNode(StringCollection const & params)
{
    // syntax: +<nodeId> faultDomainId upgradeDomainId capacities
    // syntax: +10 fd:/dc/rack upgradedomainA m1/100, m2/20
    if (params.size() < 3)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect CreateNode parameters. Type \"!help +\" for details.");
        return false;
    }

    int nodeIndex = _wtoi(params[0].c_str());
    vector<pair<wstring, wstring>> nodeVec;
    for (int i = 1; i < params.size(); i++)
    {
        nodeVec.push_back(make_pair(L"",params[i].c_str()));
    }
    map<wstring, wstring> nodeLabelledVec;
    Utility::Label(nodeVec, nodeLabelledVec, LBSimulator::DataParser::nodeLabels);
    DataParser parser(*fm_, PLBConfigObj);
    parser.CreateNode(nodeLabelledVec, nodeIndex);
    return true;
}

bool TestDispatcher::DeleteNode(StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect DeleteNode parameters. Type \"!help -\" for details.");
        return false;
    }

    wstring nodeIdStr = params[0];

    fm_->DeleteNode(_wtoi(nodeIdStr.c_str()));
    return true;
}

bool TestDispatcher::CreateServices(StringCollection const & params)
{
    // syntax: createservice index isStateful partitionCount replicaCount affinitizedService blocklist metrics
    // blocklist: node1, node2, ...
    // metrics: metric1/weight/primaryDefaultLoad/secondaryDefaultLoad, metric2/weight/primaryDefaultLoad/secondaryDefaultLoad, ...
    if (params.size() < 4)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect CreateService parameters. Type \"!help createservice\" for details.");
        return false;
    }

    int serviceIndex = _wtoi(params[0].c_str());
    vector<pair<wstring, wstring>> serviceVec;
    for (int i = 1; i < params.size(); i++)
    {
        serviceVec.push_back(make_pair(L"", params[i].c_str()));
    }
    map<wstring, wstring> serviceLabelledVec;
    Utility::Label(serviceVec, serviceLabelledVec, LBSimulator::DataParser::serviceLabels);
    DataParser parser(*fm_, PLBConfigObj);
    parser.CreateService(serviceLabelledVec, serviceIndex, true);
    return true;
}

bool TestDispatcher::DeleteService(StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect DeleteService parameters. Type \"!help deleteservice\" for details.");
        return false;
    }

    wstring serviceIdStr = params[0];

    fm_->DeleteService(_wtoi(serviceIdStr.c_str()));
    return true;
}

bool TestDispatcher::ReportLoad(StringCollection const & params)
{
    // reportload fuIndex role metric load
    if (params.size() != 4)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ReportLoad parameters. Type \"!help reportload\" for details.");
        return false;
    }

    fm_->ReportLoad(
        Common::Guid(params[0]),
        _wtoi(params[1].c_str()),
        params[2],
        static_cast<uint>(_wtoi64(params[3].c_str())));

    return true;
}

bool TestDispatcher::ReportBatchLoad(StringCollection const & params)
{
    // reportload serviceIndex metric loaddistribution
    if (params.size() != 3)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ReportBatchLoad parameters. Type \"!help reportbatchload\" for details.");
        return false;
    }

    fm_->ReportBatchLoad(
        _wtoi(params[0].c_str()),
        params[1],
        LoadDistribution::Parse(params[2], Utility::PairDelimiter));

    return true;
}

bool TestDispatcher::ReportBatchMoveCost(StringCollection const & params)
{
    int param = _wtoi(params[0].c_str());
    if (param >= 0 && param <= 3)
    {
        fm_->ReportBatchMoveCost(_wtoi(params[0].c_str()));
        return true;
    }
    else
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ReportBatchMoveCost parameters.\n");
        return false;
    }
}

bool TestDispatcher::SetDefaultMoveCost(StringCollection const & params)
{
    int param = _wtoi(params[0].c_str());
    if (param >= 0)
    {
        LBSimulatorConfigObj.DefaultMoveCost = _wtoi(params[0].c_str());
        return true;
    }
    else
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect SetDefaultMoveCost parameters.\n");
        return false;
    }
}

bool TestDispatcher::Reset(StringCollection const & params)
{
    // reset
    if (params.size() > 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect Reset parameters.\n{0}",
            L"Syntax: reset");
        return false;
    }

    fm_->Reset(fm_->NonEmpty);

    return true;
}

bool TestDispatcher::LoadPlacement(StringCollection const & params)
{
    // loadplacement placementfile.txt
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect LoadPlacement parameters. Type \"!help loadplacement\" for details.");
        return false;
    }

    if (!File::Exists(params[0]))
    {
        TestSession::WriteError(Utility::TraceSource, "File {0} doesn't exist", params[0]);
        return false;
    }
    // disable the placement temporarily
    TogglePLB(false);
    //Reset fm
    fm_->Reset(fm_->NonEmpty);
    //parse and load placement file
    DataParser parser(*fm_, PLBConfigObj);
    parser.Parse(params[0]);
    //Sleep(10000);
    //restore placement once loading of placement from file is complete
    TogglePLB(true);

    return true;
}

bool TestDispatcher::LoadRandomPlacement(StringCollection const & params)
{
    // loadplacement randplacementfile.txt seed
    if (params.size() != 2)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Incorrect LoadRandomPlacement parameters. Type \"!help loadrandomplacement\" for details.");
        return false;
    }

    if (!File::Exists(params[0]))
    {
        TestSession::WriteError(Utility::TraceSource, "File {0} doesn't exist", params[0]);
        return false;
    }

    int seed = _wtoi(params[1].c_str());
    // disable the placement temporarily
    TogglePLB(false);

    fm_->Reset();
    DataGenerator generator(*fm_, seed, PLBConfigObj);
    generator.Parse(params[0]);
    generator.Generate();
    //Sleep(10000);
    // turn placement back on
    TogglePLB(true);
    return true;
}

bool TestDispatcher::LoadPlacementDump(StringCollection const & params)
{
    // loadplacementdump placementdumpfile.txt
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Incorrect LoadPlacementDump parameters. Type \"!help loadplacementdump\" for details.");
        return false;
    }

    if (!File::Exists(params[0]))
    {
        TestSession::WriteError(Utility::TraceSource, "File {0} doesn't exist", params[0]);
        return false;
    }
    plb_.Load(params[0]);

    return true;
}

bool TestDispatcher::ExecutePLB(StringCollection const & params)
{
    if (params.size() != 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ExecutePLB parameters. Type \"!help executeplb\" for details.");
        return false;
    }

    fm_->ExecutePlacementAndLoadBalancing();
    return true;
}

bool TestDispatcher::ExecutePlacementDumpAction(StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Incorrect ExecutePlacementDumpAction parameters. Type \"!help executeplacementdump\" for details.");
        return false;
    }

    if (StringUtility::AreEqualCaseInsensitive(params[0], L"creation"))
    {
        plb_.RunCreation();
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"constraintcheck"))
    {
        plb_.RunConstraintCheck();
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"loadbalancing"))
    {
        plb_.RunLoadBalancing();
    }
    else
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ExecutePlacementDumpAction action {0}.", params[0]);
        return false;
    }

    return true;
}

bool TestDispatcher::ShowAll(StringCollection const & params)
{
    ShowServices(params);
    ShowNodes(params);
    ShowGFUM(params);
    return true;
}

bool TestDispatcher::ShowServices(StringCollection const & params)
{
    params;
    TestSession::WriteInfo(Utility::TraceSource, "{0}", fm_->ServicesToString());
    return true;
}

bool TestDispatcher::ShowNodes(StringCollection const & params)
{
    params;

    TestSession::WriteInfo(Utility::TraceSource, "{0}", fm_->NodesToString());
    return true;
}

bool TestDispatcher::ShowGFUM(StringCollection const & params)
{
    if (params.size() > 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect GFUM parameters. Type \"!help gfum\" for details.");
        return false;
    }

    if (params.empty())
    {
        TestSession::WriteInfo(Utility::TraceSource, "{0}", fm_->FailoverUnitsToString());
    }
    else
    {
        StringCollection rangeStr = Utility::Split(params[0], wstring(L"-"), false);
        size_t begin = 0;
        size_t end = SIZE_MAX;

        if (rangeStr.size() == 1)
        {
            if (!rangeStr[0].empty())
            {
                begin = static_cast<size_t>(_wtoi(rangeStr[0].c_str()));
                end = begin;
            }
        }
        else if (rangeStr.size() == 2)
        {
            if (!rangeStr[0].empty())
            {
                begin = static_cast<size_t>(_wtoi(rangeStr[0].c_str()));
            }

            if (!rangeStr[1].empty())
            {
                end = static_cast<size_t>(_wtoi(rangeStr[1].c_str()));
            }
        }

        TestSession::WriteInfo(Utility::TraceSource, "{0}", fm_->FailoverUnitsToString(begin, end));
    }
    return true;
}

bool TestDispatcher::ShowPlacement(StringCollection const & params)
{
    params;
    TestSession::WriteInfo(Utility::TraceSource, "{0}", fm_->PlacementToString());
    return true;
}

bool TestDispatcher::SetPLBConfig(StringCollection const & params)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect SetPLBConfig parameters. Type \"!help setplbconfig\" for details.");
        return false;
    }

    map<wstring, wstring> configMap;
    configMap[params[0]] = params[1];
    Utility::UpdateConfigValue(configMap, PLBConfigObj);

    return true;
}

bool TestDispatcher::GetPLBConfig(StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect GetPLBConfig parameters. Type \"!help getplbconfig\" for details.");
        return false;
    }

    TestSession::WriteInfo(Utility::TraceSource, "{0}: {1}", params[0], Utility::GetConfigValue(params[0], PLBConfigObj));
    return true;
}

bool TestDispatcher::DeleteReplica(StringCollection const & params)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect DeleteReplica parameters. Type \"!help deletereplica\" for details.");
        return false;
    }

    fm_->DeleteReplica(Common::Guid(params[0].c_str()), _wtoi(params[1].c_str()));
    return true;
}

bool TestDispatcher::PromoteReplica(StringCollection const & params)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect PromoteReplica parameters. Type \"!help promotereplica\" for details.");
        return false;
    }

    fm_->PromoteReplica(Common::Guid(params[0].c_str()), _wtoi(params[1].c_str()));
    return true;
}

bool TestDispatcher::MoveReplica(StringCollection const & params)
{
    if (params.size() != 3)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect MoveReplica parameters. Type \"!help movereplica\" for details.");
        return false;
    }

    fm_->MoveReplica(Common::Guid(params[0].c_str()), _wtoi(params[1].c_str()), _wtoi(params[2].c_str()));
    return true;
}

bool TestDispatcher::ForceRefresh(StringCollection const & params)
{
    if (params.size() > 3 || params.size() < 1)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Incorrect ForceRefresh parameters. Type \"!help forcerefresh\" for details.");
        return false;
    }
    if (!fm_->NonEmpty)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Please Load a Snapshot");
        return false;
    }
    int noOfRefreshes = 0;
    bool needPlacement = false;
    bool needConstraintCheck = false;
    for (auto param : params)
    {
        if (param == L"EnablePlacement")
        {
            needPlacement = true;
        }
        else if (param == L"EnableConstraintCheck")
        {
            needConstraintCheck = true;
        }
        else
        {
            noOfRefreshes = _wtoi(param.c_str());
            if (noOfRefreshes == 0)
            {
                //non-integral or 0 param.
                TestSession::WriteError(Utility::TraceSource,
                    "Incorrect ForceRefresh parameter {0}. Type \"!help forcerefresh\" for details.", param);
                return false;
            }
        }
    }

    Common::StopwatchTime now = Common::Stopwatch::Now();
    int startIteration = static_cast<int>(fm_->NoOfPLBRuns);
    INT64 totalNoOfMoves = 0;
    TestSession::WriteInfo(Utility::TraceSource,
        "It#,         Action, Creatns, CWithMove, Moves(O/M/S), MetricDetails, #ChainViolations, #Blocked Node-Service pairs");
    for (int i = 0; i < noOfRefreshes; i++)
    {
        fm_->ForceExecutePLB(1, needPlacement, needConstraintCheck);
        fm_->PrintNodeState();
        LBSimulator::PLBRun details = fm_->GetPLBRunDetails(startIteration + i);
        totalNoOfMoves += details.movements;

        wstringstream ss;
        ss << setw(3) << i << L"," << setw(15) << details.action << L"," << setw(4) << details.creations << L","
            << setw(4) << details.creationsWithMove << L"," << setw(4) << details.movements << L"/"
            << fm_->NoOfMovementsApplied << L"/" << fm_->NoOfSwapsApplied << L", ";
        for (int metricNo = 0; metricNo < details.metrics.size(); metricNo++)
        {
            if (fm_->IsLogged(details.metrics[metricNo]))
            {
                ss << L"{" << setw(10) << details.metrics[metricNo] << L", " << setw(4) << details.averages[metricNo]
                    << ", " << setw(4) << details.stDeviations[metricNo] << L"}, ";
            }
        }
        ss << setw(4) << details.noOfChainviolations;
        ss << setw(4) << details.blockListAfter.size();

        TestSession::WriteInfo(Utility::TraceSource, "{0}", ss.str());
    }
    Common::TimeSpan diff = Common::Stopwatch::Now() - now;
    TestSession::WriteInfo(Utility::TraceSource,
        "\n\n Total No Of Moves: {0}/{1}, Total Execution Time: {2}", totalNoOfMoves, fm_->NoOfMovementsApplied, diff);

    return true;
}

bool TestDispatcher::PrintClusterState(StringCollection const & params)
{
    if (fm_->NoOfPLBRuns == 0) { return false; }

    bool printNodeLoads = false;
    bool printUds = false;
    if (params.size() > 1)
    {
        TestSession::WriteError(Utility::TraceSource,
            "Incorrect PrintClusterState parameters. Type \"!help PrintClusterState\" for details.");
        return false;
    }
    for (auto param : params)
    {
        if (param == L"verbose")
        {
            printNodeLoads = true;
        }
        else if (param == L"graphical")
        {
            printUds = true;
        }
        else
        {
            TestSession::WriteError(Utility::TraceSource,
                "Invalid PrintClusterState parameter: {0}. Type \"!help PrintClusterState\" for details.", param);
            return false;
        }
    }
    map<std::wstring, std::pair<double, double>> metricAggregates = map<std::wstring, std::pair<double, double>>();
    fm_->GetClusterAggregates(metricAggregates);
    for (auto metricAggregate : metricAggregates)
    {
        TestSession::WriteInfo(Utility::TraceSource, "Metric: {0}, Avg: {1}, StDev {2}",
            metricAggregate.first, metricAggregate.second.first, metricAggregate.second.second);
    }
    if (printNodeLoads)
    {
        std::map<int, std::vector< std::pair<std::wstring, int64>>> nodeLoads =
            std::map<int, std::vector< std::pair<std::wstring, int64>>>();
        fm_->GetNodeLoads(nodeLoads);
        TestSession::WriteInfo(Utility::TraceSource, "NodeIndex         ,       Metrics");

        std::wstringstream nodeEntry;
        for (auto node : nodeLoads)
        {
            nodeEntry << L"NodeName: ";
            nodeEntry << std::to_wstring((INT64)node.first);
            nodeEntry << L",     ";
            for (auto load : node.second)
            {
                nodeEntry << load.first + L" : ";
                nodeEntry << std::to_wstring((INT64)load.second) + L" ";
            }
            nodeEntry << L"\r\n";
        }
        TestSession::WriteInfo(Utility::TraceSource, "{0}", nodeEntry.str());

    }
    if (printUds)
    {
        fm_->PrintNodeState(true);
    }
    return true;
}

bool TestDispatcher::PLBBatchRuns(StringCollection const & params)
{
    if (params.size() > 5 || _wtoi(params[0].c_str()) <= 0 || _wtoi(params[1].c_str()) <= 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect PLBBatchRuns parameters. Type \"!help\" for details.");
        return false;
    }

    int noOfBatches = _wtoi(params[0].c_str());
    StringCollection filename = StringCollection();
    filename.push_back(params[2]);
    StringCollection noOfPLBRunsString = StringCollection();
    noOfPLBRunsString.push_back(params[1]);
    for (int i = 3; i < params.size(); i++)
    {
        noOfPLBRunsString.push_back(params[i]);
    }
    Random rand(PLBConfigObj.InitialRandomSeed);

    for (int batch = 0; batch < noOfBatches; batch++)
    {
        PLBConfigObj.InitialRandomSeed = rand.Next();
        TestSession::WriteInfo(Utility::TraceSource, "PLBBatch#:{0}, Seed:{1}", batch, PLBConfigObj.InitialRandomSeed);
        LoadPlacement(filename);
        ForceRefresh(noOfPLBRunsString);
    }
    return true;
}

bool TestDispatcher::LogMetrics(StringCollection const & params)
{
    // reset
    if (params.size() == 0)
    {
        TestSession::WriteError(Utility::TraceSource, "No Parameters.");
        return false;
    }
    for (auto param : params)
    {
        fm_->LogMetric(param);
    }
    return true;
}
bool TestDispatcher::TogglePLB(bool turnOn)
{
    static TimeSpan oldPlacementInterval;
    static TimeSpan oldProcessPendingUpdatesInterval;
    static TimeSpan oldConstraintCheckInterval;
    static TimeSpan oldLoadBalancingInterval;
    static bool wasTurnedOff = false;

    if (!turnOn)
    {
        oldProcessPendingUpdatesInterval = PLBConfigObj.ProcessPendingUpdatesInterval;
        oldPlacementInterval = PLBConfigObj.MinPlacementInterval;
        oldConstraintCheckInterval = PLBConfigObj.MinConstraintCheckInterval;
        oldLoadBalancingInterval = PLBConfigObj.MinLoadBalancingInterval;
        wasTurnedOff = true;

        PLBConfigObj.ProcessPendingUpdatesInterval = TimeSpan::FromMilliseconds(1);
        PLBConfigObj.MinPlacementInterval = TimeSpan::MaxValue;
        PLBConfigObj.MinConstraintCheckInterval = TimeSpan::MaxValue;
        PLBConfigObj.MinLoadBalancingInterval = TimeSpan::MaxValue;
        return false;
    }
    else if (turnOn && wasTurnedOff)
    {
        PLBConfigObj.ProcessPendingUpdatesInterval = oldProcessPendingUpdatesInterval;
        PLBConfigObj.MinPlacementInterval = oldPlacementInterval;
        PLBConfigObj.MinConstraintCheckInterval = oldConstraintCheckInterval;
        PLBConfigObj.MinLoadBalancingInterval = oldLoadBalancingInterval;
        return true;
    }
    else
    {
        return  true;
    }
}
bool TestDispatcher::ForcePLBSync()
{
    fm_->ForcePLBUpdate();
    return true;
}

bool TestDispatcher::FlushTraces(StringCollection const & params)
{
    traceFileVersion_++;
    if (params.size() != 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect PLBBatchRuns parameters. Type \"!help\" for details.");
        return false;
    }
    TraceTextFileSink::SetPath(params[0] + StringUtility::ToWString(traceFileVersion_));
    return true;
}

bool TestDispatcher::ConvertNodeId(StringCollection const & params)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect ConvertNodeId parameters. Type \"!help\" for details.");
        return false;
    }
    uint64 high = 0;
    Utility::ReadUint64(params[0], high);
    uint64 low = 0;
    Utility::ReadUint64(params[1], low);
    TestSession::WriteInfo(Utility::TraceSource, "Read NodeId as {0}-{1}. Federation NodeId is {2}",
        high, low, Federation::NodeId(Common::LargeInteger(high, low)));
    return true;
}

bool TestDispatcher::LoadPlacementFromTraces(Common::StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect LoadPlacementFromTraces parameters. Type \"!help\" for details.");
        return false;
    }

    if (!File::Exists(params[0]))
    {
        TestSession::WriteError(Utility::TraceSource, "File {0} doesn't exist", params[0]);
        return false;
    }
    // disable the placement temporarily
    TogglePLB(false);

    //parse and load placement file
    TraceParser parser(PLBConfigObj);
    parser.Parse(params[0]);

     fm_->Reset(vector<Reliability::LoadBalancingComponent::NodeDescription>(parser.Nodes),
        vector<Reliability::LoadBalancingComponent::ApplicationDescription>(parser.Applications),
        vector<Reliability::LoadBalancingComponent::ServiceTypeDescription>(parser.ServiceTypes),
        vector<Reliability::LoadBalancingComponent::ServiceDescription>(parser.Services),
        vector<Reliability::LoadBalancingComponent::FailoverUnitDescription>(parser.FailoverUnits),
        vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription>(parser.LoadAndMoveCosts));

    //restore placement once loading of placement from file is complete
    TogglePLB(true);

    //disable PLB timer to run refresh by its own
    PLBConfigObj.PLBRefreshInterval = TimeSpan::MaxValue;

    return true;
}

bool TestDispatcher::PlaceholderFunction(Common::StringCollection const & params)
{
    // Create your own function
    if (params.size() != 0)
    {
        TestSession::WriteError(Utility::TraceSource, "Incorrect PlaceholderFunction parameters. Type \"!help\" for details.");
        return false;
    }

    return true;
}
