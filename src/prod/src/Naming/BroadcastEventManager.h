// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class BroadcastEventManager : public Common::FabricComponent
    {
        DENY_COPY(BroadcastEventManager)
    public:
        
        __declspec(property(get=get_NotificationRequests)) BroadcastWaiters const & NotificationRequests;
        BroadcastWaiters const & get_NotificationRequests() { return pendingLocationNotificationRequests_; }
                
        BroadcastEventManager(
            __in Reliability::ServiceResolver & resolver, 
            __in GatewayPsdCache & psdCache,
            GatewayEventSource const & trace,
            std::wstring const & gatewayTraceId);
        
        Common::ErrorCode AddRequests(
            Common::AsyncOperationSPtr const & owner,
            OnBroadcastReceivedCallback const & callback,
            BroadcastRequestVector && requests);

        void RemoveRequests(
            Common::AsyncOperationSPtr const & owner,
            std::set<Common::NamingUri> && names);
        
    protected:
        Common::ErrorCode OnOpen();

        Common::ErrorCode OnClose();

        void OnAbort();

    private:
        void OnBroadcastProcessed(Common::EventArgs const & args);
        void Cleanup();
        
        BroadcastWaiters pendingLocationNotificationRequests_;
        Reliability::ServiceResolver & resolver_;
        GatewayPsdCache & psdCache_;
        GatewayEventSource const & trace_;
        std::wstring gatewayTraceId_;
        
        Common::HHandler broadcastListenerHandle_;
    };
}
