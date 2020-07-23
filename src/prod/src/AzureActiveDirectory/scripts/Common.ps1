# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

<#
.VERSION
1.0.5

.SYNOPSIS
Common script, do not call it directly.
#>

if($headers){
    Exit
}

Try
{
    $FilePath = Join-Path $PSScriptRoot "Microsoft.IdentityModel.Clients.ActiveDirectory.dll"
    Add-Type -Path $FilePath
}
Catch
{
    Write-Warning $_.Exception.Message
}

function GetRESTHeaders()
{
	# Use common client 
    $clientId = "1950a258-227b-4e31-a9cf-717495945fc2"
    $redirectUrl = "urn:ietf:wg:oauth:2.0:oob"
    
    $authenticationContext = New-Object Microsoft.IdentityModel.Clients.ActiveDirectory.AuthenticationContext -ArgumentList $authString, $FALSE

    $accessToken = $authenticationContext.AcquireToken($resourceUrl, $clientId, $redirectUrl, [Microsoft.IdentityModel.Clients.ActiveDirectory.PromptBehavior]::RefreshSession).AccessToken
    $headers = New-Object "System.Collections.Generic.Dictionary[[String],[String]]"
    $headers.Add("Authorization", $accessToken)
    return $headers
}

function CallGraphAPI_Patch($uri, $headers, $body)
{
    $json = $body | ConvertTo-Json -Depth 4 -Compress
    return (Invoke-RestMethod $uri -Method Patch -Headers $headers -Body $json -ContentType "application/json")
}

function CallGraphAPI($uri, $headers, $body)
{
    $json = $body | ConvertTo-Json -Depth 4 -Compress
    return (Invoke-RestMethod $uri -Method Post -Headers $headers -Body $json -ContentType "application/json")
}

function AssertNotNull($obj, $msg){
    if($obj -eq $null -or $obj.Length -eq 0){ 
        Write-Warning $msg
        Exit
    }
}

# Regional settings
switch ($Location)
{
    "china"
    {
        $resourceUrl = "https://graph.chinacloudapi.cn"
        $authString = "https://login.partner.microsoftonline.cn/" + $TenantId
    }
    
    "germany"
    {
        $resourceUrl = "https://graph.cloudapi.de"
        $authString = "https://login.microsoftonline.de/" + $TenantId   
    }

    default
    {
        $resourceUrl = "https://graph.windows.net"
        $authString = "https://login.microsoftonline.com/" + $TenantId
    }
}

$headers = GetRESTHeaders

if ($ClusterName)
{
    $WebApplicationName = $ClusterName + "_Cluster"
    $WebApplicationUri = "https://$ClusterName"
    $NativeClientApplicationName = $ClusterName + "_Client"
}
