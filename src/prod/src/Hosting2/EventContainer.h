// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    typedef std::function<void(void)> EventCompletionCallback;		

    template <class TEventArgs>
    class EventContainerT : public Common::JobItem<HostingSubsystem>
    {
        DENY_COPY(EventContainerT);

    public:
        EventContainerT(
            Common::EventT<TEventArgs> & hostingEvent, 
            TEventArgs const & eventArgs, 
            Hosting2::EventCompletionCallback const & onEventCompletion)
            : JobItem(),
            hostingEvent_(hostingEvent), 
            eventArgs_(eventArgs),
            onEventCompletion_(onEventCompletion)
        {
        }

        virtual void Process(HostingSubsystem & hosting)
        {
            UNREFERENCED_PARAMETER(hosting);
            hostingEvent_.Fire(eventArgs_, false);
            onEventCompletion_();		
        }

    private:
        Common::EventT<TEventArgs> & hostingEvent_;
        TEventArgs const eventArgs_;
        Hosting2::EventCompletionCallback const onEventCompletion_;
    };
}
