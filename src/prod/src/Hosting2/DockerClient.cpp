// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/rawptrstream.h"

using namespace std;
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace Hosting2;
using namespace Common;
using namespace concurrency;
using namespace ServiceModel;

StringLiteral const TraceType_Activator("DockerClient");

struct DockerClient::ContainerInfo
{
    ContainerInfo()
        : AppHostId()
        , NodeId()
        , ReportDockerHealthStatus(false)
    {
    }

    ContainerInfo(
        wstring const & appHostId,
        wstring const & nodeId,
        bool reportDockerHealthStatus)
        : AppHostId(appHostId)
        , NodeId(nodeId)
        , ReportDockerHealthStatus(reportDockerHealthStatus)
    {
    }

    wstring AppHostId;
    wstring NodeId;
    bool ReportDockerHealthStatus;
};

class DockerClient::ContainerEventManager
{    
public:
    ContainerEventManager(
        std::string const & serverAddress,
        DockerClient & owner) 
        : owner_(owner)
        , eventClient_()
        , lastScanDateTime_(DateTime::Now())
        , untilDateTime_()
        , offsetTime_(L"1970-01-01T00:00:00.000Z")
        , lastScantimeInSec_()
        , untilTimeInSec_()
        , eventFilters_()
        , requestUri_()
        , isEventReadQuery_(false)
        , continuousRegistrationFailureCount_(0)

    {
        http_client_config config;
        utility::seconds timeout(LONG_MAX);
        config.set_timeout(timeout);

        auto eventClient = make_unique<http_client>(string_t(serverAddress), config);
        eventClient_ = move(eventClient);

        InitializeEventQueryFilters();
    }

    ~ContainerEventManager()
    {
    }

public:

    bool RegisterForEvents()
    {
        QueryEvents();

        return true;
    }

private:
    
    void InitializeEventQueryFilters()
    {
        web::json::value type = web::json::value::array(1);
        type[0] = web::json::value::string(U("container"));

        web::json::value events;
        
        if (HostingConfig::GetConfig().EnableDockerHealthCheckIntegration)
        {
            events = web::json::value::array(3);
            events[2] = web::json::value::string(U("health_status"));
        }
        else
        {
            events = web::json::value::array(2);
        }
        
        events[0] = web::json::value::string(U("stop"));
        events[1] = web::json::value::string(U("die"));

        eventFilters_[U("event")] = events;
        eventFilters_[U("type")] = type;
    }

    void QueryEvents()
    {
        if (continuousRegistrationFailureCount_ >= HostingConfig::GetConfig().ContainerEventManagerMaxContinuousFailure)
        {
            StringLiteral msgFormat("ContainerEventManager failed to register for container events. ContinuousFailureCount={0}. Terminating process for self-repair.");

            WriteError(
                TraceType_Activator,
                msgFormat,
                continuousRegistrationFailureCount_);

            continuousRegistrationFailureCount_ = 0;
            //Assert::CodingError(msgFormat, continuousRegistrationFailureCount_);
        }

        this->SetRequestUri();

        try
        {
            eventClient_->request(methods::GET, requestUri_).then(
                [this](pplx::task<http_response> const & responseTask) -> void
                {
                    OnEventRequestCompleted(responseTask);
                });
        }
        catch (std::exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::QueryEvents: failed with error='{0}', RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            QueryEvents();
        }
    }

    void OnEventRequestCompleted(pplx::task<http_response> const & responseTask)
    {
        try
        {
            auto response = responseTask.get();
            auto statusCode = response.status_code();

            if (statusCode != web::http::status_codes::OK)
            {
                isEventReadQuery_ = false;
                continuousRegistrationFailureCount_++;

                WriteWarning(
                    TraceType_Activator,
                    "ContainerEventManager::OnEventRequestCompleted returned statuscode {0}. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                    statusCode,
                    requestUri_,
                    isEventReadQuery_,
                    continuousRegistrationFailureCount_);

                QueryEvents();
                return;
            }

            if(isEventReadQuery_)
            {
                ReadAllEvents(response);
            }
            else
            {
                AwaitEvents(response);
            }
        }
        catch (exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::OnEventRequestCompleted: error='{0}'. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            RetryEventQueryWithDelay();
            return;
        }
    }

    void AwaitEvents(http_response const & response)
    {
        try
        {
            response.body().read().then(
                [this](pplx::task<streams::istream::int_type> const & readTask) -> void
                {
                    OnEventsAvailable(readTask);
                });
        }
        catch (exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::AwaitEvents: error='{0}'. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            QueryEvents();
            return;
        }
    }

    void OnEventsAvailable(pplx::task<streams::istream::int_type> const & eventsAvailableTask)
    {
        try
        {
            eventsAvailableTask.get();

            isEventReadQuery_ = true;

            QueryEvents();
        }
        catch (exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::OnEventsAvailable: error='{0}'. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            QueryEvents();
            return;
        }
    }

    void ReadAllEvents(http_response const & response)
    {
        try
        {
            response.extract_utf8string().then(
                [this](pplx::task<string> const & eventReadTask) -> void
            {
                OnReadAllEvents(eventReadTask);
            });
        }
        catch (exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::ReadAllEvents: error='{0}'. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            QueryEvents();
            return;
        }
    }

    void OnReadAllEvents(pplx::task<string> const & eventReadTask)
    {
        try
        {
            auto eventString = eventReadTask.get();

            vector<ContainerEventConfig> events;
            auto error = this->DeserializeResponse(eventString, events);
            if (!error.IsSuccess())
            {
                isEventReadQuery_ = false;
                continuousRegistrationFailureCount_++;

                WriteWarning(
                    TraceType_Activator,
                    "ContainerEventManager::DeserializeResponse failed with error {0}. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                    error,
                    requestUri_,
                    isEventReadQuery_,
                    continuousRegistrationFailureCount_);

                QueryEvents();
                return;
            }

            owner_.ProcessContainerEvents(move(events));

            lastScanDateTime_ = untilDateTime_;
            lastScantimeInSec_ = untilTimeInSec_;

            isEventReadQuery_ = false;

            // Reset the continuous failure count at the end of successful iteration.
            continuousRegistrationFailureCount_ = 0;

            QueryEvents();
        }
        catch (exception const & e)
        {
            isEventReadQuery_ = false;
            continuousRegistrationFailureCount_++;

            WriteError(
                TraceType_Activator,
                "ContainerEventManager::OnReadAllEvents: error='{0}'. RequestUri='{1}', IsEventReadQuery='{2}', ContinuousFailureCount='{3}'.",
                e.what(),
                requestUri_,
                isEventReadQuery_,
                continuousRegistrationFailureCount_);

            QueryEvents();
            return;
        }
    }

    ErrorCode DeserializeResponse(string const & jsonStr, __out vector<ContainerEventConfig> & events)
    {
        vector<CHAR> buffer;
        std::stack<CHAR> parenthesis;

        for (auto it = jsonStr.begin(); it != jsonStr.end(); it++)
        {
            buffer.push_back(*it);
            if (*it == '{')
            {
                parenthesis.push(*it);
            }
            else if (*it == '}')
            {
                parenthesis.pop();
                if (parenthesis.size() == 0)
                {
                    ContainerEventConfig config;
                    JsonReader::InitParams initParams((char*)(buffer.data()), static_cast<ULONG>(buffer.size()));
                    auto jsonReader = make_com<JsonReader>(initParams);
                    JsonReaderVisitor readerVisitor(jsonReader.GetRawPointer(), JsonSerializerFlags::Default);
                    auto hr = readerVisitor.Deserialize(config);
                    if (hr == S_OK)
                    {
                        events.push_back(config);
                    }
                    else
                    {
                        return ErrorCode::FromHResult(hr);
                    }
                    buffer.clear();
                }
            }
        }
        return ErrorCodeValue::Success;
    }

    string GetTimeQueryParameter(DateTime time)
    {
        auto sinceTime = time - offsetTime_;
        auto elapsedTime = TimeSpan::FromTicks(sinceTime.Ticks);

        return formatString("{0}", elapsedTime.TotalSeconds());
    }
    
    void SetEventQueryUntilTime()
    {
        untilDateTime_ = DateTime::Now();

        if ((untilDateTime_ - lastScanDateTime_).Seconds == 0)
        {
            untilDateTime_ = untilDateTime_ + TimeSpan::FromSeconds(1);
        }

        untilTimeInSec_ = GetTimeQueryParameter(untilDateTime_);
    }

    void SetRequestUri()
    {
        lastScantimeInSec_ = GetTimeQueryParameter(lastScanDateTime_);

        uri_builder builder(U("/events"));

        builder.append_query(U("since"), lastScantimeInSec_);

        if (isEventReadQuery_)
        {
            SetEventQueryUntilTime();

            builder.append_query(U("until"), untilTimeInSec_);
        }

        builder.append_query(U("filters"), eventFilters_);
        
        requestUri_ = builder.to_string();
    }

    void RetryEventQueryWithDelay()
    {
        auto retryDelay = HostingConfig::GetConfig().ContainerEventManagerFailureBackoffInSec;

        Threadpool::Post(
            [this]() { QueryEvents(); },
            TimeSpan::FromSeconds(retryDelay));
    }

private:
    DockerClient & owner_;
    std::unique_ptr<http_client> eventClient_;
    DateTime lastScanDateTime_;
    DateTime untilDateTime_;
    const DateTime offsetTime_;
    string lastScantimeInSec_;
    string untilTimeInSec_;
    web::json::value eventFilters_;
    string requestUri_;
    bool isEventReadQuery_;
    int continuousRegistrationFailureCount_;
};

DockerClient::DockerClient(
    std::string const & serverAddress,
    ProcessActivationManager & processActivationManager) :
    isInitialized_(false),
    isEventManagerRegistered_(false),
    processActivationManager_(processActivationManager)
{
    auto client = make_unique<http_client>(string_t(serverAddress));
    httpClient_ = move(client);
    eventManager_ = make_unique<ContainerEventManager>(serverAddress, *this);
    std::string delimiter = "//";
    std::vector<std::string> tokens;
    StringUtility::Split(serverAddress, tokens, delimiter);
    if (tokens.size() == 2)
    {
        serverAddress_ = tokens[1];
    }
}

DockerClient::~DockerClient()
{
}

bool DockerClient::CheckInitializeDocker()
{
    bool retval;
    if (isEventManagerRegistered_)
    {
        retval = true;
    }
    else
    {
        AcquireExclusiveLock lock(initializerLock_);
        if (!isEventManagerRegistered_)
        {
            retval = eventManager_->RegisterForEvents();
            isEventManagerRegistered_ = retval;
        }
        retval = true;
    }
    return isEventManagerRegistered_;
}

bool DockerClient::Initialize()
{
    return CheckInitializeDocker();
}

void DockerClient::ProcessContainerEvents(vector<ContainerEventConfig> && events)
{
    map<wstring /* sf generated container Id*/,
        ContainerEventConfig,
        IsLessCaseInsensitiveComparer<wstring>> healthEventMap;

    for (auto const & containerEvent : events)
    {
        WriteNoise(
            TraceType_Activator,
            "Received event for Container:{0}, Action:{1}, ID:{2}, Time:{3}, IsHealthEvent:{4}, IsHealthy:{5}.",
            containerEvent.ActorConfig.Attributes.Name,
            containerEvent.Action,
            containerEvent.ActorConfig.Id,
            containerEvent.Time,
            containerEvent.IsHealthEvent(),
            containerEvent.IsHealthStatusHealthy());

        auto containerSfId = containerEvent.ActorConfig.Attributes.Name;

        if (HostingConfig::GetConfig().EnableDockerHealthCheckIntegration &&
            containerEvent.IsHealthEvent())
        {
            if (!ShouldReportDockerHealthStatus(containerSfId))
            {
                WriteNoise(
                    TraceType_Activator,
                    "Skipping docker health status reporting for Container:{0}.",
                    containerSfId);

                continue;
            }

            auto iter = healthEventMap.find(containerSfId);
            if (iter == healthEventMap.end())
            {
                healthEventMap.insert(make_pair(containerSfId, containerEvent));

                continue;
            }

            //
            // Only keep the latest health status
            //
            if (iter->second.Time < containerEvent.Time)
            {
                iter->second = containerEvent;
            }
        }
        else
        {
            this->NotifyOnContainerTermination(
                containerSfId, 
                containerEvent.ActorConfig.Attributes.ExitCode);
        }
    }

    if (!(healthEventMap.empty()))
    {
        map<wstring, /* Node id*/
            vector<ContainerHealthStatusInfo>,
            IsLessCaseInsensitiveComparer<wstring>> healthStatusInfoMap;

        //
        // Group by NodeId.
        //
        for (auto const & healthEvent : healthEventMap)
        {
            wstring nodeId;
            wstring appHostId;

            if (this->TryGetNodeIdAndAppHostId(healthEvent.first, nodeId, appHostId))
            {
                auto iter = healthStatusInfoMap.find(nodeId);
                if (iter == healthStatusInfoMap.end())
                {
                    auto res = healthStatusInfoMap.insert(make_pair(move(nodeId), move(vector<ContainerHealthStatusInfo>())));

                    iter = res.first;
                }

                iter->second.push_back(
                    move(
                        ContainerHealthStatusInfo(
                            appHostId,
                            healthEvent.first,
                            healthEvent.second.Time,
                            healthEvent.second.IsHealthStatusHealthy())));
            }
        }

        for (auto const & healthStatusInfo : healthStatusInfoMap)
        {
            this->NotifyOnContainerHealthStatus(healthStatusInfo.first, healthStatusInfo.second);
        }
    }
}

void DockerClient::NotifyOnContainerTermination(
    std::wstring const & containerName,
    DWORD exitCode)
{
    std::wstring nodeId;
    std::wstring appServiceId;

    {
        AcquireExclusiveLock lock(mapLock_);
        auto it = containerIdMap_.find(containerName);
        if (it != containerIdMap_.end())
        {
            appServiceId = it->second.AppHostId;
            nodeId = it->second.NodeId;

            containerIdMap_.erase(it);
        }
    }

    if (!appServiceId.empty() && !nodeId.empty())
    {
        processActivationManager_.OnContainerTerminated(appServiceId, nodeId, exitCode);
    }
}

bool DockerClient::TryGetNodeIdAndAppHostId(
    __in std::wstring const & containerName,
    __out std::wstring & nodeId,
    __out std::wstring & appHostId)
{
    wstring tempNodeId;
    wstring tempAppHostId;

    {
        AcquireExclusiveLock lock(mapLock_);

        auto it = containerIdMap_.find(containerName);
        if (it != containerIdMap_.end())
        {
            tempAppHostId = it->second.AppHostId;
            tempNodeId = it->second.NodeId;
        }
    }

    if (!tempNodeId.empty() && !tempAppHostId.empty())
    {
        nodeId = move(tempNodeId);
        appHostId = move(tempAppHostId);

        return true;
    }

    return false;
}

bool DockerClient::ShouldReportDockerHealthStatus(wstring const & containerName)
{
    {
        AcquireExclusiveLock lock(mapLock_);

        auto it = containerIdMap_.find(containerName);
        
         if (it != containerIdMap_.end() && it->second.ReportDockerHealthStatus)
         {
             return true;
         }
    }

    return false;
}

void DockerClient::NotifyOnContainerHealthStatus(
    std::wstring const & nodeId,
    std::vector<ContainerHealthStatusInfo> const & healthStatusInfo)
{
    processActivationManager_.OnContainerHealthCheckStatusEvent(nodeId, healthStatusInfo);
}

void DockerClient::ClearContainerMap()
{
    containerIdMap_.clear();
}

void DockerClient::AddContainerToMap(
    std::wstring const & containerName,
    std::wstring const & appHostId,
    std::wstring const & nodeId,
    bool reportDockerHealthStatus)
{
    AcquireExclusiveLock lock(mapLock_);
    this->containerIdMap_[containerName] = move(ContainerInfo(appHostId, nodeId, reportDockerHealthStatus));
}
