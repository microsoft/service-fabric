# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

function LoadDll([string]$dllPath)
{
    [Reflection.Assembly]::LoadFrom($dllPath) > $null
    Get-Process | Out-File "C:\TestFolder\dllrun.txt"
    Start-Sleep -s 10
}

LoadDll $args[0]