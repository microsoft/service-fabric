// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    #define FAS_COMFABRICCLIENT_RESOURCE( ResourceProperty, ResourceName ) \
        DEFINE_STRING_RESOURCE(ComFabricClientResource, ResourceProperty, FAS, ResourceName) \

    class ComFabricClientFASResource
    {
        DECLARE_SINGLETON_RESOURCE(ComFabricClientFASResource)

        FAS_COMFABRICCLIENT_RESOURCE(GetChaosReportArgumentInvalid, GetChaosReportArgumentInvalid)
        FAS_COMFABRICCLIENT_RESOURCE(GetChaosEventsArgumentInvalid, GetChaosEventsArgumentInvalid)
    };
}
