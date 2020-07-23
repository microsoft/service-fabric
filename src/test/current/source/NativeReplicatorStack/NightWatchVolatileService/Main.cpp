// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;

StringLiteral const TraceSource("V1RPS_Main");

int main(int, char**) 
{
    ComPointer<IFabricRuntime> fabricRuntime;
    auto factory = Factory::Create();
    ComPointer<ComFactory> comFactory = make_com<ComFactory>(*factory.get());

    ASSERT_IFNOT(
        ::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    fabricRuntime->RegisterStatefulServiceFactory(
        Constants::ServiceTypeName->c_str(),
        comFactory.GetRawPointer());

    printf("Press any key to terminate..\n");
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);
}
