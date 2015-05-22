@echo off
setlocal

set VERSION=1.0.0

set OUTPUT=c:\NuGet\

%OUTPUT%nuget push %OUTPUT%Packages\MMaitre.IpCamera.%VERSION%.nupkg
%OUTPUT%nuget push %OUTPUT%Symbols\MMaitre.IpCamera.Symbols.%VERSION%.nupkg -Source http://nuget.gw.symbolsource.org/Public/NuGet