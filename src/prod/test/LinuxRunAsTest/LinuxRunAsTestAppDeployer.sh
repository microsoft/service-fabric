# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

azuresfcli servicefabric cluster connect http://127.0.0.1:10550
azuresfcli servicefabric application package copy --application-package-path LinuxRunAsTestApplication --image-store-connection-string fabric:ImageStore
azuresfcli servicefabric application type register --application-type-build-path LinuxRunAsTestApplication
azuresfcli servicefabric application create --application-name fabric:/LinuxRunAsTestApp  --application-type-name LinuxRunAsTestApplication  --application-type-version 1.0
~