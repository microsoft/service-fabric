# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Copyright (c) Microsoft Corporation. All rights reserved.

# Condition: Must be run inside Administrator Razzle
param ($dropLocation = "C:\WindowsFabric")


Stop-Service FabricHostSvc

Push-Location
$ErrorActionPreference = "Stop"

# Make the drops
cd (Join-Path $env:SDXROOT "services\winfab\prod\Setup\scripts")
if (Test-Path $dropLocation)
{
    Write-Host -Foreground Yellow "Remove old folder at drop location '$dropLocation'"
    Remove-Item -Recurse -Force $dropLocation
}
Write-Host -Foreground Yellow "Make the drop at location '$dropLocation'"
.\MakeDrop.cmd /dest:$dropLocation

Write-Host -Foreground Yellow "Creating Windows Fabric MSI..."
.\MakeMSI.cmd /drop:$dropLocation
$msiFile = Get-ChildItem $env:_NTTREE | ?{$_.Name.StartsWith("WindowsFabric.") -and ($_.Extension -eq ".msi") -and $_.LastWriteTime -gt ((Get-Date).AddMinutes(-5))}
Write-Host -Foreground Yellow "Windows Fabric MSI location: $($msiFile.FullName)"
Start-Process -Wait -FilePath msiexec -ArgumentList "/i $($msiFile.FullName) SIGNTESTDRIVER=\\winfabfs\public\Utilities"

Write-Host -Foreground Yellow "Creating Windows Fabric SDK"
.\MakeSDKMSI.cmd /drop:$dropLocation
$sdkFile = Get-ChildItem $env:_NTTREE | ?{$_.Name.StartsWith("WindowsFabricSDK.") -and ($_.Extension -eq ".msi") -and $_.LastWriteTime -gt ((Get-Date).AddMinutes(-5))}
Write-Host -Foreground Yellow "Windows Fabric SDK location: $($sdkFile.FullName)"
Start-Process -Wait -FilePath msiexec -ArgumentList "/i $($sdkFile.FullName) /log $dropLocation\msiinstall.log"

Pop-Location