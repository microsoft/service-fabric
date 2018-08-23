// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // definitions of public events exposed by hosting components
    //

    // ServiceTypeRegistered
    typedef Common::EventT<ServiceTypeRegistrationEventArgs> ServiceTypeRegisteredEvent;
    typedef ServiceTypeRegisteredEvent::EventHandler ServiceTypeRegisteredEventHandler;
    typedef ServiceTypeRegisteredEvent::HHandler ServiceTypeRegisteredEventHHandler;

    // Runtime Closed
    typedef Common::EventT<RuntimeClosedEventArgs> RuntimeClosedEvent;
    typedef RuntimeClosedEvent::EventHandler RuntimeClosedEventHandler;
    typedef RuntimeClosedEvent::HHandler RuntimeClosedEventHHandler;

    // Application Host Closed
    typedef Common::EventT<ApplicationHostClosedEventArgs> ApplicationHostClosedEvent;
    typedef ApplicationHostClosedEvent::EventHandler ApplicationHostClosedEventHandler;
    typedef ApplicationHostClosedEvent::HHandler ApplicationHostClosedEventHHandler;

    // ServiceTypeDisabled
    typedef Common::EventT<ServiceTypeStatusEventArgs> ServiceTypeDisabledEvent;
    typedef ServiceTypeDisabledEvent::EventHandler ServiceTypeDisabledEventHandler;
    typedef ServiceTypeDisabledEvent::HHandler ServiceTypeDisabledEventHHandler;

    // ServiceTypeEnabled
    typedef Common::EventT<ServiceTypeStatusEventArgs> ServiceTypeEnabledEvent;
    typedef ServiceTypeEnabledEvent::EventHandler ServiceTypeEnabledEventHandler;
    typedef ServiceTypeEnabledEvent::HHandler ServiceTypeEnabledEventHHandler;

    // SendAvailableContainerImages
    typedef Common::EventT<SendAvailableContainerImagesEventArgs> SendAvailableContainerImagesEvent;
    typedef SendAvailableContainerImagesEvent::EventHandler AvailableContainerImagesEventHandler;
    typedef SendAvailableContainerImagesEvent::HHandler AvailableContainerImagesEventHHandler;
}
