// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EventDispatcher : 
        private Common::RootedObject,
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(EventDispatcher)

    public:
        EventDispatcher(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~EventDispatcher();

    public:
        ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHandler const & handler);
        bool UnregisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHHandler const & hHandler);

        ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHandler const & handler);
        bool UnregisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHHandler const & hHandler);

        ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHandler const & handler);
        bool UnregisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHHandler const & hHandler);

        RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(RuntimeClosedEventHandler const & handler);
        bool UnregisterRuntimeClosedEventHandler(RuntimeClosedEventHHandler const & hHandler);

        ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHandler const & handler);
        bool UnregisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHHandler const & hHandler);

        AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHandler const & handler);
        bool UnregisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHHandler const & hHandler);

    public:
        void EnqueueServiceTypeEnabledEvent(ServiceTypeStatusEventArgs && eventArgs);
        void EnqueueServiceTypeDisabledEvent(ServiceTypeStatusEventArgs && eventArgs);
        void EnqueueServiceTypeRegisteredEvent(ServiceTypeRegistrationEventArgs && eventArgs);
        void EnqueueApplicationHostClosedEvent(ApplicationHostClosedEventArgs && eventArgs);
        void EnqueueRuntimeClosedEvent(RuntimeClosedEventArgs && eventArgs);
        void EnqueueAvaialableImagesEvent(SendAvailableContainerImagesEventArgs && eventArgs);

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();


    private:
        void CloseEvents();
        
    private:
        HostingSubsystem & hosting_;
        HostingJobQueueUPtr enableDisableQueue_;
        HostingJobQueueUPtr closedRegisteredQueue_;
        ServiceTypeRegisteredEvent serviceTypeRegisteredEvent_;
        ServiceTypeDisabledEvent serviceTypeDisabledEvent_;
        ServiceTypeEnabledEvent serviceTypeEnabledEvent_;
        RuntimeClosedEvent runtimeClosedEvent_;
        ApplicationHostClosedEvent applicationHostClosedEvent_;
        HostingJobQueueUPtr availableContainerImagesQueue_;
        SendAvailableContainerImagesEvent sendAvailableContainerImagesEvent_;
    };
}
