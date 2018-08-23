// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{

#define GET_COM_RC( ResourceName ) \
    Common::StringResource::Get( IDS_COMMON_##ResourceName )

#define GET_FM_RC( ResourceName ) \
    Common::StringResource::Get( IDS_FAILOVER_##ResourceName )

#define GET_NS_RC( ResourceName ) \
    Common::StringResource::Get( IDS_NAMING_##ResourceName )

#define GET_CM_RC( ResourceName ) \
    Common::StringResource::Get( IDS_CM_##ResourceName )

#define GET_HM_RC( ResourceName ) \
    Common::StringResource::Get( IDS_HM_##ResourceName )

#define GET_MODELV2_RC( ResourceName ) \
    Common::StringResource::Get( IDS_MODELV2_##ResourceName )

#define CHECK_EQUALS_IF_NON_NULL( field ) \
    if (field) \
    { \
        if (!other.field || *field != *other.field) { return false; } \
    } \
    else if (other.field) \
    { \
        return false; \
    }

}
