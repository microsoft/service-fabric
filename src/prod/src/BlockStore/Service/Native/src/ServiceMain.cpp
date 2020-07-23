// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// Service Fabric BlockStore Service entrypoint.

// TODO: Review this.
// Silence warnings for stdenv
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "../inc/common.h"
#include "../inc/CNetworkStream.h"
#include "../inc/BlockStorePayload.h"
#include "../inc/trace.h"
#include "../inc/CBlockStoreService.h"
#include "../inc/CDriverClient.h"

using namespace std;
using namespace Common;

// Global that carries the user-serviceTypeName
std::string g_userServiceTypeName;

//#ifdef RELIABLE_COLLECTION_TEST 
#if defined(RELIABLE_COLLECTION_TEST) || !defined(ETL_TRACING_ENABLED) 
// Global instance of the tracing facility for Mock  Cluster mode.
tracer g_tracer;
#endif // RELIABLE_COLLECTION_TEST


string TypeOption1("/type:");
string TypeOption2("-type:");

bool ParseArguments(__in int argc, __in_ecount(argc) char* argv[], __out vector<string> & typesToHost)
{
    if (argc < 2)
    {
        return false;
    }

    for (int i = 1; i < argc; i++)
    {
        string argument(argv[i]);
        if ((argument.find(TypeOption1) == 0) ||
            (argument.find(TypeOption2) == 0))
        {
            string value = argument.substr(TypeOption1.size());
            if (!value.empty())
            {
                typesToHost.push_back(value);
            }
        }
        else
        {
            // unknown argument
            return false;
        }
    }

    return !typesToHost.empty();
}

int __cdecl main(int argc, __in_ecount(argc) char* argv[])
{

#if !defined(PLATFORM_UNIX)
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
#endif // !defined(PLATFORM_UNIX)

    // Init the default service name.
    g_userServiceTypeName = "[NoUserServiceName]";

    TRACE_INIT("[SFBlockStoreService]", "SFBlockStoreServiceTrace.log");

    // Parse the arguments to get the service type
    vector<string> typesToHost;
    auto fParsedArguments = ParseArguments(argc, argv, typesToHost);
    if (!fParsedArguments)
    {
        TRACE_ERROR("Unable to parse command line arguments.");
        return -1;
    }

    bool fInterrupted = false;
    int32_t iExitCode = 0;

    try
    {
        // Initialize the stateful service name using the first user-service type name we have been passed.
        u16string userServiceTypeName(typesToHost[0].begin(), typesToHost[0].end());
        g_userServiceTypeName = typesToHost[0];

        // Get the service partitionId from the environment. This is required to
        // register this service instance with the Docker Volume Driver.
        string servicePartitionId;
        char *pValue = std::getenv("Fabric_PartitionId");
        if (pValue != NULL)
        {
            servicePartitionId = pValue;
            TRACE_INFO("Service PartitionId is " << servicePartitionId);
        }
        else
        {
            TRACE_ERROR("Unable to fetch PartitionId of the service.");
            return -1;
        }

        int port = BlockStore::Constants::DefaultServicePort;
        
        // Get the service port if specified in environment
        pValue = std::getenv("SF_BLOCKSTORE_SERVICE_PORT");
        if (pValue != NULL)
        {
            port = atoi(pValue);
        }

        // Are we running on a single node cluster?
        bool fIsSingleNodeCluster = false;
        pValue = std::getenv("SF_BLOCKSTORE_SERVICE_SINGLE_NODE_CLUSTER");
        if (pValue != NULL)
        {
            if (atoi(pValue) == 1)
            {
                fIsSingleNodeCluster = true;
            }
        }

        boost::asio::io_service io_service;

        // Let all KTL related async operation resume on boost threads
        ktl_awaitable_policy::set_resume_func([&io_service](auto coroutine) {
            // If we are already running on boost threads that are hosting io_service, run it directly, 
            // otherwise post on boost threads
            io_service.dispatch([coroutine]() {
                // resume the coroutine on boost threads hosting io_service
                coroutine();
            });
        });

        TRACE_INFO("Initializing Reliable Service for user-service type: " << g_userServiceTypeName);

        if (fIsSingleNodeCluster)
        {
            TRACE_INFO("Executing in single node cluster mode");
        }
        else
        {
            TRACE_INFO("Executing in multi-machine cluster mode");    
        }

        // Only register app's endpoints
        BOOL registerManifestEndpoints = TRUE;
        BOOL skipUserPortEndpointRegistration = TRUE;
        BOOL reportEndpointsOnlyOnPrimaryReplica = TRUE;

#if defined(RELIABLE_COLLECTION_TEST)
        // For debug/testing, we will not register app's endpoints
        registerManifestEndpoints = FALSE;
        skipUserPortEndpointRegistration = FALSE;
        reportEndpointsOnlyOnPrimaryReplica = FALSE;
#endif // defined(RELIABLE_COLLECTION_TEST)

        reliable_service service;
        TRACE_INFO("Initializing BlockStore Service");

        CDriverClient driverClient;
        CDriverClient *pDriverClient = nullptr;

        if (driverClient.Initialize())
        {
            pDriverClient = &driverClient;
            TRACE_INFO("DriverClient initialization successful.");
        }
        else
        {
            TRACE_ERROR("DriverClient initialization failed.");
            iExitCode = -1;
        }

        if (pDriverClient != nullptr)
        {
            CBlockStoreService *pService = new CBlockStoreService(service, io_service, userServiceTypeName, servicePartitionId, (unsigned short)port, fIsSingleNodeCluster, pDriverClient);

            try
            {
                // Register the callbacks with the Reliable Collection Service infrastructure
                service.set_ChangeRoleCallback(&CBlockStoreService::ChangeRoleCallback);
                service.set_RemovePartitionCallback(&CBlockStoreService::RemovePartitionCallback);
                service.set_AbortPartitionCallback(&CBlockStoreService::AbortPartitionCallback);
                service.set_AddPartitionCallback(&CBlockStoreService::AddPartitionCallback);

                // Initialize the Reliable Collection Service infrastructure now that we are all set.
                service.initialize(userServiceTypeName.c_str(), pService->ServicePort, registerManifestEndpoints, skipUserPortEndpointRegistration, reportEndpointsOnlyOnPrimaryReplica);

                if (!pService->Start())
                {
                    iExitCode = -1;
                }
                
                fInterrupted = pService->WasServiceInterrupted;
            }
            catch (std::exception& e)
            {
                TRACE_ERROR("Exception during service execution: " << e.what());
                iExitCode = -1;
            }
            catch (...)
            {
                TRACE_ERROR("Unknown cxception during service execution!");
                iExitCode = -1;
            }

            // Cleanup service resources
            pService->Cleanup();

            // Release service instance
            pService->Release();
        }
    }
    catch (std::exception& e)
    {
        TRACE_ERROR("Exception: " << e.what());
        iExitCode = -1;
    }
    catch (...)
    {
        TRACE_ERROR("Unknown Exception!");
        iExitCode = -1;
    }

    TRACE_INFO("Process exiting.");

#if !defined(PLATFORM_UNIX)
    if (fInterrupted)
    {
        // If the service was interrupted, on Windows, we will terminate the process
        // to avoid the unhandled exception process dialog from popping up.
        TerminateProcess(GetCurrentProcess(), iExitCode);
    }
    else
#endif // !defined(PLATFORM_UNIX)
    {
        return iExitCode;
    }
}
