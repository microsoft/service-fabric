// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace NightWatchTXRService;
using namespace TxnReplicator;

StringLiteral const TraceComponent("NightWatchTXRService");

Service::Service(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : StatefulServiceBase(partitionId, replicaId, root)
{
    testInProgress_ = false;
    testResultSptr_ = nullptr;
}

ComPointer<IFabricStateProvider2Factory> Service::GetStateProviderFactory()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    TxnReplicator::TestCommon::NoopStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::NoopStateProviderFactory::Create(allocator);
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
    Trace.WriteInfo(
        TraceComponent,
        "Received http POST with body length {0}",
        body->size());

    TestParametersBase baseParameters;
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat;

    auto error = JsonHelper::Deserialize<TestParametersBase>(baseParameters, body, flags);

    Trace.WriteInfo(
        TraceComponent,
        "Deserialized http POST request with object {0} with Error Code {1}",
        baseParameters,
        error);

    if (!error.IsSuccess())
    {
        auto response = TestError(L"Serialization error.", TestStatus::Enum::Invalid);
        return JsonHelper::Serialize<TestError>(response, responseBody, flags);
    }

    if (baseParameters.Action != TestAction::Enum::Run && baseParameters.Action != TestAction::Enum::Status)
    {
        auto response = TestError(L"Wrong action.", TestStatus::Enum::Invalid);
        return JsonHelper::Serialize<TestError>(response, responseBody, flags);
    }

    if (baseParameters.Action == TestAction::Enum::Run)
    {
        if (testInProgress_)
        {
            auto response = TestError(L"Test is already running. You cannot start a new one right now.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
        else
        {
            StartTest(body);
            auto response = TestError(L"Test is scheduled.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
    }
    else if (baseParameters.Action == TestAction::Enum::Status)
    {
        if (testInProgress_)
        {
            auto response = TestError(L"Test is still running. Check back later.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
        else if (!testInProgress_)
        {
            if (testResultSptr_ == nullptr)
            {
                auto response = TestError(L"No test is scheduled to run.", TestStatus::Enum::NotStarted);
                return JsonHelper::Serialize<TestError>(response, responseBody, flags);
            }
            else if (testResultSptr_->Exception != nullptr)
            {
                auto response = TestError(L"Exception has occured during the run.", TestStatus::Enum::Finished);
                return JsonHelper::Serialize<TestError>(response, responseBody, flags);
            }
            else
            {
                Trace.WriteInfo(
                    TraceComponent,
                    "Test result is ready: {0}",
                    testResultSptr_->ToString());
                return JsonHelper::Serialize<PerfResult>(*testResultSptr_, responseBody, flags);
            }
        }
    }

    return error;
}

ITestRunner::SPtr Service::CreateTestRunner(
    __in ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    auto sptr = TestRunner::Create(testParametersJson, txnReplicator, allocator);
    return sptr.RawPtr();
}

Task Service::StartTest(__in ByteBufferUPtr const & testParametersJson)
{
    KCoShared$ApiEntry();

    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    testInProgress_ = true;

    //resetting previous run results
    testResultSptr_ = nullptr;

    ITestRunner::SPtr runner = CreateTestRunner(testParametersJson, *TxReplicator, allocator);
    try
    {
        Trace.WriteInfo(
            TraceComponent,
            "Starting test");
        testResultSptr_ = co_await runner->Run();
    }
    catch (ktl::Exception ex)
    {
        Trace.WriteError(
            TraceComponent,
            "Ktl exception occurred");
        //setting exception flag to true, so running threads gracefully finish themselves.
        runner->ExceptionThrown = true;

        testResultSptr_ = PerfResult::Create(
            Data::Utilities::SharedException::Create(ex, allocator).RawPtr(),
            allocator);
    }
    catch (...)
    {
        Trace.WriteError(
            TraceComponent,
            "Exception occurred");
        CODING_ERROR_ASSERT(false)
    }

    Trace.WriteInfo(
        TraceComponent,
        "Test finished");
    testInProgress_ = false;

    co_return;
}
