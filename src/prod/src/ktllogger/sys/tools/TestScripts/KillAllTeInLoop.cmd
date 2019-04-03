:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

:again
echo KillTe:%date%-%time% >> c:\timerunning.txt
\idw\kill -f te.exe
sleeprand %1
goto again
                            