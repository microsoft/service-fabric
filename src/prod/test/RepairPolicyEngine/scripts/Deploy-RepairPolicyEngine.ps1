# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Copyright (c) Microsoft Corporation. All rights reserved.

# Condition:
# 1. If no application package path is provided, the default value requires the script to be run inside Razzle
param (
    $applicationPackagePath = "$env:_NTTREE\RepairPolicyEngineApplication",

    $connectionEndpoint = "localhost:19000",

    [Switch]
    $Upgrade,

    [Switch]
    $Remove
)

$ErrorActionPreference = "Stop"

function Upgrade()
{
    # Start the monitored application upgrade.
    # Start-ServiceFabricApplicationUpgrade -ApplicationName $applicationName -ApplicationTypeVersion $applicationTypeVersion -Monitored –FailureAction Rollback –HealthCheckWaitDurationSec 20 –HealthCheckRetryTimeoutSec 20 –UpgradeDomainTimeoutSec 480

    # Start the un-monitored automatic application upgrade.
    Start-ServiceFabricApplicationUpgrade -ApplicationName $applicationName -ApplicationTypeVersion $applicationTypeVersion -UnmonitoredAuto

    Do 
    { 
        Start-Sleep -s 10
        $status = Get-ServiceFabricApplicationUpgrade -ApplicationName $applicationName
        Write-Host "Upgrade status: $($status.UpgradeState)"
    } 
    While (($status.UpgradeState -ne "RollingBackCompleted") -and ($status.UpgradeState -ne "RollingForwardCompleted"))
}

function Deploy()
{
    # Create new application instance.
    New-ServiceFabricApplication -applicationName $applicationName -ApplicationTypeName $applicationTypeName -ApplicationTypeVersion $applicationTypeVersion
}

function Cleanup()
{
    if (Get-ServiceFabricApplication $applicationName)
    {
        Remove-ServiceFabricApplication $applicationName -Force
    }
    Get-ServiceFabricApplicationType -ApplicationTypeName $applicationTypeName |% {
        Unregister-ServiceFabricApplicationType -ApplicationTypeName $applicationTypeName -ApplicationTypeVersion $_.applicationTypeVersion -Force
    }
}

if ([String]::IsNullOrEmpty($applicationPackagePath))
{
    if ([String]::IsNullOrEmpty($env:_NTTREE))
    {
        throw "If applicationPackagePath is not provided, the script must be run inside a Razzle environment after PolicyEngine was built successfully"
    }
}

$applicationNamePrefix = "fabric:/myapp"

# Establish a connection to the cluster.
Connect-ServiceFabricCluster $connectionEndpoint
Get-ServiceFabricClusterConnection
Test-ServiceFabricClusterConnection

# Read XML data from cluster manifest and application manifest
[Xml]$clusterManifestRoot = Get-ServiceFabricClusterManifest
$imageStoreConnection = (($clusterManifestRoot.ClusterManifest.FabricSettings.Section | ?{$_.Name -eq "Management"}).Parameter | ?{$_.Name -eq "ImageStoreConnectionString"}).Value

[Xml]$applicationManifestRoot = Get-Content (Join-Path $applicationPackagePath "ApplicationManifest.xml")
$applicationTypeName = $applicationManifestRoot.ApplicationManifest.ApplicationTypeName
$applicationTypeVersion = $applicationManifestRoot.ApplicationManifest.ApplicationTypeVersion
$applicationName = $applicationNamePrefix + "/" + $applicationTypeName
$applicationPathInImageStore = Join-Path "incoming" $applicationTypeName

$serviceTypeName = $applicationManifestRoot.ApplicationManifest.DefaultServices.Service.Name
$serviceName = $applicationName + "/" + $serviceTypeName

if (!$Upgrade)
{
    Cleanup
}

if ($Remove)
{
    return
}

# Test the application package.  
Test-ServiceFabricApplicationPackage -applicationPackagePath $applicationPackagePath

# Copy the application package to the ImageStore. 
Copy-ServiceFabricApplicationPackage -applicationPackagePath $applicationPackagePath -ImageStoreConnectionString $imageStoreConnection -ApplicationPackagePathInImageStore $applicationPathInImageStore

# Register, or provision, the application.  
Register-ServiceFabricApplicationType -ApplicationPathInImageStore $applicationPathInImageStore

if ($Upgrade)
{
    Upgrade
}
else 
{
    Deploy
}

# List the application.
Get-ServiceFabricApplication -ApplicationName $applicationName

# List the services from the application instance.
Get-ServiceFabricService -ApplicationName $applicationName