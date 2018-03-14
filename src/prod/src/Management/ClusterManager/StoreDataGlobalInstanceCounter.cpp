// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Store;

using namespace std;

using namespace Management::ClusterManager;


StoreDataGlobalInstanceCounter::StoreDataGlobalInstanceCounter()
    : StoreData()
    , value_(0)
{
}

wstring const & StoreDataGlobalInstanceCounter::get_Type() const
{
    return Constants::StoreType_GlobalApplicationInstanceCounter;
}

wstring StoreDataGlobalInstanceCounter::ConstructKey() const
{
    return Constants::StoreKey_GlobalApplicationInstanceCounter;
}

void StoreDataGlobalInstanceCounter::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("GlobalApplicationInstanceCounter[{0}]", value_);
}

ErrorCode StoreDataGlobalInstanceCounter::GetNextApplicationId(
    StoreTransaction const & storeTx, 
    ServiceModelTypeName const & typeName, 
    ServiceModelVersion const & typeVersion, 
    __out ServiceModelApplicationId & result)
{
    UNREFERENCED_PARAMETER(typeVersion);

    ErrorCode error = storeTx.ReadExact(*this);

    if (error.IsSuccess())
    {
        ++value_;
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        value_ = 0;
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        wstring temp;
        StringWriter writer(temp);
        writer.Write("{0}_App{1}",
            typeName,
            this->Value);

        result = ServiceModelApplicationId(temp);
    }

    return error;
}

ErrorCode StoreDataGlobalInstanceCounter::UpdateNextApplicationId(
    StoreTransaction const & storeTx)
{
    if (value_ == 0)
    {
        return storeTx.Insert(*this);
    }
    else
    {
        return storeTx.Update(*this);
    }
}
