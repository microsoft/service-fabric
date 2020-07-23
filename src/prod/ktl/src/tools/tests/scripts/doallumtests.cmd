:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

c:\debuggers\cdb -g -G KAllocationBitmapTests.exe
c:\debuggers\cdb -g -G KMemChannelTests.exe
c:\debuggers\cdb -g -G KAsyncQueueTests.exe
c:\debuggers\cdb -g -G KBitmapTests.exe
c:\debuggers\cdb -g -G KBlockFileTests.exe
c:\debuggers\cdb -g -G KBufferTests.exe
c:\debuggers\cdb -g -G KChecksumTests.exe
c:\debuggers\cdb -g -G KDeferredCallTests.exe
c:\debuggers\cdb -g -G KHashTableTests.exe
c:\debuggers\cdb -g -G KInvariantTests.exe
c:\debuggers\cdb -g -G KIoBufferTests.exe
c:\debuggers\cdb -g -G KNetChannelTests.exe
c:\debuggers\cdb -g -G KNetFaultInjectorTests.exe
c:\debuggers\cdb -g -G KNetworkEndpointTests.exe
c:\debuggers\cdb -g -G KNodeTableTests.exe
c:\debuggers\cdb -g -G KPerfCounterTests.exe
c:\debuggers\cdb -g -G KRegistryTests.exe
c:\debuggers\cdb -g -G KRTTTests.exe
c:\debuggers\cdb -g -G KSerialTests.exe
c:\debuggers\cdb -g -G KStringViewTests.exe
c:\debuggers\cdb -g -G KTaskTests.exe
c:\debuggers\cdb -g -G KThreadPoolTests.exe
c:\debuggers\cdb -g -G KThreadTests.exe
c:\debuggers\cdb -g -G KtlCommonTests.exe
c:\debuggers\cdb -g -G KUriTests.exe
c:\debuggers\cdb -g -G KVariantTests.exe
c:\debuggers\cdb -g -G KVolumeNamespaceTests.exe
c:\debuggers\cdb -g -G KWeakRefTests.exe
c:\debuggers\cdb -g -G KWStringTests.exe
c:\debuggers\cdb -g -G KAsyncContextTests.exe

call dorvdloggertest.cmd
call doxmltests.cmd
call donetworktests.cmd
