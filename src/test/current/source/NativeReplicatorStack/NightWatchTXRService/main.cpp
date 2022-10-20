// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace NightWatchTXRService;

int main(int, char**)
{
    auto callback =
        [](FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, ComponentRootSPtr const & root)
    {
        ComPointer<Service> service = make_com<Service>(partitionId, replicaId, root);
        ComPointer<StatefulServiceBase> serviceBase;
        serviceBase.SetAndAddRef(service.GetRawPointer());
        return serviceBase;
    };

    shared_ptr<Factory> factory = Factory::Create(callback);

    ComPointer<ComFactory> comFactory = make_com<ComFactory>(L"NightWatchTXRServiceType", *factory.get());

    ComPointer<IFabricRuntime> fabricRuntime;
    ASSERT_IFNOT(
        ::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    fabricRuntime->RegisterStatefulServiceFactory(
        L"NightWatchTXRServiceType",
        comFactory.GetRawPointer());

    printf("Press any key to terminate..\n");
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);

    return 0;
}
