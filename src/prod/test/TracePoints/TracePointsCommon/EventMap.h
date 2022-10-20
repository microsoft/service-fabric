// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class EventMap;
    typedef std::unique_ptr<EventMap> EventMapUPtr;

    class EventMap : private DenyCopy
    {
    public:
        EventMap();
        ~EventMap();

        void Clear();
        void Add(EventId eventId, __in TracePointData const& tracePointData);
        bool Remove(EventId eventId);
        bool TryGet(EventId eventId, __out TracePointData & tracePointData) const;

    private:
        typedef std::map<EventId, TracePointData> Map;
        
        Map eventFunctions_;
    };
}
