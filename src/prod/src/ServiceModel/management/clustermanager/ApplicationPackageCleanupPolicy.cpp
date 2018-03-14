// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationPackageCleanupPolicy
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch (e)
                {
                case Enum::Invalid: w << "Invalid"; return;
                case Enum::ClusterDefault: w << "ClusterDefault"; return;
                case Enum::CleanupApplicationPackageOnSuccessfulProvision: w << "CleanupApplicationPackageOnSuccessfulProvision"; return;
                case Enum::NoCleanupApplicationPackageOnProvision: w << "NoCleanupApplicationPackageOnProvision"; return;
                };

                w << "ApplicationPackageCleanupPolicy(" << static_cast<int>(e) << ')';
            }

            FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY ToPublicApi(Enum const & toConvert)
            {
                switch (toConvert)
                {
                case Enum::ClusterDefault:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_CLUSTER_DEFAULT;

                case Enum::CleanupApplicationPackageOnSuccessfulProvision:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_CLEANUP_ON_SUCCESSFUL_PROVISION;

                case Enum::NoCleanupApplicationPackageOnProvision:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_NO_CLEANUP_ON_PROVISION;

                default:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_INVALID;
                }
            }

            Enum FromPublicApi(FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY const toConvert)
            {
                switch (toConvert)
                {
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_INVALID:
                    return Enum::Invalid;
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_CLUSTER_DEFAULT:
                    return Enum::ClusterDefault;
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_CLEANUP_ON_SUCCESSFUL_PROVISION:
                    return Enum::CleanupApplicationPackageOnSuccessfulProvision;
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_NO_CLEANUP_ON_PROVISION:
                    return Enum::NoCleanupApplicationPackageOnProvision;
                default:
                    Assert::CodingError("Unknown FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY value {0}", (int)toConvert);
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE(ApplicationPackageCleanupPolicy)

            ADD_CASTED_ENUM_MAP_VALUE_RANGE(ApplicationPackageCleanupPolicy, Invalid, NoCleanupApplicationPackageOnProvision)

            END_ENUM_STRUCTURED_TRACE(ApplicationPackageCleanupPolicy)
        }
    }
}
