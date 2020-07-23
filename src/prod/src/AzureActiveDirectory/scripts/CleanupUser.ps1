# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

<#
.VERSION
1.0.3

.SYNOPSIS
Cleanup user in Azure Active Directory Tenant

.PARAMETER TenantId
ID of tenant hosting Service Fabric cluster.

.PARAMETER UserName,
Username of user to be deleted.

.PARAMETER UserObjectId
Object ID of user to deleted.

.PARAMETER Location
Used to set metadata for specific region: china. Ignore it in global environment.

.EXAMPLE
. Scripts\CleanupUser.ps1 -TenantId '7b25ab7e-cd25-4f0c-be06-939424dc9cc9' -UserName 'SFAdmin'

Delete a user by providing username

.EXAMPLE
. Scripts\CleanupUser.ps1 -TenantId '7b25ab7e-cd25-4f0c-be06-939424dc9cc9' -UserObjectId '88ff3bbf-ea1a-4a14-aad9-807d109ac2eb'

Delete a user by providing objectId
#>

Param
(
    [Parameter(ParameterSetName='UserName',Mandatory=$true)]
    [Parameter(ParameterSetName='UserId',Mandatory=$true)]
    [String]
	$TenantId,

    [Parameter(ParameterSetName='UserName',Mandatory=$true)]
	[String]
	$UserName,

    [Parameter(ParameterSetName='UserId',Mandatory=$true)]
    [String]
    $UserObjectId,

    [Parameter(ParameterSetName='UserName')]
    [Parameter(ParameterSetName='UserId')]
    [ValidateSet('china')]
    [String]
    $Location
)

Write-Host 'TenantId = ' $TenantId

. "$PSScriptRoot\Common.ps1"

$graphAPIFormat = $resourceUrl + "/" + $TenantId + "/{0}?api-version=1.5{1}"

if($UserName)
{
    $uri = [string]::Format($graphAPIFormat, "users", [string]::Format('&$filter=displayName eq ''{0}''', $UserName))
    $UserObjectId = (Invoke-RestMethod $uri -Headers $headers).value.objectId
    AssertNotNull $UserObjectId 'User is not found'
}

Write-Host 'Deleting User objectId = '$UserObjectId
$uri = [string]::Format($graphAPIFormat, [string]::Format("users/{0}",$UserObjectId), "")
Invoke-RestMethod $uri -Method DELETE -Headers $headers | Out-Null
Write-Host 'User objectId= ' $UserObjectId ' is deleted'