// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    INITIALIZE_SIZE_ESTIMATION(ServiceLocationNotificationRequestData)

    ServiceLocationNotificationRequestData::ServiceLocationNotificationRequestData()
        : name_()
        , previousResolves_()
        , previousError_(Common::ErrorCodeValue::Success)
        , previousErrorVersion_(ServiceModel::Constants::UninitializedVersion)
        , partitionKey_()
    {
    }

    ServiceLocationNotificationRequestData::ServiceLocationNotificationRequestData(
        NamingUri const & name,
        PartitionVersionMap const & previousResolves,
        AddressDetectionFailureSPtr const & previousError,
        PartitionKey const & partitionKey)
        : name_(name)
        , previousResolves_(previousResolves)
        , previousError_(previousError ? previousError->Error : Common::ErrorCodeValue::Success)
        , previousErrorVersion_(previousError ? previousError->StoreVersion : ServiceModel::Constants::UninitializedVersion)
        , partitionKey_(partitionKey)
    {
    }

    ServiceLocationNotificationRequestData::ServiceLocationNotificationRequestData(ServiceLocationNotificationRequestData const & other)
        : name_(other.name_)
        , previousResolves_(other.previousResolves_)
        , previousError_(other.previousError_)
        , previousErrorVersion_(other.previousErrorVersion_)
        , partitionKey_(other.partitionKey_)
    {
    }

    ServiceLocationNotificationRequestData & ServiceLocationNotificationRequestData::operator = (ServiceLocationNotificationRequestData const & other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            previousResolves_ = other.previousResolves_;
            partitionKey_ = other.partitionKey_;
            previousError_ = other.previousError_;
            previousErrorVersion_ = other.previousErrorVersion_;
        }

        return *this;
    }

    ServiceLocationNotificationRequestData::ServiceLocationNotificationRequestData(ServiceLocationNotificationRequestData && other)
        : name_(std::move(other.name_))
        , previousResolves_(std::move(other.previousResolves_))
        , previousError_(std::move(other.previousError_))
        , previousErrorVersion_(std::move(other.previousErrorVersion_))
        , partitionKey_(std::move(other.partitionKey_))
    {
    }

    ServiceLocationNotificationRequestData & ServiceLocationNotificationRequestData::operator = (ServiceLocationNotificationRequestData && other)
    {
        if (this != &other)
        {
            name_ = std::move(other.name_);
            previousResolves_ = std::move(other.previousResolves_);
            partitionKey_ = std::move(other.partitionKey_);
            previousError_ = std::move(other.previousError_);
            previousErrorVersion_ = std::move(other.previousErrorVersion_);
        }

        return *this;
    }

    NameRangeTupleUPtr ServiceLocationNotificationRequestData::GetNameRangeTuple() const 
    { 
        return make_unique<NameRangeTuple>(name_, partitionKey_); 
    }

    void ServiceLocationNotificationRequestData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "NotifRequest(" << name_ << ", pk=" << partitionKey_ << ", prev=(";
        for (auto iter = previousResolves_.begin(); iter != previousResolves_.end(); ++iter)
        {
            if (iter != previousResolves_.begin()) { w << ","; }
            w << *iter;
        }

        w << "/" << previousError_ << ";" << previousErrorVersion_;
        w << "))";
    }

    void ServiceLocationNotificationRequestData::WriteToEtw(uint16 contextSequenceId) const
    {
        ServiceModel::ServiceModelEventSource::Trace->NotifRequestDataTrace(
            contextSequenceId,
            name_.ToString(),
            partitionKey_.ToString(),
            static_cast<uint64>(previousResolves_.size()),
            Common::wformatString(previousError_),
            previousErrorVersion_);
    }
}
