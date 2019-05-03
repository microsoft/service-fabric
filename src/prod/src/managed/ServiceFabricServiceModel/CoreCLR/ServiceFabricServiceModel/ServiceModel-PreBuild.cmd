:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

%PkgDotNetFxGAC_Corext%\v4.0.30319\xsd.exe /c /n:ServiceFabricServiceModel ..\..\..\..\ServiceModel\xsd\ServiceFabricServiceModel.xsd
if exist ServiceFabricServiceModel.cs sed 's/\[System.SerializableAttribute^(^)\\]//g;s/\[System.ComponentModel.DesignerCategoryAttribute^(\\"code\\"^)\]//g' ServiceFabricServiceModel.cs > bin\ServiceFabricServiceModelTemp.cs
if exist bin\ServiceFabricServiceModelTemp.cs move /Y bin\ServiceFabricServiceModelTemp.cs ServiceFabricServiceModel.cs"