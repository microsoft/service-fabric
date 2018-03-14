// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    BEGIN_DECLARE_FLAGS_CLASS( ServiceNotificationFilterFlags )

    DECLARE_FLAG_BIT( NamePrefix, 0)
    DECLARE_FLAG_BIT( PrimaryOnly, 1)

    IDL_FLAG_TYPE_CONVERSIONS( FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS )
    
    END_DECLARE_FLAGS_CLASS( )
};

