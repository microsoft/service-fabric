:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

IF NOT EXIST WdfCoInstaller01009.dll copy %windir%\system32\WdfCoInstaller01009.dll
c:\debuggers\cdb -g -G ..\taef\te testloader.dll /p:target=kernel /p:driver=%1Kernel /p:configfile=Table:%1TestInput.xml#Test /p:c=RunTests