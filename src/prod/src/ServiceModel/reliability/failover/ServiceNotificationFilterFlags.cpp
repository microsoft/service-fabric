// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Reliability
{
    BEGIN_DEFINE_FLAG_CLASS( ServiceNotificationFilterFlags, NamePrefix, PrimaryOnly )

    DEFINE_FLAG_VALUE( NamePrefix, P )
    DEFINE_FLAG_VALUE( PrimaryOnly, R )

    END_DEFINE_FLAG_CLASS( )
};

