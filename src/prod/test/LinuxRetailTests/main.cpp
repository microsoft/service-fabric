// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

int TestFabricClient();
int TestFabricCommon();
int TestFabricRuntime();
int TestFabricInfrastructureService();
int TestKPhysicalLog();

int main () {
    TestFabricClient();
    TestFabricCommon();
    TestFabricRuntime();
    TestFabricInfrastructureService();
    TestKPhysicalLog();
}
