# Usage: .\SyncToBlob.ps1 [-Overwrite]
# This script can be run from any directory.
# Uploads all *.nupkg files from WindowsFabric\src\prod\tools\linux\nugets to the Azure storage blob used in build.
# Optionally, specify -Overwrite to overwrite nuget packages that already exist in the blob with the ones 
# located in this folder.

Param(
    [switch]$Overwrite
)
$storageAccountName="sfprebuiltinternal212418"
$location="westus"
$resourceGroup="sfoss"
$containerName="binaries"

$pkgList = Get-ChildItem $PSScriptRoot\nugets\*.nupkg

foreach ($pkg in $pkgList) {
    Write-Output "Uploading $($pkg.Name)..."
    $pkgPath=$pkg.FullName
    if ([string]::IsNullOrEmpty($(Get-AzureRmContext).Account)) {Login-AzureRmAccount}
    Select-AzureRmSubscription -SubscriptionName "Service Fabric Team - Engineering Systems"
    $storageAccount = Get-AzureRmStorageAccount -ResourceGroupName $resourceGroup -Name $storageAccountName
    $blob = Get-AzureStorageBlob -Blob "$($pkg.Name)" -Container $containerName -Context $storageAccount.Context -ErrorAction Ignore
    if ((-not $blob) -or $Overwrite)
    {
        Set-AzureStorageBlobContent -File $pkgPath -Container $containerName -Blob "$($pkg.Name)" -Context $storageAccount.Context
    }
}

