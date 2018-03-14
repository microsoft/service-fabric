// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

    namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace Federation;

    StringLiteral const TestSource("NodeIdGeneratorTest");

    class NodeIdGeneratorTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(NodeIdGeneratorTestSuite,NodeIdGeneratorTest)
    
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdPassRoles)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id, L"WorkerRole1", true, L"");
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV2NodeIdPassRoles)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id, L"", true, L"");
        VERIFY_ARE_EQUAL(L"f900ff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdReadRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole1";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV2NodeIdReadRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"f900ff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdFalseFlagWithRolesFirst)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole1,WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdFalseFlagWithRolesMiddle)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole3,WorkerRole2,WorkerRole1,WorkerRole4,WorkerRole5";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdFalseFlagWithRolesLast)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole5,WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole1";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdFalseFlagWithNoRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdTrueFlagWithRolesFirst)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole1,WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdTrueFlagWithRolesMiddle)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole3,WorkerRole2,WorkerRole1,WorkerRole4,WorkerRole5";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV1NodeIdTrueFlagWithRolesLast)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole5,WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole1";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"bfff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV2NodeIdTrueFlagWithNoRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"f900ff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV2NodeIdTrueFlagWithSingleNotMatchingRole)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole2";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"f900ff7068c852abf91905604888ed3b", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV2NodeIdTrueFlagWithNoMatchingRoleList)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator
            = L"WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5,WorkerRole6";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = true;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"f900ff7068c852abf91905604888ed3b", id.ToString());
    }
#endif

    BOOST_AUTO_TEST_CASE(GenerateV3NodeIdPassRoles)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id, L"", false, L"V3");
        VERIFY_ARE_EQUAL(L"9b009e10b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV3NodeIdReadRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V3";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"9b009e10b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV3NodeIdTrueFlagWithNoRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V3";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"9b009e10b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV3NodeIdTrueFlagWithSingleNotMatchingRole)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole2";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V3";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"9b009e10b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV3NodeIdTrueFlagWithNoMatchingRoleList)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator
            = L"WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5,WorkerRole6";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V3";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"9b009e10b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdPassRoles)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id, L"", false, L"V4");
        VERIFY_ARE_EQUAL(L"6e2eb710b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdReadRoles)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"6e2eb710b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdTrueFlagWithSingleNotMatchingRole)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole2";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"6e2eb710b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdTrueFlagWithNoMatchingRoleList)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator
            = L"WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5,WorkerRole6";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"6e2eb710b98296decd1fb3d31b6e54df", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdPassRolesUnderscore)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1_0", id, L"", false, L"V4");
        VERIFY_ARE_EQUAL(L"6e2eb7445e338be080e51e68e21cba18", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdReadRolesUnderscore)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1_0", id);
        VERIFY_ARE_EQUAL(L"6e2eb7445e338be080e51e68e21cba18", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdTrueFlagWithSingleNotMatchingRoleUnderscore)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator = L"WorkerRole2";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1_0", id);
        VERIFY_ARE_EQUAL(L"6e2eb7445e338be080e51e68e21cba18", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdTrueFlagWithNoMatchingRoleListUnderscore)
    {
        ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator
            = L"WorkerRole2,WorkerRole3,WorkerRole4,WorkerRole5,WorkerRole6";
        ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator = false;
        ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion = L"V4";
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1_0", id);
        VERIFY_ARE_EQUAL(L"6e2eb7445e338be080e51e68e21cba18", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdPassRolesUnderscoreLast)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1.789_0", id, L"", false, L"V4");
        VERIFY_ARE_EQUAL(L"2afe22c982a406cb89635cde5e1d9e34", id.ToString());
    }

    BOOST_AUTO_TEST_CASE(GenerateV4NodeIdPassRolesDotLast)
    {
        NodeId id;
        NodeIdGenerator::GenerateFromString(L"WorkerRole1_789.0", id, L"", false, L"V4");
        VERIFY_ARE_EQUAL(L"43e063a71a23020814387721b1699bd3", id.ToString());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
