# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Copyright (c) Microsoft Corporation. All rights reserved.

# Condition: 
# 1. Run as administrator
# 2. The WF runtime and SDK is installed (manulally or from Install-WinFabDrop.ps1)
param ($clusterManifestPath = ".\DevEnv-FiveNodes-WithRM.xml")


if (!(Test-Path $clusterManifestPath))
{
    throw "Cluster Manifest is not found at given path '$clusterManifestPath'"
}

$connectionEndpoint = "localhost:19000"

Stop-Service FabricHostSvc
$ErrorActionPreference = "Stop"

New-ServiceFabricNodeConfiguration -ClusterManifestPath $clusterManifestPath
Start-Service FabricHostSvc

Write-Host "Waiting for cluster to be online"
(1..30) |% {
    Start-Sleep -Milliseconds 500
    Write-Host -NoNewLine "."
}

Connect-ServiceFabricCluster $connectionEndpoint
Get-ServiceFabricClusterConnection
Test-ServiceFabricClusterConnection