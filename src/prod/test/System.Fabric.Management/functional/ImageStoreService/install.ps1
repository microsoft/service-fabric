# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

Copy-ServiceFabricApplicationPackage -ApplicationPackagePath .\ImageStoreTestServicePkg -ApplicationPackagePathInImageStore ImageStoreTestServicePkg -ShowProgress
Register-ServiceFabricApplicationType ImageStoreTestServicePkg
Get-ServiceFabricApplicationType
New-ServiceFabricApplication fabric:/ImageStoreservice ImageStoreStatelessServiceType 1.0.0