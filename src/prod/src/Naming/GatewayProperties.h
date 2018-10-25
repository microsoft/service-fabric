// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GatewayProperties : 
        public Common::RootedObject
    {
        DENY_COPY (GatewayProperties)

    public:
        GatewayProperties(
            Federation::NodeInstance const &,
            __in Federation::IRouter &,
            __in Reliability::ServiceAdminClient &,
            __in Reliability::ServiceResolver &,
            __in Query::QueryGateway &,
            SystemServices::SystemServiceResolverSPtr const &,
            __in Naming::ServiceNotificationManager &,
            __in IGateway &,
            GatewayEventSource const &,
            Common::FabricNodeConfigSPtr const&,
            Transport::SecuritySettings const &,
            Common::ComponentRoot const &,
            bool isInZombieMode);

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & Instance;
        Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

        __declspec(property(get=get_NodeInstanceString)) std::wstring const & InstanceString;
        std::wstring const & get_NodeInstanceString() const { return nodeInstanceString_; }

        __declspec(property(get=get_InnerRingCommunication)) Federation::IRouter & InnerRingCommunication;
        Federation::IRouter & get_InnerRingCommunication() const { return router_; }

        __declspec(property(get=get_AdminClient)) Reliability::ServiceAdminClient & AdminClient;
        Reliability::ServiceAdminClient & get_AdminClient() const { return adminClient_; }

        __declspec(property(get=get_Resolver)) Reliability::ServiceResolver & Resolver;
        Reliability::ServiceResolver & get_Resolver() const { return resolver_; }

        __declspec(property(get=get_QueryGateway)) Query::QueryGateway & QueryGateway;
        Query::QueryGateway & get_QueryGateway() const { return queryGateway_; }

        __declspec(property(get=get_SystemServiceResolver)) SystemServices::SystemServiceResolverSPtr const & SystemServiceResolver;
        SystemServices::SystemServiceResolverSPtr const & get_SystemServiceResolver() const { return systemServiceResolver_; }

        __declspec(property(get=get_SystemServiceClient,put=put_SystemServiceClient)) SystemServices::ServiceDirectMessagingClientSPtr const & SystemServiceClient;
        SystemServices::ServiceDirectMessagingClientSPtr const & get_SystemServiceClient() const { return systemServiceClient_; }
        void put_SystemServiceClient(SystemServices::ServiceDirectMessagingClientSPtr const & value) { systemServiceClient_ = value; }

        __declspec(property(get=get_ServiceNotificationManager)) Naming::ServiceNotificationManager & ServiceNotificationManager;
        Naming::ServiceNotificationManager & get_ServiceNotificationManager() const { return serviceNotificationManager_; }

        __declspec(property(get=get_Gateway)) IGateway & Gateway;
        IGateway & get_Gateway() const { return gateway_; }

        __declspec(property(get=get_NamingCuidCollection)) NamingServiceCuidCollection const & NamingCuidCollection;
        NamingServiceCuidCollection const & get_NamingCuidCollection() const { return namingServiceCuids_; }

        __declspec(property(get=get_OperationRetryInterval)) Common::TimeSpan OperationRetryInterval;
        Common::TimeSpan get_OperationRetryInterval() const { return operationRetryInterval_; }
        
        __declspec(property(get=get_PsdCache)) GatewayPsdCache & PsdCache;
        GatewayPsdCache & get_PsdCache() { return psdCache_; }
        
        __declspec(property(get=get_PrefixPsdCache)) Common::LruPrefixCache<std::wstring, GatewayPsdCacheEntry> & PrefixPsdCache;
        Common::LruPrefixCache<std::wstring, GatewayPsdCacheEntry> & get_PrefixPsdCache() { return prefixPsdCache_; }
        
        __declspec(property(get=get_Trace)) Naming::GatewayEventSource const & Trace;
        Naming::GatewayEventSource const & get_Trace() const { return trace_; }

        __declspec(property(get=get_ReceivingChannel, put=put_ReceivingChannel)) EntreeServiceTransportSPtr const & ReceivingChannel;
        EntreeServiceTransportSPtr const & get_ReceivingChannel() const { return transport_; }
        void put_ReceivingChannel(EntreeServiceTransportSPtr const & value) { transport_ = value; }

        __declspec(property(get=get_BroadcastEventManager)) Naming::BroadcastEventManager & BroadcastEventManager;
        Naming::BroadcastEventManager & get_BroadcastEventManager() { return broadcastEventManager_; }

        __declspec(property(get=get_RequestInstance)) Naming::RequestInstance & RequestInstance;
        Naming::RequestInstance & get_RequestInstance() { return requestInstance_; }

         __declspec(property(get=get_NodeName)) std::wstring const NodeName;
        std::wstring const get_NodeName() { return nodeConfig_->InstanceName; }

         __declspec(property(get=get_AbsolutePathToStartStopFile)) std::wstring const & AbsolutePathToStartStopFile;
        std::wstring const & get_AbsolutePathToStartStopFile() { return absolutePathToStartStopFile_; }

         __declspec(property(get=get_ZombieModeFabricClientSecuritySettings)) Transport::SecuritySettings const & ZombieModeFabricClientSecuritySettings;
        Transport::SecuritySettings const & get_ZombieModeFabricClientSecuritySettings() { return zombieModeFabricClientSecuritySettings_; }

        __declspec(property(get=get_IsInZombieMode)) bool IsInZombieMode;
        bool get_IsInZombieMode() { return isInZombieMode_; }

        __declspec(property(get=get_NodeConfig)) Common::FabricNodeConfigSPtr const &NodeConfigSPtr;
        Common::FabricNodeConfigSPtr const & get_NodeConfig() const { return nodeConfig_; }

        bool TryGetTvsCuid(__out Common::Guid &);
        void SetTvsCuid(Common::Guid const &);

    private:
        friend class StoreService;

        Federation::NodeInstance nodeInstance_;
        std::wstring nodeInstanceString_;
        Federation::IRouter & router_;
        Reliability::ServiceAdminClient & adminClient_;
        Reliability::ServiceResolver & resolver_;
        Query::QueryGateway & queryGateway_;
        SystemServices::SystemServiceResolverSPtr systemServiceResolver_;
        SystemServices::ServiceDirectMessagingClientSPtr systemServiceClient_;
        Naming::ServiceNotificationManager & serviceNotificationManager_;
        IGateway & gateway_;        
        NamingServiceCuidCollection namingServiceCuids_; 
        Common::TimeSpan operationRetryInterval_;
        GatewayPsdCache psdCache_;        
        Common::LruPrefixCache<std::wstring, GatewayPsdCacheEntry> prefixPsdCache_;
        GatewayEventSource const & trace_;
        EntreeServiceTransportSPtr transport_;
        Naming::BroadcastEventManager broadcastEventManager_;
        Naming::RequestInstance requestInstance_;

        Common::Guid tvsCuid_;
        Common::RwLock tvsCuidLock_;

        Common::FabricNodeConfigSPtr const& nodeConfig_;

        std::wstring absolutePathToStartStopFile_;
        Transport::SecuritySettings zombieModeFabricClientSecuritySettings_;

        // This is used to indicate if the node is in a "stopped" state, controlled by StopNode() and StartNode().  When the node is in stopped state
        // the process is running, but it is fully functional, and not part of the ring.
        bool isInZombieMode_;
    };

    typedef std::unique_ptr<GatewayProperties> GatewayPropertiesUPtr;
    typedef std::shared_ptr<GatewayProperties> GatewayPropertiesSPtr;
}
