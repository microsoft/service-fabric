//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;
using namespace std;

INITIALIZE_SIZE_ESTIMATION(SettingDescription);

bool SettingDescription::operator==(SettingDescription const & other) const
{
    if (this->Name == other.Name
        && this->Value == other.Value)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool SettingDescription::operator!=(SettingDescription const & other) const
{
    return !(*this == other);
}

ErrorCode SettingDescription::TryValidate(wstring const & traceId) const
{
    // TODO: Invalid path
    if (this->Name.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    if (this->Value.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ValueNotSpecified), traceId));
    }

    return ErrorCodeValue::Success;
}
