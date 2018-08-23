//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ImageRegistryCredential)

bool ImageRegistryCredential::operator==(ImageRegistryCredential const & other) const
{
    if (containerRegistryServer_ == other.containerRegistryServer_
        && containerRegistryUserName_ == other.containerRegistryUserName_
        && containerRegistryPassword_ == other.containerRegistryPassword_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode ImageRegistryCredential::TryValidate(wstring const &traceId) const
{
    if (!containerRegistryUserName_.empty() && containerRegistryPassword_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerRegistryPasswordNotSpecified), traceId));
    }

    return ErrorCodeValue::Success;
}
