// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;
    using namespace ServiceModel;

    INITIALIZE_SIZE_ESTIMATION( AddressDetectionFailure )

    AddressDetectionFailure::AddressDetectionFailure()
      : name_()
      , partitionData_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON)
      , error_(ErrorCodeValue::Success)
      , storeVersion_(ServiceModel::Constants::UninitializedVersion)
    {
    }
    
    AddressDetectionFailure::AddressDetectionFailure(
        NamingUri const & name,
        PartitionKey const & partitionData,
        ErrorCodeValue::Enum const error,
        __int64 storeVersion)
      : name_(name)
      , partitionData_(partitionData)
      , error_(error)
      , storeVersion_(storeVersion)
    {
    }

    AddressDetectionFailure::AddressDetectionFailure(AddressDetectionFailure && other)
      : name_(move(other.name_))
      , partitionData_(move(other.partitionData_))
      , error_(move(other.error_))
      , storeVersion_(move(other.storeVersion_))
    {
    }
        
    AddressDetectionFailure & AddressDetectionFailure::operator = (AddressDetectionFailure && other)
    {
        if (this != &other)
        {
            name_ = move(other.name_);
            partitionData_ = move(other.partitionData_);
            error_ = move(other.error_);
            storeVersion_ = move(other.storeVersion_);
        }

        return *this;
    }
         
    AddressDetectionFailure::AddressDetectionFailure(AddressDetectionFailure const & other)
      : name_(other.name_)
      , partitionData_(other.partitionData_)
      , error_(other.error_)
      , storeVersion_(other.storeVersion_)
    {
    }

    AddressDetectionFailure & AddressDetectionFailure::operator = (AddressDetectionFailure const & other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            partitionData_ = other.partitionData_;
            error_ = other.error_;
            storeVersion_ = other.storeVersion_;
        }

        return *this;
    }

    bool AddressDetectionFailure::IsObsolete(AddressDetectionFailure const & update) const
    {
        return IsObsolete(error_, storeVersion_, update.Error, update.StoreVersion);
    }

    bool AddressDetectionFailure::IsObsolete(
        Common::ErrorCodeValue::Enum const oldError, 
        __int64 oldStoreVersion,
        Common::ErrorCodeValue::Enum const newError, 
        __int64 newStoreVersion) 
    {
        if (oldError == newError)
        {
            return false;
        }

        if (newStoreVersion == ServiceModel::Constants::UninitializedVersion)
        {
            // Special case: uninitialized is used for some general errors.
            // These errors can only be equal (if same version) or newer
            return oldStoreVersion == ServiceModel::Constants::UninitializedVersion;
        }

        if (oldStoreVersion != ServiceModel::Constants::UninitializedVersion)
        {
            return (oldStoreVersion < newStoreVersion);
        }
        else
        {
            return true;
        }
    }

    void AddressDetectionFailure::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("ADF(serviceName='{0}',error={1},version={2})", name_.Name, error_, storeVersion_);
    }

    void AddressDetectionFailure::WriteToEtw(uint16 contextSequenceId) const
    {
        ServiceModelEventSource::Trace->ADFTrace(
            contextSequenceId,
            name_.Name,
            ErrorCode(error_),
            storeVersion_);
    }

    wstring AddressDetectionFailure::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
