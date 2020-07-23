// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SerializationInteropTest.h"

using namespace Common;
using namespace std;
using namespace NativeAndManagedSerializationInteropTest;

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    auto traceCode = TraceTaskCodes::Common;
    StringLiteral traceType = "SerializationInteropTestExe";
    Common::TraceNoise(traceCode, traceType, "argc: {0}", argc);

    /// Too few argument?
    if (argc < 4)
    {
        Trace.WriteError(traceType, "Error: Too few arguments. Expected 4, found {0}", argc);
        return ErrorCodeValue::InvalidArgument;
    }

    Common::TraceNoise(traceCode, traceType, "arg1: {0}, arg2: {1}, arg3: {2}", argv[1], argv[2], argv[3]);
    ErrorCode error = NativeAndManagedSerializationInteropTest::DeserializeAndSerialize(argv[1], argv[2], argv[3]);

    if (!error.IsSuccess())
    {
        Trace.WriteError(traceType, "Error: {0}", error.Message);
        Trace.WriteError(traceType, "Arguments were: arg1: {0}, arg2: {1}, arg3: {2}", argv[1], argv[2], argv[3]);
        return -1;
    }

    Trace.WriteInfo(traceType, "Exiting with success.");
    return 0;
}
