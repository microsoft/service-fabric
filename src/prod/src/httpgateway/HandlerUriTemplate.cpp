// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace HttpGateway
{
    double HandlerUriTemplate::GetDefaultMaxApiVersion()
    {
        double ver = 0.0;

        std::wstring currentVersion;
        auto error = FabricEnvironment::GetFabricVersion(currentVersion);
        if (!error.IsSuccess())
        {
            Assert::CodingError(
                "Error {0} when fetching version number from FabricEnvironment::GetFabricVersion. Setup corrupted, do not proceed.",
                error);
        }

        vector<wstring> versionVec;
        StringUtility::Split<wstring>(currentVersion, versionVec, L".");
        if (versionVec.size() != 4)
        {
            Assert::CodingError(
                "Error splitting the cluster version string {0}.Setup corrupted, do not proceed.",
                currentVersion);
        }

        auto versionStr = wformatString("{0}.{1}", versionVec[0], versionVec[1]);
        ver = std::stod(versionStr);

        return ver;
    }

    double HandlerUriTemplate::defaultMaxApiVersion_ = HandlerUriTemplate::GetDefaultMaxApiVersion();
}
