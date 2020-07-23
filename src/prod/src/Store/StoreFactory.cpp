// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Store
{
    StringLiteral const TraceComponent("StoreFactory");

    ErrorCode StoreFactory::CreateLocalStore(
        StoreFactoryParameters const & parameters, 
        ILocalStoreSPtr & store)
    {
        HealthReport unused;
        return CreateLocalStore(parameters, store, unused);
    }

    ErrorCode StoreFactory::CreateLocalStore(
        StoreFactoryParameters const & parameters, 
        ILocalStoreSPtr & store,
        __out HealthReport & healthReport)
    {
        switch (parameters.Type)
        {
        case StoreType::Local:
        {
            auto error = CreateEseLocalStore(parameters, store);

            if (error.IsSuccess()) 
            {
                error = HandleEseMigration(parameters);
            }

            if (error.IsSuccess())
            {
                healthReport = CreateHealthyReport(parameters, ProviderKind::ESE);
            }
            else
            {
                healthReport = CreateUnhealthyReport(parameters, ProviderKind::ESE, error.Message);
            }

            return error;
        }
        case StoreType::TStore:
        {
            
            CreateTSLocalStore(parameters, store);

            auto error = HandleTStoreMigration(parameters);

            if (error.IsSuccess())
            {
                healthReport = CreateHealthyReport(parameters, ProviderKind::TStore);
            }
            else
            {
                healthReport = CreateUnhealthyReport(parameters, ProviderKind::TStore, error.Message);
            }

            return ErrorCodeValue::Success;
        }

        default:
            Assert::CodingError("Unknown type {0}", static_cast<int>(parameters.Type));
        }
    }
    
    ErrorCode StoreFactory::CreateEseLocalStore(StoreFactoryParameters const & parameters, __out ILocalStoreSPtr & store)
    {
#ifdef PLATFORM_UNIX
        UNREFERENCED_PARAMETER(parameters);
        UNREFERENCED_PARAMETER(store);

        return ErrorCode(ErrorCodeValue::NotImplemented, L"ESE not supported in Linux for RA");
#else

        auto error = Directory::Create2(parameters.DirectoryPath);
        if (!error.IsSuccess()) { return error; }

        EseLocalStoreSettings settings;
        settings.StoreName = parameters.FileName;
        settings.AssertOnFatalError = parameters.AssertOnFatalError;

        store = make_shared<EseLocalStore>(
            parameters.DirectoryPath,
            settings,
            EseLocalStore::LOCAL_STORE_FLAG_NONE);

        return ErrorCodeValue::Success;
#endif
    }

    void StoreFactory::CreateTSLocalStore(StoreFactoryParameters const & parameters, __out ILocalStoreSPtr & store)
    {
        store = make_shared<TSLocalStore>(
            make_unique<TSReplicatedStoreSettings>(parameters.DirectoryPath),
            parameters.KtlLogger);
    }

    ErrorCode StoreFactory::HandleEseMigration(StoreFactoryParameters const & parameters)
    {
        if (parameters.KtlLogger.get() == nullptr)
        {
            //
            // Expected in RA unit tests
            //
            Trace.WriteInfo(TraceComponent, "KtlLogger is null - skipping TStore database files check");

            return ErrorCodeValue::Success;
        }

        ILocalStoreSPtr store;
        CreateTSLocalStore(parameters, store);

        auto tstoreCasted = dynamic_cast<TSLocalStore*>(store.get());

        auto sharedLogFilePath = parameters.KtlLogger->SystemServicesSharedLogSettings->Settings.Path;
        
        if (File::Exists(sharedLogFilePath))
        {
            if (parameters.AllowMigration)
            {
                Trace.WriteInfo(TraceComponent, "Deleting TStore database files for migration since '{0}' exists", sharedLogFilePath);

                return tstoreCasted->DeleteDatabaseFiles(sharedLogFilePath);
            }
            else
            {
                auto msg = wformatString(StringResource::Get ( IDS_RA_STORE_ESE_MIGRATION_BLOCKED ), sharedLogFilePath);
                Trace.WriteWarning(TraceComponent, "{0}", msg);

                return ErrorCode(ErrorCodeValue::OperationFailed, move(msg));
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode StoreFactory::HandleTStoreMigration(StoreFactoryParameters const & parameters)
    {
#ifdef PLATFORM_UNIX
        UNREFERENCED_PARAMETER(parameters);

        return ErrorCodeValue::Success;
#else
        ILocalStoreSPtr store;
        auto error = CreateEseLocalStore(parameters, store);
        if (!error.IsSuccess()) { return error; }

        auto eseCasted = dynamic_cast<EseLocalStore*>(store.get());

        auto eseDirectory = eseCasted->Directory;
        auto eseFileName = eseCasted->FileName;
        auto edbFilePath = Path::Combine(eseDirectory, eseFileName);

        if (File::Exists(edbFilePath))
        {
            if (parameters.AllowMigration)
            {
                Trace.WriteInfo(TraceComponent, "Deleting '{0}' for migration since '{1}' exists", eseDirectory, edbFilePath);

                error = eseCasted->Cleanup();
            }
            else
            {
                auto msg = wformatString(StringResource::Get ( IDS_RA_STORE_TSTORE_MIGRATION_BLOCKED ), edbFilePath);
                Trace.WriteWarning(TraceComponent, "{0}", msg);

                error = ErrorCode(ErrorCodeValue::OperationFailed, move(msg));
            }
        }

        return error;
#endif
    }

    HealthReport StoreFactory::CreateHealthyReport(
        StoreFactoryParameters const & parameters, 
        ProviderKind::Enum providerKind)
    {
        EntityHealthInformationSPtr entity;
        AttributeList attributes;

        CreateHealthReportAttributes(parameters, entity, attributes);

        return HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_StoreProviderHealthy,
            move(entity),
            wformatString(StringResource::Get( IDS_RA_STORE_PROVIDER_HEALTHY ), providerKind),
            move(attributes));
    }

    HealthReport StoreFactory::CreateUnhealthyReport(
        StoreFactoryParameters const & parameters,
        ProviderKind::Enum providerKind,
        wstring const & details)
    {
        EntityHealthInformationSPtr entity;
        AttributeList attributes;

        CreateHealthReportAttributes(parameters, entity, attributes);

        return HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_StoreProviderUnhealthy,
            move(entity),
            wformatString(StringResource::Get( IDS_RA_STORE_PROVIDER_UNHEALTHY ), providerKind, details),
            move(attributes));
    }

    void StoreFactory::CreateHealthReportAttributes(
        StoreFactoryParameters const & parameters, 
        __out EntityHealthInformationSPtr & entity,
        __out AttributeList & attributes)
    {
        auto const & nodeInstance = parameters.NodeInstance;
        auto const & nodeName = parameters.NodeName;

        entity = EntityHealthInformation::CreateNodeEntityHealthInformation(
            nodeInstance.Id.IdValue,
            nodeName,
            nodeInstance.InstanceId);

        attributes.AddAttribute(*HealthAttributeNames::NodeId, wformatString(nodeInstance.Id));
        attributes.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(nodeInstance.InstanceId));
        attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }
}
