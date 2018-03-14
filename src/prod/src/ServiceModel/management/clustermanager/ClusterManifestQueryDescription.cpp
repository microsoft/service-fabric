// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ClusterManifestQueryDescription");

ClusterManifestQueryDescription::ClusterManifestQueryDescription() 
    : version_()
{
}

ClusterManifestQueryDescription::ClusterManifestQueryDescription(FabricConfigVersion && version) 
    : version_(move(version))
{
}

ClusterManifestQueryDescription::ClusterManifestQueryDescription(ClusterManifestQueryDescription && other) 
    : version_(move(other.version_))
{
}

ErrorCode ClusterManifestQueryDescription::FromPublicApi(FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION const & publicDescription)
{
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL2( publicDescription.ClusterManifestVersion, versionString );

    return FabricConfigVersion::FromString(versionString, version_);
}
