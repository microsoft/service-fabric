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
                case Enum::Default: w << "Default"; return;
                case Enum::Automatic: w << "Automatic"; return;
                case Enum::Manual: w << "Manual"; return;
                };

                w << "ApplicationPackageCleanupPolicy(" << static_cast<int>(e) << ')';
            }

            FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY ToPublicApi(Enum const & toConvert)
            {
                switch (toConvert)
                {
                case Enum::Default:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_DEFAULT;

                case Enum::Automatic:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_AUTOMATIC;

                case Enum::Manual:
                    return FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_MANUAL;

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
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_DEFAULT:
                    return Enum::Default;
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_AUTOMATIC:
                    return Enum::Automatic;
                case FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_MANUAL:
                    return Enum::Manual;
                default:
                    Assert::CodingError("Unknown FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY value {0}", (int)toConvert);
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE(ApplicationPackageCleanupPolicy)

            ADD_CASTED_ENUM_MAP_VALUE_RANGE(ApplicationPackageCleanupPolicy, Invalid, Manual)

            END_ENUM_STRUCTURED_TRACE(ApplicationPackageCleanupPolicy)
        }
    }
}
