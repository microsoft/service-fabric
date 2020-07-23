# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

$regpath = "HKLM:\System\CurrentControlSet\Services\HTTP\Parameters"
$keyname = "UriEnableCache"
$desired_value = 0
$value_type = "DWord"

$need_restart = $False
$value = (Get-ItemProperty -Path $regpath -Name $keyname -ErrorAction SilentlyContinue).$keyname

If ($value -eq $null) {
    # Key does not exist, create it
    New-ItemProperty -Path $regpath -Name $keyname -Value $desired_value -PropertyType $value_type | Out-Null
    $need_restart = $True

} Else {
    # Key exists, set value to 0 if it isn't already
    If ($value -ne $desired_value) {
        Set-ItemProperty -Path $regpath -Name $keyname -Value $desired_value
        $need_restart = $True
    }
}

if ($need_restart) {
    # OS Patch plugin will reboot automatically
    #Write-Host "Rebooting in 5 seconds..."
    #shutdown.exe /r /t 5 /c "$keyname has been set to $desired_value" /f /d p:2:4
}
