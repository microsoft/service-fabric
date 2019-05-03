// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace CrashTestTXRService;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

StringLiteral const TraceComponent("CrashTestService");

Service::Service(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : StatefulServiceBase(partitionId, replicaId, root)
    , random_(GetTickCount())
    , randomCrashChance_(-1)
{
    GetRandomCrashChanceConfig();
}

void Service::GetRandomCrashChanceConfig()
{
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadTransactionalReplicatorSettings");

    ComPointer<IFabricConfigurationPackage> configPackageCPtr;
    activationContext.GetRawPointer()->GetConfigurationPackage(L"Config", configPackageCPtr.InitializationAddress());

    int64 randomCrashChanceValue;
    bool hasValue = false;

    ErrorCode error = ServiceModel::Parser::ReadSettingsValue(configPackageCPtr, L"TestConfig", L"RandomCrashChance", randomCrashChanceValue, hasValue);

    if (!error.IsSuccess() || !hasValue)
    {
        Trace.WriteInfo(TraceComponent, "Random Crash Chance config not found.", randomCrashChanceValue);
    }
    else
    {
        Trace.WriteInfo(TraceComponent, "Random Crash Chance = {0} read from config.", randomCrashChanceValue);
        randomCrashChance_ = randomCrashChanceValue;
    }
}

ComPointer<IFabricStateProvider2Factory> Service::GetStateProviderFactory()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    TxnReplicator::TestCommon::TestStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(allocator);
    ComPointer<IFabricStateProvider2Factory> comFactory = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
        *factory, allocator);

    return comFactory;
}

ComPointer<IFabricDataLossHandler> Service::GetDataLossHandler()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();
    return TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(allocator);
}

ErrorCode Service::OnHttpPostRequest(
    __in ByteBufferUPtr && body,
    __in ByteBufferUPtr & responseBody)
{
    UNREFERENCED_PARAMETER(body);
    UNREFERENCED_PARAMETER(responseBody);
    return ErrorCodeValue::Success;
}

void Service::OnChangeRole(
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
    
    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        Trace.WriteInfo(TraceComponent, "Role is Priamry. Starting the run to crash.");
        RunToCrash();
    }
}

Task Service::RunToCrash()
{
    KCoShared$ApiEntry();

    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    Trace.WriteInfo(TraceComponent, "pushing the initial load.");

    // This will ensure we had a write quorum of replicas to start off with in the beginning of the test
    co_await AddSPAsync(L"fabrictest:/txrsp/:firstSP", allocator);

    Trace.WriteInfo(TraceComponent, "first SP added.");

    while (true)
    {
        bool toCrash = random_.Next((int)randomCrashChance_) == 0;
        Trace.WriteInfo(TraceComponent, "Going to crash: {0}", toCrash);
        if (toCrash)
        {
            NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, CRASHTEST_TAG, 1000, nullptr);
            TESTASSERT_IFNOT(NT_SUCCESS(status), "Failed to create a timer during crash block");

            exit(EXIT_SUCCESS);
        }
        else
        {
            Common::Guid key = Common::Guid::NewGuid();
            wstring keyStateProvider = wformatString("fabrictest:/txrsp/:{0}", key);

            Trace.WriteInfo(TraceComponent, "Adding StateProvider {0}", keyStateProvider);

            co_await AddSPAsync(keyStateProvider, allocator);
            
            NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, CRASHTEST_TAG, random_.Next(10, 10000), nullptr);
            TESTASSERT_IFNOT(NT_SUCCESS(status), "Failed to create a timer after SP add.");

            Trace.WriteInfo(TraceComponent, "StateProvider {0} is created.", keyStateProvider);
        }
    }

    co_return;
}

Awaitable<void> Service::AddSPAsync(__in std::wstring const & key, __in KAllocator & allocator)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    bool createdSP = false;

    while (!createdSP)
    {
        try
        {
            TxnReplicator::Transaction::SPtr transaction;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(transaction));
            
            TxnReplicator::IStateProvider2::SPtr stateProvider;
            KUriView keyUri(&key[0]);
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());
            std::wstring valueString = L"i";
            KStringView value(&valueString[0]);

            status = TxReplicator->Get(keyUri, stateProvider);

            Trace.WriteInfo(TraceComponent, "Key Exists: {0}. Get API returns {1}", NT_SUCCESS(status), status);

            if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                Data::Utilities::OperationData::SPtr op = Data::Utilities::OperationData::Create(allocator);

                Data::Utilities::BinaryWriter bw(allocator);

                bw.Write(value, Data::Utilities::UTF16);

                op->Append(*bw.GetBuffer(0));

                status = co_await TxReplicator->AddAsync(
                    *transaction,
                    keyUri,
                    typeName,
                    op.RawPtr());
                THROW_ON_FAILURE(status);
            }
            else if (!NT_SUCCESS(status))
            {
                throw Exception(status);
            }

            THROW_ON_FAILURE(co_await transaction->CommitAsync());
            transaction->Dispose();

            createdSP = true;
        }
        catch (Exception ex)
        {
            //ignore exceptions and retry
        }
        catch (...)
        {
            Trace.WriteWarning(TraceComponent, "unknown exception type has thrown.");
        }

        //adding a 1 second pause, hence if the call to GetAdd is failing, thread will be switched and we retry
        status = co_await KTimer::StartTimerAsync(allocator, CRASHTEST_TAG, 1000, nullptr);
        TESTASSERT_IFNOT(NT_SUCCESS(status), "Failed to create a timer after SP add.");
    }
}
