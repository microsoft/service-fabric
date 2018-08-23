// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ResourceDescription)

ErrorCode ResourceDescription::TryValidate(wstring const & traceId) const
{
    if (Name.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    if (!invalidNameCharacters_.empty())
    {
        if (Name.find_first_of(invalidNameCharacters_) != wstring::npos)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(InvalidCharactersInName), traceId, invalidNameCharacters_, Name));
        }
    }

    return ErrorCodeValue::Success;
}
