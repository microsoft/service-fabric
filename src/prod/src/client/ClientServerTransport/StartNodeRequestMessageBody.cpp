// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace ServiceModel;

namespace Naming
{
    StartNodeRequestMessageBody::StartNodeRequestMessageBody()
    : nodeName_(),
      instanceId_(),
      ipAddressOrFQDN_(),
      clusterConnectionPort_(0)
    {
    }

    StartNodeRequestMessageBody::StartNodeRequestMessageBody(
        wstring && nodeName,
        uint64 instanceId,
        wstring && ipAddressOrFQDN,
        ULONG clusterConnectionPort)
    : nodeName_(move(nodeName)),
      instanceId_(instanceId),
      ipAddressOrFQDN_(move(ipAddressOrFQDN)),
      clusterConnectionPort_(clusterConnectionPort)
    {
    }

    StartNodeRequestMessageBody::StartNodeRequestMessageBody(
        wstring const & nodeName,
        uint64 instanceId,
        wstring const & ipAddressOrFQDN,
        ULONG clusterConnectionPort)
    : nodeName_(nodeName),
      instanceId_(instanceId),
      ipAddressOrFQDN_(ipAddressOrFQDN),
      clusterConnectionPort_(clusterConnectionPort)
    {
    }

    StartNodeRequestMessageBody::StartNodeRequestMessageBody(StartNodeRequestMessageBody && other)
    : nodeName_(move(other.nodeName_)),
      instanceId_(instanceId_),
      ipAddressOrFQDN_(move(other.ipAddressOrFQDN_)),
      clusterConnectionPort_(other.clusterConnectionPort_)
    {
    }

    StartNodeRequestMessageBody & StartNodeRequestMessageBody::operator=(StartNodeRequestMessageBody && other)
    {
        if (this != &other)
        {
            nodeName_ = move(other.nodeName_);
            instanceId_ = other.instanceId_;
            ipAddressOrFQDN_ = move(other.ipAddressOrFQDN_);
            clusterConnectionPort_ = move(other.clusterConnectionPort_);
        }

        return *this;
    }
}

