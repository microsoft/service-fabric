// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define EXE_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FabricServiceResource, ResourceProperty, FABRIC_SERVICE, ResourceName ) \

    class FabricServiceResource
    {
        DECLARE_SINGLETON_RESOURCE( FabricServiceResource )

        EXE_RESOURCE( FMReady, FMREADY )
        EXE_RESOURCE( Opening, OPENING )
        EXE_RESOURCE( RequestedVersion, REQ_VERSION )
        EXE_RESOURCE( ExecutableVersion, EXE_VERSION )
        EXE_RESOURCE( Opened, OPENED )
        EXE_RESOURCE( ExitPrompt, EXIT_PROMPT )
        EXE_RESOURCE( WarnShutdown, WARN_SHUTDOWN )
        EXE_RESOURCE( Closing, CLOSING )
        EXE_RESOURCE( Closed, CLOSED )
        EXE_RESOURCE( CtrlC, CTRL_C )
        EXE_RESOURCE( CtrlBreak, CTRL_BRK )
    };

