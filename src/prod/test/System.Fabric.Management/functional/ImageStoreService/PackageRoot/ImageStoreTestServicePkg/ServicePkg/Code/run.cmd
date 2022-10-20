echo "_NTTREE is : " %_NTTREE% > %~dp0\out.out

if "%_NTTREE%" == "" (
    dotnet %~dp0\MS.Test.ImageStoreService.dll >> %~dp0\out.out
) else (
    %_NTTREE%\DotnetCoreCli\dotnet %~dp0\MS.Test.ImageStoreService.dll >> %~dp0\out.out
)
