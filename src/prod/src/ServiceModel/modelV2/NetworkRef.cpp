//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(NetworkRef)

bool NetworkRef::operator==(NetworkRef const & other) const
{
    if (this->Name == other.Name)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode NetworkRef::TryValidate(wstring const & traceId) const
{
    if (Name.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }
                
    return ErrorCodeValue::Success;
}
