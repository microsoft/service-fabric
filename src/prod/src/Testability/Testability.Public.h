// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// External Header Files required by public Testability header files
//

#include "Federation/Federation.h"
#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Transport/Transport.h"
#include "query/Query.h"
#include "Hosting2/Hosting.Public.h"
#include "client/client.h"
#include "client/ClientPointers.h"
//
// Testability Public Header Files
//
#include "Testability/ITestabilitySubsystem.h"
#include "Testability/TestabilityPointers.h"

namespace Testability
{
    struct TestabilitySubsystemConstructorParameters
    {
        Federation::FederationSubsystemSPtr Federation;
        Transport::IpcServer * IpcServer;
        std::wstring NodeWorkingDirectory;
        Common::FabricNodeConfigSPtr NodeConfig;
        Common::ComponentRoot const * Root;
        std::wstring DeploymentFolder;
        Hosting2::IHostingSubsystemWPtr HostingSubSytem;
    };

    typedef ITestabilitySubsystemSPtr TestabilitySubsystemFactoryFunctionPtr(TestabilitySubsystemConstructorParameters & parameters);

    TestabilitySubsystemFactoryFunctionPtr TestabilitySubsystemFactory;
}
