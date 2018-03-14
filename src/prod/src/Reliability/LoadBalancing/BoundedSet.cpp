// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementAndLoadBalancing.h"
#include "PlacementReplica.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

template<typename T>
void BoundedSet<T>::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    writer.Write("[");
    for (auto it = elements_.begin(); it != elements_.end(); ++it)
    {
        writer.Write("{0} ", Dereference(*it));
    }
    writer.Write("]");
}

template<typename T>
void BoundedSet<T>::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        template class BoundedSet<PlacementReplica const*>;
        template class BoundedSet<Common::Guid>;
    }
}
