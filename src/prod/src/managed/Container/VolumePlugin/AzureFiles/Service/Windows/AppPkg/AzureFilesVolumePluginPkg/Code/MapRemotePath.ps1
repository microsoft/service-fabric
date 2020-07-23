param
(
    [Parameter(Mandatory=$True)]
    [string] $RemotePath,

    [Parameter(Mandatory=$False)]
    [string] $StorageAccountName = "",

    [Parameter(Mandatory=$False)]
    [string] $StorageAccountKey = "",

    [Parameter(Mandatory=$True)]
    [string] $LocalPath
)

function getTimestamp
{
    return $(Get-Date).ToUniversalTime().ToString("MM/dd/yyyy HH:mm:ss.fff")
}

try
{
    if ($StorageAccountKey -ne "")
    {
        $password = ConvertTo-SecureString -String $StorageAccountKey -AsPlainText -Force
        $credential = New-Object -TypeName "System.Management.Automation.PSCredential" -ArgumentList $StorageAccountName\$StorageAccountName, $password
        New-SmbGlobalMapping -RemotePath $RemotePath -Credential $credential -ErrorAction Stop
        Write-Host "$(getTimestamp) - Mapped $RemotePath."
    }
}
catch
{
    Write-Host "$(getTimestamp) - Failed to map $RemotePath. $($_.Exception)"
    exit 1
}

try
{
    $symLinkExists = Test-Path -Path $LocalPath
    if ($False -eq $symLinkExists)
    {
        New-Item -ItemType SymbolicLink -Path $LocalPath -Value $RemotePath -ErrorAction Stop
        Write-Host "$(getTimestamp) - Created symbolic link $LocalPath to $RemotePath"
    }
    else
    {
        Write-Host "$(getTimestamp) - Symbolic link $LocalPath already exists"
    }
}
catch
{
    Write-Host "$(getTimestamp) - Failed to create symbolic link $LocalPath to $RemotePath. $($_.Exception)"
    exit 1
}
