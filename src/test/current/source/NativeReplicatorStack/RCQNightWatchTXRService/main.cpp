//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace RCQNightWatchTXRService;

int main(int, char**)
{
    auto callback =
        [](FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, ComponentRootSPtr const & root)
    {
        ComPointer<RCQService> service = make_com<RCQService>(partitionId, replicaId, root);
        ComPointer<StatefulServiceBase> serviceBase;
        serviceBase.SetAndAddRef(service.GetRawPointer());
        return serviceBase;
    };

    shared_ptr<Factory> factory = Factory::Create(callback);

    ComPointer<ComFactory> comFactory = make_com<ComFactory>(L"RCQNightWatchTXRServiceType", *factory.get());

    ComPointer<IFabricRuntime> fabricRuntime;
    ASSERT_IFNOT(
        ::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    fabricRuntime->RegisterStatefulServiceFactory(
        L"RCQNightWatchTXRServiceType",
        comFactory.GetRawPointer());

    printf("Press any key to terminate..\n");
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);

    return 0;
}
