// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <iostream>


#include <boost/test/unit_test.hpp>

#define VERIFY_IS_TRUE BOOST_REQUIRE
#define VERIFY_ARE_EQUAL(a,b) BOOST_REQUIRE((a) == (b))
#define VERIFY_FAIL BOOST_FAIL

using namespace std;
using namespace Common;

//namespace Common
//{
    class TestFabricSerializationHelper
    {
    protected:
        template <class T, class TestObjV2>
        void ForwardCompatibilityTestHelper(T value1, T value2, T value3);

        template <class T, class TestObjV2>
        void RoundtripCompatibilityTestHelper(T value1, T value2, T value3);
    };

    struct TestMapBody : public Serialization::FabricSerializable
    {
        map<wstring, wstring> map1;
        map<wstring, int> map2;

        FABRIC_FIELDS_02(map1, map2);
    };

    struct TestLargeOperation : public Serialization::FabricSerializable
    {
        wstring name_;
        Guid guid_;
        vector<byte> bytes_;
        ULONG64 checksum_;
        wstring id_;

        FABRIC_FIELDS_05(name_, guid_, bytes_, checksum_, id_);
    };

    DEFINE_USER_ARRAY_UTILITY(TestLargeOperation);

    struct TestLargeObjectContainer :  public Serialization::FabricSerializable
    {
        wstring name_;
        vector<TestLargeOperation> operations_;

        FABRIC_FIELDS_02(name_, operations_);
    };

    DEFINE_USER_MAP_UTILITY(std::wstring, int);

    namespace TestFlags
    {
        enum Enum
        {
            TestMin = 0x80000000,
            Test0 = -1,
            Test1 = 1,
            Test2 = 2,
            Test3 = 4,
            TestMax = 0x7FFFFFFF,
        };
    }

    namespace TestFlags2
    {
        enum Enum : ULONG
        {
            Test1 = 1,
            TestMax = 0xFFFFFFFF,
        };
    }

    // For use in a vector
    DEFINE_AS_BLITTABLE(TestFlags::Enum);

    struct TestEnumVector : public Serialization::FabricSerializable
    {
        vector<TestFlags::Enum> flags;
        TestFlags::Enum enum1;
        TestFlags::Enum enum2;
        TestFlags2::Enum enum3;

        FABRIC_FIELDS_04(flags, enum1, enum2, enum3);
    };

    struct WStringBody : public Serialization::FabricSerializable
    {
        wstring s;

        FABRIC_FIELDS_01(s);
    };

    struct TestUniquePtrBody : public Serialization::FabricSerializable
    {
        unique_ptr<WStringBody> s1;
        unique_ptr<WStringBody> s2;

        FABRIC_FIELDS_02(s1, s2);
    };

    template <class TKey, class TValue>
    bool MapsAreEqual(std::map<TKey, TValue> & map1, std::map<TKey, TValue> & map2)
    {
        if (map1.size() != map2.size())
        {
            return false;
        }

        for(pair<TKey, TValue> item : map1)
        {
            auto findIter = map2.find(item.first);

            if (findIter == map2.end())
            {
                return false;
            }

            if (item.second != findIter->second)
            {
                return false;
            }
        }

        return true;
    }

    template <class T>
    struct MockPointerObj : public Serialization::FabricSerializable
    {
        MockPointerObj() : Value() { }
        MockPointerObj(T init) : Value(init) { }
        FABRIC_FIELDS_01(Value);
        T Value;
    };

    template <class T>
    struct PointerObjV1 : public Serialization::FabricSerializable
    {
        PointerObjV1(T init) : Field1(new MockPointerObj<T>()) { }
        FABRIC_FIELDS_01(Field1);
        unique_ptr<MockPointerObj<T>> Field1;
    };

    template <class T>
    struct PointerObjV2 : public Serialization::FabricSerializable
    {
        PointerObjV2(T init1, T init2) : Field1(new MockPointerObj<T>(init1)), Field2(new MockPointerObj<T>(init2)) { }
        FABRIC_FIELDS_02(Field1, Field2);
        unique_ptr<MockPointerObj<T>> Field1;
        unique_ptr<MockPointerObj<T>> Field2;
    };

    TestLargeOperation CreateRandomLargeOperation(Common::Random & random)
    {
        size_t value = random.Next(0x20, 0x1000);

        TestLargeOperation operation;
        operation.bytes_.resize(value);
        operation.id_ = L"Test id for this test";
        operation.guid_ = Guid::NewGuid();
        operation.name_ = operation.guid_.ToString();

        for (size_t i = 0; i < value; ++i)
        {
            byte b = random.NextByte();
            operation.checksum_ += b;
        }

        return operation;
    }

    TestLargeObjectContainer CreateRandomLargeObjectContainer(Common::Random & random)
    {
        TestLargeObjectContainer container;

        container.name_ = L"This is the best container ever";

        size_t value = random.Next(0x20, 0x100);

        container.operations_.reserve(value);

        for (size_t i = 0; i < value; ++i)
        {
            container.operations_.push_back(CreateRandomLargeOperation(random));
        }

        return container;
    }

	    // *** Serialization compatibility test helpers for constructing fields of various types

    namespace Test
    {
        enum Enum
        {
            First = 11,
            Second = 22,
            Third = 33,
        };
    }

    template <class T>
    struct USER_TYPE
    {
        USER_TYPE() : Field() { }
        USER_TYPE(T init) : Field(init) { }
        bool operator == (USER_TYPE<T> const & other) const { return (this->Field == other.Field); }
        bool operator != (USER_TYPE<T> const & other) const { return !(*this == other); }
        T Field;
    };

    typedef USER_TYPE<uint64> USER_TYPE_UINT64;
    FABRIC_SERIALIZE_AS_BYTEARRAY(USER_TYPE_UINT64);

    DEFINE_USER_MAP_UTILITY(int, int);

    // V1 classes

    template <class T>
    struct TestObjV1 : public Serialization::FabricSerializable
    {
        TestObjV1() : Field1() { }
        TestObjV1(T init) : Field1(init) { }
        bool operator == (TestObjV1<T> const & other) const { return (this->Field1 == other.Field1); }
        FABRIC_FIELDS_01(Field1);
        T Field1;
    };

    DEFINE_USER_ARRAY_UTILITY(TestObjV1<int>);
    // V2 classes

    template <class T>
    struct TestObjV2Extend : public Serialization::FabricSerializable
    {
    public:
        TestObjV2Extend() : Field1(), Field2() { }
        TestObjV2Extend(T init1, T init2) : Field1(init1), Field2(init2) { }
        FABRIC_FIELDS_02(Field1, Field2);
        T Field1;
        T Field2;
    };

    template <class T>
    struct TestObjV2Derive : public TestObjV1<T>
    {
        TestObjV2Derive() : TestObjV1<T>(), Field2() { }
        TestObjV2Derive(T init1, T init2) : TestObjV1<T>(init1), Field2(init2) { }
        FABRIC_FIELDS_01(Field2);
        T Field2;
    };

    // V3 classes

    template <class T>
    struct TestObjV3Extend : public Serialization::FabricSerializable
    {
    public:
        TestObjV3Extend() : Field1(), Field2(), Field3() { }
        TestObjV3Extend(T init1, T init2, T init3) : Field1(init1), Field2(init2), Field3(init3) { }
        FABRIC_FIELDS_03(Field1, Field2, Field3);
        T Field1;
        T Field2;
        T Field3;
    };
    // Helper functions

    template <class T>
    vector<T> MakeVector(T value1, T value2)
    {
        vector<T> temp;
        temp.push_back(value1);
        temp.push_back(value2);
        return temp;
    }

    template <class K, class V>
    map<K, V> MakeMap(K key1, V value1, K key2, V value2)
    {
        map<K, V> temp;
        temp[key1] = value1;
        temp[key2] = value2;
        return temp;
    }

#define MakeMap(TYPE) MakeMap<TYPE, TYPE>

    typedef map<int, int> MapInt;
    typedef map<wstring, wstring> MapWString;

    template <class T, class TestObjV2>
    void SerializeFromTruncated(TestObjV1<T> const &,  __out TestObjV2 &);

    // Helper macros

#define VERIFY_AND_BREAK(expr) \
    if (!expr) \
    { \
        DebugBreak(); \
        VERIFY_IS_TRUE(expr); \
    } \

#define ForwardCompatibilityTest(TEST_OBJV2, TYPE, ...) \
    ForwardCompatibilityTestHelper<TYPE, TEST_OBJV2<TYPE>>(__VA_ARGS__);

#define RoundtripCompatibilityTest(TEST_OBJV2, TYPE, ...) \
    RoundtripCompatibilityTestHelper<TYPE, TEST_OBJV2<TYPE>>(__VA_ARGS__);

#define VAR_ARGS(...) __VA_ARGS__

#define VersionCompatibilityTest(TEST_TYPE, TEST_OBJV2, ...) \
    { \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, bool, true, false, false) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, CHAR, 'a', 'b', 'c') \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, UCHAR, 'a', 'b', 'c') \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, SHORT, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, USHORT, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, int, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, uint, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, LONG, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, ULONG, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, LONG64, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, ULONG64, 11, 22, 33) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, GUID, Guid::NewGuid().AsGUID(), Guid::NewGuid().AsGUID(), Guid::NewGuid().AsGUID()) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, DOUBLE, 11.1, 22.2, 33.3) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, Test::Enum, Test::Enum::First, Test::Enum::Second, Test::Enum::Third) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, wstring, L"first", L"second", L"third") \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, USER_TYPE_UINT64, USER_TYPE_UINT64(11), USER_TYPE_UINT64(22), USER_TYPE_UINT64(33)) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, vector<LONG>, \
            MakeVector<LONG>( VAR_ARGS(11, 12)), \
            MakeVector<LONG>( VAR_ARGS(22, 23)), \
            MakeVector<LONG>( VAR_ARGS(33, 34))) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, vector<byte>, \
            MakeVector<byte>( VAR_ARGS(11, 12)), \
            MakeVector<byte>( VAR_ARGS(22, 23)), \
            MakeVector<byte>( VAR_ARGS(33, 34))) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, vector<wstring>, \
            MakeVector<wstring>( VAR_ARGS(L"a", L"b")), \
            MakeVector<wstring>( VAR_ARGS(L"c", L"d")), \
            MakeVector<wstring>( VAR_ARGS(L"e", L"f"))) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, vector<TestObjV1<int>>, \
            MakeVector<TestObjV1<int>>( TestObjV1<int>(11), TestObjV1<int>(12)), \
            MakeVector<TestObjV1<int>>( TestObjV1<int>(22), TestObjV1<int>(23)), \
            MakeVector<TestObjV1<int>>( TestObjV1<int>(33), TestObjV1<int>(34))) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, MapInt, \
            MakeMap(int)( VAR_ARGS(11, 12, 13, 14)), \
            MakeMap(int)( VAR_ARGS(22, 23, 24, 25)), \
            MakeMap(int)( VAR_ARGS(33, 34, 35, 36))) \
        TEST_TYPE##CompatibilityTest( TEST_OBJV2, MapWString, \
            MakeMap(wstring)( VAR_ARGS(L"keyA", L"valA", L"keyB", L"valB")), \
            MakeMap(wstring)( VAR_ARGS(L"keyC", L"valC", L"keyD", L"valD")), \
            MakeMap(wstring)( VAR_ARGS(L"keyE", L"valE", L"keyF", L"valF"))) \
    } \

    template <class T, class TestObjV2>
    void TestFabricSerializationHelper::ForwardCompatibilityTestHelper(T value1, T value2, T value3)
    {
        TestObjV1<T> objV1(value1);
        TestObjV2 objV2(value2, value3);

        bool success = (objV1.Field1 == value1);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field1 == value2);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field2 == value3);
        VERIFY_AND_BREAK(success);

        SerializeFromTruncated(objV1, objV2);

        success = (objV1.Field1 == value1);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field1 == value1);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field2 == value3);
        VERIFY_AND_BREAK(success);
    }

    template <class T, class TestObjV2>
    void TestFabricSerializationHelper::RoundtripCompatibilityTestHelper(T value1, T value2, T value3)
    {
        TestObjV1<T> objV1(value1);
        TestObjV2 objV2(value2, value3);

        bool success = (objV1.Field1 == value1);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field1 == value2);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field2 == value3);
        VERIFY_AND_BREAK(success);

        vector<byte> buffer;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&objV2, buffer).IsSuccess());

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(objV1, buffer).IsSuccess());

        success = (objV1.Field1 == value2);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field1 == value2);
        VERIFY_AND_BREAK(success);

        success = (objV2.Field2 == value3);
        VERIFY_AND_BREAK(success);

        // Roundtrip from object that should be carrying around unknown data
        //
        TestObjV2 objV3;
        SerializeFromTruncated(objV1, objV3);

        success = (objV3.Field1 == objV2.Field1);
        VERIFY_AND_BREAK(success);

        success = (objV3.Field2 == objV2.Field2);
        VERIFY_AND_BREAK(success);
    }

    template <class T, typename TestObjV2>
    void SerializeFromTruncated(TestObjV1<T> const & objTruncated,  __out TestObjV2 & objResult)
    {
        vector<byte> buffer;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&objTruncated, buffer).IsSuccess());

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(objResult, buffer).IsSuccess());

        bool success = (objResult.Field1 == objTruncated.Field1);
        VERIFY_AND_BREAK(success);
    }

    BOOST_FIXTURE_TEST_SUITE(FabricSerializationHelperSuite, TestFabricSerializationHelper)

    //void TestFabricSerializationHelper::MapTest()
    BOOST_AUTO_TEST_CASE(MapTest)
    {
        map<wstring, wstring> myStrings;
        myStrings.insert(pair<wstring, wstring>(L"Hello", L"World"));
        myStrings.insert(pair<wstring, wstring>(L"Hello with a really long key", L"World"));
        myStrings.insert(pair<wstring, wstring>(L"Hello", L"World with a really long value"));
        myStrings.insert(pair<wstring, wstring>(L"Hello long key and long value", L"World long key and long value"));

        map<wstring, int> myStringAndInt;
        myStringAndInt.insert(pair<wstring, int>(L"one", 1));
        myStringAndInt.insert(pair<wstring, int>(L"two", 2));
        myStringAndInt.insert(pair<wstring, int>(L"three", 3));

        TestMapBody body1;
        body1.map1 = myStrings;
        body1.map2 = myStringAndInt;

        vector<byte> buffer;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&body1, buffer).IsSuccess());

        TestMapBody body2;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(body2, buffer).IsSuccess());

        VERIFY_IS_TRUE(MapsAreEqual(body2.map1, myStrings));
        VERIFY_IS_TRUE(MapsAreEqual(body2.map2, myStringAndInt));
    }

    //void TestFabricSerializationHelper::VectorTest()
    BOOST_AUTO_TEST_CASE(VectorTest)
    {
        vector<TestFlags::Enum> flags;
        flags.push_back(TestFlags::Test2);
        flags.push_back(TestFlags::Test1);
        flags.push_back(TestFlags::Test3);
        flags.push_back(TestFlags::Test2);
        flags.push_back(TestFlags::Test3);
        flags.push_back(TestFlags::Test3);
        flags.push_back(static_cast<TestFlags::Enum>(TestFlags::Test3 | TestFlags::Test1));

        TestEnumVector body1;
        body1.flags = flags;
        body1.enum1 = TestFlags::TestMin;
        body1.enum2 = TestFlags::TestMax;
        body1.enum3 = TestFlags2::TestMax;

        vector<byte> buffer;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&body1, buffer).IsSuccess());

        TestEnumVector body2;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(body2, buffer).IsSuccess());

        VERIFY_IS_TRUE(flags.size() == body2.flags.size());

        for (size_t i = 0; i < flags.size(); ++i)
        {
            VERIFY_IS_TRUE(flags[i] == body2.flags[i]);
        }

        VERIFY_IS_TRUE(TestFlags::TestMin == body2.enum1);
        VERIFY_IS_TRUE(TestFlags::TestMax == body2.enum2);
        VERIFY_IS_TRUE(TestFlags2::TestMax == body2.enum3);
    }

    //void TestFabricSerializationHelper::UniquePtrTest()
    BOOST_AUTO_TEST_CASE(UniquePtrTest)
    {
        WStringBody body;
        body.s = L"Hello, World!";

        TestUniquePtrBody body1;
        body1.s1 = nullptr;
        body1.s2 = make_unique<WStringBody>(body);

        vector<byte> buffer;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&body1, buffer).IsSuccess());

        TestUniquePtrBody body2;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(body2, buffer).IsSuccess());

        VERIFY_IS_TRUE(body1.s1 == nullptr);
        VERIFY_IS_TRUE(body2.s1 == nullptr);

        VERIFY_ARE_EQUAL(body1.s2->s, body2.s2->s);

        // Forward compatibility
        {
            PointerObjV1<wstring> objV1(L"a");
            PointerObjV2<wstring> objV2(L"b", L"c");

            buffer.clear();

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&objV1, buffer).IsSuccess());

            wstring expectedField2 = objV2.Field2->Value;

            VERIFY_IS_TRUE(FabricSerializer::Deserialize(objV2, buffer).IsSuccess());

            VERIFY_IS_TRUE(objV2.Field1->Value == objV1.Field1->Value);
            VERIFY_IS_TRUE(objV2.Field2->Value != objV1.Field1->Value);
            VERIFY_IS_TRUE(objV2.Field2->Value == expectedField2);
        }

        // Roundtrip compatibility
        {
            PointerObjV1<wstring> objV1(L"a");
            PointerObjV2<wstring> objV2(L"b", L"c");
            PointerObjV2<wstring> objV3(L"d", L"e");

            buffer.clear();

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&objV2, buffer).IsSuccess());

            VERIFY_IS_TRUE(FabricSerializer::Deserialize(objV1, buffer).IsSuccess());

            VERIFY_IS_TRUE(objV1.Field1->Value == objV2.Field1->Value);

            buffer.clear();

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&objV1, buffer).IsSuccess());
            
            VERIFY_IS_TRUE(FabricSerializer::Deserialize(objV3, buffer).IsSuccess());

            VERIFY_IS_TRUE(objV3.Field1->Value == objV2.Field1->Value);
            VERIFY_IS_TRUE(objV3.Field2->Value == objV2.Field2->Value);
        }
    }

    //void TestFabricSerializationHelper::LargeMessageTest()
    BOOST_AUTO_TEST_CASE(LargeMessageTest)
    {
        for (ULONG k = 0; k < 1000; ++k)
        {
            Random random;

            TestLargeObjectContainer container1 = CreateRandomLargeObjectContainer(random);

            vector<byte> buffer;
            //cout << "Serializing " << container1.operations_.size() << " operations..." << endl;
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&container1, buffer).IsSuccess());

            TestLargeObjectContainer container2;

            VERIFY_IS_TRUE(FabricSerializer::Deserialize(container2, buffer).IsSuccess());

            VERIFY_IS_TRUE(container1.name_ == container2.name_);
            VERIFY_IS_TRUE(container1.operations_.size() == container2.operations_.size());

            for (size_t i = 0; i < container1.operations_.size(); ++i)
            {
                bool check1 = container1.operations_[i].guid_ ==       container2.operations_[i].guid_;
                if(!check1) {VERIFY_FAIL(L"check1 failed");}
                bool check2 = container1.operations_[i].checksum_ ==   container2.operations_[i].checksum_;
                if(!check2) {VERIFY_FAIL(L"check2 failed");}
                bool check3 = container1.operations_[i].id_ ==         container2.operations_[i].id_;
                if(!check3) {VERIFY_FAIL(L"check3 failed");}
                bool check4 = container1.operations_[i].name_ ==       container2.operations_[i].name_;
                if(!check4) {VERIFY_FAIL(L"check4 failed");}
                bool check5 = container1.operations_[i].bytes_ ==       container2.operations_[i].bytes_;
                if(!check5) {VERIFY_FAIL(L"check5 failed");}
            }
        }

    }

    //void TestFabricSerializationHelper::VersionCompatibilityTest1()
    BOOST_AUTO_TEST_CASE(VersionCompatibilityTest1)
    {
        VersionCompatibilityTest( Forward, TestObjV2Extend ) 
    }

    //void TestFabricSerializationHelper::VersionCompatibilityTest2()
    BOOST_AUTO_TEST_CASE(VersionCompatibilityTest2)
    {
        VersionCompatibilityTest( Roundtrip, TestObjV2Extend ) 
    }

    //void TestFabricSerializationHelper::VersionCompatibilityTest3()
    BOOST_AUTO_TEST_CASE(VersionCompatibilityTest3)
    {
        VersionCompatibilityTest( Forward, TestObjV2Derive ) 
    }

    //void TestFabricSerializationHelper::VersionCompatibilityTest4()
    BOOST_AUTO_TEST_CASE(VersionCompatibilityTest4)
    {
        VersionCompatibilityTest( Roundtrip, TestObjV2Derive ) 
    }

    //void TestFabricSerializationHelper::VersionCompatibilityTest5()
    BOOST_AUTO_TEST_CASE(VersionCompatibilityTest5)
    {
        int value1 = 11;
        int value2 = 22;
        int value3 = 33;
        int value4 = 44;

        // Cover reading more than one extra field beyond EndScope marker
        //
        {
            TestObjV1<int> objV1(value1);
            TestObjV3Extend<int> objV3(value2, value3, value4);

            SerializeFromTruncated(objV1, objV3);

            bool success = (objV3.Field2 != objV1.Field1);
            VERIFY_AND_BREAK(success);

            success = (objV3.Field2 == value3);
            VERIFY_AND_BREAK(success);

            success = (objV3.Field3 != objV1.Field1);
            VERIFY_AND_BREAK(success);

            success = (objV3.Field3 == value4);
            VERIFY_AND_BREAK(success);
        }

        // Roundtrip more than one extra field as well
        //
        {
            TestObjV1<int> objV1(value1);
            TestObjV3Extend<int> objV3(value2, value3, value4);

            vector<byte> buffer;
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&objV3, buffer).IsSuccess());

            VERIFY_IS_TRUE(FabricSerializer::Deserialize(objV1, buffer).IsSuccess());

            TestObjV3Extend<int> objV4;

            SerializeFromTruncated(objV1, objV4);

            bool success = (objV4.Field1 == objV3.Field1);
            VERIFY_AND_BREAK(success);
            
            success = (objV4.Field2 == objV3.Field2);
            VERIFY_AND_BREAK(success);

            success = (objV4.Field3 == objV3.Field3);
            VERIFY_AND_BREAK(success);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
//}
