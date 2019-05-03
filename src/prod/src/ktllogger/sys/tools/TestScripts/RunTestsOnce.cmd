:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

logman ktl stop
del /s/q c:\windows\ktl\*.etl
logman ktl start
del /s/q c:\rvdlog
c:\debuggers\cdb -g -G te /inproc KtlLogKernel.Test.dll   /name:%1

