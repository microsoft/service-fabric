// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <intsafe.h>
#include <Serialization.h>
using namespace Serialization;

#include "SerializationTests.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include <KSerializationHelper.h>
#include "TestUtility.h"
#include "BasicObject.h"
#include "BasicNestedObject.h"
#include "BasicObjectWithPointers.h"
#include "PolymorphicObject.h"
#include "BasicVersionedObjects.h"
#include "BasicRvdObject.h"
#include "TreeObject.h"
#include "BadObject.h"

#ifdef KDbgPrintf
#undef KDbgPrintf
#endif
#define KDbgPrintf(...)     if (enablePrint) printf(__VA_ARGS__)
#define TurnOffPrint        enablePrint = false;   //used when doing performance test.
#define TurnOnPrint         enablePrint = true; 
bool enablePrint = true;

typedef NTSTATUS (*SerializationFunction)(void);
typedef struct FunctionTable {
    SerializationFunction funcPtr;
    char * funcName;
} FunctionTable;

int perfTestIter = 1;    
__int64 totalDuration = 0;
static __int64 const TicksPerMillisecond = 10000;

template <class T, class TPtr>
NTSTATUS SimpleSerializationTest(T * object, FabricActivatorFunction activator = nullptr, TPtr ** outPtr = nullptr);

template <class TIn, class TOut>
NTSTATUS VersionedSerializationTest(TIn * object1, TOut & object2);

NTSTATUS BasicSerializationTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicSerializationTest\n");

    // Create a sample object
    BasicObject object;

    object._short1 = -10;
    object._ushort1 = 10;
    object._bool1 = true;
    object._uchar1 = 0xF8;
    object._char1 = 'd';
    object._ulong64_1 = 0xFFFFFFFFFFFFFFFF;
    object._long64_1 = 0x0FFFFFFFFFFFFFFF;
    object._double = 89.3;

    object._ulong64ArraySize = 16;
    object._ulong64Array = new ULONG64[object._ulong64ArraySize];

    for (ULONG i = 0; i < object._ulong64ArraySize; ++i)
    {
        object._ulong64Array[i] = i;
    }

    wchar_t str[] = L"Hello object";

    wcsncpy_s(object._string, BasicObject::StringSize, str, _TRUNCATE);

    // {14E4F405-BA48-4B51-8084-0B6C5523F29E}
    GUID guid = { 0x14e4f405, 0xba48, 0x4b51, { 0x80, 0x84, 0xb, 0x6c, 0x55, 0x23, 0xf2, 0x9e } };
    object._guid = guid;

    NTSTATUS status = SimpleSerializationTest<BasicObject, BasicObject>(&object);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicSerializationWithChildTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicSerializationWithChildTest\n");

    // Create a sample object
    BasicObjectChild object;

    object.c_long1 = 0xfab61c;

    object._short1 = 1000;
    object._ushort1 = 10454;
    object._bool1 = false;
    object._uchar1 = 0x0b;
    object._char1 = 'v';
    object._ulong64_1 = 0xFFFFFFFFFF;
    object._long64_1 = -1;
    object._double = -9.343;

    object._ulong64ArraySize = 100;
    object._ulong64Array = new ULONG64[object._ulong64ArraySize];

    for (ULONG i = 0; i < object._ulong64ArraySize; ++i)
    {
        object._ulong64Array[i] = (0xFFFFFFFFF - i * 13);
    }

    wchar_t str[] = L"striiiing";

    wcsncpy_s(object._string, BasicObject::StringSize, str, _TRUNCATE);

    // {14E4F405-BA48-4B51-8084-0B6C5523F29E}
    GUID guid = { 0x14e4f405, 0xba48, 0x4b51, { 0x80, 0x84, 0xb, 0x6c, 0x55, 0x23, 0xf2, 0x9e } };
    object._guid = guid;

    NTSTATUS status = SimpleSerializationTest<BasicObjectChild, BasicObjectChild>(&object);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicVersioningChildReadBaseTest()
{
    KDbgPrintf("\nBasicVersioningChildReadBaseTest1\n");
    // Create a sample object
    BasicObjectUsingMacro object1;

    object1._bool = true;
    object1._ulong = 0x10;

    NTSTATUS status;

    {
        BasicChildObjectUsingMacro object2;

        status = VersionedSerializationTest<BasicObjectUsingMacro, BasicChildObjectUsingMacro>(&object1, object2);

        if (NT_SUCCESS(status))
        {
            bool equal = object1.operator==(object2) && object2.HasDefaultValues();

            if (!equal)
            {
                status = STATUS_CANCELLED;
            }
        }
    }

    if (NT_ERROR(status)) return status;

    {
        BasicChild2ObjectUsingMacro object2;

        status = VersionedSerializationTest<BasicObjectUsingMacro, BasicChildObjectUsingMacro>(&object1, object2);

        if (NT_SUCCESS(status))
        {
            bool equal = object1.operator==(object2) && object2.HasDefaultValues();

            if (!equal)
            {
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicVersioningChildTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicVersioningChildTest\n");

    BasicChildObjectUsingMacro object1;

    object1._bool = false;
    object1._short = 999;
    object1._ulong = 0xDDDD;
    object1._guid.CreateNew();

    BasicObjectUsingMacro object2;

    NTSTATUS status = VersionedSerializationTest<BasicChildObjectUsingMacro, BasicObjectUsingMacro>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._bool != object2._bool || object1._ulong != object2._ulong)
        {
            KDbgPrintf("Objects 1 and 2 not equal\n");
            status = STATUS_CANCELLED;
        }

        if (NT_ERROR(status)) return status;

        BasicChildObjectUsingMacro object3;

        // Deserialize again to make sure data was carried along for the ride        
        status = VersionedSerializationTest<BasicObjectUsingMacro, BasicChildObjectUsingMacro>(&object2, object3);

        if (NT_SUCCESS(status))
        {
            if (!(object3 == object1))
            {
                KDbgPrintf("Objects 1 and 3 not equal\n");
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicVersioningChild2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicVersioningChild2Test\n");

    BasicChild2ObjectUsingMacro object1;

    object1._bool = false;
    object1._short = 999;
    object1._ulong = 0xDDDD;
    //object1._guid.CreateNew(); leave as empty guid
    object1._char = '\0';
    object1._long64 = 0xF123456;

    BasicObjectUsingMacro object2;

    NTSTATUS status = VersionedSerializationTest<BasicChild2ObjectUsingMacro, BasicObjectUsingMacro>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._bool != object2._bool || object1._ulong != object2._ulong)
        {
            KDbgPrintf("Objects 1 and 2 not equal\n");
            status = STATUS_CANCELLED;
        }

        if (NT_ERROR(status)) return status;

        BasicChild2ObjectUsingMacro object3;

        // Deserialize again to make sure data was carried along for the ride
        status = VersionedSerializationTest<BasicObjectUsingMacro, BasicChild2ObjectUsingMacro>(&object2, object3);

        if (NT_SUCCESS(status))
        {
            if (!(object3 == object1))
            {
                KDbgPrintf("Objects 1 and 3 not equal\n");
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicNestedObjectTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicNestedObject\n");

    BasicNestedObject parent;
    parent._char1 = 'z';
    parent._short1 = -10324;

    parent._basicObject._short1 = 1000;
    parent._basicObject._ushort1 = 10454;
    parent._basicObject._bool1 = false;
    parent._basicObject._uchar1 = 0x0b;
    parent._basicObject._char1 = 'v';
    parent._basicObject._ulong64_1 = 0xFFFFFFFFFF;
    parent._basicObject._long64_1 = 0x0FFFFFFFFFFFF;
    parent._basicObject._double = -9.343;

    parent._basicObject._ulong64ArraySize = 100;
    parent._basicObject._ulong64Array = new ULONG64[parent._basicObject._ulong64ArraySize];

    for (ULONG i = 0; i < parent._basicObject._ulong64ArraySize; ++i)
    {
        parent._basicObject._ulong64Array[i] = (0xFFFFFFFFF - i * 13);
    }

    for (ULONG i = 0; i < 10; ++i)
    {
        BasicObjectUsingMacro object;
        object._ulong = i;
        object._bool = ((i & 1) == 1);
        parent._basicObjectArray.Append(Ktl::Move(object));
    }

    wchar_t str[] = L"striiiing";

    wcsncpy_s(parent._basicObject._string, BasicObject::StringSize, str, _TRUNCATE);

    // {14E4F405-BA48-4B51-8084-0B6C5523F29E}
    GUID guid = { 0x14e4f405, 0xba48, 0x4b51, { 0x80, 0x84, 0xb, 0x6c, 0x55, 0x23, 0xf2, 0x9e } };
    parent._basicObject._guid = guid;

    NTSTATUS status = SimpleSerializationTest<BasicNestedObject, BasicNestedObject>(&parent);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicObjectWithPointersTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicObjectWithPointers\n");

    BasicObjectWithPointers parent;
    parent._basicObject1 = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) BasicObject();

    parent._basicObject1->_short1 = 1000;
    parent._basicObject1->_ushort1 = 10454;
    parent._basicObject1->_bool1 = false;
    parent._basicObject1->_uchar1 = 0x0b;
    parent._basicObject1->_char1 = 'v';
    parent._basicObject1->_ulong64_1 = 0xFFFFFFFFFF;
    parent._basicObject1->_long64_1 = 0x0FFFFFFFFFFFF;
    parent._basicObject1->_double = -9.343;

    // {14E4F405-BA48-4B51-8084-0B6C5523F29E}
    GUID guid = { 0x14e4f405, 0xba48, 0x4b51, { 0x80, 0x84, 0xb, 0x6c, 0x55, 0x23, 0xf2, 0x9e } };
    parent._basicObject1->_guid = guid;

    NTSTATUS status = SimpleSerializationTest<BasicObjectWithPointers, BasicObjectWithPointers>(&parent);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS PolymorphicObjectTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nPolymorphicObjectTest\n");
    // Create a sample object
    PolymorphicObject object;

    object._short1 = 1000;
    object._string = L"My sample KWString";

    NTSTATUS status;

    for (ULONG i = 0; i < 10; ++i)
    {
        status = object._ulongArray1.Append(i);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    status = SimpleSerializationTest<PolymorphicObject, PolymorphicObject>(&object, PolymorphicObject::Activator);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS PolymorphicObjectChildTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nPolymorphicObjectChildTest\n");
    // Create a sample object
    PolymorphicObjectChild1 object;

    object._short1 = 1000;
    object.c_long1 = 4324;
    object._string = L"My sample KWString for a child";

    NTSTATUS status;

    for (ULONG i = 0; i < 10; ++i)
    {
        status = object._ulongArray1.Append(i * 50);
        if (NT_ERROR(status))
        {
            return status;
        }
    }

    PolymorphicObject * deserializedPtr = nullptr;
    status = SimpleSerializationTest<PolymorphicObjectChild1, PolymorphicObject>(&object, PolymorphicObject::Activator, &deserializedPtr);

    //if (NT_SUCCESS(success))
    {
        //KDbgPrintf("Id of deserialized object from virtual function call: %i\n", deserializedPtr->GetId());
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicObjectVersioningV1ToV2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicObjectVersioningV1ToV2Test\n");
    // Create a sample object
    BasicObjectV1 object1;

    object1._char1 = 'F';

    BasicObjectV2 object2;

    NTSTATUS status = VersionedSerializationTest<BasicObjectV1, BasicObjectV2>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._char1 != object2._char1 || object2._ulong64 != BasicObjectV2::DefaultULong64)
        {
            status = STATUS_CANCELLED;
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicObjectVersioningV2ToV1ToV2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicObjectVersioningV2ToV1ToV2Test\n");
    // Create a sample object
    BasicObjectV2 object1;

    object1._char1 = 'F';
    object1._ulong64 = 0xF00D;
    object1._short1 = 0xBAD;
    object1._charArray.Append('y');
    object1._charArray.Append('e');
    object1._charArray.Append('s');

    object1._basicUnknownNested._char1 = 'y';
    object1._basicUnknownNestedUPtr = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) BasicUnknownNestedObject();
    object1._basicUnknownNestedUPtr->_char1 = 'r';

    BasicObjectV1 object2;

    NTSTATUS status = VersionedSerializationTest<BasicObjectV2, BasicObjectV1>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._char1 != object2._char1)
        {
            KDbgPrintf("Objects 1 and 2 not equal\n");
            status = STATUS_CANCELLED;
        }

        if (NT_ERROR(status)) return status;

        BasicObjectV2 object3;

        BasicObjectV1 object2b(object2);

        // Deserialize again to make sure data was carried along for the ride
        status = VersionedSerializationTest<BasicObjectV1, BasicObjectV2>(&object2b, object3);

        if (NT_SUCCESS(status))
        {
            if (!(object3 == object1))
            {
                KDbgPrintf("Objects 1 and 3 not equal\n");
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS ObjectWithArraysVersioningV1ToV2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nObjectWithArraysVersioningV1ToV2Test\n");
    // Create a sample object
    BasicObjectWithArraysV1 object1;

    object1._long64 = 0x4234;

    BasicObjectWithArraysV2 object2;

    NTSTATUS status = VersionedSerializationTest<BasicObjectWithArraysV1, BasicObjectWithArraysV2>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._long64 != object2._long64)
        {
            status = STATUS_CANCELLED;
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS ObjectWithArraysVersioningV2ToV1ToV2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nObjectWithArraysVersioningV2ToV1ToV2Test\n");
    // Create a sample object
    BasicObjectWithArraysV2 object1;

    object1._long64 = 0xfafafaf;

    NTSTATUS status;

    status = object1._boolArray.Append(true);
    if (NT_ERROR(status)) return status;

    status = object1._boolArray.Append(false);
    if (NT_ERROR(status)) return status;

    status = FillArray<CHAR>(object1._charArray, 'a', 10);
    if (NT_ERROR(status)) return status;

    status = FillArray<UCHAR>(object1._ucharArray, 0x20, 5);
    if (NT_ERROR(status)) return status;

    status = FillArray<SHORT>(object1._shortArray, 1, 15);
    if (NT_ERROR(status)) return status;

    status = FillArray<USHORT>(object1._ushortArray, 0x23, 5);
    if (NT_ERROR(status)) return status;

    status = FillArray<LONG>(object1._longArray, 0x2f0, 53);
    if (NT_ERROR(status)) return status;

    status = FillArray<ULONG>(object1._ulongArray, 0x20, 12);
    if (NT_ERROR(status)) return status;

    status = FillArray<LONG64>(object1._long64Array, 0x2f0, 53);
    if (NT_ERROR(status)) return status;

    status = FillArray<ULONG64>(object1._ulong64Array, 0x20, 12);
    if (NT_ERROR(status)) return status;

    status = FillArray<DOUBLE>(object1._doubleArray, -5.33, 20);
    if (NT_ERROR(status)) return status;

    for (ULONG i = 0; i < 13; ++i)
    {
        KGuid guid;
        guid.CreateNew();

        status = object1._guidArray.Append(guid);
        if (NT_ERROR(status)) return status;
    }

    for (ULONG i = 0; i < 3; ++i)
    {
        BasicUnknownNestedObject nested;
        nested._char1 = 'y';
        nested._ulong64 = i;

        status = object1._basicUnknownNestedObjectArray.Append(Ktl::Move(nested));
        if (NT_ERROR(status)) return status;
    }

    for (ULONG i = 0; i < 9; ++i)
    {
        BasicUnknownNestedObject * nested = nullptr;

        if ((i % 3) == 0)
        {
            nested = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) BasicUnknownNestedObject();
            nested->_char1 = 'x';
            nested->_ulong64 = i * 3;
        }

        status = object1._basicUnknownNestedPointerArray.Append(KUniquePtr<BasicUnknownNestedObject>(nested));
        if (NT_ERROR(status)) return status;
    }

    BasicObjectWithArraysV1 object2;

    status = VersionedSerializationTest<BasicObjectWithArraysV2, BasicObjectWithArraysV1>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._long64 != object2._long64)
        {
            KDbgPrintf("Objects 1 and 2 not equal\n");
            status = STATUS_CANCELLED;
        }

        if (NT_ERROR(status)) return status;
        BasicObjectWithArraysV2 object3;

        // Deserialize again to make sure data was carried along for the ride
        status = VersionedSerializationTest<BasicObjectWithArraysV1, BasicObjectWithArraysV2>(&object2, object3);

        if (NT_SUCCESS(status))
        {
            if (!(object3 == object1))
            {
                KDbgPrintf("Objects 1 and 3 not equal\n");
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS InheritedObjectV2ToV1ToV2Test()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nInheritedObjectV2ToV1ToV2Test\n");

    // Create a sample object
    BasicInheritedObjectV2Child object1;

    object1._char1 = 'Z';
    object1._ulong64 = 0xF00D5;
    object1._newString = L"Step 1";
    object1._kguid.CreateNew();
    object1._short1 = -6000;

    BasicInheritedObjectV1Child object2;

    NTSTATUS status = VersionedSerializationTest<BasicInheritedObjectV2Child, BasicInheritedObjectV1Child>(&object1, object2);

    if (NT_SUCCESS(status))
    {
        if (object1._char1 != object2._char1 || object1._ulong64 != object2._ulong64)
        {
            KDbgPrintf("Objects 1 and 2 not equal\n");
            status = STATUS_CANCELLED;
        }

        if (NT_ERROR(status)) return status;
        BasicInheritedObjectV2Child object3;

        // Deserialize again to make sure data was carried along for the ride
        status = VersionedSerializationTest<BasicInheritedObjectV1Child, BasicInheritedObjectV2Child>(&object2, object3);

        if (NT_SUCCESS(status))
        {
            if (!(object3 == object1))
            {
                KDbgPrintf("Objects 1 and 3 not equal\n");
                status = STATUS_CANCELLED;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BasicRvdObjectTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBasicRvdObjectTest\n");

    // Create a sample object
    BasicRvdObject::SPtr object;

    if (NT_ERROR(BasicRvdObject::Create(object)))
    {
        KDbgPrintf("Failed to construct object\n");
    }

    object->_short1 = 1000;
    object->_string = L"My sample KWString that is awesome";

    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < 4; ++i)
    {
        status = object->AllocateLargeBuffer(1024 * (i + 1), (UCHAR)i);

        if (NT_ERROR(status))
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        for (ULONG64 i = 0; i < 64; ++i)
        {
            status = object->_array.Append(i);
            if (NT_ERROR(status))
            {
                break;
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        status = SimpleSerializationTest<BasicRvdObject, BasicRvdObject>(object.Detach(), BasicRvdObject::Activator);
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS TreeObjectTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nTreeObjectTest\n");

    TreeObject::UPtr uptr = CreateTree();

    NTSTATUS status = uptr->Status();

    if (NT_ERROR(status))
    {
        return status;
    }

    status = SimpleSerializationTest<TreeObject, TreeObject>(uptr);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS TreeObjectMixedVersionsTest()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nTreeObjectMixedVersionsTest\n");

    TreeObject::UPtr uptr = CreateTree(true);

    NTSTATUS status = uptr->Status();

    if (NT_ERROR(status))
    {
        return status;
    }

    status = SimpleSerializationTest<TreeObject, TreeObject>(uptr);

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

NTSTATUS BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy()
{
    //EventRegisterMicrosoft_RvdProv();

    KDbgPrintf("\nBadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy\n");

    NTSTATUS status;

    {
        // Create a sample object with empty byte arrays
        BadObjectV2 object1;

        BadObject object2;

        status = VersionedSerializationTest<BadObjectV2, BadObject>(&object1, object2);

        if (NT_SUCCESS(status))
        {
            KDbgPrintf("Object deserialized when it shouldn't\n");
            status = STATUS_CANCELLED;
        }
        else
        {
            if (status == K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS)
            {
                KDbgPrintf("Deserialize noticed unknown extension buffers and failed appropriately\n");
                status = STATUS_SUCCESS;
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        // Create a sample object with byte arrays
        BadObjectV2 object1(true);

        BadObject object2;

        status = VersionedSerializationTest<BadObjectV2, BadObject>(&object1, object2);

        if (NT_SUCCESS(status))
        {
            KDbgPrintf("Object deserialized when it shouldn't\n");
            status = STATUS_CANCELLED;
        }
        else
        {
            if (status == K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS)
            {
                KDbgPrintf("Deserialize noticed unknown extension buffers and failed appropriately\n");
                status = STATUS_SUCCESS;
            }
        }
    }

    //EventUnregisterMicrosoft_RvdProv();

    return status;
}

class MockTransport
{
public:

    // TODO: use different default size when mem channel is stable
    MockTransport()
        : _message(Common::GetSFDefaultPagedKAllocator(), 4096 * 10)
        , _extensions(Common::GetSFDefaultPagedKAllocator())
    {
    }

    NTSTATUS Send(KArray<FabricIOBuffer> & messageBuffers, KArray<FabricIOBuffer> & extensionBuffers)
    {
        NTSTATUS status = STATUS_SUCCESS;
        for (ULONG i = 0; i < messageBuffers.Count(); ++i)
        {
            FabricIOBuffer buffer = messageBuffers[i];

            status = this->_message.Write(buffer.length, buffer.buffer);

            if (NT_ERROR(status))
            {
                return status;
            }
        }

        for (ULONG i = 0; i < extensionBuffers.Count(); ++i)
        {
            FabricIOBuffer buffer = extensionBuffers[i];

            UCHAR * byteArray = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) UCHAR[buffer.length];
            if (byteArray == nullptr)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> byteArrayUPtr(byteArray);

            memcpy(byteArray, buffer.buffer, buffer.length);

            DataContext context(buffer.length, Ktl::Move(byteArrayUPtr));

            status = this->_extensions.Append(Ktl::Move(context));

            if (NT_ERROR(status))
            {
                return status;
            }
        }

        return status;
    }

    NTSTATUS Receive(KArray<FabricIOBuffer> & messageBuffers, KArray<FabricIOBuffer> & extensionBuffers)
    {
        NTSTATUS status = STATUS_SUCCESS;

        KMemRef memoryReference;

        for (ULONG i = 0; i < this->_message.GetBlockCount(); ++i)
        {
            status = this->_message.GetBlock(i, memoryReference);

            if (NT_ERROR(status))
            {
                return status;
            }

            FabricIOBuffer buffer;
            buffer.buffer = static_cast<UCHAR*>(memoryReference._Address);
            buffer.length = memoryReference._Size;

            status = messageBuffers.Append(buffer);

            if (NT_ERROR(status))
            {
                return status;
            }
        }

        for (ULONG i = 0; i < this->_extensions.Count(); ++i)
        {
            DataContext context = Ktl::Move(this->_extensions[i]);

            FabricIOBuffer buffer(&(*context._data), context._length);

            status = extensionBuffers.Append(Ktl::Move(buffer));

            if (NT_ERROR(status))
            {
                return status;
            }

            // Release ownership of buffer
            context._data.Detach();
        }

        return STATUS_SUCCESS;
    }

private:

    struct DataContext
    {
    private:
        K_DENY_COPY(DataContext);

    public:
        DataContext()
            : _length(0)
        {
        }

        DataContext(DataContext && context)
        {
            this->_length = context._length;
            context._length = 0;


            this->_data = Ktl::Move(context._data);
            context._data = nullptr;
        }

        DataContext& operator=(DataContext && context)
        {
            this->_length = context._length;
            context._length = 0;

            this->_data = Ktl::Move(context._data);
            context._data = nullptr;

            return *this;
        }

        DataContext(ULONG length, KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> && data)
            : _length(length)
            , _data(Ktl::Move(data))
        {
        }

        ULONG _length;
        KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> _data;
    };

    KMemChannel _message;
    KArray<DataContext> _extensions;
};

NTSTATUS SerializeObjectAndGetDeserializeStream(IFabricSerializable * object, IFabricSerializableStreamUPtr & readStreamUPtr)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (object == nullptr)
    {
        KDbgPrintf("object pointer was null\n");
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_ERROR(status)) return status;

    IFabricSerializableStream * writeStreamPtr = nullptr;
    status = FabricSerializableStream::Create(&writeStreamPtr);

    IFabricSerializableStreamUPtr writeStreamUPtr(writeStreamPtr);

    if (NT_ERROR(status))
    {
        KDbgPrintf("Allocation of FabricSerializableStream failed\n");
        return status;
    }

    MockTransport transport;

    KDbgPrintf("Writing object\n");
    status = writeStreamUPtr->WriteSerializable(object);

    if (NT_ERROR(status))
    {
        KDbgPrintf("Writing object failed\n");
        return status;
    }

    KArray<FabricIOBuffer> sendingMessageBuffers(Common::GetSFDefaultPagedKAllocator());
    KArray<FabricIOBuffer> sendingExtensionBuffers(Common::GetSFDefaultPagedKAllocator());

    status = sendingMessageBuffers.Status();

    if (NT_ERROR(status))
    {
        KDbgPrintf("sendingMessageBuffers constructor failed\n");
        return status;
    }

    status = sendingExtensionBuffers.Status();

    if (NT_ERROR(status))
    {
        KDbgPrintf("sendingExtensionBuffers constructor failed\n");
        return status;
    }

    FabricIOBuffer buffer;
    bool isExtensionBuffer;

    status = writeStreamUPtr->GetNextBuffer(buffer, isExtensionBuffer);

    while (NT_SUCCESS(status))
    {
        KDbgPrintf("Buffer found: %p: %i\n", buffer.buffer, buffer.length);

        if (isExtensionBuffer)
        {
            status = sendingExtensionBuffers.Append(buffer);
        }
        else
        {
            status = sendingMessageBuffers.Append(buffer);
        }

        if (NT_ERROR(status))
        {
            KDbgPrintf("Appending a buffer failed: extension = %i\n", isExtensionBuffer);
            return status;
        }

        status = writeStreamUPtr->GetNextBuffer(buffer, isExtensionBuffer);
    }

    if (status != K_STATUS_NO_MORE_EXTENSION_BUFFERS)
    {
        KDbgPrintf("GetNextBuffer returned an error other than K_STATUS_NO_MORE_EXTENSION_BUFFERS: %X", status);
    }

    KDbgPrintf("Buffer summary: %i message buffers and %i extension buffers\n", sendingMessageBuffers.Count(), sendingExtensionBuffers.Count());

    status = transport.Send(sendingMessageBuffers, sendingExtensionBuffers);

    if (NT_ERROR(status))
    {
        KDbgPrintf("Mock transport failed send\n");
        return status;
    }

    // We have now simulated a successful send which means the sending buffers are no longer needed by the transport or serialization runtime, this call
    // will invoke any callbacks associated with cleaning up memory or post send work
    status = writeStreamUPtr->InvokeCallbacks();

    if (NT_ERROR(status))
    {
        KDbgPrintf("Invoking callbacks failed\n");
        return status;
    }

    KArray<FabricIOBuffer> receivingMessageBuffers(Common::GetSFDefaultPagedKAllocator());
    KArray<FabricIOBuffer> receivingExtensionBuffers(Common::GetSFDefaultPagedKAllocator());

    status = receivingMessageBuffers.Status();

    if (NT_ERROR(status))
    {
        KDbgPrintf("receivingMessageBuffers constructor failed\n");
        return status;
    }

    status = receivingExtensionBuffers.Status();

    if (NT_ERROR(status))
    {
        KDbgPrintf("receivingExtensionBuffers constructor failed\n");
        return status;
    }

    status = transport.Receive(receivingMessageBuffers, receivingExtensionBuffers);

    if (NT_ERROR(status))
    {
        KDbgPrintf("Mock transport failed receive\n");
        return status;
    }

    KDbgPrintf("Received buffer summary: %i message buffers and %i extension buffers\n", receivingMessageBuffers.Count(), receivingExtensionBuffers.Count());

    KMemChannel::UPtr memoryStream = KMemChannel::UPtr(_new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) KMemChannel(Common::GetSFDefaultPagedKAllocator()));

    if (!memoryStream)
    {
        KDbgPrintf("Allocation of KMemChannel failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = memoryStream->Status();

    if (status)
    {
        KDbgPrintf("KMemChannel constructor failed\n");
        return status;
    }

    for (ULONG i = 0; i < receivingMessageBuffers.Count(); ++i)
    {
        FabricIOBuffer rcvBuffer;
		rcvBuffer = receivingMessageBuffers[i];

        status = memoryStream->Write(rcvBuffer.length, rcvBuffer.buffer);

        if (NT_ERROR(status))
        {
            KDbgPrintf("Write failed\n");
            return status;
        }
    }

    status = memoryStream->SetCursor(0, KMemChannel::eFromBegin);

    if (NT_ERROR(status))
    {
        KDbgPrintf("SetCursor failed\n");
        return status;
    }

    IFabricStream * receiveByteStreamPtr = nullptr;
    status = FabricStream::Create(&receiveByteStreamPtr, Ktl::Move(memoryStream), Ktl::Move(receivingExtensionBuffers));

    if (NT_ERROR(status))
    {
        KDbgPrintf("Allocation of FabricStream failed\n");
        return status;
    }

    IFabricStreamUPtr receiveByteStream(receiveByteStreamPtr);
    IFabricSerializableStream * readStreamPtr = nullptr;

    status = FabricSerializableStream::Create(&readStreamPtr, Ktl::Move(receiveByteStream));

    if (NT_ERROR(status))
    {
        KDbgPrintf("Allocation of FabricSerializableStream failed\n");
        return status;
    }

    readStreamUPtr = IFabricSerializableStreamUPtr(readStreamPtr);

    return status;
}

template <class T, class TPtr>
NTSTATUS SimpleSerializationTest(T * object, FabricActivatorFunction activator,  TPtr ** outPtr)
{
    int iteration = 1;
    if (perfTestIter>1)
    {
        iteration = perfTestIter;        
    }
        
    LARGE_INTEGER currentTimestamp;
    QueryPerformanceCounter(&currentTimestamp);
    __int64 timeStart = currentTimestamp.QuadPart;         

    bool success = true;
    while(iteration-->0)
    {                
        IFabricSerializableStreamUPtr streamUPtr;

        NTSTATUS status = SerializeObjectAndGetDeserializeStream(object, streamUPtr);

        if (NT_ERROR(status))
        {
            return status;
        }

        success = true;

        if (activator == nullptr)
        {
            // If not a shared pointer
            __if_not_exists(T::Create)
            {
                T deserializedObject;

                __if_exists(T::Status)
                {
                    status = deserializedObject.Status();

                    if (status)
                    {
                        KDbgPrintf("Constructing a T failed\n");
                        return status;
                    }
                }

                status = streamUPtr->ReadSerializable(&deserializedObject);

                if (status)
                {
                    KDbgPrintf("Deserialize failed\n");
                    return status;
                }

                success = (*object == deserializedObject);
            }
            __if_exists(T::Create)
            {
                KDbgPrintf("Type has a Create method which this test does not expect\n");
                success = false;
            }
        }
        else
        {
            IFabricSerializable * deserializedPtr = nullptr;

            status = streamUPtr->ReadSerializableAsPointer(&deserializedPtr, activator);

            TPtr * casted = static_cast<TPtr*>(deserializedPtr);

            TPtr * originalCasted = static_cast<TPtr*>(object);

            __if_exists(T::Create)
            {
                // The activated object was a shared pointer, we must attach to it to gain ownership
                T::SPtr sptr;

                if (deserializedPtr != nullptr)
                {
                    sptr.Attach(casted);
                }
            }

            if (status)
            {
                KDbgPrintf("Deserialize failed\n");
                return status;
            }

            success = (*originalCasted == *casted);

            // cleanup created
            if (outPtr == nullptr)
            {
                __if_not_exists(T::Create)
                {
                    // Only delete if it wasn't a shared pointer, otherwise it is handled already
                    _delete(deserializedPtr);
                }
            }
            else
            {
                *outPtr = casted;
            }
        }
        if (!success)
        {
            KDbgPrintf("operator== returned false\n");
            return STATUS_CANCELLED;
        }
    }

    if (perfTestIter>1)
    {           
        QueryPerformanceCounter(&currentTimestamp);
        __int64 timeEnd = currentTimestamp.QuadPart;

        totalDuration += timeEnd - timeStart;
    }

    if (success)
    {
        KDbgPrintf("operator== returned true\n");
        return STATUS_SUCCESS;
    }
    else
    {
        KDbgPrintf("operator== returned false\n");
        return STATUS_CANCELLED;
    }
}

template <class TIn, class TOut>
NTSTATUS VersionedSerializationTest(TIn * object1, TOut & object2)
{
    int iteration = 1;    
    if (perfTestIter>1)
    {
        iteration = perfTestIter;    
    }

    LARGE_INTEGER currentTimestamp;
    QueryPerformanceCounter(&currentTimestamp);
    __int64 timeStart = currentTimestamp.QuadPart;

    NTSTATUS status = ERROR_SUCCESS;
    while(iteration-->0)
    {
        IFabricSerializableStreamUPtr streamUPtr;

        status = SerializeObjectAndGetDeserializeStream(object1, streamUPtr);

        if (NT_ERROR(status))
        {
            return status;
        }

        __if_exists(TOut::Status)
        {
            status = object2.Status();

            if (status)
            {
                KDbgPrintf("Constructing a TOut failed\n");
                return status;
            }
        }
        
        status = streamUPtr->ReadSerializable(&object2);
    }

    if (perfTestIter>1)
    {           
        QueryPerformanceCounter(&currentTimestamp);
        __int64 timeEnd = currentTimestamp.QuadPart;

        totalDuration += timeEnd - timeStart;
    }

    if (status)
    {
        KDbgPrintf("Deserialize failed\n");
        return status;
    }

    return status;
}

FunctionTable funcArr[] = {
	{ &BasicSerializationTest, "BasicSerializationTest" },
	{ &BasicSerializationWithChildTest, "BasicSerializationWithChildTest" },
	{ &BasicVersioningChildReadBaseTest, "BasicVersioningChildReadBaseTest" },
	{ &BasicVersioningChildTest, "BasicVersioningChildTest" },
	{ &BasicVersioningChild2Test, "BasicVersioningChild2Test" },
	{ &BasicNestedObjectTest, "BasicNestedObjectTest" },
	{ &BasicObjectWithPointersTest, "BasicObjectWithPointersTest" },
	{ &PolymorphicObjectTest, "PolymorphicObjectTest" },
	{ &PolymorphicObjectChildTest, "PolymorphicObjectChildTest" },
	{ &BasicObjectVersioningV1ToV2Test, "BasicObjectVersioningV1ToV2Test" },
	{ &BasicObjectVersioningV2ToV1ToV2Test, "BasicObjectVersioningV2ToV1ToV2Test" },
	{ &ObjectWithArraysVersioningV1ToV2Test, "ObjectWithArraysVersioningV1ToV2Test" },
	{ &ObjectWithArraysVersioningV2ToV1ToV2Test, "ObjectWithArraysVersioningV2ToV1ToV2Test" },
	{ &InheritedObjectV2ToV1ToV2Test, "InheritedObjectV2ToV1ToV2Test" },
	{ &BasicRvdObjectTest, "BasicRvdObjectTest" },
	{ &BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy, "BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy" }
};

FunctionTable randomFuncArr[] = {
	{ &TreeObjectTest, "TreeObjectTest" },
	{ &TreeObjectMixedVersionsTest, "TreeObjectMixedVersionsTest" }
};
class SerializationTests
{
protected:
	SerializationTests() { BOOST_REQUIRE(Setup()); }
	~SerializationTests() { BOOST_REQUIRE(Cleanup()); }

	bool Setup();
	bool Cleanup();
	int BasicTest(NTSTATUS status, bool RunAll);
	int RandomTest(NTSTATUS status, bool RunAll);
	int PerfTest(NTSTATUS status, bool RunAll);

};

bool SerializationTests::Setup()
{
	// TODO
	return true;
}

bool SerializationTests::Cleanup()
{
	// TODO
	return true;
}

int SerializationTests::BasicTest(NTSTATUS status, bool RunAll)
{
	perfTestIter = 1;

	for (int i = 0; i<sizeof(funcArr) / sizeof(FunctionTable); i++)
	{
		if (RunAll)
		{
			status = funcArr[i].funcPtr();
		}

		if (NT_ERROR(status))
		{
			KDbgPrintf("\n%s failed with error code %x", funcArr[i].funcName, status);
			return status;
		}
	}
	return status;
}

int SerializationTests::RandomTest(NTSTATUS status, bool RunAll)
{
	ULONG randomIterations = 500;
	for (ULONG i = 0; i < randomIterations; ++i)
	{
		for (int j = 0; j<sizeof(randomFuncArr) / sizeof(FunctionTable); j++)
		{
			if (RunAll)
			{
				TurnOffPrint
					status = randomFuncArr[j].funcPtr();
				TurnOnPrint
			}

			if (NT_ERROR(status))
			{
				KDbgPrintf("\n%s failed with error code %x", randomFuncArr[j].funcName, status);
				return status;
			}
		}
	}
	return status;
}

int SerializationTests::PerfTest(NTSTATUS status, bool RunAll)
{
	perfTestIter = 50000;
	for (int i = 0; i<sizeof(funcArr) / sizeof(FunctionTable); i++)
	{
		if (RunAll)
		{
			TurnOffPrint
				status = funcArr[i].funcPtr();
			TurnOnPrint
		}

		if (NT_ERROR(status))
		{
			KDbgPrintf("\nSerializtion performance test terminated because %s failed", funcArr[i].funcName);
			return status;
		}
	}
	KDbgPrintf("Serializtion performance test spent %I64d Milliseconds", totalDuration / TicksPerMillisecond);
	return status;
}

BOOST_FIXTURE_TEST_SUITE(SerializationTestsSuite, SerializationTests)

BOOST_AUTO_TEST_CASE(RullAllTests)
{
	VERIFY_IS_TRUE(NT_SUCCESS(BasicTest(STATUS_SUCCESS, true)));
	VERIFY_IS_TRUE(NT_SUCCESS(RandomTest(STATUS_SUCCESS, true)));
	KDbgPrintf("\nAll tests passed.\nDoing performance testing...");
	VERIFY_IS_TRUE(NT_SUCCESS(PerfTest(STATUS_SUCCESS, true)));
}

BOOST_AUTO_TEST_SUITE_END()

