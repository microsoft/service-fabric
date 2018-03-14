// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Reliability;
    using namespace std;

    GatewayProperties::GatewayProperties(
        Federation::NodeInstance const & nodeInstance,
        __in Federation::IRouter & router,
        __in Reliability::ServiceAdminClient & adminClient,
        __in Reliability::ServiceResolver & resolver,
        __in Query::QueryGateway & queryGateway,
        SystemServices::SystemServiceResolverSPtr const & systemServiceResolver,
        __in Naming::ServiceNotificationManager & serviceNotificationManager,
        __in IGateway & gateway,
        GatewayEventSource const & trace,
        Common::FabricNodeConfigSPtr const& nodeConfig,
        Transport::SecuritySettings const & zombieModeFabricClientSecuritySettings,
        Common::ComponentRoot const & root,
        bool isInZombieMode)
        : Common::RootedObject(root)
        , nodeInstance_(nodeInstance)
        , nodeInstanceString_(nodeInstance_.ToString())
        , router_(router)
        , adminClient_(adminClient)
        , resolver_(resolver)
        , queryGateway_(queryGateway)
        , systemServiceResolver_(systemServiceResolver)
        , serviceNotificationManager_(serviceNotificationManager)
        , gateway_(gateway)
        , namingServiceCuids_(NamingConfig::GetConfig().PartitionCount)
        , operationRetryInterval_(NamingConfig::GetConfig().OperationRetryInterval)
        , psdCache_(ServiceModel::ServiceModelConfig::GetConfig().GatewayServiceDescriptionCacheLimit)
        , prefixPsdCache_(psdCache_, [](NamingUri const & name) { return name.ToString(); })
        , trace_(trace)
        , transport_()
        , broadcastEventManager_(resolver, psdCache_, trace, nodeInstanceString_)
        , requestInstance_(nodeInstanceString_)
        , tvsCuid_(Guid::Empty())
        , tvsCuidLock_()
        , nodeConfig_(nodeConfig)
        , zombieModeFabricClientSecuritySettings_(zombieModeFabricClientSecuritySettings)
        , isInZombieMode_(isInZombieMode)
    {
        absolutePathToStartStopFile_ = Path::Combine(nodeConfig_->WorkingDir, nodeConfig_->StartStopFileName);    
    }

    bool GatewayProperties::TryGetTvsCuid(__out Common::Guid & cuid)
    {
        AcquireReadLock lock(tvsCuidLock_);

        if (tvsCuid_ != Guid::Empty())
        {
            cuid = tvsCuid_;
            return true;
        }

        return false;
    }

    void GatewayProperties::SetTvsCuid(Common::Guid const & cuid)
    {
        AcquireWriteLock lock(tvsCuidLock_);

        tvsCuid_ = cuid;
    }
}
