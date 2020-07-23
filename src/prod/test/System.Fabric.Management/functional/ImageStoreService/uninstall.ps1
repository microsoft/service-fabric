# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

Remove-ServiceFabricApplication fabric:/ImageStoreservice -ForceRemove -Force
Unregister-ServiceFabricApplicationType ImageStoreStatelessServiceType 1.0.0 -Force
Remove-ServiceFabricApplicationPackage -ApplicationPackagePathInImageStore ImageStoreTestServicePkg