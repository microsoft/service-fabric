//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {  
        class CentralSecretServiceReplica;
        typedef std::shared_ptr<CentralSecretServiceReplica> CentralSecretServiceReplicaSPtr;

        class CentralSecretServiceFactory;
    }
}