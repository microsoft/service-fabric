
//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetChaosEventsMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            GetChaosEventsMessageBody() : getChaosEventsDescription_() { }

            explicit GetChaosEventsMessageBody(GetChaosEventsDescription const & getChaosEventsDescription) : getChaosEventsDescription_(getChaosEventsDescription) { }

            __declspec(property(get = get_EventsDescription)) GetChaosEventsDescription const & EventsDescription;
            GetChaosEventsDescription const & get_EventsDescription() const { return getChaosEventsDescription_; }

            FABRIC_FIELDS_01(getChaosEventsDescription_);

        private:
            GetChaosEventsDescription getChaosEventsDescription_;
        };
    }
}
