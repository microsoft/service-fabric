// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType("EventDispatcher");

// ********************************************************************************************************************
// EventDispatcher Implementation
//

EventDispatcher::EventDispatcher(
    Common::ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    hosting_(hosting),
    enableDisableQueue_(),
    closedRegisteredQueue_(),
    serviceTypeRegisteredEvent_(),
    serviceTypeDisabledEvent_(),
    serviceTypeEnabledEvent_(),
    runtimeClosedEvent_(),
    applicationHostClosedEvent_(),
    availableContainerImagesQueue_(),
    sendAvailableContainerImagesEvent_()
{
}

EventDispatcher::~EventDispatcher()
{
}

ErrorCode EventDispatcher::OnOpen()
{
    closedRegisteredQueue_ = make_unique<HostingJobQueue>(L"HostingClosedRebisteredEventQueue", hosting_, false, 1, nullptr, UINT64_MAX, DequePolicy::FifoLifo);
    enableDisableQueue_ = make_unique<HostingJobQueue>(L"HostingEnableDisableEventQueue", hosting_, false, 1, nullptr, UINT64_MAX, DequePolicy::FifoLifo);
    availableContainerImagesQueue_ = make_unique<HostingJobQueue>(L"AvailableContainerImagesEventQueue", hosting_, false, 1, nullptr, UINT64_MAX, DequePolicy::FifoLifo);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode EventDispatcher::OnClose()
{
    OnAbort();

    return ErrorCode(ErrorCodeValue::Success);
}

void EventDispatcher::OnAbort()
{
    if (closedRegisteredQueue_)
    {
        closedRegisteredQueue_->Close();
    }
    if (enableDisableQueue_)
    {
        enableDisableQueue_->Close();
    }
    if (availableContainerImagesQueue_)
    {
        availableContainerImagesQueue_->Close();
    }
    CloseEvents();
}

void EventDispatcher::EnqueueServiceTypeRegisteredEvent(ServiceTypeRegistrationEventArgs && eventArgs)
{
    ServiceTypeRegistrationEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing ServiceTypeRegisteredEvent: EventArgs={0}",
        args);

    this->closedRegisteredQueue_->Enqueue(
        make_unique<EventContainerT<ServiceTypeRegistrationEventArgs>>(
            this->serviceTypeRegisteredEvent_,
            args,
            [this,args]()
            {
                hostingTrace.ServiceTypeRegistered(
                Root.TraceId,
                args.SequenceNumber,
                args.Registration->VersionedServiceTypeId.ToString(),
                args.Registration->ActivationContext.ToString(),
                args.Registration->ServicePackagePublicActivationId,
                args.Registration->HostId,
                args.Registration->RuntimeId);
            }));
}

void EventDispatcher::EnqueueServiceTypeDisabledEvent(ServiceTypeStatusEventArgs && eventArgs)
{
    ServiceTypeStatusEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing ServiceTypeDisabledEvent: EventArgs={0}",
        args);

    this->enableDisableQueue_->Enqueue(
        make_unique<EventContainerT<ServiceTypeStatusEventArgs>>(
            this->serviceTypeDisabledEvent_,
            args,
            [this, args]()
            {
                hostingTrace.ServiceTypeDisabled(
                    Root.TraceId,
                    args.SequenceNumber,
                    args.ServiceTypeId.ToString(),
                    args.ActivationContext.ToString(),
                    args.ServicePackagePublicActivationId);
            }));
}

void EventDispatcher::EnqueueServiceTypeEnabledEvent(ServiceTypeStatusEventArgs && eventArgs)
{
    ServiceTypeStatusEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing ServiceTypeEnabledEvent: EventArgs={0}",
        args);

    this->enableDisableQueue_->Enqueue(
        make_unique<EventContainerT<ServiceTypeStatusEventArgs>>(
            this->serviceTypeEnabledEvent_,
            args,
            [this, args]()
            {
                hostingTrace.ServiceTypeEnabled(
                    Root.TraceId,
                    args.SequenceNumber,
                    args.ServiceTypeId.ToString(),
                    args.ActivationContext.ToString(),
                    args.ServicePackagePublicActivationId);
            }));
}

void EventDispatcher::EnqueueRuntimeClosedEvent(RuntimeClosedEventArgs && eventArgs)
{
    RuntimeClosedEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing RuntimeClosedEvent: EventArgs={0}",
        args);

    this->closedRegisteredQueue_->Enqueue(
        make_unique<EventContainerT<RuntimeClosedEventArgs>>(
        this->runtimeClosedEvent_,
        args,
        [this, args]()
        {
            hostingTrace.FabricRuntimeClosed(
                Root.TraceId,
                args.SequenceNumber,
                args.RuntimeId,
                args.ActivationContext.ToString(),
                args.ServicePackagePublicActivationId,
                args.HostId);
        }));
}

void EventDispatcher::EnqueueApplicationHostClosedEvent(ApplicationHostClosedEventArgs && eventArgs)
{
    ApplicationHostClosedEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing ApplicationHostClosedEvent: EventArgs={0}",
        args);

     this->closedRegisteredQueue_->Enqueue(
        make_unique<EventContainerT<ApplicationHostClosedEventArgs>>(
        this->applicationHostClosedEvent_,
        args,
        [this, args]()
        {
            hostingTrace.ApplicationHostClosed(
                Root.TraceId,
                args.SequenceNumber,
                args.HostId,
                args.ActivationContext.ToString(),
                args.ServicePackagePublicActivationId);
        }));
}

void EventDispatcher::EnqueueAvaialableImagesEvent(SendAvailableContainerImagesEventArgs && eventArgs)
{
    SendAvailableContainerImagesEventArgs args(move(eventArgs));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Enqueuing SendAvailableContainerImagesEventArgs: EventArgs={0}",
        args);

    this->availableContainerImagesQueue_->Enqueue(
        make_unique<EventContainerT<SendAvailableContainerImagesEventArgs>>(
        this->sendAvailableContainerImagesEvent_,
        args,
        [this, args]()
    {
        hostingTrace.AvailableContainerImages(
            Root.TraceId,
            args.NodeId,
            args.AvailableImages.size());
    }));
}

void EventDispatcher::CloseEvents()
{
    this->serviceTypeRegisteredEvent_.Close();
    this->serviceTypeDisabledEvent_.Close();
    this->serviceTypeEnabledEvent_.Close();
    this->runtimeClosedEvent_.Close();
    this->applicationHostClosedEvent_.Close();
    this->sendAvailableContainerImagesEvent_.Close();
}

ServiceTypeRegisteredEventHHandler EventDispatcher::RegisterServiceTypeRegisteredEventHandler(
    ServiceTypeRegisteredEventHandler const & handler)
{
    return this->serviceTypeRegisteredEvent_.Add(handler);
}

bool EventDispatcher::UnregisterServiceTypeRegisteredEventHandler(
    ServiceTypeDisabledEventHHandler const & hHandler)
{
    return this->serviceTypeRegisteredEvent_.Remove(hHandler);
}

ServiceTypeDisabledEventHHandler EventDispatcher::RegisterServiceTypeDisabledEventHandler(
    ServiceTypeDisabledEventHandler const & handler)
{
    return this->serviceTypeDisabledEvent_.Add(handler);
}

bool EventDispatcher::UnregisterServiceTypeDisabledEventHandler(
    ServiceTypeDisabledEventHHandler const & hHandler)
{
    return this->serviceTypeDisabledEvent_.Remove(hHandler);
}

ServiceTypeEnabledEventHHandler EventDispatcher::RegisterServiceTypeEnabledEventHandler(
    ServiceTypeEnabledEventHandler const & handler)
{
    return this->serviceTypeEnabledEvent_.Add(handler);
}

bool EventDispatcher::UnregisterServiceTypeEnabledEventHandler(
    ServiceTypeEnabledEventHHandler const & hHandler)
{
    return this->serviceTypeEnabledEvent_.Remove(hHandler);
}

RuntimeClosedEventHHandler EventDispatcher::RegisterRuntimeClosedEventHandler(
    RuntimeClosedEventHandler const & handler)
{
    return this->runtimeClosedEvent_.Add(handler);
}

bool EventDispatcher::UnregisterRuntimeClosedEventHandler(
    RuntimeClosedEventHHandler const & hHandler)
{
    return this->runtimeClosedEvent_.Remove(hHandler);
}

ApplicationHostClosedEventHHandler EventDispatcher::RegisterApplicationHostClosedEventHandler(
    ApplicationHostClosedEventHandler const & handler)
{
    return this->applicationHostClosedEvent_.Add(handler);
}

bool EventDispatcher::UnregisterApplicationHostClosedEventHandler(
    ApplicationHostClosedEventHHandler const & hHandler)
{
    return this->applicationHostClosedEvent_.Remove(hHandler);
}

AvailableContainerImagesEventHHandler EventDispatcher::RegisterSendAvailableContainerImagesEventHandler(
    AvailableContainerImagesEventHandler const & handler)
{
    return this->sendAvailableContainerImagesEvent_.Add(handler);
}

bool EventDispatcher::UnregisterSendAvailableContainerImagesEventHandler(
    AvailableContainerImagesEventHHandler const & hHandler)
{
    return this->sendAvailableContainerImagesEvent_.Remove(hHandler);
}
