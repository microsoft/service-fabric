// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SerializationInteropTest.h"

namespace NativeAndManagedSerializationInteropTest
{
    using namespace std;
    using namespace Common;

    StringLiteral traceType = "SerializationInteropTest";

    /// itemKind to choose between Query or Health
    ErrorCode DeserializeAndSerialize(wstring itemKind, wstring filepath1, wstring filepath2)
    {
        if (itemKind[0] == L"Q"[0] || itemKind[0] == L"q"[0]) // Query
        {
            Trace.WriteInfo(traceType, "Running test 'QuerySerializationInteropTest' with fileIn:{0} fileOut: {1} ", filepath1, filepath2);
			auto query = make_shared<QuerySerializationInteropTest>();
            return DeserializeAndSerializeItem<QuerySerializationInteropTest>(*query, filepath1, filepath2);
        }
        else if (itemKind[0] == L"D"[0] || itemKind[0] == L"d"[0]) // Description
        {
            Trace.WriteInfo(traceType, "Running test 'DescriptionSerializationInteropTest' with fileIn:{0} fileOut: {1} ", filepath1, filepath2);
            auto description = make_shared<DescriptionSerializationInteropTest>();
            return DeserializeAndSerializeItem<DescriptionSerializationInteropTest>(*description, filepath1, filepath2);
        }
        else
        {
            Trace.WriteInfo(traceType, "Running test 'HealthSerializationInteropTest' with fileIn:{0} fileOut: {1} ", filepath1, filepath2);
			auto health = make_shared<HealthSerializationInteropTest>();
            return DeserializeAndSerializeItem<HealthSerializationInteropTest>(*health, filepath1, filepath2);
        }
    }

    template<typename T>
    ErrorCode DeserializeAndSerializeItem(T &item, wstring filepath1, wstring filepath2)
    {
        ErrorCode er = DeserializeFromFile(item, filepath1);
        if (!er.IsSuccess())
        {
            Trace.WriteError(traceType, "DeserializeAndSerializeItem(): Error: call DeserializeFromFile() failed with {0}", er);
            // Don't return here as serialization of item will help in finding where deserialization failed.
        }

        er = SerializeToFile(item, filepath2);
        return er;
    }

    template<typename T>
    ErrorCode DeserializeFromFile(T &item, const wstring &fileIn)
    {
        Common::File fr;
        ErrorCode er = fr.TryOpen(fileIn);
        if (!er.IsSuccess()) 
        {
            Trace.WriteError(traceType, "Error: call to File.TryOpen() for file {0} failed with {1}", fileIn, er);
            return er; 
        }

        ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>(fr.size());
        byte *bytes = bufferUPtr->data();
        fr.Read(bytes, (int)bufferUPtr->capacity());
        fr.Close();
        er = JsonHelper::Deserialize(item, bufferUPtr);

        if (!er.IsSuccess())
        {
            Trace.WriteError(traceType, "Error: call to JsonHelper::Deserialize() failed with {0}", er);
        }

        return er;
    }

    template<typename T>
    ErrorCode SerializeToFile(T &item, const wstring &fileOut)
    {
        // Get serialized json in buffer
        ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
        ErrorCode er = JsonHelper::Serialize(item, bufferUPtr, JsonSerializerFlags::DateTimeInIsoFormat);
        if (!er.IsSuccess()) 
        {
            Trace.WriteError(traceType, "Error: call to JsonHelper::Serialize() failed with {0}", er);
            return er;
        }

        // Write buffer to file
        Common::FileWriter fw;
        er = fw.TryOpen(fileOut);
        if (!er.IsSuccess())
        {
            Trace.WriteError(traceType, "Error: call to FileWriter::TryOpen({0}) failed with {1}", fileOut, er);
            return er;
        }

        fw.WriteBuffer((char*)bufferUPtr->data(), bufferUPtr->size());
        fw.Flush();
        fw.Close();
        return ErrorCode::Success();
    }
}
