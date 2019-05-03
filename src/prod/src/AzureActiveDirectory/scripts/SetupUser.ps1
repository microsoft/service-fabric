# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

<#
.VERSION
1.0.3

.SYNOPSIS
Setup user in a Service Fabric cluster Azure Active Directory tenant.

.DESCRIPTION
This script can create 2 types of users: Admin user assigned admin app role; Read-only user assigned readonly app role.

.PREREQUISITE
1. An Azure Active Directory Tenant
2. Service Fabric web and native client applications are setup. Or run SetupApplications.ps1.

.PARAMETER TenantId
ID of tenant hosting Service Fabric cluster.

.PARAMETER WebApplicationId
ObjectId of web application representing Service Fabric cluster.

.PARAMETER UserName,
Username of new user.

.PARAMETER Password
Password of new user.

.PARAMETER IsAdmin
User is assigned admin app role if indicated; otherwise, readonly app role.

.PARAMETER ConfigObj
Temporary variable of tenant setup result returned by SetupApplications.ps1.

.PARAMETER Location
Used to set metadata for specific region: china. Ignore it in global environment.

.EXAMPLE
<#[SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Example command, not a real password")]#>
. Scripts\SetupUser.ps1 -ConfigObj $ConfigObj -UserName 'SFuser' -Password '<password>'

Setup up a read-only user with return SetupApplications.ps1

.EXAMPLE
<#[SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Example command, not a real password")]#>
. Scripts\SetupUser.ps1 -TenantId '7b25ab7e-cd25-4f0c-be06-939424dc9cc9' -WebApplicationId '9bf7c6f3-53ce-4c63-8ab3-928c7bf4200b' -UserName 'SFAdmin' -Password '<password>' -IsAdmin

Setup up an admin user providing values for parameters
#>

Param
(
    [Parameter(ParameterSetName='Setting',Mandatory=$true)]
    [String]
	$TenantId,
	
    [Parameter(ParameterSetName='Setting',Mandatory=$true)]
	[String]
	$WebApplicationId,

    [Parameter(ParameterSetName='Setting')]
    [Parameter(ParameterSetName='ConfigObj')]
	[String]
	$UserName,

    [Parameter(ParameterSetName='Setting',Mandatory=$true)]
    [Parameter(ParameterSetName='ConfigObj',Mandatory=$true)]
	[String]
	$Password,

    [Parameter(ParameterSetName='Setting')]
    [Parameter(ParameterSetName='ConfigObj')]
    [Switch]
    $IsAdmin,

    [Parameter(ParameterSetName='ConfigObj',Mandatory=$true)]
    [Hashtable]
    $ConfigObj,

    [Parameter(ParameterSetName='Setting')]
    [Parameter(ParameterSetName='ConfigObj')]
    [ValidateSet('china')]
    [String]
    $Location
)

if($ConfigObj)
{
    $TenantId = $ConfigObj.TenantId
}

Write-Host 'TenantId = ' $TenantId

. "$PSScriptRoot\Common.ps1"

$graphAPIFormat = $resourceUrl + "/" + $TenantId + "/{0}?api-version=1.5{1}"

$uri = [string]::Format($graphAPIFormat, "tenantDetails", "")
$domain = (Invoke-RestMethod $uri -Headers $headers).value.verifiedDomains[0].name

$servicePrincipalId = ""
if($ConfigObj)
{
    $WebApplicationId = $ConfigObj.WebAppId
    $servicePrincipalId = $ConfigObj.ServicePrincipalId
}
else
{
    $uri = [string]::Format($graphAPIFormat, "servicePrincipals", [string]::Format('&$filter=appId eq ''{0}''', $WebApplicationId))
    $servicePrincipalId = (Invoke-RestMethod $uri -Headers $headers -ContentType "application/json").value.objectId
    AssertNotNull $servicePrincipalId 'Service principal of web application is not found'
}

$uri = [string]::Format($graphAPIFormat, "applications", [string]::Format('&$filter=appId eq ''{0}''', $WebApplicationId))
$appRoles = (Invoke-RestMethod $uri -Headers $headers -ContentType "application/json").value.appRoles
AssertNotNull $appRoles 'AppRoles of web application is not found'

if (!$UserName)
{
    if($IsAdmin)
    {
	    $UserName = 'ServiceFabricAdmin'
    }
    else
    {
        $UserName = 'ServiceFabricUser'
    }
}

#Create User
$roleId = @{}
$userId = ""
$uri = [string]::Format($graphAPIFormat, "users", "")
$newUser = @{
        accountEnabled = "true"
        displayName = $UserName
        passwordProfile = @{
            password = $Password
            forceChangePasswordNextLogin = "false"
        }
        mailNickname = $UserName
        userPrincipalName = [string]::Format("{0}@{1}", $UserName, $domain)
    }
#Admin
if($IsAdmin)
{
    Write-Host 'Creating Admin User: Name = ' $UserName 'Password = ' $Password
    $userId = (CallGraphAPI $uri $headers $newUser).objectId
    AssertNotNull $userId 'Admin User Creation Failed'
    Write-Host 'Admin User Created:' $userId
    $roleId = foreach ($appRole in $appRoles) 
    {
        if($appRole.value -eq "Admin") 
        {
            $appRole.id
            break
        }
    }
}
#Read-Only User
else{
    Write-Host 'Creating Read-Only User: Name = ' $UserName 'Password = ' $Password
    $userId = (CallGraphAPI $uri $headers $newUser).objectId
    AssertNotNull $userId 'Read-Only User Creation Failed'
    Write-Host 'Read-Only User Created:' $userId
    $roleId = foreach ($appRole in $appRoles) 
    {
        if($appRole.value -eq "User") 
        {
            $appRole.id
            break
        }
    }
}

#User Role
$uri = [string]::Format($graphAPIFormat, [string]::Format("users/{0}/appRoleAssignments", $userId), "")
$appRoleAssignments = @{
    id = $roleId
    principalId = $userId
    principalType = "User"
    resourceId = $servicePrincipalId
}
CallGraphAPI $uri $headers $appRoleAssignments | Out-Null