// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define EXE_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FabricApplicationGatewayResource, ResourceProperty, FABRICAPPLICATIONGATEWAY, ResourceName ) \

    class FabricApplicationGatewayResource
    {
        DECLARE_SINGLETON_RESOURCE( FabricApplicationGatewayResource )

        EXE_RESOURCE( Starting, STARTING )
        EXE_RESOURCE( Started, STARTED )
        EXE_RESOURCE( ExitPrompt, EXIT_PROMPT )
        EXE_RESOURCE( WarnShutdown, WARN_SHUTDOWN )
        EXE_RESOURCE( Closing, CLOSING )
        EXE_RESOURCE( Closed, CLOSED )
        EXE_RESOURCE( CtrlC, CTRL_C )
        EXE_RESOURCE( CtrlBreak, CTRL_BRK )
    };

