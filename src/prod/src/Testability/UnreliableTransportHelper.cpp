// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Hosting2;
using namespace Management::FileStoreService;
using namespace Query;
using namespace std;
using namespace ServiceModel;
using namespace Testability;
using namespace Transport;

StringLiteral const TraceType("UnreliableTransport");
wstring const TransportBehaviorWarning = L"UnreliableTransportHelper Warning:";

UnreliableTransportHelper::UnreliableTransportHelper(
    __in FabricNodeConfigSPtr nodeConfig) :
    nodeConfig_(nodeConfig)
    , transportBehaviorsListLock_()
{
}

UnreliableTransportHelper::~UnreliableTransportHelper()
{
    Close();
}

UnreliableTransportHelperSPtr UnreliableTransportHelper::CreateSPtr(
    __in FabricNodeConfigSPtr nodeConfig)
{
    return make_shared<UnreliableTransportHelper>(nodeConfig);
}

UnreliableTransportHelperUPtr UnreliableTransportHelper::CreateUPtr(
    __in FabricNodeConfigSPtr nodeConfig)
{
    return make_unique<UnreliableTransportHelper>(nodeConfig);
}

void UnreliableTransportHelper::InitializeHealthReportingComponent(
    Client::HealthReportingComponentSPtr const & healthClient)
{
    ASSERT_IF(healthClient == nullptr, "HealthReportingComponent is null on UnreliableTransportHelper");
    
    healthClient_ = healthClient;

    ASSERT_IF(nodeHealthReportManager_ != nullptr, "NodeEntity Manager already set on UnreliableTransportHelper");
    nodeHealthReportManager_ = Common::make_unique<NodeHealthReportManager>(
        *this, healthClient_, nodeConfig_);
    StartTimer();
}

ErrorCode UnreliableTransportHelper::Open()
{
    vector<pair<wstring, wstring>> transportBehaviors;
    GetTransportBehaviors(transportBehaviors);
    for (auto& behavior : transportBehaviors)
    {
        transportBehaviors_.emplace_back(behavior.first);
    }
    return ErrorCodeValue::Success;
}

ErrorCode UnreliableTransportHelper::Close()
{
    AcquireWriteLock lock(transportBehaviorsListLock_);
    transportBehaviors_.clear();
    StopTimer();
    return ErrorCodeValue::Success;
}

ErrorCode UnreliableTransportHelper::AddUnreliableTransportBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    UNREFERENCED_PARAMETER(queryResult);

    WriteInfo(
        TraceType,
        TraceId,
        "{0} - Enter AddUnreliableTransportBehavior[{1}]",
        activityId,
        queryArgument);

    wstring name;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableTransportBehavior::Name, name) ||
        name.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring behavior;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableTransportBehavior::Behavior, behavior) ||
        behavior.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    behavior.append(L" ");
    wstring workingDir = nodeConfig_->WorkingDir;
    wstring absolutePathToUnreliableTransportSettingsFile = Path::Combine(workingDir, UnreliableTransportConfig::SettingsFileName);

    // acquiring file lock
    FileWriterLock fileLock(absolutePathToUnreliableTransportSettingsFile);
    if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Failed to acquire UnreliableTransportSettings",
            "Lock file for adding new behavior. Aborting addition. Associated file={0}",
            absolutePathToUnreliableTransportSettingsFile);
        return ErrorCode(ErrorCodeValue::Timeout);
    }

    // reading existing parameters to avoid collisions of names		
    vector <pair<wstring, wstring>> parameters;
    auto errorCode = UnreliableTransportConfig::ReadFromINI(absolutePathToUnreliableTransportSettingsFile, parameters, false);
    if (!errorCode.IsSuccess())
    {
       WriteWarning(
           TraceType,
           "AddUnreliableTransportBehavior - ReadFromINI failed with '{0}'. Associated file = {1}",
           errorCode,
           absolutePathToUnreliableTransportSettingsFile);
       return errorCode;
    }

    std::pair<wstring, wstring> name_behavior = make_pair(name, behavior);
    bool found = false;

    // checking for conflict with existing name
    for (auto parameter = parameters.begin(); parameter != parameters.end(); ++parameter)
    {
        if (name == parameter->first)
        {
            // overwrites behavior with same name
            *parameter = name_behavior;
            found = true;

            WriteWarning(
                TraceType,
                TraceId,
                "Unreliable Transport Behavior {0} already exists, overwriting with new behavior",
                name);
        }
    }

    if (!found)
    {
        parameters.push_back(name_behavior);
        TransportBehavior transportBehavior(behavior);
        {
            AcquireWriteLock lock(transportBehaviorsListLock_);
            transportBehaviors_.emplace_back(name);
        }
    }

    errorCode = UnreliableTransportConfig::WriteToINI(absolutePathToUnreliableTransportSettingsFile, parameters, false /* it is already locked*/);
    if (errorCode.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "AddUnreliableTransportBehavior - UnreliableTransportConfig::WriteToINI on '{0}' failed with '{1}'",
            absolutePathToUnreliableTransportSettingsFile,
            errorCode);
    }

    return errorCode;
}

ErrorCode UnreliableTransportHelper::RemoveUnreliableTransportBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    UNREFERENCED_PARAMETER(queryResult);

    WriteInfo(
        TraceType,
        TraceId,
        "{0} - Enter RemoveUnreliableTransporBehavior[{1}]",
        activityId,
        queryArgument);

    wstring nodeName;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Node::Name, nodeName) ||
        nodeName.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring name;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableTransportBehavior::Name, name) ||
        name.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring workingDir = nodeConfig_->WorkingDir;
    wstring absolutePathToUnreliableTransportSettingsFile = Path::Combine(workingDir, UnreliableTransportConfig::SettingsFileName);

    // acquiring file lock
    FileWriterLock fileLock(absolutePathToUnreliableTransportSettingsFile);
    if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Failed to acquire UnreliableTransportSettings",
            "Lock file for removing new behavior. Aborting removal. Associated file={0}",
            absolutePathToUnreliableTransportSettingsFile);
        return ErrorCode(ErrorCodeValue::Timeout);
    }

    // reading existing parameters to a vector
    vector <pair<wstring, wstring>> parameters;
    auto errorCode = UnreliableTransportConfig::ReadFromINI(absolutePathToUnreliableTransportSettingsFile, parameters, false /*it is already locked */);
    if (!errorCode.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "RemoveUnreliableTransportBehavior - ReadFromINI failed with '{0}'. Associated file = {1}",
            errorCode,
            absolutePathToUnreliableTransportSettingsFile);
        return errorCode;
    }

    if (name != L"*")
    {
        // removing requested parameter
        bool foundBehavior = false;
        for (auto parameter = parameters.begin(); parameter != parameters.end(); ++parameter)
        {
            // checking for colision
            if (name == parameter->first)
            {
                parameters.erase(parameter);
                foundBehavior = true;
                WriteInfo(TraceType, TraceId, "Removing behavior:{0} from collection", name);
                break;
            }
        }

        if (!foundBehavior)
        {
            WriteWarning(
                TraceType,
                TraceId,
                "Unreliable Transport Behavior {0} does not exist",
                name);
            return ErrorCode(ErrorCodeValue::NotFound);
        }
    }
    else
    {
        // clearing all behaviors
        parameters.clear();
    }
    {
        AcquireWriteLock lock(transportBehaviorsListLock_);
        transportBehaviors_.erase(std::remove(transportBehaviors_.begin(), transportBehaviors_.end(), name), transportBehaviors_.end());
    }

    WriteInfo(TraceType, TraceId, "Removing behavior:{0} from settings", name);
    errorCode = UnreliableTransportConfig::WriteToINI(absolutePathToUnreliableTransportSettingsFile, parameters, false /* it is already locked */);
    if (errorCode.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "RemoveUnreliableTransportBehavior - UnreliableTransportConfig::WriteToINI on '{0}' failed with '{1}'",
            absolutePathToUnreliableTransportSettingsFile,
            errorCode);
    }

    return errorCode;
}

ErrorCode UnreliableTransportHelper::GetTransportBehaviors(
    std::vector<pair<wstring, wstring>>& transportBehaviors)
{
    wstring absolutePathToUnreliableTransportSettingsFile = Path::Combine(nodeConfig_->WorkingDir, UnreliableTransportConfig::SettingsFileName);

    // acquiring file lock
    FileWriterLock fileLock(absolutePathToUnreliableTransportSettingsFile);
    if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Failed to acquire UnreliableTransportSettings",
            "Lock file for removing new behavior. Aborting removal. Associated file={0}",
            absolutePathToUnreliableTransportSettingsFile);
        return ErrorCode(ErrorCodeValue::Timeout);
    }

    return UnreliableTransportConfig::ReadFromINI(absolutePathToUnreliableTransportSettingsFile, transportBehaviors, false/*lock*/);
}

ErrorCode UnreliableTransportHelper::GetTransportBehaviors(
    QueryArgumentMap const &,
    ActivityId const &,
    __out QueryResult & queryResult)
{
    vector <pair<wstring, wstring>> parameters;
    ErrorCode errorCode = GetTransportBehaviors(parameters);
    if (errorCode.IsSuccess())
    {
        vector<wstring> tranportBehaviors;
        tranportBehaviors.reserve(parameters.size());
        for (auto& behavior : parameters)
        {
            tranportBehaviors.emplace_back(behavior.first + L"=" + behavior.second);
        }
        queryResult = QueryResult(move(tranportBehaviors));
    }
    return errorCode;
}

void UnreliableTransportHelper::StartTimer()
{
    weak_ptr<ComponentRoot> rootWPtr = this->shared_from_this();
    wstring nodeName = nodeConfig_->InstanceName;

    unrelibleTxTimer_ = Timer::Create(TimerTagDefault, [rootWPtr, nodeName](TimerSPtr const& timer) {
        timer->Cancel();
        UnreliableTransportHelper::UnreliableTransportTimerCallback(rootWPtr, wformatString("{0}{1}", TransportBehaviorWarning, nodeName));
    });
    auto span = TestabilityConfig::GetConfig().UnreliableTransportRecurringTimer;
    unrelibleTxTimer_->Change(span);
}

void UnreliableTransportHelper::StopTimer()
{
    if (unrelibleTxTimer_ != nullptr)
    {
        unrelibleTxTimer_->Cancel();
        unrelibleTxTimer_.reset();
    }
}

void UnreliableTransportHelper::UnreliableTransportTimerCallback(
    weak_ptr<ComponentRoot> const & rootWPtr,
    wstring behaviorName)
{
    auto thisSPtr = rootWPtr.lock();
    if (!thisSPtr)
    {
        return;
    }
    UnreliableTransportHelper& thisRef = (UnreliableTransportHelper&) (*thisSPtr);
    thisRef.HandleUnreliableTransportTimerCallback(behaviorName);
}

void UnreliableTransportHelper::HandleUnreliableTransportTimerCallback(const std::wstring&)
{
    AcquireWriteLock lock(transportBehaviorsListLock_);
    unrelibleTxTimer_.reset();

    ASSERT_IF(nodeHealthReportManager_ == nullptr, "HealthReportingComponent is null on UnreliableTransportHelper");

    // check if any of the unreliable behaviors are still active.
    if (transportBehaviors_.size() > 0)
    {
        wstring warnings;
        for (auto& transportBehavior : transportBehaviors_)
        {
            int64 elapsedTime = -transportBehavior.initedTime_.UnSafeRemainingDuration().get_Ticks(); // DateTime::UnSafeRemainingDuration is prevTime - Now 
            int64 thresholdTime = TestabilityConfig::GetConfig().UnreliableTransportWarningTimeout.get_Ticks();
            if (elapsedTime > thresholdTime)
            {
                warnings += transportBehavior.behaviorName_ + L":";
            }
        }
        if (warnings.length() > 0)
        {
            wstring sourceName = wformatString("{0}{1}", TransportBehaviorWarning, nodeConfig_->InstanceName);
            nodeHealthReportManager_->ReportHealthWithTTL(
                sourceName,
                SystemHealthReportCode::Testability_UnreliableTransportBehaviorTimeOut,
                wformatString("{0}{1}", StringResource::Get(IDS_TESTABILITY_UnreliableTransportWarning), warnings),
                SequenceNumber::GetNext(),
                TestabilityConfig::GetConfig().UnreliableTransportRecurringTimer);
        }
    }
    StartTimer();
}
