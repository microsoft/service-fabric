// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Store;
using namespace std;
using namespace Management::ClusterManager;

StoreDataSingleInstanceDeploymentCounter::StoreDataSingleInstanceDeploymentCounter()
    : StoreData()
    , value_(0)
{
}

wstring const & StoreDataSingleInstanceDeploymentCounter::get_Type() const
{
    return Constants::StoreType_ComposeDeploymentInstanceCounter;
}

wstring StoreDataSingleInstanceDeploymentCounter::ConstructKey() const
{
    return Constants::StoreKey_ComposeDeploymentInstanceCounter;
}

void StoreDataSingleInstanceDeploymentCounter::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("SingleInstanceDeploymentCounter[{0}]", value_);
}

ErrorCode StoreDataSingleInstanceDeploymentCounter::RefreshAndIncreaseCounter(
    StoreTransaction const &storeTx)
{
    // refresh the latest version from store.
    ErrorCode error = storeTx.ReadExact(*this);

    if (error.IsSuccess())
    {
        ++value_;
        error = storeTx.Update(*this);
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        value_ = 0;
        error = storeTx.Insert(*this);
    }

    return error;
}

ErrorCode StoreDataSingleInstanceDeploymentCounter::GetComposeDeploymentTypeNameAndVersion(
    StoreTransaction const &storeTx,
    __out ServiceModelTypeName &typeName,
    __out ServiceModelVersion &version)
{
    ErrorCode error = this->RefreshAndIncreaseCounter(storeTx);
    if (error.IsSuccess())
    {
        //
        // Application type name is used as a part of the directory name for the package, so using a sequence number to limit the length of
        // name.
        //
        typeName = ServiceModelTypeName(wformatString("{0}{1}", *Constants::ComposeDeploymentTypePrefix, this->Value));
        version = StoreDataSingleInstanceDeploymentCounter::GenerateServiceModelVersion(this->Value);
    }

    return error;
}

ErrorCode StoreDataSingleInstanceDeploymentCounter::GetSingleInstanceDeploymentTypeNameAndVersion(
    StoreTransaction const & storeTx,
    __out ServiceModelTypeName & typeName,
    __out ServiceModelVersion & version)
{
    ErrorCode error = this->RefreshAndIncreaseCounter(storeTx);
    if (error.IsSuccess())
    {
        typeName = ServiceModelTypeName(wformatString("{0}{1}", *Constants::SingleInstanceDeploymentTypePrefix, this->Value));
        version = StoreDataSingleInstanceDeploymentCounter::GenerateServiceModelVersion(this->Value);
    }
    return error;
}

ErrorCode StoreDataSingleInstanceDeploymentCounter::GetVersionNumber(ServiceModelVersion const &version, uint64 *versionNumber)
{
    auto versionString = version.Value;
    StringUtility::TrimLeading<wstring>(versionString, L"v");

    if (!StringUtility::TryFromWString(versionString, *versionNumber))
    {
        return ErrorCodeValue::NotFound;
    }

    return ErrorCodeValue::Success;
}

ServiceModelVersion StoreDataSingleInstanceDeploymentCounter::GenerateServiceModelVersion(uint64 number)
{
    return ServiceModelVersion(wformatString("v{0}", number));
}
