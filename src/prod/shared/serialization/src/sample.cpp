// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


// Sample.CPP
// Compiles for both user mode and kernel mode

#include <Serialization.h>

using namespace Serialization;

void TestFunction()
{
    auto type = FabricSerializationTypes::MakeArray(FabricSerializationTypes::Int32);
    type;


    IFabricSerializableStream * streamPtr = nullptr;
    NTSTATUS status = FabricSerializableStream::Create(&streamPtr);

    if (NT_SUCCESS(status))
    {
        streamPtr->Release();
        streamPtr = nullptr;
    }
}

