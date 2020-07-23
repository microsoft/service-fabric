:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------


khttptests -get http://%1:8081/testurl/suffix1 foo1
type foo1
 khttptests -get http://%1:8081/testurl/suffix2/suffix3 foo2
type foo2
 khttptests -post http://%1:8081/testurl/suffix1 text/plain testfile0.xml bar1
type bar1
 khttptests -post http://%1:8081/testurl/suffix2/suffix3 text/plain testfile0.xml  bar2
type bar2
 khttptests -mppost http://%1:8081/testurl/suffix1 text/plain testfile0.xml bar1
type bar1
rem expect HTTP error 413
 khttptests -post http://%1:8081/testurl/suffix2/suffix3 text/plain XmlTests.exe  bar4

khttptests -advancedclient http://%1:8082/testurl 512
