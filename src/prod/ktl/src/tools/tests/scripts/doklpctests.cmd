:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

c:\debuggers\cdb -g -G klpctests.exe KLpcTest
pause
c:\debuggers\cdb -g -G klpctests.exe RemotingTests
pause
