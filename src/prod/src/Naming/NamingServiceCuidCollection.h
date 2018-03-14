// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingServiceCuidCollection
    {
    public:
        NamingServiceCuidCollection(size_t partitionCount);

        __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
        std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const { return namingServiceCuids_; }

        Reliability::ConsistencyUnitId HashToNameOwner(Common::NamingUri const &) const;

    private:
        std::vector<Reliability::ConsistencyUnitId> namingServiceCuids_;
    };
}
