// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

FabricUpgradeRequest::FabricUpgradeRequest() 
    : useFabricInstallerService_(false),
    programToRun_(),
    arguments_(),
    downloadedFabricPackageLocation_()
{
}

FabricUpgradeRequest::FabricUpgradeRequest(
    bool const useFabricInstallerService,
    wstring const & programToRun,
    wstring const & arguments,
    wstring const & downloadedFabricPackageLocation)
    : useFabricInstallerService_(useFabricInstallerService),
    programToRun_(programToRun),
    arguments_(arguments),
    downloadedFabricPackageLocation_(downloadedFabricPackageLocation)
{
}

void FabricUpgradeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FabricUpgradeRequest { ");
    w.Write("UseFabricInstallerService = {0}", useFabricInstallerService_);
    w.Write("ProgramToRun = {0}", programToRun_);
    w.Write("Arguments = {0}", arguments_);
    w.Write("DownloadedFabricPackageLocation = {0}", downloadedFabricPackageLocation_);
    w.Write("}");
}
