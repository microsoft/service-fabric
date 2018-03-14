// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;
    using namespace ServiceModel;

    class ServiceGroupDescriptorTest
    {
    protected:
        ServiceGroupDescriptorTest() { BOOST_REQUIRE(ClassSetup()); }
        TEST_CLASS_SETUP(ClassSetup)
	public:
		class TestInitData;

        ServiceDescription CreateServiceDescription(wstring const & name, bool stateful, CServiceGroupDescription & serviceGroup, int partitionCount = 1);
        CServiceGroupDescription CreateServiceGroupDescription(wstring const & name, TestInitData & groupInitData, vector<wstring> & memberNames, vector<TestInitData> & memberInitData, bool isStateful, bool hasPersistedState);

        wstring placementConstraints_;
        int scaleoutCount_;
        vector<Reliability::ServiceLoadMetricDescription> metrics_;

        wstring groupName_;
        vector<wstring> memberNames_;

        class TestInitData : public Serialization::FabricSerializable
        {
		public:
            wstring data_;

            TestInitData() {}
            TestInitData(std::wstring const & data) : data_(data) { }

            static void Verify(TestInitData & expected, const byte* actual, ULONG actualSize)
            {
                TestInitData other;

                vector<byte> data;
                data.resize(actualSize);
                
                ::CopyMemory(data.data(), actual, actualSize);

                VERIFY_IS_TRUE(FabricSerializer::Deserialize(other, data).IsSuccess());

                VERIFY_ARE_EQUAL(expected.data_, other.data_);
            }

            FABRIC_FIELDS_01(data_);
        };

        TestInitData groupInitData_;
        vector<TestInitData> memberInitData_;
    };


    BOOST_FIXTURE_TEST_SUITE(ServiceGroupDescriptorTestSuite,ServiceGroupDescriptorTest)

    BOOST_AUTO_TEST_CASE(TestExtractServiceGroupDescription)
    {
        auto group = CreateServiceGroupDescription(groupName_, groupInitData_, memberNames_, memberInitData_, true, true);
        auto service = CreateServiceDescription(groupName_, true, group);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(service, psd).IsSuccess());
        psd.IsServiceGroup = true;

        ServiceGroupDescriptor descriptor;
        VERIFY_IS_TRUE(ServiceGroupDescriptor::Create(psd, descriptor).IsSuccess());
        ScopedHeap heap;

        auto serviceGroupDescription = heap.AddItem<FABRIC_SERVICE_GROUP_DESCRIPTION>();

        descriptor.ToPublicApi(heap, *serviceGroupDescription);

        // verify all members present
        VERIFY_IS_TRUE(serviceGroupDescription->MemberDescriptions != nullptr);
        VERIFY_IS_TRUE(serviceGroupDescription->MemberCount == memberNames_.size());

        // verify member names and types
        VERIFY_ARE_EQUAL(groupName_ + L"#" + memberNames_[0], serviceGroupDescription->MemberDescriptions[0].ServiceName);
        VERIFY_ARE_EQUAL(memberNames_[0], serviceGroupDescription->MemberDescriptions[0].ServiceType);

        VERIFY_ARE_EQUAL(groupName_ + L"#" + memberNames_[1], serviceGroupDescription->MemberDescriptions[1].ServiceName);
        VERIFY_ARE_EQUAL(memberNames_[1], serviceGroupDescription->MemberDescriptions[1].ServiceType);

        // verify member init data
        TestInitData::Verify(
            memberInitData_[0], 
            serviceGroupDescription->MemberDescriptions[0].InitializationData, 
            serviceGroupDescription->MemberDescriptions[0].InitializationDataSize);

        TestInitData::Verify(
            memberInitData_[1], 
            serviceGroupDescription->MemberDescriptions[1].InitializationData, 
            serviceGroupDescription->MemberDescriptions[1].InitializationDataSize);

        // verify service description for service group
        VERIFY_IS_TRUE(serviceGroupDescription->Description != nullptr);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, serviceGroupDescription->Description->Kind);

        FABRIC_STATEFUL_SERVICE_DESCRIPTION* stateful = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceGroupDescription->Description->Value);

        VERIFY_ARE_EQUAL(groupName_, wstring(stateful->ServiceName));
        VERIFY_IS_TRUE(!!stateful->HasPersistedState);

        // verify service group init data
        VERIFY_IS_TRUE(stateful->InitializationData != nullptr);
        VERIFY_IS_TRUE(stateful->InitializationDataSize > 0);

        TestInitData::Verify(
            groupInitData_,
            stateful->InitializationData,
            stateful->InitializationDataSize);
    }

    BOOST_AUTO_TEST_CASE(TestExtractMemberServiceDescription)
    {
        auto group = CreateServiceGroupDescription(groupName_, groupInitData_, memberNames_, memberInitData_, true, true);
        auto service = CreateServiceDescription(groupName_, true, group);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(service, psd).IsSuccess());
        psd.IsServiceGroup = true;

        ServiceGroupDescriptor descriptor;
        VERIFY_IS_TRUE(ServiceGroupDescriptor::Create(psd, descriptor).IsSuccess());

        for (unsigned int i = 0; i < memberNames_.size(); i++)
        {
            PartitionedServiceDescriptor member;

            NamingUri memberName;
            VERIFY_IS_TRUE(NamingUri::TryParse(groupName_ + L"#" + memberNames_[i], memberName));

            auto error = descriptor.GetServiceDescriptorForMember(memberName, member);

            VERIFY_IS_TRUE(error.IsSuccess());

            ScopedHeap heap;

            auto memberDescription = heap.AddItem<FABRIC_SERVICE_DESCRIPTION>();

            member.ToPublicApi(heap, *memberDescription);

            // verify service description for member service
            VERIFY_IS_TRUE(memberDescription->Value != nullptr);
            VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, memberDescription->Kind);

            FABRIC_STATEFUL_SERVICE_DESCRIPTION* stateful = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(memberDescription->Value);

            VERIFY_ARE_EQUAL(groupName_ + L"#" + memberNames_[i], wstring(stateful->ServiceName));
            VERIFY_ARE_EQUAL(memberNames_[i], wstring(stateful->ServiceTypeName));
            VERIFY_IS_TRUE(!!stateful->HasPersistedState);

            // verify member service init data
            VERIFY_IS_TRUE(stateful->InitializationData != nullptr);
            VERIFY_IS_TRUE(stateful->InitializationDataSize > 0);

            TestInitData::Verify(
                memberInitData_[i],
                stateful->InitializationData,
                stateful->InitializationDataSize);
        }
    }

    BOOST_AUTO_TEST_CASE(TestExtractMemberServiceDescription_UnknownMember)
    {
        auto group = CreateServiceGroupDescription(groupName_, groupInitData_, memberNames_, memberInitData_, true, true);
        auto service = CreateServiceDescription(groupName_, true, group);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(service, psd).IsSuccess());
        psd.IsServiceGroup = true;

        ServiceGroupDescriptor descriptor;
        VERIFY_IS_TRUE(ServiceGroupDescriptor::Create(psd, descriptor).IsSuccess());

        PartitionedServiceDescriptor member;

        NamingUri memberName;
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/group#bloodynine", memberName));

        auto error = descriptor.GetServiceDescriptorForMember(memberName, member);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NameNotFound));
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool ServiceGroupDescriptorTest::ClassSetup()
    {
        placementConstraints_ = L"";
        scaleoutCount_ = 0;
        metrics_ = vector<Reliability::ServiceLoadMetricDescription>();

        groupInitData_ = wstring(L"GroupInitData");

        memberNames_.push_back(L"A");
        memberNames_.push_back(L"B");

        memberInitData_.push_back(TestInitData(L"A"));
        memberInitData_.push_back(TestInitData(L"B"));

        groupName_ = wstring(L"fabric:/group");

        return true;
    }



    CServiceGroupDescription ServiceGroupDescriptorTest::CreateServiceGroupDescription(wstring const & name, TestInitData & groupInitData, vector<wstring> & memberNames, vector<TestInitData> & memberInitData, bool isStateful, bool hasPersistedSate)
    {
        CServiceGroupDescription description;

        description.HasPersistedState = isStateful && hasPersistedSate;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&groupInitData, description.ServiceGroupInitializationData).IsSuccess());

        auto nameIter = begin(memberNames);
        auto dataIter = begin(memberInitData);

        for (; nameIter != end(memberNames) && dataIter != end(memberInitData); ++nameIter, ++dataIter)
        {
            CServiceGroupMemberDescription member;

            member.ServiceName = name + L"#" + *nameIter;
            member.ServiceType = ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), *nameIter).ToString();
            member.ServiceDescriptionType = isStateful ? FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL : FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
            member.Identifier = Common::Guid::NewGuid().AsGUID();

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&(*dataIter), member.ServiceGroupMemberInitializationData).IsSuccess());

            description.ServiceGroupMemberData.push_back(member);
        }

        return description;
    }

    ServiceDescription ServiceGroupDescriptorTest::CreateServiceDescription(wstring const & name, bool stateful, CServiceGroupDescription & serviceGroup, int partitionCount)
    {
        vector<byte> initData;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&serviceGroup, initData).IsSuccess());

        return ServiceDescription(
            name,
            0,
            0,
            partitionCount, // partition count
            1,  // replica count 
            1,  // min write
            stateful,  // is stateful
            stateful && serviceGroup.HasPersistedState,  // has persisted
            TimeSpan::FromSeconds(300.0), //replica restart wait duration
            TimeSpan::MaxValue, // quorum loss wait duration
            TimeSpan::FromSeconds(3600.0), // stand by replica keep duration
            ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"ServiceGroupType"), 
            vector<Reliability::ServiceCorrelationDescription>(),
            L"",    // placement constraints
            0, 
            vector<Reliability::ServiceLoadMetricDescription>(), 
            0,
            initData);
    }

}
 

