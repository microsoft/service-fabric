# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

<#
.VERSION
1.0.3

.SYNOPSIS
Cleanup applications in Azure Active Directory Tenant.

.PARAMETER TenantId
ID of tenant hosting Service Fabric cluster.

.PARAMETER WebApplicationName
Name of web application representing Service Fabric cluster to be deleted.

.PARAMETER NativeClientApplicationName
Name of native client application representing client to be deleted.

.PARAMETER ClusterName
A friendly Service Fabric cluster name. Applications whose names starting with ClusterName will be cleaned up.

.PARAMETER CleanupUsers
Cleanup application related users if indicated.

.PARAMETER Location
Used to set metadata for specific region: china. Ignore it in global environment.

.EXAMPLE
. Scripts\CleanupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -ClusterName 'MyCluster'

Cleanup applications with default settings generated from a friendly cluster name.

.EXAMPLE
. Scripts\CleanupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -ClusterName 'MyCluster' -CleanupUsers

Cleanup applications and users with default settings generated from a friendly cluster name.

.EXAMPLE
. Scripts\CleanupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -WebApplicationName 'SFWeb' -NativeClientApplicationName 'SFnative'

Cleanup tenant with explicit application settings.
#>

Param
(
    [Parameter(ParameterSetName='Customize',Mandatory=$true)]
    [Parameter(ParameterSetName='Prefix',Mandatory=$true)]
    [String]
	$TenantId,

    [Parameter(ParameterSetName='Customize')]	
	[String]
	$WebApplicationName,
	
    [Parameter(ParameterSetName='Customize')]
	[String]
	$NativeClientApplicationName,

    [Parameter(ParameterSetName='Prefix',Mandatory=$true)]
    [String]
    $ClusterName,

    [Parameter(ParameterSetName='Customize')]
    [Parameter(ParameterSetName='Prefix')]
    [Switch]
    $CleanupUsers,

    [Parameter(ParameterSetName='Prefix')]
    [Parameter(ParameterSetName='Customize')]
    [ValidateSet('china')]
    [String]
    $Location
)

Write-Host 'TenantId = ' $TenantId

. "$PSScriptRoot\Common.ps1"

$graphAPIFormat = $resourceUrl + "/" + $TenantId + "/{0}?api-version=1.5{1}"

$apps = @()

if($WebApplicationName)
{
    $uri = [string]::Format($graphAPIFormat, "applications", [string]::Format('&$filter=displayName eq ''{0}''', $WebApplicationName))
    $apps += (Invoke-RestMethod $uri -Headers $headers).value
    AssertNotNull $apps 'Cluster application is not found'
}

if($NativeClientApplicationName)
{
    $uri = [string]::Format($graphAPIFormat, "applications", [string]::Format('&$filter=displayName eq ''{0}''', $NativeClientApplicationName))
    $apps += (Invoke-RestMethod $uri -Headers $headers).value
    AssertNotNull $apps 'Native Client application is not found'
}

foreach($app in $apps)
{
    #Cleanup Users
    if($CleanupUsers)
    {
        #service principal
        $uri = [string]::Format($graphAPIFormat, "servicePrincipals", [string]::Format('&$filter=appId eq ''{0}''', $app.appId))
        $servicePrincipalId = (Invoke-RestMethod $uri -Headers $headers).value.objectId

        if($servicePrincipalId -ne $null){
            $uri = [string]::Format($graphAPIFormat, [string]::Format("servicePrincipals/{0}/appRoleAssignedTo", $servicePrincipalId), "")
            foreach($appRole in (Invoke-RestMethod $uri -Headers $headers).value){
                $userObjectId = $appRole.principalId
                $args = @()
                $args += ("-TenantId", $TenantId)
                $args += ("-UserObjectId", $userObjectId)
                Invoke-Expression "$PSScriptRoot\CleanupUser.ps1 $args"
            }
        }
    }

    Write-Host 'Deleting Application objectId = '$app.objectId
    $uri = [string]::Format($graphAPIFormat, [string]::Format("applications/{0}",$app.objectId), "")
    Invoke-RestMethod $uri -Method DELETE -Headers $headers | Out-Null
    Write-Host 'Application objectId= ' $app.objectId 'Name = ' $app.displayName ' is deleted'
}