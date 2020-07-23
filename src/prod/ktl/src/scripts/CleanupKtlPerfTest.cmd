:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

WinFabric.TestInfra.TestTools.exe //disablecrashdump //uninstall //execute RunTests.exe /tests:KtlLogKernel.Test.dll /copyLogs:[LOGLOCATION],crash
WinFabric.TestInfra.PerfUploader.exe