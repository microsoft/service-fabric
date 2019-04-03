# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Arguments for the entrypoint are declared at the very top
Param (
    # Port at which the VolumeDriver should listen for requests from the service
    [string]$Port = '40000',

    # Endpoint of the cluster to which to deploy the VolumeDriver
    [string]$ClusterEndpoint = 'localhost:19000',

    # Number of instances to be installed
    [string]$InstanceCount = '1',

    # Absolue path to the folder containing the VolumeDriver Application package (i.e. folder containing the ApplicationManifest.xml)
    [Parameter(Mandatory=$true)][string]$DeployFromPath,

    # Set to true if we are deploying the BlockStore service
    [bool]$DeployingService = $false
 )

# Returns a string representing the version of the ApplicationType passed in
function GetAppTypeVersion
{
    Param([string]$AppType)
    
    $refAppType = Get-ServiceFabricApplicationType -ApplicationTypeName $AppType
    if ($refAppType)
    {
        return $refAppType.ApplicationTypeVersion
    }
    else
    {
        return ""
    }
}

# Returns a boolean indicating if the specified Application exists in the cluster
function DoesApplicationExist
{
    Param([string]$ApplicationName)
    
    $refApp = Get-ServiceFabricApplication -ApplicationName $ApplicationName
    if ($refApp)
    {
        return $true
    }
    else
    {
        return $false
    }   
}

function SetupServicePackage
{
    Param([string]$ServicePackageLocation)

    # Create a new location in temp where we will create the Service Package
    $tempFolder = [System.IO.Path]::GetTempPath()
    [string] $servicePkgPath = [System.Guid]::NewGuid()
    [string] $servicePkgPath = $tempFolder+$servicePkgPath
    $objFolder = New-Item -ItemType Directory -Path ($servicePkgPath)

    # Copy the files to the target location
    # Copy-Item $ServicePackageLocation -Destination $servicePkgPath -Recurse
    Get-ChildItem -Path $ServicePackageLocation | % { 
        Copy-Item $_.fullname "$servicePkgPath" -Recurse -Force
      }

    # Create the Code package folder in the target location
    [string] $codePkgPath = Join-Path $servicePkgPath "SFBlockstoreServicePkg\\Code"
    $objFolder = New-Item -ItemType Directory -Path ($codePkgPath)

    Move-Item -Path (Join-Path $servicePkgPath "SFBlockStoreService.exe") -Destination $codePkgPath

    # Return the new path to be used
    Write-Host "Set up service package at $($servicePkgPath)"
    return $servicePkgPath
}

Write-Host "Connecting to cluster at $($ClusterEndpoint)"
$obj = Connect-ServiceFabricCluster -ConnectionEndpoint $ClusterEndpoint
$obj = $null

# Default value of various constants we are going to use
$AppTypeName = "ServiceFabricVolumeDriverType"
$AppName = "fabric:/ServiceFabricVolumeDriver"
$AppImageStorePath = "ServiceFabricVolumeDriverPath"

# Specify constant values for the service if we are deploying it.
if ($DeployingService)
{
    $AppTypeName = "ServiceFabricVolumeDiskType"
    $AppName = "fabric:/ServiceFabricVolumeDisk"
    $AppImageStorePath = "ServiceFabricVolumeDiskPath"

    $DeployFromPath = SetupServicePackage $DeployFromPath

    Write-Host "Deploying BlockStore service from $($DeployFromPath)"
}
else {
    Write-Host "Deploying VolumeDriver from $($DeployFromPath)"
}

# Remove the application if it exists
$doesAppExist = DoesApplicationExist $AppName
if ($doesAppExist)
{
    Write-Host "Removing existing instance of $($AppName)"
    Remove-ServiceFabricApplication -ApplicationName $AppName -Force
}
else
{
    Write-Host "Application instance does not exist."
}

# Remove the specified version of AppType if it exists
$versionToRemove = GetAppTypeVersion $AppTypeName
if ($versionToRemove)
{
    Write-Host "Unregistering application type version: $($versionToRemove)"
    Unregister-ServiceFabricApplicationType -ApplicationTypeName $AppTypeName -ApplicationTypeVersion $versionToRemove -Force 
}
else {
    Write-Host "Application type is not registered."
}

Write-Host "Uploading the package to the image store from: $($DeployFromPath)"
Copy-ServiceFabricApplicationPackage -ApplicationPackagePath $DeployFromPath -ImageStoreConnectionString "fabric:ImageStore" -ApplicationPackagePathInImageStore $AppImageStorePath

Write-Host "Registering the application type"
Register-ServiceFabricApplicationType -ApplicationPathInImageStore $AppImageStorePath

$versionToCreate = GetAppTypeVersion $AppTypeName
if ($versionToCreate)
{
    if (!$DeployingService)
    {
        Write-Host "Creating new application instance with version: $($versionToCreate), with instance count: $($InstanceCount) listening at port: $($Port)"
        New-ServiceFabricApplication -ApplicationName $AppName -ApplicationTypeName $AppTypeName -ApplicationTypeVersion $versionToCreate -ApplicationParameter @{InstanceCount=$InstanceCount; ListenPort=$Port}
    }
    else {
        Write-Host "Creating new application instance with version: $($versionToCreate), with instance count: $($InstanceCount)"
        New-ServiceFabricApplication -ApplicationName $AppName -ApplicationTypeName $AppTypeName -ApplicationTypeVersion $versionToCreate
    }
}
else {
    Write-Host "ERROR: Failed to create application instance due to inability to get version of the application type!"
}