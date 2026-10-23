Error: Set symbol path attempts to access 'G:\C++\ArcMeta\ArcMeta\ArcMeta' failed: 0x2 - 系统找不到指定的文件。

************* Path validation summary **************
Response                         Time (ms)     Location
Deferred                                       SRV*
Error                                          G:\C++\ArcMeta\ArcMeta\ArcMeta

************* Preparing the environment for Debugger Extensions Gallery repositories **************
   ExtensionRepository : Implicit
   UseExperimentalFeatureForNugetShare : true
   AllowNugetExeUpdate : true
   NonInteractiveNuget : true
   AllowNugetMSCredentialProviderInstall : true
   AllowParallelInitializationOfLocalRepositories : true
   EnableRedirectToChakraJsProvider : false

   -- Configuring repositories
      ----> Repository : LocalInstalled, Enabled: true
      ----> Repository : UserExtensions, Enabled: true

>>>>>>>>>>>>> Preparing the environment for Debugger Extensions Gallery repositories completed, duration 0.000 seconds

************* Waiting for Debugger Extensions Gallery to Initialize **************

>>>>>>>>>>>>> Waiting for Debugger Extensions Gallery to Initialize completed, duration 0.610 seconds
   ----> Repository : UserExtensions, Enabled: true, Packages count: 0
   ----> Repository : LocalInstalled, Enabled: true, Packages count: 46

Microsoft (R) Windows Debugger Version 10.0.29547.1002 AMD64
Copyright (c) Microsoft Corporation. All rights reserved.


Loading Dump File [C:\Users\fachu\AppData\Local\CrashDumps\QuarkMeta.exe.19204.dmp]
User Mini Dump File: Only registers, stack and portions of memory are available

Error: Change all symbol paths attempts to access 'G:\C++\ArcMeta\ArcMeta\ArcMeta' failed: 0x2 - 系统找不到指定的文件。

************* Path validation summary **************
Response                         Time (ms)     Location
Deferred                                       SRV*
Error                                          G:\C++\ArcMeta\ArcMeta\ArcMeta
Symbol search path is: SRV*;G:\C++\ArcMeta\ArcMeta\ArcMeta
Executable search path is: 
Windows 10 Version 19045 MP (8 procs) Free x64
Product: WinNt, suite: SingleUserTS
Edition build lab: 19041.1.amd64fre.vb_release.191206-1406
Debug session time: Wed Oct  7 21:42:52.000 2026 (UTC + 7:00)
System Uptime: not available
Process Uptime: 0 days 0:00:21.000
................................................................
...........................................
Loading unloaded module list
...
This dump file has an exception of interest stored in it.
The stored exception information can be accessed via .ecxr
(4b04.3da0): Access violation - code c0000005 (first/second chance not available)
ReadVirtual() failed in GetXStateConfiguration() first read attempt (error == 0.)
Invalid mini-dump Miscellaneous information in InitializeConfiguration()
For analysis of this file, run !analyze -v
ntdll!NtWaitForMultipleObjects+0x14:
00007ff8`8b7ee0f4 c3              ret
0:000> .ecxr
rax=0000000000000067 rbx=000000000000002f rcx=0000000100000010
rdx=000000000000002f rsi=0000023235dd1f00 rdi=0000000100000000
rip=00007ff830b269a2 rsp=00000096f0afa1a8 rbp=00000096f0afa340
 r8=00000001000000ce  r9=0000000100000000 r10=0000000100000000
r11=0000000100000000 r12=00000232318a5448 r13=0000000000000000
r14=000002322fa63620 r15=0000023235dd1f00
iopl=0         nv up ei ng nz ac pe cy
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010297
Qt6Core!QtPrivate::qustrchr+0x32:
00007ff8`30b269a2 f3410f6f01      movdqu  xmm0,xmmword ptr [r9] ds:00000001`00000000=????????????????????????????????
0:000> !analyze -v
................................................................
...........................................
Loading unloaded module list
...
*******************************************************************************
*                                                                             *
*                        Exception Analysis                                   *
*                                                                             *
*******************************************************************************

*** WARNING: Unable to verify checksum for QuarkMeta.exe

KEY_VALUES_STRING: 1

    Key  : AV.Type
    Value: Read

    Key  : Analysis.CPU.mSec
    Value: 890

    Key  : Analysis.Elapsed.mSec
    Value: 4708

    Key  : Analysis.IO.Other.Mb
    Value: 0

    Key  : Analysis.IO.Read.Mb
    Value: 1

    Key  : Analysis.IO.Write.Mb
    Value: 0

    Key  : Analysis.Init.CPU.mSec
    Value: 562

    Key  : Analysis.Init.Elapsed.mSec
    Value: 34483

    Key  : Analysis.Memory.CommitPeak.Mb
    Value: 133

    Key  : Analysis.Version.DbgEng
    Value: 10.0.29547.1002

    Key  : Analysis.Version.Description
    Value: 10.2602.27.2 amd64fre

    Key  : Analysis.Version.Ext
    Value: 1.2602.27.2

    Key  : Failure.Bucket
    Value: INVALID_POINTER_READ_c0000005_Qt6Core.dll!Unknown

    Key  : Failure.Exception.Code
    Value: 0xc0000005

    Key  : Failure.Exception.IP.Address
    Value: 0x7ff830b269a2

    Key  : Failure.Exception.IP.Module
    Value: Qt6Core

    Key  : Failure.Exception.IP.Offset
    Value: 0x1869a2

    Key  : Failure.Hash
    Value: {11f85110-02eb-30ad-d320-ae4675a7890a}

    Key  : Failure.ProblemClass.Primary
    Value: INVALID_POINTER_READ

    Key  : Faulting.IP.Type
    Value: Paged

    Key  : Timeline.Process.Start.DeltaSec
    Value: 21

    Key  : WER.OS.Branch
    Value: vb_release

    Key  : WER.OS.Version
    Value: 10.0.19041.1


FILE_IN_CAB:  QuarkMeta.exe.19204.dmp

NTGLOBALFLAG:  0

APPLICATION_VERIFIER_FLAGS:  0

CONTEXT:  (.ecxr)
rax=0000000000000067 rbx=000000000000002f rcx=0000000100000010
rdx=000000000000002f rsi=0000023235dd1f00 rdi=0000000100000000
rip=00007ff830b269a2 rsp=00000096f0afa1a8 rbp=00000096f0afa340
 r8=00000001000000ce  r9=0000000100000000 r10=0000000100000000
r11=0000000100000000 r12=00000232318a5448 r13=0000000000000000
r14=000002322fa63620 r15=0000023235dd1f00
iopl=0         nv up ei ng nz ac pe cy
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010297
Qt6Core!QtPrivate::qustrchr+0x32:
00007ff8`30b269a2 f3410f6f01      movdqu  xmm0,xmmword ptr [r9] ds:00000001`00000000=????????????????????????????????
Resetting default scope

EXCEPTION_RECORD:  (.exr -1)
ExceptionAddress: 00007ff830b269a2 (Qt6Core!QtPrivate::qustrchr+0x0000000000000032)
   ExceptionCode: c0000005 (Access violation)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 0000000000000000
   Parameter[1]: 0000000100000000
Attempt to read from address 0000000100000000

PROCESS_NAME:  QuarkMeta.exe

READ_ADDRESS:  0000000100000000 

ERROR_CODE: (NTSTATUS) 0xc0000005 - 0x%p            0x%p                    %s

EXCEPTION_CODE_STR:  c0000005

EXCEPTION_PARAMETER1:  0000000000000000

EXCEPTION_PARAMETER2:  0000000100000000

STACK_TEXT:  
00000096`f0afa1a8 00007ff8`309b5303     : 00000001`000000ce 00000233`02000002 00000000`00000000 00007ff8`90002aba : Qt6Core!QtPrivate::qustrchr+0x32
00000096`f0afa1b0 00007ff8`309f1029     : 00000232`317d85c0 00000232`6e001a74 00000232`2cf79c20 00007ff8`8b76e249 : Qt6Core!QString::indexOf+0x63
00000096`f0afa1f0 00007ff7`022f559c     : 00000232`317d85c0 00000000`00000000 00000233`0a6635c0 00000233`09ba0000 : Qt6Core!QDir::toNativeSeparators+0x29
00000096`f0afa240 00007ff7`022428b5     : 00000233`0a6635e0 00000233`0a68e150 00000232`317d85c0 00000232`2cfe98d8 : QuarkMeta+0x21559c
00000096`f0afa690 00007ff7`0226ecdf     : 00000232`35dd1f00 00000000`00000000 00000000`00000000 00000232`319baa90 : QuarkMeta+0x1628b5
00000096`f0afa6c0 00007ff7`022737a1     : 00000096`f0afa801 00000000`00000000 00000233`09f56520 00000096`f0afa889 : QuarkMeta+0x18ecdf
00000096`f0afa780 00007ff8`30a8b60f     : 00000233`09c129b0 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta+0x1937a1
00000096`f0afa7b0 00007ff8`30a8e314     : 00000232`2cfe9760 00000232`00000008 00000000`00000007 00000232`356a1801 : Qt6Core!QObject::qt_static_metacall+0x15bf
00000096`f0afa8f0 00007ff7`020f668e     : 00000232`319f9000 00000232`356a0ff0 00000232`318a5448 00000000`00000001 : Qt6Core!QMetaObject::activate+0x84
00000096`f0afa920 00007ff7`0229a9fb     : 00000000`00000000 00000232`356a1820 00000232`356a1820 00000232`356a1820 : QuarkMeta+0x1668e
00000096`f0afa970 00007ff8`30a8b60f     : 00000000`00000000 00000096`f0afab81 00000000`00000000 00007ff8`38946a17 : QuarkMeta+0x1ba9fb
00000096`f0afaa90 00007ff8`30a8e314     : 00000232`356a1550 00007ff8`00000009 00000000`00000007 00000140`00000801 : Qt6Core!QObject::qt_static_metacall+0x15bf
00000096`f0afabd0 00007ff8`389f6047     : 00000232`35773f40 00000096`f0afad01 00000232`317d4390 0000000f`0000000f : Qt6Core!QMetaObject::activate+0x84
00000096`f0afac00 00007ff8`389f5ce8     : 00000232`35773f00 00000232`356a1550 00000232`35773f40 00000232`356a1550 : Qt6Widgets!QAbstractButton::clicked+0x327
00000096`f0afac40 00007ff8`389f728f     : 00000096`f0afb760 00000096`f0afad39 00000232`356a1550 00007ff8`3721e7f9 : Qt6Widgets!QAbstractButton::click+0x198
00000096`f0afac70 00007ff8`389297a0     : 00000004`0000000c 41dfffff`ffc00000 40290000`00000000 00000096`f0afad39 : Qt6Widgets!QAbstractButton::mouseReleaseEvent+0x13f
00000096`f0afacc0 00007ff8`388e3701     : 00000232`2cfb3250 00000232`2cff25a0 00000232`2cfb3250 00000232`356a1550 : Qt6Widgets!QWidget::event+0x160
00000096`f0afada0 00007ff8`388e18c4     : 00000096`f0afb760 00000096`f0afaed0 00000096`f0afb0d8 00000096`f0afb760 : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000096`f0afadd0 00007ff8`30a49c01     : 00000096`f0aff748 00000232`356a1550 00000096`f0afb760 00000232`00000000 : Qt6Widgets!QApplication::notify+0x634
00000096`f0afb270 00007ff8`388e6149     : 00000000`00000000 00007ff8`3721e7f9 00000000`ffffffff 00007ff8`3721361c : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000096`f0afb2e0 00007ff8`3895059b     : 00000000`00000001 00000000`00000000 00000096`f0afb500 00000000`00000000 : Qt6Widgets!QApplicationPrivate::sendMouseEvent+0x449
00000096`f0afb430 00007ff8`3894dc59     : 00000000`00000000 00000232`35853390 00000232`319cad10 00000232`319cad10 : Qt6Widgets!QWidgetRepaintManager::updateStaticContentsSize+0x384b
00000096`f0afb8b0 00007ff8`388e3701     : 00000232`2cfb3250 00000232`2cff25a0 00000096`f0afc160 00000232`319cad10 : Qt6Widgets!QWidgetRepaintManager::updateStaticContentsSize+0xf09
00000096`f0afb950 00007ff8`388e27fe     : 00000232`2cfb3250 00000096`f0afba80 00000096`f0afc100 00000096`f0afc160 : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000096`f0afb980 00007ff8`30a49c01     : 00000096`f0aff748 00000232`319cad10 00000096`f0afc160 00000232`00000000 : Qt6Widgets!QApplication::notify+0x156e
00000096`f0afbe20 00007ff8`3722f46a     : 00000000`00000000 00000233`0a005080 00000000`00000000 00007ff8`85037fe3 : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000096`f0afbe90 00007ff8`372908fb     : 00000000`000000a4 00000000`00000002 00000232`31a2a6e0 00000000`00000000 : Qt6Gui!QGuiApplicationPrivate::processMouseEvent+0x99a
00000096`f0afc3f0 00007ff8`30be91c0     : 00000232`3575e420 00000000`000000a4 00000096`f0afc520 00000000`0000000f : Qt6Gui!QWindowSystemInterface::sendWindowSystemEvents+0xfb
00000096`f0afc420 00007ff8`37555dd9     : 00000096`f0aff690 00000000`00000000 00000232`3575e420 00000232`2d199290 : Qt6Core!QEventDispatcherWin32::processEvents+0x90
00000096`f0aff5a0 00007ff8`30a50604     : 00000000`000000a4 00000232`3575e420 00000232`2cfb3250 00000000`00000000 : Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
00000096`f0aff5d0 00007ff8`30a4812a     : 00000096`f0aff690 00000232`2cf76940 00000232`2cf76940 00000096`f0aff7d0 : Qt6Core!QEventLoop::exec+0x1c4
00000096`f0aff670 00007ff7`0210298d     : 00007ff8`30d93370 00000096`f0aff7d0 00000000`00000000 00000000`00000000 : Qt6Core!QCoreApplication::exec+0x16a
00000096`f0aff6d0 00007ff7`0233c320     : 00000000`00000000 00007ff8`895e7830 00000000`00000001 00000000`00000000 : QuarkMeta+0x2298d
00000096`f0aff8d0 00007ff7`0233bd06     : 00000000`00000001 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta+0x25c320
00000096`f0aff960 00007ff8`8a5d7374     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta+0x25bd06
00000096`f0aff9a0 00007ff8`8b79cc91     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : kernel32!BaseThreadInitThunk+0x14
00000096`f0aff9d0 00000000`00000000     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : ntdll!RtlUserThreadStart+0x21


STACK_COMMAND: ~0s; .ecxr ; kb

IP_IN_PAGED_CODE: 
Qt6Core!QtPrivate::qustrchr+32
00007ff8`30b269a2 f3410f6f01      movdqu  xmm0,xmmword ptr [r9]

SYMBOL_NAME:  Qt6Core+1869a2

MODULE_NAME: Qt6Core

IMAGE_NAME:  Qt6Core.dll

FAILURE_BUCKET_ID:  INVALID_POINTER_READ_c0000005_Qt6Core.dll!Unknown

OS_VERSION:  10.0.19041.1

BUILDLAB_STR:  vb_release

OSPLATFORM_TYPE:  x64

OSNAME:  Windows 10

IMAGE_VERSION:  6.10.2.0

FAILURE_ID_HASH:  {11f85110-02eb-30ad-d320-ae4675a7890a}

Followup:     MachineOwner
---------

0:000> k
 # Child-SP          RetAddr               Call Site
00 00000096`f0af8cd8 00007ff8`88fe0d00     ntdll!NtWaitForMultipleObjects+0x14
01 00000096`f0af8ce0 00007ff8`88fe0bfe     KERNELBASE!WaitForMultipleObjectsEx+0xf0
02 00000096`f0af8fd0 00007ff8`8a631f5a     KERNELBASE!WaitForMultipleObjects+0xe
03 00000096`f0af9010 00007ff8`8a631996     kernel32!WerpReportFaultInternal+0x58a
04 00000096`f0af9130 00007ff8`890bc6f9     kernel32!WerpReportFault+0xbe
05 00000096`f0af9170 00007ff8`8b7f58d8     KERNELBASE!UnhandledExceptionFilter+0x3d9
06 00000096`f0af9290 00007ff8`8b7dce46     ntdll!RtlUserThreadStart$filt$0+0xa2
07 00000096`f0af92d0 00007ff8`8b7f296f     ntdll!_C_specific_handler+0x96
08 00000096`f0af9340 00007ff8`8b7a2554     ntdll!RtlpExecuteHandlerForException+0xf
09 00000096`f0af9370 00007ff8`8b7f147e     ntdll!RtlDispatchException+0x244
0a 00000096`f0af9a80 00007ff8`30b269a2     ntdll!KiUserExceptionDispatch+0x2e
0b 00000096`f0afa1a8 00007ff8`309b5303     Qt6Core!QtPrivate::qustrchr+0x32
0c 00000096`f0afa1b0 00007ff8`309f1029     Qt6Core!QString::indexOf+0x63
0d 00000096`f0afa1f0 00007ff7`022f559c     Qt6Core!QDir::toNativeSeparators+0x29
0e 00000096`f0afa240 00007ff7`022428b5     QuarkMeta+0x21559c
0f 00000096`f0afa690 00007ff7`0226ecdf     QuarkMeta+0x1628b5
10 00000096`f0afa6c0 00007ff7`022737a1     QuarkMeta+0x18ecdf
11 00000096`f0afa780 00007ff8`30a8b60f     QuarkMeta+0x1937a1
12 00000096`f0afa7b0 00007ff8`30a8e314     Qt6Core!QObject::qt_static_metacall+0x15bf
13 00000096`f0afa8f0 00007ff7`020f668e     Qt6Core!QMetaObject::activate+0x84
14 00000096`f0afa920 00007ff7`0229a9fb     QuarkMeta+0x1668e
15 00000096`f0afa970 00007ff8`30a8b60f     QuarkMeta+0x1ba9fb
16 00000096`f0afaa90 00007ff8`30a8e314     Qt6Core!QObject::qt_static_metacall+0x15bf
17 00000096`f0afabd0 00007ff8`389f6047     Qt6Core!QMetaObject::activate+0x84
18 00000096`f0afac00 00007ff8`389f5ce8     Qt6Widgets!QAbstractButton::clicked+0x327
19 00000096`f0afac40 00007ff8`389f728f     Qt6Widgets!QAbstractButton::click+0x198
1a 00000096`f0afac70 00007ff8`389297a0     Qt6Widgets!QAbstractButton::mouseReleaseEvent+0x13f
1b 00000096`f0afacc0 00007ff8`388e3701     Qt6Widgets!QWidget::event+0x160
1c 00000096`f0afada0 00007ff8`388e18c4     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
1d 00000096`f0afadd0 00007ff8`30a49c01     Qt6Widgets!QApplication::notify+0x634
1e 00000096`f0afb270 00007ff8`388e6149     Qt6Core!QCoreApplication::notifyInternal2+0x101
1f 00000096`f0afb2e0 00007ff8`3895059b     Qt6Widgets!QApplicationPrivate::sendMouseEvent+0x449
20 00000096`f0afb430 00007ff8`3894dc59     Qt6Widgets!QWidgetRepaintManager::updateStaticContentsSize+0x384b
21 00000096`f0afb8b0 00007ff8`388e3701     Qt6Widgets!QWidgetRepaintManager::updateStaticContentsSize+0xf09
22 00000096`f0afb950 00007ff8`388e27fe     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
23 00000096`f0afb980 00007ff8`30a49c01     Qt6Widgets!QApplication::notify+0x156e
24 00000096`f0afbe20 00007ff8`3722f46a     Qt6Core!QCoreApplication::notifyInternal2+0x101
25 00000096`f0afbe90 00007ff8`372908fb     Qt6Gui!QGuiApplicationPrivate::processMouseEvent+0x99a
26 00000096`f0afc3f0 00007ff8`30be91c0     Qt6Gui!QWindowSystemInterface::sendWindowSystemEvents+0xfb
27 00000096`f0afc420 00007ff8`37555dd9     Qt6Core!QEventDispatcherWin32::processEvents+0x90
28 00000096`f0aff5a0 00007ff8`30a50604     Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
29 00000096`f0aff5d0 00007ff8`30a4812a     Qt6Core!QEventLoop::exec+0x1c4
2a 00000096`f0aff670 00007ff7`0210298d     Qt6Core!QCoreApplication::exec+0x16a
2b 00000096`f0aff6d0 00007ff7`0233c320     QuarkMeta+0x2298d
2c 00000096`f0aff8d0 00007ff7`0233bd06     QuarkMeta+0x25c320
2d 00000096`f0aff960 00007ff8`8a5d7374     QuarkMeta+0x25bd06
2e 00000096`f0aff9a0 00007ff8`8b79cc91     kernel32!BaseThreadInitThunk+0x14
2f 00000096`f0aff9d0 00000000`00000000     ntdll!RtlUserThreadStart+0x21
