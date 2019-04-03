// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class RegHandleMap;
    typedef std::unique_ptr<RegHandleMap> RegHandleMapUPtr;
    
    class RegHandleMap : private DenyCopy
    {
    public:
        RegHandleMap();
        ~RegHandleMap();

        void Add(REGHANDLE regHandle, ProviderId providerId);
        bool Remove(REGHANDLE regHandle);
        ProviderId Get(REGHANDLE regHandle) const;

    private:
        typedef std::unordered_map<REGHANDLE, ProviderId> Map;

        Map regHandles_;
        ReaderWriterLockSlim mutable rwLock_;
    };
}
