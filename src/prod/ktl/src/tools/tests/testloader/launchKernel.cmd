:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

TE.exe TestLoader.dll /miniDumpOnCrash /stackTraceOnError /inproc /unicodeOutput:false /p:dll=%1Kernel /p:c=RunTests /p:target=kernel /p:driver=%1Kernel /p:configfile=Table:%1TestInput.xml#Test