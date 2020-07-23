// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class TracePointsState : private DenyCopy
    {
    public:
        TracePointsState();
        ~TracePointsState();

        void Setup(EventWriteFunc eventWrite);
        void Cleanup();

        void OnEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData);
        void OnEventRegister(LPCGUID ProviderId, PREGHANDLE RegHandle);
        void OnEventUnregister(REGHANDLE RegHandle);

    private:
        static GUID const TracePointsProviderId;
        REGHANDLE regHandle_;
        std::unique_ptr<ServerEventSource> eventSource_;
        ProviderMapUPtr tracePointMap_;
        RegHandleMapUPtr regHandleMap_;
        TracePointServerUPtr tracePointServer_;

        static REGHANDLE RegisterProvider();
    };
}
