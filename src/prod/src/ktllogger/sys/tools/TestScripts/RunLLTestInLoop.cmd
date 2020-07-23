:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

del c:\timerunning.txt
:again
logman ktl stop
del /s/q c:\windows\ktl\*.etl
logman ktl start
del /s/q c:\rvdlog
echo %1:%date%-%time% >> c:\timerunning.txt
c:\debuggers\cdb -g -G -o te /inproc LLWorkloadKernel.Test.dll   /name:KtlPhysicalLogTest::KtlLLWorkloadController::LLManyLogsTest /p:NumberConcurrentHosts=%1 /p:NumberLogicalLogs=%2 /p:TestDurationInSeconds=%3
goto again

