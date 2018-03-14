// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

#define TEST_COM_STATE_PROVIDER_FACTORY_TAG 'FSTF'
 
ComTestStateProvider2Factory::ComTestStateProvider2Factory(
    __in std::wstring const & nodeId,
    __in std::wstring const & serviceName)
    : KObject()
    , KShared()
    , innerComFactory_()
    , nodeId_(nodeId)
    , serviceName_(serviceName)
{ 
    TxnReplicator::TestCommon::TestStateProviderFactory::SPtr innerFactory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(GetThisAllocator());

    innerComFactory_ = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
        *innerFactory, 
        GetThisAllocator());
}

ComTestStateProvider2Factory::~ComTestStateProvider2Factory()
{
}

ComPointer<IFabricStateProvider2Factory> ComTestStateProvider2Factory::Create(
    __in wstring const & nodeId,
    __in wstring const & serviceName,
    __in KAllocator & allocator)
{
    ComTestStateProvider2Factory * pointer = _new(TEST_COM_STATE_PROVIDER_FACTORY_TAG, allocator) ComTestStateProvider2Factory(nodeId, serviceName);
    CODING_ERROR_ASSERT(pointer != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(pointer->Status()))

    ComPointer<IFabricStateProvider2Factory> result;
    result.SetAndAddRef(pointer);
    return result;
}

HRESULT ComTestStateProvider2Factory::Create(
    __in FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
    __out HANDLE * stateProvider) // IStateProvider2* *
{
    HANDLE innerStateProviderHandle = NULL;
    NTSTATUS status = innerComFactory_->Create(factoryArguments, &innerStateProviderHandle);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    TxnReplicator::IStateProvider2::SPtr innerStateProvider;
    innerStateProvider.Attach((TxnReplicator::IStateProvider2 *)innerStateProviderHandle);

    TxnReplicator::TestCommon::TestStateProvider * innerTestStateProvider = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider *>(innerStateProvider.RawPtr());
    TestStateProvider2::SPtr testStateProvider = TestStateProvider2::Create(nodeId_, serviceName_, *innerTestStateProvider, GetThisAllocator());

    TxnReplicator::IStateProvider2::SPtr outerStateProvider = testStateProvider.RawPtr();
    *stateProvider = outerStateProvider.Detach();

    return S_OK;
}
