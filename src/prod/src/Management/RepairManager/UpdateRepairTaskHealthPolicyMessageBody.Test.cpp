// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ServiceModel/ServiceModel.h"
#include "RepairManager.External.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

namespace Management
{
    namespace RepairManager
    {
        class UpdateRepairTaskHealthPolicyMessageBodyTest
        {
        };

        BOOST_FIXTURE_TEST_SUITE2(UpdateRepairTaskHealthPolicyMessageBodyTestSuite, UpdateRepairTaskHealthPolicyMessageBodyTest)

        BOOST_AUTO_TEST_CASE(JsonSerializeTest)
        {
            UpdateRepairTaskHealthPolicyMessageBody body(
                make_shared<ClusterRepairScopeIdentifier>(), 
                L"task1", 
                0, 
                make_shared<bool>(true), 
                make_shared<bool>(true));

            wstring serializedString;
            auto error = JsonHelper::Serialize(body, serializedString);
            VERIFY_IS_TRUE(error.IsSuccess());

            wstring expectedString = L"{\"Scope\":{\"Kind\":1},\"TaskId\":\"task1\",\"Version\":\"0\",\"PerformPreparingHealthCheck\":true,\"PerformRestoringHealthCheck\":true}";
            VERIFY_ARE_EQUAL(expectedString, serializedString);

            UpdateRepairTaskHealthPolicyMessageBody body1;
            error = JsonHelper::Deserialize(body1, serializedString);
            VERIFY_IS_TRUE(error.IsSuccess());
            
            VERIFY_ARE_EQUAL(body.TaskId, body1.TaskId);
            VERIFY_ARE_EQUAL(*body.Scope, *body1.Scope);
            VERIFY_ARE_EQUAL(body.Version, body1.Version);
            VERIFY_ARE_EQUAL(*body.PerformPreparingHealthCheck, *body1.PerformPreparingHealthCheck);
            VERIFY_ARE_EQUAL(*body.PerformRestoringHealthCheck, *body1.PerformRestoringHealthCheck);
        }

        BOOST_AUTO_TEST_SUITE_END()
    }
}
