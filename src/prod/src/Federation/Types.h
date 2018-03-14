// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header file contains all shared, unique, weak pointer types well-known in federation
// as well as forward declarations of corresponding types

namespace Federation
{
    class SiteNode;
    typedef std::shared_ptr<SiteNode> SiteNodeSPtr;
    
    class RoutingTable;

    struct IMultipleReplyContext;
    typedef std::shared_ptr<IMultipleReplyContext> IMultipleReplyContextSPtr;

    struct IRouter;
    typedef std::shared_ptr<IRouter> IRouterSPtr;

    class LeasePartner;
    typedef std::shared_ptr<LeasePartner> LeasePartnerSPtr;

    class PartnerNode;
    typedef std::shared_ptr<PartnerNode const> PartnerNodeSPtr;

    class FederationConfig;
    typedef std::unique_ptr<FederationConfig> FederationConfigPtr;
}
