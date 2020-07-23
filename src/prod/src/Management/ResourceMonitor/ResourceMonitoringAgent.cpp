// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Hosting2;
using namespace Transport;

StringLiteral const TraceComponent("ResourceMonitoringAgent");

ResourceMonitoringAgent::ResourceMonitoringAgent(Hosting2::ComFabricRuntime * pRuntime, Transport::IpcClient & ipcClient) :
    pRuntime_(pRuntime),
    ipcClient_(ipcClient)
{
    WriteNoise(TraceComponent, "ResourceMonitoringAgent constructed.");
}

ResourceMonitoringAgent::~ResourceMonitoringAgent()
{
    WriteNoise(TraceComponent, "ResourceMonitoringAgent destructed.");
}

void ResourceMonitoringAgent::MonitorCallback(TimerSPtr const & timer)
{
    UNREFERENCED_PARAMETER(timer);
    MeasureResourceUsage(timer);
}

void ResourceMonitoringAgent::SendCallback(TimerSPtr const & timer)
{
    UNREFERENCED_PARAMETER(timer);
    SendResourceUsage(timer);
}

void ResourceMonitoringAgent::ProcessIpcMessage(Message & message, IpcReceiverContextUPtr & context)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(context);

    if (message.Action == Management::ResourceMonitor::HostUpdateRM::HostUpdateRMAction)
    {
        Management::ResourceMonitor::HostUpdateRM requestBody;
        if (!message.GetBody<Management::ResourceMonitor::HostUpdateRM>(requestBody))
        {
            auto error = ErrorCode::FromNtStatus(message.Status);
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "GetBody<HostUpdateRM> failed: Message={0}, ErrorCode={1}",
                message,
                error);
            return;
        }
        UpdateFromHost(requestBody);
        MessageUPtr reply = make_unique<Message>();

        context->Reply(move(reply));
    }
    else
    {
        WriteError(
            TraceComponent,
            this->TraceId,
            "Unknown Message={0}",
            message);
    }
}

void ResourceMonitoringAgent::UpdateFromHost(Management::ResourceMonitor::HostUpdateRM & update)
{
    WriteInfo(TraceComponent, "ResourceMonitoringAgent got an update from host {0}", update.HostEvents.size());

    auto & hostEvents = update.HostEvents;

    {
        AcquireExclusiveLock grab(lock_);

        for (auto updateEvent : hostEvents)
        {
            if (ResourceMonitorServiceConfig::GetConfig().IsTestMode)
            {
                WriteInfo(TraceComponent, "Update event host {0}", updateEvent);
            }

            if (updateEvent.IsUp)
            {
                if (updateEvent.IsLinuxContainerIsolated)
                {
                    auto partitionIter = partitionsToMonitor_.find(updateEvent.PartitionId);
                    if (partitionIter == partitionsToMonitor_.end())
                    {
                        partitionsToMonitor_[updateEvent.PartitionId] = ResourceDescription(
                            updateEvent.CodePackageInstanceIdentifier,
                            updateEvent.AppName,
                            updateEvent.IsLinuxContainerIsolated);
                    }
                    partitionsToMonitor_[updateEvent.PartitionId].UpdateAppHosts(updateEvent.AppHostId, true);
                }
                else
                {
                    hostsToMonitor_[updateEvent.AppHostId] = ResourceDescription(
                        updateEvent.CodePackageInstanceIdentifier,
                        updateEvent.AppName,
                        updateEvent.IsLinuxContainerIsolated);
                }
            }
            else
            {
                if (updateEvent.IsLinuxContainerIsolated)
                {
                    auto partitionIter = partitionsToMonitor_.find(updateEvent.PartitionId);
                    if (partitionIter != partitionsToMonitor_.end())
                    {
                        partitionsToMonitor_[updateEvent.PartitionId].UpdateAppHosts(updateEvent.AppHostId, false);
                        if (partitionIter->second.AppHosts.size() == 0)
                        {
                            partitionsToMonitor_.erase(updateEvent.PartitionId);
                        }
                    }
                }
                else
                {
                    hostsToMonitor_.erase(updateEvent.AppHostId);
                }

            }
        }
    }
}

void ResourceMonitoringAgent::MeasureResourceUsage(TimerSPtr const & timer)
{
    auto hosts = GetHostsToMeasure();

    WriteInfo(TraceComponent, "ResourceMonitoringAgent {0} hosts to measure", hosts.size());

    if (hosts.size() > 0)
    {
        Management::ResourceMonitor::ResourceMeasurementRequest measureRequest(move(hosts), nodeId_.ToString());

        MessageUPtr request = make_unique<Message>(measureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Management::ResourceMonitor::ResourceMeasurementRequest::ResourceMeasureRequestAction));

        auto resourceOperation = fabricHostClient_->BeginRequest(
            move(request),
            ResourceMonitorServiceConfig::GetConfig().FabricHostCommunicationInterval,
            [this, timer](AsyncOperationSPtr const & operation) { OnGetResponse(operation, false, timer); },
            CreateAsyncOperationRoot()
        );

        OnGetResponse(resourceOperation, true, timer);
    }
    else
    {
        timer->Change(ResourceMonitorServiceConfig::GetConfig().ResourceMonitorInterval);
    }
}

void ResourceMonitoringAgent::OnGetResponse(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, TimerSPtr const & timer)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    auto thisSPtr = operation->Parent;

    MessageUPtr message;
    auto error = fabricHostClient_->EndRequest(operation, message);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "ResourceMonitoringAgent::OnGetResponse error={0}",
            error);
    }
    else
    {
        Management::ResourceMonitor::ResourceMeasurementResponse requestBody;
        if (!message->GetBody<Management::ResourceMonitor::ResourceMeasurementResponse>(requestBody))
        {
            error = ErrorCode::FromNtStatus(message->Status);
            WriteWarning(
                TraceComponent,
                "GetBody<ResourceMeasurementResponse> failed: Message={0}, ErrorCode={1}",
                message,
                error);
        }
        else
        {
            UpdateWithMeasurements(requestBody.Measurements);
        }
    }

    timer->Change(ResourceMonitorServiceConfig::GetConfig().ResourceMonitorInterval);
}

void ResourceMonitoringAgent::SendResourceUsage(TimerSPtr const & timer)
{
    // Mapping from partition id to a pair of cpu usage and memory usage
    // When a partition is activated in exclusive mode there may be multiple CPs.
    std::map<Reliability::PartitionId, Management::ResourceMonitor::ResourceUsage> partitionUsage;

    {
        AcquireReadLock grab(lock_);

        for (auto & hostMeasured : hostsToMonitor_)
        {
            if (hostMeasured.second.resourceUsage_.IsValid())
            {
                if (hostMeasured.second.IsExclusive)
                {
                    Reliability::PartitionId partitionId(hostMeasured.second.PartitionId);
                    // For other containers summarize usage for all code packages
                    partitionUsage[partitionId].CpuUsage += hostMeasured.second.resourceUsage_.cpuRate_;
                    partitionUsage[partitionId].MemoryUsage += hostMeasured.second.resourceUsage_.memoryUsage_;

                    partitionUsage[partitionId].CpuUsageRaw += hostMeasured.second.resourceUsage_.cpuRateCurrent_;
                    partitionUsage[partitionId].MemoryUsageRaw += hostMeasured.second.resourceUsage_.memoryUsageCurrent_;
                }
            }
        }

        for (auto & hostMeasured : partitionsToMonitor_)
        {
            if (hostMeasured.second.resourceUsage_.IsValid() && hostMeasured.second.AppHosts.size() > 0)
            {
                if (hostMeasured.second.IsExclusive && hostMeasured.second.IsLinuxContainerIsolated)
                {
                    Reliability::PartitionId partitionId(hostMeasured.second.PartitionId);
                    // For Linux isolated containers (crio) - stats are retrieved by reading pod cgroup
                    // This represents the usage on service package level (for all code packages)
                    // Therefore there is no need in summarizing usage for each code package
                    partitionUsage[partitionId].CpuUsage = hostMeasured.second.resourceUsage_.cpuRate_;
                    partitionUsage[partitionId].MemoryUsage = hostMeasured.second.resourceUsage_.memoryUsage_;

                    partitionUsage[partitionId].CpuUsageRaw = hostMeasured.second.resourceUsage_.cpuRateCurrent_;
                    partitionUsage[partitionId].MemoryUsageRaw = hostMeasured.second.resourceUsage_.memoryUsageCurrent_;
                }
            }
        }
    }

    WriteInfo(TraceComponent, "ResourceMonitoringAgent Sending ResourceUsage for {0} partitions", partitionUsage.size());

    if (ResourceMonitorServiceConfig::GetConfig().IsTestMode)
    {
        for (auto & usage : partitionUsage)
        {
            WriteInfo(TraceComponent, "Resource send {0} {1}", usage.first, usage.second);
        }
    }

    if (partitionUsage.size() != 0)
    {
        Management::ResourceMonitor::ResourceUsageReport resourceReport(move(partitionUsage));
        MessageUPtr message = make_unique<Message>(resourceReport);

        message->Headers.Add(Transport::ActorHeader(Transport::Actor::RA));
        message->Headers.Add(Transport::ActionHeader(Management::ResourceMonitor::ResourceUsageReport::ResourceUsageReportAction));

        auto operation = ipcClient_.SendOneWay(move(message));
    }

    timer->Change(ResourceMonitorServiceConfig::GetConfig().ResourceSendingInterval);
}

void ResourceMonitoringAgent::TraceCallback(Common::TimerSPtr const & timer)
{
    WriteInfo(TraceComponent, "Tracing started with {0} hosts and {1} partitions", hostsToMonitor_.size(), partitionsToMonitor_.size());

    AcquireReadLock grab(lock_);

    for (auto & hostMeasured : hostsToMonitor_)
    {
        if (hostMeasured.second.resourceUsage_.IsValid())
        {
            //this means that this is shared host
            //for these RM will trace out resource info
            //for exclusive RA is tracing
            if (!hostMeasured.second.IsExclusive)
            {
                resourceMonitorEventSource_.ResourceUsageReportSharedHost(
                    hostMeasured.second.appName_,
                    hostMeasured.second.codePackageIdentifier_.ServicePackageInstanceId.ServicePackageName,
                    hostMeasured.second.codePackageIdentifier_.CodePackageName,
                    nodeId_.ToString(),
                    hostMeasured.second.resourceUsage_.cpuRate_,
                    hostMeasured.second.resourceUsage_.memoryUsage_,
                    hostMeasured.second.resourceUsage_.cpuRateCurrent_,
                    hostMeasured.second.resourceUsage_.memoryUsageCurrent_
                );
            }
        }
    }

    for (auto & hostMeasured : partitionsToMonitor_)
    {
        if (hostMeasured.second.resourceUsage_.IsValid())
        {
            //this means that this is shared host
            //for these RM will trace out resource info
            //for exclusive RA is tracing
            if (!hostMeasured.second.IsExclusive)
            {
                resourceMonitorEventSource_.ResourceUsageReportSharedHost(
                    hostMeasured.second.appName_,
                    hostMeasured.second.codePackageIdentifier_.ServicePackageInstanceId.ServicePackageName,
                    hostMeasured.second.codePackageIdentifier_.CodePackageName,
                    nodeId_.ToString(),
                    hostMeasured.second.resourceUsage_.cpuRate_,
                    hostMeasured.second.resourceUsage_.memoryUsage_,
                    hostMeasured.second.resourceUsage_.cpuRateCurrent_,
                    hostMeasured.second.resourceUsage_.memoryUsageCurrent_
                );
            }
        }
    }

    timer->Change(ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval);
}

std::vector<std::wstring> ResourceMonitoringAgent::GetHostsToMeasure()
{
    std::vector<std::wstring> hosts;
    {
        AcquireReadLock grab(lock_);

        auto now = DateTime::Now();

        for (auto & host : hostsToMonitor_)
        {
            if (host.second.resourceUsage_.ShouldCheckUsage(now))
            {
                hosts.push_back(host.first);
            }
        }

        for (auto & host : partitionsToMonitor_)
        {
            if (host.second.resourceUsage_.ShouldCheckUsage(now) && host.second.AppHosts.size() > 0)
            {
                // take first app host for the partition based reports
                hosts.push_back(*host.second.AppHosts.begin());
            }
        }
    }

    if (hosts.size() > ResourceMonitorServiceConfig::GetConfig().MaxHostsToMeasure)
    {
        std::random_shuffle(hosts.begin(), hosts.end());
        hosts.resize(ResourceMonitorServiceConfig::GetConfig().MaxHostsToMeasure);
    }

    return hosts;
}

void ResourceMonitoringAgent::UpdateWithMeasurements(std::map<std::wstring, Management::ResourceMonitor::ResourceMeasurement> const & measurements)
{
    WriteInfo(TraceComponent, "ResourceMonitoringAgent received {0} measurements", measurements.size());

    {
        AcquireExclusiveLock grab(lock_);

        for (auto & measurement : measurements)
        {
            auto itHost = hostsToMonitor_.find(measurement.first);
            if (itHost != hostsToMonitor_.end())
            {
                itHost->second.resourceUsage_.Update(measurement.second);
                if (ResourceMonitorServiceConfig::GetConfig().IsTestMode)
                {
                    WriteInfo(TraceComponent, "Resource update {0} {1}", itHost->first, itHost->second.resourceUsage_);
                }
            }
            else
            {
                // check partitions
                for (auto& partition : partitionsToMonitor_)
                {
                    auto foundAppHost = partition.second.AppHosts.find(measurement.first);
                    if (foundAppHost != partition.second.AppHosts.end())
                    {
                        partition.second.resourceUsage_.Update(measurement.second);
                        if (ResourceMonitorServiceConfig::GetConfig().IsTestMode)
                        {
                            WriteInfo(TraceComponent, "Resource update for partition {0} {1}", partition.first, partition.second.resourceUsage_);
                        }
                    }
                }
            }
        }
    }
}

void ResourceMonitoringAgent::CreateTimers()
{
    auto measurementRoot = CreateComponentRoot();
    auto sendReportsRoot = CreateComponentRoot();

    monitorTimerSPtr_ = Timer::Create(
        "ResourceMonitor.Monitor",
        [this, measurementRoot](TimerSPtr const & monitorTimerSPtr_)
    {
        this->MonitorCallback(monitorTimerSPtr_);
    });

    sendingTimerSPtr_ = Timer::Create(
        "ResourceMonitor.Sender",
        [this, sendReportsRoot](TimerSPtr const & sendingTimerSPtr_)
    {
        this->SendCallback(sendingTimerSPtr_);
    });

    tracingTimerSPtr_ = Timer::Create(
        "ResourceMonitor.Tracing",
        [this, sendReportsRoot](TimerSPtr const & tracingTimerSPtr_)
    {
        this->TraceCallback(tracingTimerSPtr_);
    });
}

void ResourceMonitoringAgent::RegisterMessageHandler()
{ 
    auto root = CreateComponentRoot();
    ipcClient_.RegisterMessageHandler(
        Transport::Actor::ResourceMonitor,
        [root, this](MessageUPtr & message, IpcReceiverContextUPtr & context){ProcessIpcMessage(*message, context); },
        false
    );
    WriteInfo(TraceComponent, "ResourceMonitoringAgent registered the messagehandler ");
}

class ResourceMonitoringAgent::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(
        __in ResourceMonitoringAgent & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
         owner_(owner),
         timeoutHelper_(timeout)
    {
    }

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        WriteNoise(TraceComponent, "ResourceMonitoringAgent OpenAsyncOperation::End");
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceComponent, "ResourceMonitoringAgent::OpenAsyncOperation Started");

        ComFabricRuntime * castedRuntimePtr = owner_.pRuntime_;

        owner_.RegisterMessageHandler();
        owner_.hostId_ = castedRuntimePtr->Runtime->Host.get_Id();

        FabricNodeContextSPtr fabricNodeContext;
        auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "ResourceMonitoringAgent unable to get fabric node context");
            TryComplete(thisSPtr, error);
            return;
        }

        Federation::NodeId nodeId;
        if (!Federation::NodeId::TryParse(fabricNodeContext->NodeId, nodeId))
        {
            WriteWarning(TraceComponent, "ResourceMonitoringAgent unable to get fabric node id");
            TryComplete(thisSPtr, error);
            return;
        }

        owner_.nodeId_ = nodeId;

        Management::ResourceMonitor::ResourceMonitorServiceRegistration body(owner_.hostId_);

        MessageUPtr request = make_unique<Message>(body);

        request->Headers.Add(Transport::ActorHeader(Transport::Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Management::ResourceMonitor::ResourceMonitorServiceRegistration::ResourceMonitorServiceRegistrationAction));

        auto registrationOperation = owner_.ipcClient_.BeginRequest(
            move(request),
            TimeSpan::FromSeconds(5),
            [this](AsyncOperationSPtr const & operation) { OnRegistrationCompleted(operation, false); },
            thisSPtr
        );

        OnRegistrationCompleted(registrationOperation, true);
    }

    void OnRegistrationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        MessageUPtr reply;

        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.ipcClient_.EndRequest(operation, reply);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "ResourceMonitoringAgent::OpenAsyncOperation::OnRegistrationCompleted() EndRequest() failed with error {0}",
                error);
            auto thisSPtr = operation->Parent;
            TryComplete(thisSPtr, error);
            return;
        }

        // Get the address
        Management::ResourceMonitor::ResourceMonitorServiceRegistrationResponse response;
        auto success = reply->GetBody(response);

        if (!success)
        {
            WriteWarning(
                TraceComponent,
                "ResourceMonitoringAgent::OpenAsyncOperation::OnRegistrationCompleted() unable to read reponse body");
            auto thisSPtr = operation->Parent;
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            return;
        }

        auto ipcClient = make_unique<IpcClient>(owner_,
            Guid::NewGuid().ToString(),
            response.FabricHostAddress,
            false /* disallow use of unreliable transport */,
            L"ResourceMonitorService");

        SecuritySettings ipcClientSecuritySettings;
        error = SecuritySettings::CreateNegotiateClient(SecurityConfig::GetConfig().FabricHostSpn, ipcClientSecuritySettings);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "Failed to create security settings {0}", error);
            auto thisSPtr = operation->Parent;
            TryComplete(thisSPtr, error);
            return;
        }

        ipcClient->SecuritySettings = ipcClientSecuritySettings;
        owner_.fabricHostClient_ = move(ipcClient);

        error = owner_.fabricHostClient_->Open();

        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "Failed to open fabric host ipc client {0}", error);
            auto thisSPtr = operation->Parent;
            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(TraceComponent, "FabricHostAddress is {0}", response.FabricHostAddress);

        // Create timers and start them
        owner_.CreateTimers();

        owner_.monitorTimerSPtr_->Change(
            ResourceMonitorServiceConfig::GetConfig().ResourceMonitorInterval);
        owner_.sendingTimerSPtr_->Change(
            ResourceMonitorServiceConfig::GetConfig().ResourceSendingInterval);
        owner_.tracingTimerSPtr_->Change(
            ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval);

        TryComplete(operation->Parent, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    ResourceMonitoringAgent & owner_;
};

class ResourceMonitoringAgent::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in ResourceMonitoringAgent & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CloseAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.WriteInfo(TraceComponent, "ResourceMonitoringAgent CloseAsyncOperation Started");
        if (owner_.monitorTimerSPtr_)
        {
            owner_.monitorTimerSPtr_->Cancel();
        }
        if (owner_.sendingTimerSPtr_)
        {
            owner_.sendingTimerSPtr_->Cancel();
        }
        if (owner_.tracingTimerSPtr_)
        {
            owner_.tracingTimerSPtr_->Cancel();
        }
        if (owner_.fabricHostClient_)
        {
            owner_.fabricHostClient_->Close();
        }

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    TimeoutHelper timeoutHelper_;
    ResourceMonitoringAgent & owner_;
};

AsyncOperationSPtr ResourceMonitoringAgent::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(TraceComponent, "ResourceMonitoringAgent OnBeginOpen() called.");
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ResourceMonitoringAgent::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ResourceMonitoringAgent::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode ResourceMonitoringAgent::OnEndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void ResourceMonitoringAgent::OnAbort()
{
    WriteInfo(TraceComponent, "ResourceMonitoringAgent aborted");
    if (monitorTimerSPtr_)
    {
        monitorTimerSPtr_->Cancel();
    }
    if (sendingTimerSPtr_)
    {
        sendingTimerSPtr_->Cancel();
    }
    if (tracingTimerSPtr_)
    {
        tracingTimerSPtr_->Cancel();
    }
    if (fabricHostClient_)
    {
        fabricHostClient_->Close();
    }
}

