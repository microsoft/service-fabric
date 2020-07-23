:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

start c:\debuggers\cdb -g -G  khttptests -server http://%1:8081/testurl 10000
start c:\debuggers\cdb -g -G  khttptests -advancedserver http://%1:8082/testurl 256 10000