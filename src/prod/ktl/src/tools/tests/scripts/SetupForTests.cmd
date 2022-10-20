:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

md c:\temp\Ktl\XmlTest
copy testfile0.xml    c:\temp\Ktl\XmlTest\testfile0.xml
md %windir%\rvd
copy hosts.txt %windir%\rvd