param
(
    [Parameter(Mandatory=$True)]
    [string] $RemotePath = "",

    [Parameter(Mandatory=$True)]
    [string] $LocalPath
)

function getTimestamp
{
    return $(Get-Date).ToUniversalTime().ToString("MM/dd/yyyy HH:mm:ss.fff")
}

cmd /C rmdir $LocalPath
if ($LASTEXITCODE -ne 0)
{
    Write-Host "$(getTimestamp) - Failed to deleted symbolic link $LocalPath to $RemotePath. Error $($LASTEXITCODE)."
    exit 1
}
else
{
    Write-Host "$(getTimestamp) - Deleted symbolic link $LocalPath to $RemotePath"
}

try
{
    if ($RemotePath -ne "")
    {
        Remove-SmbGlobalMapping -RemotePath $RemotePath -Force -ErrorAction Stop
        Write-Host "$(getTimestamp) - Unmapped $RemotePath"
    }
}
catch
{
    Write-Host "$(getTimestamp) - Failed to unmap $RemotePath. $($_.Exception)"
    exit 1
}