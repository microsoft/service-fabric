# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

<#
.VERSION
1.0.6

.SYNOPSIS
Setup applications in a Service Fabric cluster Azure Active Directory tenant.

.PREREQUISITE
1. An Azure Active Directory tenant.
2. A Global Admin user within tenant.

.PARAMETER TenantId
ID of tenant hosting Service Fabric cluster.

.PARAMETER WebApplicationName
Name of web application representing Service Fabric cluster.

.PARAMETER WebApplicationUri
App ID URI of web application.

.PARAMETER WebApplicationReplyUrl
Reply URL of web application. Format: https://<Domain name of cluster>:<Service Fabric Http gateway port>

.PARAMETER NativeClientApplicationName
Name of native client application representing client.

.PARAMETER ClusterName
A friendly Service Fabric cluster name. Application settings generated from cluster name: WebApplicationName = ClusterName + "_Cluster", NativeClientApplicationName = ClusterName + "_Client"

.PARAMETER Location
Used to set metadata for specific region: china. Ignore it in global environment.

.PARAMETER AddResourceAccess
Used to add the cluster application's resource access to "Windows Azure Active Directory" application explicitly when AAD is not able to add automatically. This may happen when the user account does not have adequate permission under this subscription.

.EXAMPLE
. Scripts\SetupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -ClusterName 'MyCluster' -WebApplicationReplyUrl 'https://mycluster.westus.cloudapp.azure.com:19080'

Setup tenant with default settings generated from a friendly cluster name.

.EXAMPLE
. Scripts\SetupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -WebApplicationName 'SFWeb' -WebApplicationUri 'https://SFweb' -WebApplicationReplyUrl 'https://mycluster.westus.cloudapp.azure.com:19080' -NativeClientApplicationName 'SFnative'

Setup tenant with explicit application settings.

.EXAMPLE
. $ConfigObj = Scripts\SetupApplications.ps1 -TenantId '4f812c74-978b-4b0e-acf5-06ffca635c0e' -ClusterName 'MyCluster' -WebApplicationReplyUrl 'https://mycluster.westus.cloudapp.azure.com:19080'

Setup and save the setup result into a temporary variable to pass into SetupUser.ps1
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
	$WebApplicationUri,

    [Parameter(ParameterSetName='Customize',Mandatory=$true)]
    [Parameter(ParameterSetName='Prefix',Mandatory=$true)]
	[String]
    $WebApplicationReplyUrl,
	
    [Parameter(ParameterSetName='Customize')]
	[String]
	$NativeClientApplicationName,

    [Parameter(ParameterSetName='Prefix',Mandatory=$true)]
    [String]
    $ClusterName,

    [Parameter(ParameterSetName='Prefix')]
    [Parameter(ParameterSetName='Customize')]
    [ValidateSet('china','germany')]
    [String]
    $Location,

    [Parameter(ParameterSetName='Customize')]
    [Parameter(ParameterSetName='Prefix')]
    [Switch]
    $AddResourceAccess
)

Write-Host 'TenantId = ' $TenantId

. "$PSScriptRoot\Common.ps1"

$graphApiFormat = $resourceUrl + "/" + $TenantId + "/{0}?api-version=1.5"
$ConfigObj = @{}
$ConfigObj.TenantId = $TenantId

$readOnlyRoleId = [guid]::NewGuid()
$adminRoleId = [guid]::NewGuid()

$appRole = 
@{
    allowedMemberTypes = @("User", "Application")
    description = "ReadOnly roles have limited query access"
    displayName = "ReadOnly"
    id = $readOnlyRoleId
    isEnabled = "true"
    value = "User"
},
@{
    allowedMemberTypes = @("User", "Application")
    description = "Admins can manage roles and perform all task actions"
    displayName = "Admin"
    id = $adminRoleId
    isEnabled = "true"
    value = "Admin"
}

$requiredResourceAccess =
@(@{
    resourceAppId = "00000002-0000-0000-c000-000000000000"
    resourceAccess = @(@{
        id = "311a71cc-e848-46a1-bdf8-97ff7156d8e6"
        type= "Scope"
    })
})

if (!$WebApplicationName)
{
	$WebApplicationName = "ServiceFabricCluster"
}

if (!$WebApplicationUri)
{
	$WebApplicationUri = "https://ServiceFabricCluster"
}

if (!$NativeClientApplicationName)
{
	$NativeClientApplicationName =  "ServiceFabricClusterNativeClient"
}

#Create Web Application
$uri = [string]::Format($graphApiFormat, "applications")

$webApp = @{
    displayName = $WebApplicationName
    identifierUris = @($WebApplicationUri)
    homepage = $WebApplicationReplyUrl #Not functionally needed. Set by default to avoid AAD portal UI displaying error
    replyUrls = @($WebApplicationReplyUrl)
    appRoles = $appRole
}

switch ($Location)
{
    "china"
    {
        $oauth2Permissions = @(@{
            adminConsentDescription = "Allow the application to access " + $WebApplicationName + " on behalf of the signed-in user."
            adminConsentDisplayName = "Access " + $WebApplicationName
            id = [guid]::NewGuid()
            isEnabled = $true
            type = "User"
            userConsentDescription = "Allow the application to access " + $WebApplicationName + " on your behalf."
            userConsentDisplayName = "Access " + $WebApplicationName
            value = "user_impersonation"
        })
        $webApp.oauth2Permissions = $oauth2Permissions
    }
}

$webApp = CallGraphAPI $uri $headers $webApp
AssertNotNull $webApp 'Web Application Creation Failed'
$ConfigObj.WebAppId = $webApp.appId
Write-Host 'Web Application Created:' $webApp.appId

#Service Principal
$uri = [string]::Format($graphApiFormat, "servicePrincipals")
$servicePrincipal = @{
    accountEnabled = "true"
    appId = $webApp.appId
    displayName = $webApp.displayName
    appRoleAssignmentRequired = "true"
}
$servicePrincipal = CallGraphAPI $uri $headers $servicePrincipal
$ConfigObj.ServicePrincipalId = $servicePrincipal.objectId

#Add secret key support
$uri = [string]::Format($graphApiFormat, "applications/" + $webApp.objectId)

$webAppResourceAccess =
@{
    resourceAppId = $webApp.appId
    resourceAccess = @(
    @{
        id = $webApp.oauth2Permissions[0].id
        type= "Scope"
    },
    @{
        id = $adminRoleId
        type= "Role"
    })
}

$webAppUpdate =
@{
    requiredResourceAccess = @($webAppResourceAccess)
}

If($AddResourceAccess)
{
    $webAppUpdate.requiredResourceAccess += $requiredResourceAccess;
}

CallGraphAPI_Patch $uri $headers $webAppUpdate
Write-Host 'Web Application Updated:' $webApp.appId

#Create Native Client Application
$uri = [string]::Format($graphApiFormat, "applications")
$nativeAppResourceAccess = $requiredResourceAccess +=
@{
    resourceAppId = $webApp.appId
    resourceAccess = @(@{
        id = $webApp.oauth2Permissions[0].id
        type= "Scope"
    })
}
$nativeApp = @{
    publicClient = "true"
    displayName = $NativeClientApplicationName
    replyUrls = @("urn:ietf:wg:oauth:2.0:oob")
    requiredResourceAccess = $nativeAppResourceAccess
}
$nativeApp = CallGraphAPI $uri $headers $nativeApp
AssertNotNull $nativeApp 'Native Client Application Creation Failed'
Write-Host 'Native Client Application Created:' $nativeApp.appId
$ConfigObj.NativeClientAppId = $nativeApp.appId

#Service Principal
$uri = [string]::Format($graphApiFormat, "servicePrincipals")
$servicePrincipal = @{
    accountEnabled = "true"
    appId = $nativeApp.appId
    displayName = $nativeApp.displayName
}
$servicePrincipal = CallGraphAPI $uri $headers $servicePrincipal

#OAuth2PermissionGrant

#AAD service principal
$uri = [string]::Format($graphApiFormat, "servicePrincipals") + '&$filter=appId eq ''00000002-0000-0000-c000-000000000000'''
$AADServicePrincipalId = (Invoke-RestMethod $uri -Headers $headers).value.objectId

$uri = [string]::Format($graphApiFormat, "oauth2PermissionGrants")
$oauth2PermissionGrants = @{
    clientId = $servicePrincipal.objectId
    consentType = "AllPrincipals"
    resourceId = $AADServicePrincipalId
    scope = "User.Read"
    startTime = (Get-Date).ToUniversalTime().ToString("s")
    expiryTime = (Get-Date).AddYears(1800).ToUniversalTime().ToString("s")
}
CallGraphAPI $uri $headers $oauth2PermissionGrants | Out-Null
$oauth2PermissionGrants = @{
    clientId = $servicePrincipal.objectId
    consentType = "AllPrincipals"
    resourceId = $ConfigObj.ServicePrincipalId
    scope = "user_impersonation"
    startTime = (Get-Date).ToUniversalTime().ToString("s")
    expiryTime = (Get-Date).AddYears(1800).ToUniversalTime().ToString("s")
}
CallGraphAPI $uri $headers $oauth2PermissionGrants | Out-Null

$ConfigObj

#ARM template
Write-Host
Write-Host '-----ARM template-----'
Write-Host '"azureActiveDirectory": {'
Write-Host ("  `"tenantId`":`"{0}`"," -f $ConfigObj.TenantId)
Write-Host ("  `"clusterApplication`":`"{0}`"," -f $ConfigObj.WebAppId)
Write-Host ("  `"clientApplication`":`"{0}`"" -f $ConfigObj.NativeClientAppId)
Write-Host "},"
