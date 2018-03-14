// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Common;
using namespace ServiceModel;
using namespace std;

class NodeUpOperationFactoryTest
{
protected:
    NodeUpOperationFactoryTest() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~NodeUpOperationFactoryTest() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    class ServicePackageName
    {
	public:
        enum Enum
        {
            SP1 = 0,
            SP2 = 1,
            SP2V1 = 2,
            Invalid = 3,
        };
    };

	void AddClosedFT(FailoverUnitId const & ftId);
    void AddOpenFT(FailoverUnitId const & ftId, ServicePackageName::Enum e);

    void ValidateAnyReplicaFound(bool fmm, bool fm);

    void ValidatePackageList(
        NodeUpOperationFactory::NodeUpData const & data,
        ServicePackageName::Enum e1 = ServicePackageName::Invalid,
        ServicePackageName::Enum e2 = ServicePackageName::Invalid
        );

    void ValidateFmmPackageList(
        ServicePackageName::Enum e1 = ServicePackageName::Invalid,
        ServicePackageName::Enum e2 = ServicePackageName::Invalid
        );

    void ValidateFMPackageList(
        ServicePackageName::Enum e1 = ServicePackageName::Invalid,
        ServicePackageName::Enum e2 = ServicePackageName::Invalid
        );

    void ValidatePackage(ServicePackageName::Enum e, ServicePackageInfo const & package);

    pair<ServicePackageIdentifier*, ServicePackageVersionInstance*> Get(ServicePackageName::Enum e);

    ServicePackageIdentifier sp1Id_;
    ServicePackageVersionInstance sp1Instance_;
    ServicePackageIdentifier sp2Id_;
    ServicePackageVersionInstance sp2Instance_;
    ServicePackageVersionInstance sp2InstanceV1_;

    NodeUpOperationFactory factory_;

    FailoverUnitId fmFTId_;
    FailoverUnitId ft1Id_;
    FailoverUnitId ft2Id_;
};

bool NodeUpOperationFactoryTest::TestSetup()
{
	sp1Id_ = ServicePackageIdentifier(L"app1", L"pkg1");
	sp1Instance_ = ServicePackageVersionInstance(ServicePackageVersion(), 1);

	sp2Id_ = ServicePackageIdentifier(L"app2", L"pkg2");
	sp2Instance_ = ServicePackageVersionInstance(ServicePackageVersion(), 2);
    sp2InstanceV1_ = ServicePackageVersionInstance(ServicePackageVersion(), 1);

	fmFTId_ = FailoverUnitId(Reliability::Constants::FMServiceGuid);

	factory_ = move(NodeUpOperationFactory());
	return true;
}

bool NodeUpOperationFactoryTest::TestCleanup()
{
	return true;
}

void NodeUpOperationFactoryTest::ValidatePackageList(
    NodeUpOperationFactory::NodeUpData const & data,
    ServicePackageName::Enum e1,
    ServicePackageName::Enum e2)
{
    size_t count = 0;
    if (e1 != ServicePackageName::Invalid)
    {
        count++;            
    }

    if (e2 != ServicePackageName::Invalid)
    {
        count++;
    }

    auto packages = data.GetServicePackages();
    Verify::AreEqual(count, packages.size(), L"Count");

    if (e1 != ServicePackageName::Invalid)
    {
        ValidatePackage(e1, packages[0]);
    }

    if (e2 != ServicePackageName::Invalid)
    {
        ValidatePackage(e2, packages[1]);
    }
}

void NodeUpOperationFactoryTest::ValidateFmmPackageList(
    ServicePackageName::Enum e1,
    ServicePackageName::Enum e2)
{
    ValidatePackageList(factory_.NodeUpDataForFmm, e1, e2);
}

void NodeUpOperationFactoryTest::ValidateFMPackageList(
    ServicePackageName::Enum e1,
    ServicePackageName::Enum e2)
{
    ValidatePackageList(factory_.NodeUpDataForFM, e1, e2);
}

void NodeUpOperationFactoryTest::ValidatePackage(ServicePackageName::Enum e, ServicePackageInfo const & package)
{
    auto rv = Get(e);
    ServicePackageIdentifier * spId = rv.first;
    ServicePackageVersionInstance * spInstance = rv.second;

	Verify::AreEqual(wformatString(*spId), wformatString(package.PackageId), L"PackageId");
    Verify::AreEqual(wformatString(*spInstance), wformatString(package.VersionInstance), L"VersionInstance");
}

void NodeUpOperationFactoryTest::ValidateAnyReplicaFound(bool fmm, bool fm)
{
	VERIFY_ARE_EQUAL(fm, factory_.NodeUpDataForFM.AnyReplicaFound);
	VERIFY_ARE_EQUAL(fmm, factory_.NodeUpDataForFmm.AnyReplicaFound);
}

void NodeUpOperationFactoryTest::AddClosedFT(FailoverUnitId const & ftId)
{
	factory_.AddClosedFailoverUnitFromLfumLoad(ftId);
}

pair<ServicePackageIdentifier*, ServicePackageVersionInstance*> NodeUpOperationFactoryTest::Get(ServicePackageName::Enum e)
{
    ServicePackageIdentifier * spId = nullptr;
    ServicePackageVersionInstance * spInstance = nullptr;

    switch (e)
    {
    case ServicePackageName::SP1:
        spId = &sp1Id_;
        spInstance = &sp1Instance_;
        break;
    case ServicePackageName::SP2:
        spId = &sp2Id_;
        spInstance = &sp2Instance_;
        break;
    case ServicePackageName::SP2V1:
        spId = &sp2Id_;
        spInstance = &sp2InstanceV1_;
        break;
    case ServicePackageName::Invalid:
    default:
        Assert::CodingError("unknown value {0}", static_cast<int>(e));
        break;
    }

    return make_pair(spId, spInstance);
}

void NodeUpOperationFactoryTest::AddOpenFT(FailoverUnitId const & ftId, NodeUpOperationFactoryTest::ServicePackageName::Enum e)
{
    auto rv = Get(e);
    ServicePackageIdentifier * spId = rv.first;
    ServicePackageVersionInstance * spInstance = rv.second;
        
	factory_.AddOpenFailoverUnitFromLfumLoad(ftId, *spId, *spInstance);
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(NodeUpOperationFactoryTestSuite,NodeUpOperationFactoryTest)

BOOST_AUTO_TEST_CASE(IfLfumIsEmptyAnyReplicaFoundIsFalseForFmAndFmm)
{
    ValidateAnyReplicaFound(false, false);
}

BOOST_AUTO_TEST_CASE(ClosedFMFTImpliesReplicaFoundForFmm)
{
    AddClosedFT(fmFTId_);
    ValidateAnyReplicaFound(true, false);
}

BOOST_AUTO_TEST_CASE(OpenFMFTImpliesReplicaFoundForFmm)
{
    AddOpenFT(fmFTId_, ServicePackageName::SP1);
    ValidateAnyReplicaFound(true, false);
}

BOOST_AUTO_TEST_CASE(ClosedNonFMFTImpliesReplicaFoundForFMAndFmm)
{
    AddClosedFT(ft1Id_);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(OpenNonFMFTImpliesReplicaFoundForFMAndFmm)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(MultipleFTsImplyReplicaFoundForFMAndFmm1)
{
    AddClosedFT(fmFTId_);
    AddClosedFT(ft1Id_);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(MultipleFTsImplyReplicaFoundForFMAndFmm2)
{
    AddOpenFT(fmFTId_, ServicePackageName::SP1);
    AddClosedFT(ft1Id_);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(MultipleFTsImplyReplicaFoundForFMAndFmm3)
{
    AddClosedFT(fmFTId_);
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(MultipleFTsImplyReplicaFoundForFMAndFmm4)
{
    AddOpenFT(fmFTId_, ServicePackageName::SP1);
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    ValidateAnyReplicaFound(true, true);
}

BOOST_AUTO_TEST_CASE(ServicePackageListIsInitiallyEmpty)
{
    ValidateFmmPackageList();
    ValidateFMPackageList();
}

BOOST_AUTO_TEST_CASE(FmmFTIsNotAddedToServicePackageList)
{
    AddOpenFT(fmFTId_, ServicePackageName::SP1);
    ValidateFmmPackageList();
    ValidateFMPackageList();
}

BOOST_AUTO_TEST_CASE(MultipleFTWithoutFMFTIsAdded)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    AddOpenFT(ft2Id_, ServicePackageName::SP2);

    ValidateFmmPackageList();
    ValidateFMPackageList(ServicePackageName::SP1, ServicePackageName::SP2);
}

BOOST_AUTO_TEST_CASE(MultipleFTWithFMFTIsAdded)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    AddOpenFT(fmFTId_, ServicePackageName::SP2);

    ValidateFmmPackageList();
    ValidateFMPackageList(ServicePackageName::SP1);
}

BOOST_AUTO_TEST_CASE(SameServicePackageDoesNotRepeat)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    AddOpenFT(ft2Id_, ServicePackageName::SP1);
    AddOpenFT(FailoverUnitId(), ServicePackageName::SP2);

    ValidateFmmPackageList();
    ValidateFMPackageList(ServicePackageName::SP1, ServicePackageName::SP2);
}

BOOST_AUTO_TEST_CASE(SameServicePackageDoesNotRepeat2)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP1);
    AddOpenFT(ft2Id_, ServicePackageName::SP1);

    ValidateFmmPackageList();
    ValidateFMPackageList(ServicePackageName::SP1);
}

/*
During application upgrade the version on the FTs being impacted is updated 
one at a time and not under a single transaction

This means that if the node fails while part of the FTs are updated then there 
is an inconsistency in the package version instance across all the FTs for that app

During node up, the node should report the lowest package version instance it finds

This will cause the FM to send the correct package version instance for the service package
in the node up ack which will cause all the FTs to be updated to the correct package version instance
*/
BOOST_AUTO_TEST_CASE(LowerVersionIsUsedInServicePackage)
{
    AddOpenFT(ft1Id_, ServicePackageName::SP2);
    AddOpenFT(ft2Id_, ServicePackageName::SP2V1);
    ValidateFMPackageList(ServicePackageName::SP2V1);
}

BOOST_AUTO_TEST_CASE(LowerVersionIsUsedInServicePackageReverseOrder)
{
    AddOpenFT(ft2Id_, ServicePackageName::SP2V1);
    AddOpenFT(ft1Id_, ServicePackageName::SP2);
    ValidateFMPackageList(ServicePackageName::SP2V1);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
