// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ServiceModel/ServiceModel.h"

using namespace Common;
using namespace ServiceModel;
using namespace Naming;
using namespace std;
using namespace Management::HealthManager;

namespace Common
{
    namespace ObjectKind
    {
        enum Enum
        {
            OBJECT_KIND_1 = 1,
            OBJECT_KIND_2 = 2,
            OBJECT_KIND_3 = 3,
            OBJECT_KIND_INVALID = 4
        };

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(OBJECT_KIND_1)
            ADD_ENUM_VALUE(OBJECT_KIND_2)
            ADD_ENUM_VALUE(OBJECT_KIND_3)
            ADD_ENUM_VALUE(OBJECT_KIND_INVALID)
        END_DECLARE_ENUM_SERIALIZER()

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case OBJECT_KIND_1:
                w << L"OBJECT_KIND_1";
                return;
            case OBJECT_KIND_2:
                w << L"OBJECT_KIND_2";
                return;
            case OBJECT_KIND_3:
                w << L"OBJECT_KIND_3";
                return;
            case OBJECT_KIND_INVALID:
                w << L"OBJECT_KIND_INVALID";
                return;
            default:
                Common::Assert::CodingError("Unknown ObjectKind value {0}", static_cast<uint>(val));
            }
        }
    }

    class TempObject;
    typedef shared_ptr<TempObject> TempObjectSPtr;
    class TempObject1;
    typedef shared_ptr<TempObject1> TempObject1SPtr;
    class ObjectBase;
    typedef shared_ptr<ObjectBase> ObjectBaseSPtr;

    class ObjectBase : public IFabricJsonSerializable
    {
    public:
        ObjectBase()
            : kind_(ObjectKind::OBJECT_KIND_INVALID)
        {
            StringUtility::GenerateRandomString(10, id_);
        }

        explicit ObjectBase(ObjectKind::Enum kind)
            : kind_(kind)
        {
            StringUtility::GenerateRandomString(10, id_);
        }

        ObjectBase(ObjectKind::Enum kind, wstring const &id)
            : kind_(kind)
            , id_(id)
        {
        }

        static ObjectBaseSPtr CreateSPtr(ObjectKind::Enum kind);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(L"Kind", kind_)
            SERIALIZABLE_PROPERTY(L"ObjectId", id_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(ObjectBase, ObjectKind::Enum, kind_, CreateSPtr)

        ObjectKind::Enum kind_;
        std::wstring id_;
    };

    class TempObject : public ObjectBase
    {
    public:
        TempObject()
            : ObjectBase(ObjectKind::OBJECT_KIND_1)
        {}

        TempObject(std::wstring name, int val)
            : ObjectBase(ObjectKind::OBJECT_KIND_1)
            , name_(name)
            , val_(val)
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(L"ObjectName", name_)
            SERIALIZABLE_PROPERTY(L"ObjectValue", (LONG&)val_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (TempObject const& other) const
        {
            if ((val_ == other.val_)
                && (name_ == other.name_)
                && (id_ == other.id_))
            {
                return true;
            }
            return false;
        }

        std::wstring name_;
        int val_;
    };

    // field val_ is of type wstring instead of int
    class SimilarToTempObject : public ObjectBase
    {
    public:
        SimilarToTempObject()
            : ObjectBase(ObjectKind::OBJECT_KIND_1)
        {}

        SimilarToTempObject(std::wstring name, std::wstring val)
            : ObjectBase(ObjectKind::OBJECT_KIND_1)
            , name_(name)
            , val_(val)
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(L"ObjectName", name_)
            SERIALIZABLE_PROPERTY(L"ObjectValue", val_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            bool operator == (SimilarToTempObject const& other) const
        {
                if ((val_ == other.val_)
                    && (name_ == other.name_)
                    && (id_ == other.id_))
                {
                    return true;
                }
                return false;
            }

        std::wstring name_;

        // field val_ is of type wstring instead of int
        std::wstring val_;
    };

    class TempObject1 : public ObjectBase
    {
    public:
        TempObject1()
            : ObjectBase(ObjectKind::OBJECT_KIND_2)
        {}

        TempObject1(bool state, int val)
            : ObjectBase(ObjectKind::OBJECT_KIND_2)
            , state_(state)
            , val_(val)
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(L"ObjectState", state_)
            SERIALIZABLE_PROPERTY(L"ObjectValue", (LONG&)val_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (TempObject1 const& other) const
        {
            if ((val_ == other.val_)
                && (state_ == other.state_)
                && (id_ == other.id_))
            {
                return true;
            }
            return false;
        }

        bool state_;
        int val_;
    };

    class TempObjectWithArray : public ObjectBase
    {
    public:
        TempObjectWithArray()
            : ObjectBase(ObjectKind::OBJECT_KIND_3)
        {}

        TempObjectWithArray(wstring id, vector<int> &&array)
            : ObjectBase(ObjectKind::OBJECT_KIND_3, id)
            , array_(move(array))
        {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(L"Array", array_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (TempObjectWithArray const& other) const
        {
            if (other.id_ != id_)
            {
                return false;
            }

            if (other.array_.size() != array_.size())
            {
                return false;
            }

            for (int i = 0; i < array_.size(); ++i)
            {
                if (array_[i] != other.array_[i])
                {
                    return false;
                }
            }

            return true;
        }

        vector<int> array_;
    };

    template<ObjectKind::Enum kind>
    class TypeActivator
    {
    public:
        static ObjectBaseSPtr CreateSPtr()
        {
            return make_shared<ObjectBase>(ObjectKind::OBJECT_KIND_INVALID);
        }
    };

    template<> class TypeActivator<ObjectKind::OBJECT_KIND_1>
    {
    public:
        static ObjectBaseSPtr CreateSPtr()
        {
            return make_shared<TempObject>();
        }
    };

    template<> class TypeActivator<ObjectKind::OBJECT_KIND_2>
    {
    public:
        static ObjectBaseSPtr CreateSPtr()
        {
            return make_shared<TempObject1>();
        }
    };

    template<> class TypeActivator<ObjectKind::OBJECT_KIND_3>
    {
    public:
        static ObjectBaseSPtr CreateSPtr()
        {
            return make_shared<TempObjectWithArray>();
        }
    };

    ObjectBaseSPtr ObjectBase::CreateSPtr(ObjectKind::Enum kind)
    {
        switch(kind)
        {
        case ObjectKind::OBJECT_KIND_1 : { return TypeActivator<ObjectKind::OBJECT_KIND_1>::CreateSPtr(); }
        case ObjectKind::OBJECT_KIND_2 : { return TypeActivator<ObjectKind::OBJECT_KIND_2>::CreateSPtr(); }
        case ObjectKind::OBJECT_KIND_3 : { return TypeActivator<ObjectKind::OBJECT_KIND_3>::CreateSPtr(); }
        default: return make_shared<ObjectBase>();
        }
    }

    class NestedObjectArray : public IFabricJsonSerializable
    {
    public:
        NestedObjectArray()
            : NestedObjectArray(0)
        {
        }

        NestedObjectArray(int elements)
        {
            for (int i = 0; i < elements; i++)
            {
                std::wstring str;
                Common::StringUtility::GenerateRandomString(10, str);
                TempObject obj(str, i);
                nestedArray_.push_back(obj);
                nestedArray1_.push_back(obj);
            }
            obj_.name_ = L"objectname";
            obj_.val_ = elements;

            delimiter_ = 12345;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Array", nestedArray_)
            SERIALIZABLE_PROPERTY(L"Delimiter", delimiter_)
            SERIALIZABLE_PROPERTY(L"Array1", nestedArray1_)
            SERIALIZABLE_PROPERTY(L"TempObject", obj_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (NestedObjectArray const& other) const
        {
            if (nestedArray_.size() != other.nestedArray_.size())
            {
                return false;
            }

            for (size_t i = 0; i < nestedArray_.size(); i++)
            {
                if (!(nestedArray_[i] == other.nestedArray_[i])) { return false; }
            }

            return (obj_ == other.obj_);
        }

        std::vector<TempObject> nestedArray_;
        LONG delimiter_;
        TempObject obj_;
        std::vector<TempObject> nestedArray1_;
    };

    class ObjectSimilarToNestedObjectArray : public IFabricJsonSerializable
    {
    public:
        ObjectSimilarToNestedObjectArray(int elements)
        {
            for (int i = 0; i < elements; i++)
            {
                std::wstring temp_str;
                Common::StringUtility::GenerateRandomString(10, temp_str);
                TempObject obj(temp_str, i);
                nestedArray_.push_back(obj);
            }
            delimiter_ = 12345;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Array", nestedArray_)
            SERIALIZABLE_PROPERTY(L"Delimiter", delimiter_)
            SERIALIZABLE_PROPERTY(L"Array1", str)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (NestedObjectArray const& other) const
        {
            if (nestedArray_.size() != other.nestedArray_.size())
            {
                return false;
            }

            for (size_t i = 0; i < nestedArray_.size(); i++)
            {
                if (!(nestedArray_[i] == other.nestedArray_[i])) { return false; }
            }
            return true;
        }

        std::vector<TempObject> nestedArray_;
        LONG delimiter_;
        std::wstring str;
    };

    class SimpleArray : public IFabricJsonSerializable
    {
    public:
        SimpleArray(int elements)
        {
            for (int i = 0; i < elements; i++)
            {
                std::wstring str;
                Common::StringUtility::GenerateRandomString(10, str);
                stringArray_.push_back(str);
                intArray_.push_back(i);
            }
            delimiter_ = 12345;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Array", stringArray_)
            SERIALIZABLE_PROPERTY(L"Delimiter", delimiter_)
            SERIALIZABLE_PROPERTY(L"Array1", intArray_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (SimpleArray const& other) const
        {
            if (stringArray_.size() != other.stringArray_.size() ||
                intArray_.size() != other.intArray_.size())
            {
                return false;
            }

            for (size_t i = 0; i < stringArray_.size(); i++)
            {
                if (!(stringArray_[i] == other.stringArray_[i])) { return false; }
            }

            for (size_t i = 0; i < intArray_.size(); i++)
            {
                if (!(intArray_[i] == other.intArray_[i])) { return false; }
            }
            return true;
        }

        std::vector<wstring> stringArray_;
        LONG delimiter_;
        std::vector<LONG> intArray_;;
    };

    class ObjectA : public IFabricJsonSerializable
    {
    public:
        ObjectA(int elements)
        {
            i_ = elements;
            b_ = true;
            s_ = L"String";

            for (int i = 0; i < elements; i++)
            {
                intArray_.push_back(i);
            }
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"i", i_)
            SERIALIZABLE_PROPERTY(L"b", b_)
            SERIALIZABLE_PROPERTY(L"s", s_)
            SERIALIZABLE_PROPERTY(L"array", intArray_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        LONG i_;
        bool b_;
        std::wstring s_;
        std::vector<LONG> intArray_;;
    };

    class OutOfOrderObjectA : public IFabricJsonSerializable
    {
    public:
        OutOfOrderObjectA(int elements)
        {
            i_ = elements;
            b_ = true;
            s_ = L"String";

            for (int i = 0; i < elements; i++)
            {
                intArray_.push_back(i);
            }
        }

        bool operator == (ObjectA const& objA) const
        {
            if (intArray_.size() != objA.intArray_.size())
            {
                return false;
            }

            for (int i = 0; i < intArray_.size(); i++)
            {
                if (intArray_[i] != objA.intArray_[i]) { return false; }
            }

            if ((s_ != objA.s_) ||
                (b_ != objA.b_) ||
                (i_ != objA.i_))
            {
                return false;
            }

            return true;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"s", s_)
            SERIALIZABLE_PROPERTY(L"i", i_)
            SERIALIZABLE_PROPERTY(L"b", b_)
            SERIALIZABLE_PROPERTY(L"array", intArray_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        LONG i_;
        bool b_;
        std::wstring s_;
        std::vector<LONG> intArray_;;
    };

    class NestedObject : public IFabricJsonSerializable
    {
    public:
        NestedObject()
        :objArray1_(0)
        {};

        NestedObject(wstring name, int val)
            : obj1_(name, val)
            , obj2_(name + L"1", val+1)
            , obj3_(name + L"2", val+2)
            , objArray1_(val)
        {
        }

        bool operator == (NestedObject const& other) const
        {
            return ((obj1_ == other.obj1_) &&
                    (obj2_ == other.obj2_) &&
                    (obj3_ == other.obj3_) &&
                    (objArray1_ == other.objArray1_) &&
                    (i_ == other.i_));
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Obj1", obj1_)
            SERIALIZABLE_PROPERTY(L"ObjArray1", objArray1_)
            SERIALIZABLE_PROPERTY(L"Obj2", obj2_)
            SERIALIZABLE_PROPERTY(L"Int", i_)
            SERIALIZABLE_PROPERTY(L"Obj3", obj3_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        TempObject obj1_;
        NestedObjectArray objArray1_;
        LONG i_;
        TempObject obj2_;
        TempObject obj3_;
    };

    class ObjectWithMap : public IFabricJsonSerializable
    {
    public:
        ObjectWithMap(int count)
        {
            i_ = count;
            for (int i = 0; i < count; i++)
            {
                std::wstring str;
                Common::StringUtility::GenerateRandomString(10, str);
                TempObject obj(str, i);
                NestedObject nestedObject(str, i);
                map1_.insert(make_pair(str, obj));
                map2_.insert(make_pair(i, nestedObject));
            }
        }

        bool operator == (ObjectWithMap const& obj) const
        {

            if ((i_ != obj.i_) ||
                (map1_.size() != obj.map1_.size()) ||
                (map2_.size() != obj.map2_.size()))
            {
                return false;
            }

            auto item1 = obj.map1_.begin();
            for (auto item = map1_.begin(); item != map1_.end(); ++item, ++item1)
            {
                if ((item->first != item1->first) || !(item->second == item1->second))
                {
                    return false;
                }
            }

            auto map2item1 = obj.map2_.begin();
            for (auto map2item = map2_.begin(); map2item != map2_.end(); ++map2item, ++map2item1)
            {
                if (!(map2item->first == map2item1->first) || !(map2item->second == map2item1->second))
                {
                    return false;
                }
            }

            return true;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"i", i_)
            SERIALIZABLE_PROPERTY(L"Map", map1_)
            SERIALIZABLE_PROPERTY(L"Map2", map2_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        LONG i_;
        map<wstring, TempObject> map1_;
        map<LONG, NestedObject> map2_;
    };


    class ObjectWithoutSPtr : public IFabricJsonSerializable
    {
    public:
        ObjectWithoutSPtr() {}

        ObjectWithoutSPtr(wstring name)
        {
            name_ = name;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ObjectName", name_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring name_;
    };

    class SimpleMapTestObject : public IFabricJsonSerializable
    {
        public:
            SimpleMapTestObject() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"StartMarkerObject", startMarkerObject_)
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(L"TestMap", map1_)
            SERIALIZABLE_PROPERTY(L"EndMarkerObject", endMarkerObject_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        ObjectWithoutSPtr startMarkerObject_;
        map<wstring, wstring> map1_;
        ObjectWithoutSPtr endMarkerObject_;
    };

    class SimpleMapTestObject2 : public IFabricJsonSerializable
    {
    public:
        SimpleMapTestObject2() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(L"TestMap", map1_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        map<wstring, vector<wstring>> map1_;
    };

    class WStringMapTestObject : public IFabricJsonSerializable
    {
        public:
            WStringMapTestObject() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(L"WStringMap", map1_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        map<wstring, wstring> map1_;
    };

    class ObjectWithConstProperty : public IFabricJsonSerializable
    {
    public:

        ObjectWithConstProperty(bool x) 
        {
            this->conditionalField = x;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZE_CONSTANT_IF(L"ConditionalConstantProperty", L"TheValueIsTrue", conditionalField)
            SERIALIZE_CONSTANT_IF(L"ConditionalConstantProperty", L"TheValueIsFalse", !conditionalField)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool conditionalField;
    };

    class ObjectWithSPtr : public IFabricJsonSerializable
    {
    public:
        ObjectWithSPtr() {}

        ObjectWithSPtr(wstring name)
        {
            name_ = name;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ObjectName", name_)
            SERIALIZABLE_PROPERTY(L"SPtr", tempObjSPtr_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring name_;
        std::shared_ptr<TempObject> tempObjSPtr_;
    };

    class ObjectWithSPtrArray : public Common::IFabricJsonSerializable
    {
    public:
        ObjectWithSPtrArray() {}
        ObjectWithSPtrArray(wstring name)
        {
            name_ = name;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ObjectName", name_)
            SERIALIZABLE_PROPERTY(L"SPtrArray", objectSPtrArray_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        wstring name_;
        vector<shared_ptr<ObjectBase>> objectSPtrArray_;
    };

    class TestObjectV1 : public Common::IFabricJsonSerializable
    {
        public:
            TestObjectV1() {}
            TestObjectV1(int i, int j)
            {
                i_ = i;
                j_ = j;
            }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"i", i_)
            SERIALIZABLE_PROPERTY(L"j", j_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        int i_;
        int j_;
    };

    class TestObjectV2 : public Common::IFabricJsonSerializable
    {
        public:
            TestObjectV2() {}
            TestObjectV2(int i, int j, wstring foo)
            {
                i_ = i;
                j_ = j;
                foo_ = foo;
                nestedObjectArray_.push_back(std::make_shared<NestedObjectArray>(1));
            }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"i", i_)
            SERIALIZABLE_PROPERTY(L"NestedObjectArray", nestedObjectArray_)
            SERIALIZABLE_PROPERTY(L"j", j_)
            SERIALIZABLE_PROPERTY(L"Foo", foo_)
            SERIALIZABLE_PROPERTY(L"Map", map_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::vector<std::shared_ptr<NestedObjectArray>> nestedObjectArray_;
        int i_;
        int j_;
        wstring foo_;
        map<int, wstring> map_;
    };

    class TestObjectInt64AsNum : public Common::IFabricJsonSerializable
    {
    public:
        TestObjectInt64AsNum() {}
        TestObjectInt64AsNum(unsigned __int64 v1, __int64 v2)
        {
            uint64_ = v1;
            int64_ = v2;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(L"UInt64", uint64_, true)
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(L"Int64", int64_, true)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (TestObjectInt64AsNum const& other) const
        {
            return int64_ == other.int64_ && uint64_ == other.uint64_;
        }

        unsigned __int64 uint64_;
        __int64 int64_;
    };
    
    class ObjectWithNums : public Common::IFabricJsonSerializable
    {
    public:
        ObjectWithNums() {}
        ObjectWithNums(byte i, int j, unsigned int k, LONG l, ULONG m, __int64 n, unsigned __int64 o)
        {
            byte_ = i;
            int_ = j;
            uint_ = k;
            long_ = l;
            ulong_ = m;
            int64_ = n;
            uint64_ = o;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Byte", byte_)
            SERIALIZABLE_PROPERTY(L"Int", int_)
            SERIALIZABLE_PROPERTY(L"UInt", uint_)
            SERIALIZABLE_PROPERTY(L"LONG", long_)
            SERIALIZABLE_PROPERTY(L"ULONG", ulong_)
            SERIALIZABLE_PROPERTY(L"Int64", int64_)
            SERIALIZABLE_PROPERTY(L"UInt64", uint64_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        bool operator == (ObjectWithNums const& other) const
        {
            return byte_ == other.byte_ &&
                int_ == other.int_ &&
                uint_ == other.uint_ &&
                long_ == other.long_ &&
                ulong_ == other.ulong_ &&
                int64_ == other.int64_ &&
                uint64_ == other.uint64_;
        }

        byte byte_;
        int int_;
        unsigned int uint_;
        LONG long_;
        ULONG ulong_;
        __int64 int64_;
        unsigned __int64 uint64_;
    };


    class JSonSerializationTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(JSonSerializationTestSuite,JSonSerializationTest)

    BOOST_AUTO_TEST_CASE(SimpleMapWithVectorTest)
    {
        SimpleMapTestObject2 test;
        test.map1_[L"random"].push_back(L"val1");
        test.map1_[L"random"].push_back(L"val2");
        test.map1_[L"random2"].push_back(L"val1");
        wstring jsonString;

        auto error = JsonHelper::Serialize(test, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        SimpleMapTestObject2 test2;
        error = JsonHelper::Deserialize(test2, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(test.map1_ == test2.map1_);
    }

    BOOST_AUTO_TEST_CASE(JsonTestNull)
    {
        TestObjectV2 test;
        test.j_ = 2;
        wstring jsonInput = L"{\"NestedObjectArray\":null,\"i\":3,\"j\":null,\"Foo\":null,\"random\":null}";

        auto error = JsonHelper::Deserialize(test, jsonInput);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(test.foo_ == L"");
        VERIFY_IS_TRUE(test.nestedObjectArray_.size() == 0);
        VERIFY_IS_TRUE(test.i_ == 3);
        VERIFY_IS_TRUE(test.j_ == 2);

        ObjectWithSPtr test2;
        jsonInput = L"{\"ObjectName\":\"name\", \"SPtr\":{\"Kind\":null, \"ObjectName\":null, \"ObjectValue\":\"value\"}}";

        error = JsonHelper::Deserialize(test2, jsonInput);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(test2.name_ == L"name");
        VERIFY_IS_TRUE(test2.tempObjSPtr_->name_ == L"");
        VERIFY_IS_TRUE(test2.tempObjSPtr_->val_ == 0);
        VERIFY_IS_TRUE(test2.tempObjSPtr_->kind_ == ObjectKind::OBJECT_KIND_1);
    }

    BOOST_AUTO_TEST_CASE(PSDTest)
    {
        Reliability::ServiceCorrelationDescription correlation(L"temp", FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY);
        vector<Reliability::ServiceCorrelationDescription> correlations;

        correlations.push_back(correlation);
        correlations.push_back(correlation);

        Reliability::ServiceLoadMetricDescription slmd(L"three", FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, 2, 3);
        vector<Reliability::ServiceLoadMetricDescription> slmVector;
        slmVector.push_back(slmd);
        slmVector.push_back(slmd);
        wstring appName = L"appName";
        wstring serviceName = L"serviceName";
        wstring typeName = L"serviceTypeName";
        vector<wstring> partitionNames;
        partitionNames.push_back(L"four");
        partitionNames.push_back(L"five");
        ByteBuffer data;
        data.push_back(0x10);
        data.push_back(0xff);

        auto averageLoadPolicy = make_shared<Reliability::AveragePartitionLoadScalingTrigger>(L"servicefabric:/_MemoryInMB", 0.5, 1.2, 50);
        auto instanceMechanism = make_shared<Reliability::InstanceCountScalingMechanism>(2, 5, 1);

        Reliability::ServiceScalingPolicyDescription scalingPolicy(averageLoadPolicy, instanceMechanism);
        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
        scalingPolicies.push_back(scalingPolicy);

        PartitionedServiceDescWrapper psdWrapper(
            FABRIC_SERVICE_KIND_STATEFUL,
            appName,
            serviceName,
            typeName,
            data,
            PartitionSchemeDescription::Named,
            2,
            3,
            4,
            partitionNames,
            5,
            6,
            7,
            true,
            L"constraints",
            correlations,
            slmVector,
            vector<ServiceModel::ServicePlacementPolicyDescription>(),
            static_cast<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS>(FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION | FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION | FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION),
            8,
            9,
            10,
            FABRIC_MOVE_COST_MEDIUM,
            ServicePackageActivationMode::SharedProcess,
            L"",
            scalingPolicies);

        PartitionedServiceDescriptor psd;
        PartitionedServiceDescriptor psd1;

        wstring serializedString;
        auto error = JsonHelper::Serialize(psdWrapper, serializedString);
        VERIFY_IS_TRUE(error.IsSuccess());

        PartitionedServiceDescWrapper psdWrapper1;
        error = JsonHelper::Deserialize(psdWrapper1, serializedString);
        VERIFY_IS_TRUE(error.IsSuccess());
        psd.FromWrapper(psdWrapper);
        psd1.FromWrapper(psdWrapper1);

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(psd.ToString(), psd1.ToString()));
        VERIFY_IS_TRUE(psd.Service.ScalingPolicies == psd1.Service.ScalingPolicies);
    }

    BOOST_AUTO_TEST_CASE(OutofOrderFieldsTest)
    {
        ObjectA objA(3);
        OutOfOrderObjectA oobjA(0);

        //
        // Verify successful Serialization.
        //
        wstring data;
        auto error = JsonHelper::Serialize(objA, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(oobjA, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(oobjA == objA);
    }

    BOOST_AUTO_TEST_CASE(NestedObjectTest)
    {
        NestedObject obj1(L"NestedObject", 1);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(obj1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Verify successful DeSerialization.
        //
        NestedObject obj2;
        error = JsonHelper::Deserialize(obj2, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(obj1 == obj2);
    }

    BOOST_AUTO_TEST_CASE(MapTest)
    {
        ObjectWithMap obj1(2);

        //
        // Verify successful Serialization.
        //
        wstring data;
        auto error = JsonHelper::Serialize(obj1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ObjectWithMap obj2(0);

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(obj2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(obj1 == obj2);
    }

    BOOST_AUTO_TEST_CASE(NestedObjectArraySerializationTest)
    {
        NestedObjectArray objArray(2);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(objArray, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Verify successful DeSerialization.
        //
        NestedObjectArray objArray1(0);
        error = JsonHelper::Deserialize(objArray1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(objArray == objArray1);
    }

    BOOST_AUTO_TEST_CASE(NestedObjectArraySerializationFailureTest)
    {
        NestedObjectArray objArray(1);

        ComPointer<JsonWriter> jsonWriter = make_com<JsonWriter>();
        shared_ptr<ByteBuffer> bufferSPtr = make_shared<ByteBuffer>();

        JsonWriterVisitor writerVisitor(jsonWriter.GetRawPointer());

        //
        // Verify successful Serialization.
        //
        VERIFY_IS_TRUE(!FAILED(objArray.VisitSerializable(&writerVisitor, nullptr)));
        jsonWriter->GetBytes(*bufferSPtr.get());

        //
        // Trash the array and object end.
        //
        bufferSPtr->data()[bufferSPtr->size() - 1] = (BYTE) 0;
        bufferSPtr->data()[bufferSPtr->size() - 2] = (BYTE) 0;

        JsonReader::InitParams initParams((char*)(bufferSPtr->data()), (ULONG)bufferSPtr->size());
        ComPointer<JsonReader> jsonReader = make_com<JsonReader>(initParams);
        JsonReaderVisitor readerVisitor(jsonReader.GetRawPointer());
        NestedObjectArray objArray1(0);

        //
        // Verify Unsuccessful DeSerialization.
        //
        VERIFY_IS_TRUE(FAILED(readerVisitor.Deserialize(objArray1)));
    }

    BOOST_AUTO_TEST_CASE(SimpleArrayTest)
    {
        SimpleArray obj(2);
        wstring data;

        auto error = JsonHelper::Serialize(obj, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        SimpleArray objArray1(0);
        error = JsonHelper::Deserialize(objArray1, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(obj == objArray1);
    }

    BOOST_AUTO_TEST_CASE(EmptyVector)
    {
        NestedObjectArray objArray(0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(objArray, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        NestedObjectArray objArray1(0);
        error = JsonHelper::Deserialize(objArray1, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(objArray1.nestedArray1_.size() == 0 && objArray1.nestedArray_.size() == 0);
    }

    BOOST_AUTO_TEST_CASE(WrongDataTest)
    {
        ObjectSimilarToNestedObjectArray obj(2);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(obj, data);

        //
        // Verify deserialization failure
        //
        NestedObjectArray objArray1(0);
        error = JsonHelper::Deserialize(objArray1, data);
        VERIFY_IS_TRUE(!error.IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(NodesTest)
    {
        wstring name = L"Node1";
        wstring ip = L"localhost";
        wstring nodeType = L"TestNode";
        wstring codeVersion = L"1.0";
        wstring configVersion = L"1.0";
        int64 nodeUpTimeInSeconds = 3600;
        int64 nodeDownTimeInSeconds = 0;
        DateTime nodeUpAt = DateTime::Now();
        DateTime nodeDownAt = DateTime::Zero;
        bool isSeed = true;
        wstring upgradeDomain = L"TestUpgradeDomain";
        wstring faultDomain = L"TestFaultDomain";
        Federation::NodeId nodeId;

        NodeQueryResult nodeResult(
            name,
            ip,
            nodeType,
            codeVersion,
            configVersion,
            FABRIC_QUERY_NODE_STATUS_UP,
            nodeUpTimeInSeconds,
            nodeDownTimeInSeconds,
            nodeUpAt,
            nodeDownAt,
            isSeed,
            upgradeDomain,
            faultDomain,
            nodeId,
            0,
            NodeDeactivationQueryResult(),
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(nodeResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        NodeQueryResult nodeResult2;
        error = JsonHelper::Deserialize(nodeResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(nodeResult2.ToString(), nodeResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(ApplicationsTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");
        wstring appTypeName = L"appType1";
        wstring appTypeVersion = L"1.0";

        std::map<std::wstring, std::wstring> applicationParameters;
        ApplicationQueryResult appQueryResult(uri, appTypeName, appTypeVersion, ApplicationStatus::Ready, applicationParameters, FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION, L"", map<wstring, wstring>());
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(appQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ApplicationQueryResult appQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(appQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(appQueryResult2.ToString(), appQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulServicesTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");

        ServiceQueryResult statefulQueryResult =
            ServiceQueryResult::CreateStatefulServiceQueryResult(uri, L"Type1", L"1.0", true, FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceQueryResult statefulQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulQueryResult2.ToString(), statefulQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessServicesTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");

        ServiceQueryResult statelessQueryResult =
            ServiceQueryResult::CreateStatelessServiceQueryResult(uri, L"Type1", L"1.0", FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceQueryResult statelessQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessQueryResult2.ToString(), statelessQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessServicePartitionTest)
    {
        Guid partitionId = Guid::NewGuid();
        ServicePartitionInformation int64Partition = ServicePartitionInformation(partitionId, -1, 1);
        ServicePartitionQueryResult statelessPartQueryResult = ServicePartitionQueryResult::CreateStatelessServicePartitionQueryResult(
            int64Partition, 2, FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessPartQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServicePartitionQueryResult statelessPartQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessPartQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessPartQueryResult2.ToString(), statelessPartQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulServicePartitionTest)
    {
        Guid partitionId = Guid::NewGuid();
        Reliability::Epoch primaryEpoch(2, 2);
        ServicePartitionInformation int64Partition = ServicePartitionInformation(partitionId, -1, 1);
        ServicePartitionQueryResult statefulPartQueryResult = ServicePartitionQueryResult::CreateStatefulServicePartitionQueryResult(
            int64Partition, 4, 2, FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY, 0, primaryEpoch);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulPartQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServicePartitionQueryResult statefulPartQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulPartQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulPartQueryResult2.ToString(), statefulPartQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulReplicaTest)
    {
        ServiceReplicaQueryResult statefulReplicaResult = ServiceReplicaQueryResult::CreateStatefulServiceReplicaQueryResult(
            -1,
            FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS::FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
            L"Address",
            L"Node",
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulReplicaResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceReplicaQueryResult statefulReplicaResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulReplicaResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulReplicaResult2.ToString(), statefulReplicaResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessInstanceTest)
    {
        ServiceReplicaQueryResult statelessInstanceResult = ServiceReplicaQueryResult::CreateStatelessServiceInstanceQueryResult(
            -1,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS::FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
            L"Address",
            L"Node",
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessInstanceResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceReplicaQueryResult statelessInstanceResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessInstanceResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessInstanceResult2.ToString(), statelessInstanceResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(SerializationToWideCharTest)
    {
        wstring data;
        ObjectWithMap obj1(3);

        auto error = JsonHelper::Serialize(obj1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ObjectWithMap obj2(0);

        error = JsonHelper::Deserialize(obj2, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(obj1 == obj2);

        ApplicationHealthPolicy policy1;
        ApplicationHealthPolicy policy2;

        wstring policy1String = policy1.ToString();
        wstring policy2String = policy2.ToString();

        VERIFY_IS_TRUE(!policy1String.empty());
        VERIFY_IS_TRUE(!policy2String.empty());
        VERIFY_IS_TRUE((policy1String == policy2String));

        error = ApplicationHealthPolicy::FromString(policy1String, policy1);
        VERIFY_IS_TRUE(error.IsSuccess());
        error = ApplicationHealthPolicy::FromString(policy2String, policy2);
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(HealthEventsTest)
    {
        wstring data;
        FILETIME time;
        time.dwHighDateTime = 1234;
        time.dwLowDateTime = 1234;
        DateTime dateTime(time);
        HealthEvent healthEvent(
            L"source1",
            L"property1",
            TimeSpan::MaxValue,
            FABRIC_HEALTH_STATE_INVALID,
            L"no description",
            123,
            dateTime,
            dateTime,
            false,
            false,
            dateTime,
            dateTime,
            dateTime);
        std::vector<HealthEvent> healthEvents;
        healthEvents.push_back(healthEvent);

        wstring appname = L"Name";

        ServiceAggregatedHealthState serviceAggregatedHealthState;
        std::vector<ServiceModel::ServiceAggregatedHealthState> serviceAggHealthStates;
        serviceAggHealthStates.push_back(serviceAggregatedHealthState);

        DeployedApplicationAggregatedHealthState deployedAppHealthState;
        std::vector<DeployedApplicationAggregatedHealthState> deployedAppHealthStates;
        deployedAppHealthStates.push_back(deployedAppHealthState);

        HealthEvent healthEventCopy = healthEvent;
        HealthEvaluation evaluationReason = HealthEvaluation(make_shared<EventHealthEvaluation>(FABRIC_HEALTH_STATE_WARNING, move(healthEventCopy), true));
        std::vector<HealthEvaluation> unhealthyEvaluations;
        unhealthyEvaluations.push_back(evaluationReason);

        auto healthStats = make_unique<HealthStatistics>();
        healthStats->Add(EntityKind::Replica, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::Partition, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::Service, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::DeployedApplication, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::DeployedServicePackage, HealthStateCount(100, 10, 20));

        ApplicationHealth obj1(
            appname,
            move(healthEvents),
            FABRIC_HEALTH_STATE_UNKNOWN,
            move(unhealthyEvaluations),
            move(serviceAggHealthStates),
            move(deployedAppHealthStates),
            move(healthStats));

        auto error = JsonHelper::Serialize(obj1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ApplicationHealth obj2;

        error = JsonHelper::Deserialize(obj2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring string1 = obj1.ToString();
        wstring string2 = obj2.ToString();
        VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(string1, string2));

        string1 = obj1.Events[0].ToString();
        string2 = obj2.Events[0].ToString();
        VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(string1, string2));
    }

    BOOST_AUTO_TEST_CASE(SerializationOfObjectWithSPtr)
    {
        ObjectWithoutSPtr object1(L"objectOne");
        wstring data;
        auto error = JsonHelper::Serialize(object1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ObjectWithSPtr object2;
        error = JsonHelper::Deserialize(object2, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(object1.name_ == object2.name_);
        VERIFY_IS_TRUE(object2.tempObjSPtr_ == nullptr);

        wstring data1;
        error = JsonHelper::Serialize(object2, data1);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(data == data1);

        object2.tempObjSPtr_ = make_shared<TempObject>(L"object2", 1);

        error = JsonHelper::Serialize(object2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ObjectWithSPtr object3;
        error = JsonHelper::Deserialize(object3, data);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(*object2.tempObjSPtr_ == *object3.tempObjSPtr_);
    }

    BOOST_AUTO_TEST_CASE(DynamicActivationWithSPtr)
    {
        ObjectWithSPtrArray object(L"ObjectWithSPtrArray");

        object.objectSPtrArray_.push_back(make_shared<TempObject>(L"1.TempObject", 0xA5A5));
        object.objectSPtrArray_.push_back(make_shared<TempObject1>(false, 0xDEADBEEF));
        object.objectSPtrArray_.push_back(make_shared<TempObject>(L"2.TempObject", 0xF00D));

        wstring data;
        auto error = JsonHelper::Serialize(object, data, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        ObjectWithSPtrArray object1;
        error = JsonHelper::Deserialize(object1, data, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring data1;
        error = JsonHelper::Serialize(object1, data1, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(data == data1);
    }

    BOOST_AUTO_TEST_CASE(DynamicActivationOfSPtr)
    {
        shared_ptr<TempObject> object = make_shared<TempObject>(L"1.TempObject", 0xA5A5);
        wstring data;
        auto error = JsonHelper::Serialize(*object, data, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        shared_ptr<ObjectBase> objectBase;
        error = JsonHelper::Deserialize(objectBase, data, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        shared_ptr<TempObject> tempObj = dynamic_pointer_cast<TempObject>(objectBase);
        VERIFY_IS_TRUE(tempObj != nullptr);

        wstring data1;
        error = JsonHelper::Serialize(*tempObj, data1, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(data1 == data);

        // Test if we are handling newlines correctly when parsing the object for dynamic activation.
        data = L"{\"Kind\":\"OBJECT_KIND_3\",\"ObjectId\":\"SpBYuHd0Mi\",\"Array\": \r\n[\r\n      1,\r\n 2,\r\n 3\r\n]\r\n}";
        error = JsonHelper::Deserialize(objectBase, data, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());

        vector<int> numbers{1, 2, 3};
        TempObjectWithArray tmpObjWithArray(L"SpBYuHd0Mi", move(numbers));
        shared_ptr<TempObjectWithArray> tempObj1 = dynamic_pointer_cast<TempObjectWithArray>(objectBase);

        VERIFY_IS_TRUE(tmpObjWithArray == *tempObj1);
    }

    BOOST_AUTO_TEST_CASE(DynamicActivationWithSPtrRoot)
    {
        TempObject1 originalDerivedObject(false, 0xDEADBEEF);
        ObjectBase& baseRef = originalDerivedObject;

        wstring data;
        auto error = JsonHelper::Serialize(baseRef, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        TempObject1SPtr deserializedDerivedPtr;
        error = JsonHelper::Deserialize(deserializedDerivedPtr, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(originalDerivedObject == *deserializedDerivedPtr);
    }

    BOOST_AUTO_TEST_CASE(TestSerializeIntFromQuotedString)
    {
        // Serialize instance of "SimilarToTempObject" which will produce json string where json property "ObjectValue" is a quoted string.
        // Then deserialize that json to instance of "TempObject" which will convert proptery "ObjectValue" to type int.
        SimilarToTempObject obj1WithString(L"testObject1", L"123");
        wstring jsonString;
        auto error = JsonHelper::Serialize(obj1WithString, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        // DeSerialization of json to instance of "TempObject"
        TempObject obj2WithInt;
        error = JsonHelper::Deserialize(obj2WithInt, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Compare field corresponding to json property "ObjectValue"
        // Assert that "123" was read as integer 123 was read correctly.
        VERIFY_ARE_EQUAL(123, obj2WithInt.val_);

        // Compare all other fields.
        VERIFY_ARE_EQUAL(obj1WithString.name_, obj2WithInt.name_);
        VERIFY_ARE_EQUAL(obj1WithString.id_, obj2WithInt.id_);
        VERIFY_ARE_EQUAL(obj1WithString.kind_, obj2WithInt.kind_);
    }

    BOOST_AUTO_TEST_CASE(CompatibilityTest)
    {
        TestObjectV1 object1(1, 2);
        wstring jsonString;
        auto error = JsonHelper::Serialize(object1, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // This tests if the new version of the object can deserialize
        // the old version of the object
        //
        TestObjectV2 object2;
        error = JsonHelper::Deserialize(object2, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        TestObjectV2 object3(2, 3, L"Foo");
        error = JsonHelper::Serialize(object3, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // This tests if the old version of the object can deserialize
        // the new version of the object.
        //
        error = JsonHelper::Deserialize(object1, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(object1.i_ == object3.i_);
        VERIFY_IS_TRUE(object1.j_ == object3.j_);

        //
        // This is just a sanity test for the V2 object serialization.
        //
        TestObjectV2 object4;
        error = JsonHelper::Deserialize(object4, jsonString);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring jsonString1;
        error = JsonHelper::Serialize(object4, jsonString1);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(jsonString == jsonString1);
    }

    BOOST_AUTO_TEST_CASE(SimpleMapTest)
    {
        SimpleMapTestObject object;

        wstring jsonInput = L"{\"StartMarkerObject\":{\"ObjectName\":\"MyTest\"},\"TestMap\":{\"MyKey1\":\"MyValue1\",\"MyKey2\":\"MyValue2\",\"MyKey3\":\"MyValue3\"},\"EndMarkerObject\":{\"ObjectName\":\"MyTest\"}}";

        auto error = JsonHelper::Deserialize(object, jsonInput);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(object.startMarkerObject_.name_ == L"MyTest");
        VERIFY_IS_TRUE(object.endMarkerObject_.name_ == L"MyTest");
        VERIFY_IS_TRUE(object.map1_.size() == 3);
        VERIFY_IS_TRUE(object.map1_[L"MyKey1"] == L"MyValue1");
        VERIFY_IS_TRUE(object.map1_[L"MyKey2"] == L"MyValue2");
        VERIFY_IS_TRUE(object.map1_[L"MyKey3"] == L"MyValue3");

        wstring jsonOutput;
        error = JsonHelper::Serialize(object, jsonOutput);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo("SimpleMapTest", "input={0} output={1}", jsonInput, jsonOutput);

        VERIFY_IS_TRUE(jsonInput == jsonOutput);
    }

    BOOST_AUTO_TEST_CASE(WStringMapTest)
    {
        WStringMapTestObject object;

        wstring jsonInput = L"{\"WStringMap\":{\"MyKey1\":\"MyValue1\",\"MyKey2\":\"MyValue2\",\"MyKey3\":\"MyValue3\"}}";

        auto error = JsonHelper::Deserialize(object, jsonInput);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(object.map1_.size() == 3);
        VERIFY_IS_TRUE(object.map1_[L"MyKey1"] == L"MyValue1");
        VERIFY_IS_TRUE(object.map1_[L"MyKey2"] == L"MyValue2");
        VERIFY_IS_TRUE(object.map1_[L"MyKey3"] == L"MyValue3");

        wstring jsonOutput;
        error = JsonHelper::Serialize(object, jsonOutput);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo("WStringMapTest", "input={0} output={1}", jsonInput, jsonOutput);

        VERIFY_IS_TRUE(jsonInput == jsonOutput);
    }

    BOOST_AUTO_TEST_CASE(EnumToStringTest)
    {
        wstring jsonOutput;
        TempObject object(L"TempObject", 10);
        auto error = JsonHelper::Serialize(object, jsonOutput, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo("EnumToStringTest", "output = {0}", jsonOutput);
        wprintf(L"%s", jsonOutput.c_str());

        TempObject object1;
        error = JsonHelper::Deserialize(object1, jsonOutput, JsonSerializerFlags::EnumInStringFormat);
        VERIFY_IS_TRUE(object == object1);
    }

    BOOST_AUTO_TEST_CASE(ConditionalConstantStringTest)
    {
        {
            wstring jsonOutput;
            ObjectWithConstProperty object(false);
            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);

            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("ConditionalConstantStringTest(false)", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"ConditionalConstantProperty\":\"TheValueIsFalse\"}")));
        }
        {
            wstring jsonOutput;
            ObjectWithConstProperty object(true);
            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);

            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("ConditionalConstantStringTest(true)", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"ConditionalConstantProperty\":\"TheValueIsTrue\"}")));
        }
    }

    BOOST_AUTO_TEST_CASE(Int64AsNumTest)
    {
        {
            wstring jsonOutput;
            unsigned __int64 value1 = 5000000000;
            __int64 value2 = 5000000000;

            TestObjectInt64AsNum object(value1, value2);

            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("Int64AsNumTest", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"UInt64\":5000000000,\"Int64\":5000000000}")));

            TestObjectInt64AsNum object1;
            error = JsonHelper::Deserialize(object1, jsonOutput);
            Trace.WriteInfo("Int64AsNumTest", "deserialize = {0},{1}", object1.int64_, object1.uint64_);
            VERIFY_IS_TRUE(object == object1);
        }
        {
            wstring jsonOutput;
            unsigned __int64 value1 = 10;
            __int64 value2 = 20;

            TestObjectInt64AsNum object(value1, value2);

            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("Int64AsNumTest", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"UInt64\":10,\"Int64\":20}")));

            TestObjectInt64AsNum object1;
            error = JsonHelper::Deserialize(object1, jsonOutput);
            Trace.WriteInfo("Int64AsNumTest", "deserialize = {0},{1}", object1.int64_, object1.uint64_);
            VERIFY_IS_TRUE(object == object1);
        }
        {
            wstring jsonOutput;
            unsigned __int64 value1 = 18446744073709551615;
            __int64 value2 = -9223372036854775807 - 1;

            TestObjectInt64AsNum object(value1, value2);

            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("Int64AsNumTest", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"UInt64\":18446744073709551615,\"Int64\":-9223372036854775808}")));
        }
        {
            wstring jsonOutput;
            unsigned __int64 value1 = 18446744073709551615;
            __int64 value2 = 9223372036854775807;

            TestObjectInt64AsNum object(value1, value2);

            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("Int64AsNumTest", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"UInt64\":18446744073709551615,\"Int64\":9223372036854775807}")));
        }
        {
            wstring jsonOutput;
            unsigned __int64 value1 = 10;
            __int64 value2 = -20;

            TestObjectInt64AsNum object(value1, value2);

            auto error = JsonHelper::Serialize(
                object,
                jsonOutput);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo("Int64AsNumTest", "output = {0}", jsonOutput);
            VERIFY_IS_TRUE(StringUtility::Contains(jsonOutput, wstring(L"{\"UInt64\":10,\"Int64\":-20}")));

            TestObjectInt64AsNum object1;
            error = JsonHelper::Deserialize(object1, jsonOutput);
            Trace.WriteInfo("Int64AsNumTest", "deserialize = {0},{1}", object1.int64_, object1.uint64_);
            VERIFY_IS_TRUE(object == object1);
        }
    }
    
    BOOST_AUTO_TEST_CASE(NumValuesLowerBoundsTest)
    {
        {
            // All valid values
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            ObjectWithNums expected(1, 1, 1, 1, 1, 1, 1);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(object == expected);
        }
        {
            // Byte out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": -1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // Int out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": –2147483649,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // UInt out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": -1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // LONG out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": –2147483649,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // ULONG out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": -1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // Int64 out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
#if !defined(PLATFORM_UNIX)
                L"\"Int64\": \"-9223372036854775809\","
#else
                // due to floating point (double) comparison precision on Linux,
                // use a number with a greater difference from the min
                L"\"Int64\": \"-9223372036854785809\","
#endif
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // UInt64 out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"-1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }

        {
            // Int64 lower bound test.
            ObjectWithNums object(0, 0, 0, 0, 0, 0x8000000000000000, 0);
            wstring data;
            auto error = JsonHelper::Serialize(object, data);
            VERIFY_IS_TRUE(error.IsSuccess());

            ObjectWithNums object1;
            error = JsonHelper::Deserialize(object1, data);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(object == object1);  
        }
    }
    
    BOOST_AUTO_TEST_CASE(NumValuesUpperBoundsTest)
    {
        {
            // All valid values
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            ObjectWithNums expected(1, 1, 1, 1, 1, 1, 1);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(object == expected);
        }
        {
            // Byte out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 256,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // Int out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 2147483648,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // UInt out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 4294967296,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // LONG out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 2147483648,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // ULONG out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 4294967296,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }
        {
            // Int64 out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
#if !defined(PLATFORM_UNIX)
                L"\"Int64\": \"9223372036854775808\","
#else
                // due to floating point (double) comparison precision on Linux,
                // use a number with a greater difference from the max
                L"\"Int64\": \"9223372036854785808\","
#endif
                L"\"UInt64\": \"1\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }

        {
            // UInt64 out-of-bounds
            wstring jsonInput = L"{"
                L"\"Byte\": 1,"
                L"\"Int\": 1,"
                L"\"UInt\": 1,"
                L"\"LONG\": 1,"
                L"\"ULONG\": 1,"
                L"\"Int64\": \"1\","
                L"\"UInt64\": \"18446744073709551616\"}";

            ObjectWithNums object(0, 0, 0, 0, 0, 0, 0);
            auto error = JsonHelper::Deserialize(object, jsonInput);
            VERIFY_IS_FALSE(error.IsSuccess());
        }

        {
            // Int64 upper bound test.
            ObjectWithNums object(0, 0, 0, 0, 0, 0x7FFFFFFFFFFFFFFF, 0);
            wstring data;
            auto error = JsonHelper::Serialize(object, data);
            VERIFY_IS_TRUE(error.IsSuccess());

            ObjectWithNums object1;
            error = JsonHelper::Deserialize(object1, data);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(object == object1);  
        }

        {
            // UInt64 upper bound test.
            ObjectWithNums object {0, 0, 0, 0, 0, 0, 0xFFFFFFFFFFFFFFFF};
            wstring data;
            auto error = JsonHelper::Serialize(object, data);
            VERIFY_IS_TRUE(error.IsSuccess());
            
            ObjectWithNums object1;
            error = JsonHelper::Deserialize(object1, data);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(object == object1);
        }
    }

    BOOST_AUTO_TEST_CASE(LoadInformationTest) 
    {
        LoadMetricInformation loadMetricInformation;
        loadMetricInformation.Name = L"Metric1";
        loadMetricInformation.IsBalancedBefore = false;
        loadMetricInformation.IsBalancedAfter = true;
        loadMetricInformation.DeviationBefore = 0.62;
        loadMetricInformation.DeviationAfter = 0.5;
        loadMetricInformation.BalancingThreshold = 100;
        loadMetricInformation.Action = L"Balancing";
        loadMetricInformation.ActivityThreshold = 1;
        loadMetricInformation.ClusterCapacity = 100;
        loadMetricInformation.ClusterLoad = 50;
        loadMetricInformation.IsClusterCapacityViolation = false;
        loadMetricInformation.RemainingUnbufferedCapacity = 50;
        loadMetricInformation.NodeBufferPercentage = 0.0;
        loadMetricInformation.BufferedCapacity = 100;
        loadMetricInformation.RemainingBufferedCapacity = 0;
        loadMetricInformation.MinNodeLoadValue = 5;
        loadMetricInformation.MinNodeLoadNodeId = Federation::NodeId(Common::LargeInteger(0, 1));
        loadMetricInformation.MaxNodeLoadValue = 30;
        loadMetricInformation.MaxNodeLoadNodeId = Federation::NodeId(Common::LargeInteger(0, 5));
        loadMetricInformation.CurrentClusterLoad = 50.0;
        loadMetricInformation.BufferedClusterCapacityRemaining = 0.0;
        loadMetricInformation.ClusterCapacityRemaining = 50.0;
        loadMetricInformation.MaximumNodeLoad = 29.1;
        loadMetricInformation.MinimumNodeLoad = 4.5;

        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(loadMetricInformation, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        LoadMetricInformation loadMetricInformationDeserialized;
        error = JsonHelper::Deserialize(loadMetricInformationDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(loadMetricInformation.ToString(), loadMetricInformationDeserialized.ToString()));
    }

    BOOST_AUTO_TEST_CASE(NodeLoadInformationTest)
    {
        NodeLoadMetricInformation nodeLoadInformation;
        nodeLoadInformation.Name = L"Node1";
        nodeLoadInformation.NodeCapacity = 8;
        nodeLoadInformation.NodeLoad = 6;
        nodeLoadInformation.NodeRemainingCapacity = 2;
        nodeLoadInformation.IsCapacityViolation = false;
        nodeLoadInformation.NodeBufferedCapacity = 0;
        nodeLoadInformation.NodeRemainingBufferedCapacity = 0;
        nodeLoadInformation.CurrentNodeLoad = 6.0;
        nodeLoadInformation.NodeCapacityRemaining = 2.0;
        nodeLoadInformation.BufferedNodeCapacityRemaining = 0.0;

        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(nodeLoadInformation, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        NodeLoadMetricInformation nodeLoadInformationDeserialized;
        error = JsonHelper::Deserialize(nodeLoadInformationDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(nodeLoadInformation.ToString(), nodeLoadInformationDeserialized.ToString()));

    }

    BOOST_AUTO_TEST_CASE(ControlCharactersEscapeTest)
    {
        wstring testData = L"\x1b""[40m""\x1b""[1m""\x1b""[33myellow""\x1b""[39m""\x1b""[22m""\x1b""[49m ""\x1b""[2J""\x1b""[H""\x1b""[37;40m ""\x01""\x02""\x03""\x04""\x05""\x06""\x07""\b\f\n\r\v\t""\x0e""\x0f""\x10""\x11""\x12""\x13""\x14""\x15""\x16""\x17""\x18""\x19""\x1a""\x1b""\x1c""\x1d""\x1e""\x1f"" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=[]\\;',./`~!@#$%^&*()_+{}|:\"<>?""\x00";
        ContainerInfoData containerInfoData(move(testData));
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(containerInfoData, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ContainerInfoData containerInfoDataDeserialized;
        error = JsonHelper::Deserialize(containerInfoDataDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(containerInfoData.Content, containerInfoDataDeserialized.Content));
    }

    BOOST_AUTO_TEST_CASE(VectorOfObjects)
    {
        vector<TempObject> vectorObject;
        vectorObject.push_back(move(TempObject(L"TempObject", 10)));
        vectorObject.push_back(move(TempObject(L"TempObject1", 11)));

        ByteBufferUPtr data;
        auto error = JsonHelper::Serialize(vectorObject, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        vector<TempObject> vectorObject1;
        error = JsonHelper::Deserialize(vectorObject1, data);
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
