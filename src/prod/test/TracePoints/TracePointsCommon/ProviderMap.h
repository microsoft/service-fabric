// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class ProviderMap;
    typedef std::unique_ptr<ProviderMap> ProviderMapUPtr;

    class ProviderMap : private DenyCopy
    {
    public:
        ProviderMap(ServerEventSource & eventSource);
        ~ProviderMap();

        ServerEventSource & get_EventSource() const
        {
            return eventSource_;
        }

        void ClearEventMap(ProviderId providerId);
        bool RemoveEventMap(ProviderId providerId);
        bool RemoveEventMapEntry(ProviderId providerId, EventId eventId);
        bool TryGetEventMapEntry(ProviderId providerId, EventId eventId, __out TracePointData & tracePointData) const;        
        void AddEventMapEntry(ProviderId providerId, EventId eventId, __in TracePointData const& tracePointData);

    private:
        class GuidHashCode
        {
        public:
            size_t operator()(ProviderId const providerId) const
            {
                DWORD a = providerId.Data1;
                USHORT b = providerId.Data2;
                USHORT c = providerId.Data3;
                BYTE f = providerId.Data4[2];
                BYTE k = providerId.Data4[7];

                return (a ^ ((b << 0x10) | c)) ^ ((f << 0x18) | k);
            }
        };

        class GuidEquals
        {
        public:
            bool operator()(ProviderId const a, ProviderId const b) const { return memcmp(&a, &b, sizeof(ProviderId)) == 0; }
        };

        typedef std::unordered_map<ProviderId, EventMapUPtr, GuidHashCode, GuidEquals> Map;

        Map providerEvents_;
        ReaderWriterLockSlim mutable rwLock_;
        ServerEventSource & eventSource_;
    };
}
