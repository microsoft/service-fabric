// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    ServiceResolutionRequestData::ServiceResolutionRequestData()
        : key_()
        , version_()
        , cuid_(Guid::Empty())
        , includePsd_(false)
    {
    }

    ServiceResolutionRequestData::ServiceResolutionRequestData(PartitionKey const & key)
        : key_(key)
        , version_()
        , cuid_(Guid::Empty())
        , includePsd_(false)
    {
    }

    ServiceResolutionRequestData::ServiceResolutionRequestData(PartitionInfo const & existingData)
        : key_(existingData.LowKey)
        , version_()
        , cuid_(Guid::Empty())
        , includePsd_(false)
    {
    }

    ServiceResolutionRequestData::ServiceResolutionRequestData(
        PartitionKey const & key, 
        ServiceLocationVersion const & version)
        : key_(key)
        , version_(version)
        , cuid_(Guid::Empty())
        , includePsd_(false)
    {
    }

    ServiceResolutionRequestData::ServiceResolutionRequestData(
        PartitionKey const & key, 
        ServiceLocationVersion const & version,
        Reliability::ConsistencyUnitId const & cuid)
        : key_(key)
        , version_(version)
        , cuid_(cuid)
        , includePsd_(false)
    {
    }

    ServiceResolutionRequestData::ServiceResolutionRequestData(ServiceResolutionRequestData const & other)
        : key_(other.key_)
        , version_(other.version_)
        , cuid_(other.cuid_)
        , includePsd_(other.includePsd_)
    {
    }
    
    ServiceResolutionRequestData & ServiceResolutionRequestData::operator = (ServiceResolutionRequestData const & other)
    {
        if (this != &other)
        {
            key_ = other.key_;
            version_ = other.version_;
            cuid_ = other.cuid_;
            includePsd_ = other.includePsd_;
        }

        return *this;
    }

    void ServiceResolutionRequestData::SetIncludePsd(bool value)
    {
        includePsd_ = value;
    }

    void ServiceResolutionRequestData::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const &) const
    {
        w.Write("[{0}, {1}, {2}, psd={3}]", key_, version_, cuid_, includePsd_);
    }

    std::wstring ServiceResolutionRequestData::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
