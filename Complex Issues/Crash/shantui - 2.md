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

>>>>>>>>>>>>> Waiting for Debugger Extensions Gallery to Initialize completed, duration 0.047 seconds
   ----> Repository : UserExtensions, Enabled: true, Packages count: 0
   ----> Repository : LocalInstalled, Enabled: true, Packages count: 46

Microsoft (R) Windows Debugger Version 10.0.29547.1002 AMD64
Copyright (c) Microsoft Corporation. All rights reserved.


Loading Dump File [C:\Users\fachu\AppData\Local\CrashDumps\QuarkMeta.exe.19368.dmp]
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
Debug session time: Tue Nov 10 14:34:22.000 2026 (UTC + 7:00)
System Uptime: not available
Process Uptime: 0 days 0:00:32.000
................................................................
...........................................
Loading unloaded module list
...
This dump file has an exception of interest stored in it.
The stored exception information can be accessed via .ecxr
(4ba8.4f48): Access violation - code c0000005 (first/second chance not available)
ReadVirtual() failed in GetXStateConfiguration() first read attempt (error == 0.)
Invalid mini-dump Miscellaneous information in InitializeConfiguration()
For analysis of this file, run !analyze -v
ntdll!NtWaitForMultipleObjects+0x14:
00007ffb`e310e0f4 c3              ret
0:000> .ecxr
rax=0000000000000000 rbx=0000000638afb800 rcx=0000015b3caf56e0
rdx=0000000638afb760 rsi=0000015b3ca1c2e0 rdi=0000015b41055428
rip=00007ffb6f54bcd6 rsp=0000000638afb740 rbp=0000000638afb8c9
 r8=00000000000000ff  r9=0000015b40e229f8 r10=0000000000008000
r11=0000000638afbfa0 r12=0000015b3ac90bf0 r13=0000015b3ac90bf0
r14=0000015b3ac90bf0 r15=0000000000000000
iopl=0         nv up ei pl nz na po nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010202
Qt6Gui!QStandardItemModel::flags+0x46:
00007ffb`6f54bcd6 488b4008        mov     rax,qword ptr [rax+8] ds:00000000`00000008=????????????????
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
    Value: 812

    Key  : Analysis.Elapsed.mSec
    Value: 4633

    Key  : Analysis.IO.Other.Mb
    Value: 0

    Key  : Analysis.IO.Read.Mb
    Value: 58

    Key  : Analysis.IO.Write.Mb
    Value: 56

    Key  : Analysis.Init.CPU.mSec
    Value: 593

    Key  : Analysis.Init.Elapsed.mSec
    Value: 18255

    Key  : Analysis.Memory.CommitPeak.Mb
    Value: 170

    Key  : Analysis.Version.DbgEng
    Value: 10.0.29547.1002

    Key  : Analysis.Version.Description
    Value: 10.2602.27.2 amd64fre

    Key  : Analysis.Version.Ext
    Value: 1.2602.27.2

    Key  : Failure.Bucket
    Value: INVALID_POINTER_READ_c0000005_Qt6Gui.dll!Unknown

    Key  : Failure.Exception.Code
    Value: 0xc0000005

    Key  : Failure.Exception.IP.Address
    Value: 0x7ffb6f54bcd6

    Key  : Failure.Exception.IP.Module
    Value: Qt6Gui

    Key  : Failure.Exception.IP.Offset
    Value: 0x38bcd6

    Key  : Failure.Hash
    Value: {673a7bfd-3e35-2a7b-f83f-919214cc16b7}

    Key  : Failure.ProblemClass.Primary
    Value: INVALID_POINTER_READ

    Key  : Faulting.IP.Type
    Value: Paged

    Key  : Timeline.Process.Start.DeltaSec
    Value: 32

    Key  : WER.OS.Branch
    Value: vb_release

    Key  : WER.OS.Version
    Value: 10.0.19041.1


FILE_IN_CAB:  QuarkMeta.exe.19368.dmp

NTGLOBALFLAG:  0

APPLICATION_VERIFIER_FLAGS:  0

CONTEXT:  (.ecxr)
rax=0000000000000000 rbx=0000000638afb800 rcx=0000015b3caf56e0
rdx=0000000638afb760 rsi=0000015b3ca1c2e0 rdi=0000015b41055428
rip=00007ffb6f54bcd6 rsp=0000000638afb740 rbp=0000000638afb8c9
 r8=00000000000000ff  r9=0000015b40e229f8 r10=0000000000008000
r11=0000000638afbfa0 r12=0000015b3ac90bf0 r13=0000015b3ac90bf0
r14=0000015b3ac90bf0 r15=0000000000000000
iopl=0         nv up ei pl nz na po nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010202
Qt6Gui!QStandardItemModel::flags+0x46:
00007ffb`6f54bcd6 488b4008        mov     rax,qword ptr [rax+8] ds:00000000`00000008=????????????????
Resetting default scope

EXCEPTION_RECORD:  (.exr -1)
ExceptionAddress: 00007ffb6f54bcd6 (Qt6Gui!QStandardItemModel::flags+0x0000000000000046)
   ExceptionCode: c0000005 (Access violation)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 0000000000000000
   Parameter[1]: 0000000000000008
Attempt to read from address 0000000000000008

PROCESS_NAME:  QuarkMeta.exe

READ_ADDRESS:  0000000000000008 

ERROR_CODE: (NTSTATUS) 0xc0000005 - 0x%p            0x%p                    %s

EXCEPTION_CODE_STR:  c0000005

EXCEPTION_PARAMETER1:  0000000000000000

EXCEPTION_PARAMETER2:  0000000000000008

STACK_TEXT:  
00000006`38afb740 00007ffb`6fdc72f3     : 00000006`38afb8c9 00007ff7`768723c7 0000015b`41055410 0000015b`3ac90bf0 : Qt6Gui!QStandardItemModel::flags+0x46
00000006`38afb7b0 00007ffb`5793780b     : 00000000`00000000 0000015b`40e229b0 0000015b`3ac90bf0 0000015b`3ac90bf0 : Qt6Widgets!QAbstractItemView::setCurrentIndex+0x73
00000006`38afb7f0 00007ffb`579355b7     : 00000000`00000000 0000015b`3ac90bf8 3ff00000`00000000 00000000`00000000 : Qt6Core!QMetaCallEvent::placeMetaCall+0x3b
00000006`38afb830 00007ffb`6fb4a508     : 0000015b`3ac90bf8 00007ffb`6fc021e4 0000015b`40e229b0 0000015b`3ac90bf0 : Qt6Core!QObject::event+0x187
00000006`38afb930 00007ffb`6fba95f3     : 00000000`00000005 000002d8`000001ef 0000015b`3ac90bf8 0000015b`37ca2c50 : Qt6Widgets!QWidget::event+0xec8
00000006`38afba10 00007ffb`6fb03701     : 0000015b`37c8a5c0 0000015b`40e229b0 0000015b`40e229b0 0000015b`3ac90bf0 : Qt6Widgets!QFrame::event+0x33
00000006`38afba40 00007ffb`6fb027fe     : 00000000`00000000 00000006`38afbb70 0000015b`40e22900 000002d8`000001ef : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000006`38afba70 00007ff7`769b9f8a     : 00000006`38aff4b8 0000015b`3ac90bf0 0000015b`40e229b0 00000000`00000000 : Qt6Widgets!QApplication::notify+0x156e
00000006`38afbf10 00007ffb`578e9c01     : 0000015b`3ac90bf0 00000000`00000001 0000015b`40e229b0 0000015b`3ac90bf0 : QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a
00000006`38afc000 00007ffb`578ec423     : 0000015b`37c355d0 00000000`00000000 0000015b`37c355d0 00000000`00470e2a : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000006`38afc070 00007ffb`6f585e5f     : 0000015b`37f399b0 0000015b`37f399b0 00000000`00000003 0000015b`40e229b0 : Qt6Core!QCoreApplicationPrivate::sendPostedEvents+0x203
00000006`38afc160 00007ffb`57a891c0     : 00000000`000000a4 00000006`38afc290 0000015b`40c016b0 00000000`0000000f : Qt6Gui!QWindowsGuiEventDispatcher::sendPostedEvents+0xf
00000006`38afc190 00007ffb`6f585dd9     : 00000006`38aff400 00000000`00000000 0000015b`40c016b0 0000015b`37f399b0 : Qt6Core!QEventDispatcherWin32::processEvents+0x90
00000006`38aff310 00007ffb`578f0604     : 00000000`000000a4 0000015b`40c016b0 0000015b`37c8a5c0 00000000`00000000 : Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
00000006`38aff340 00007ffb`578e812a     : 00000006`38aff400 0000015b`37c355d0 0000015b`37c355d0 00000006`38aff540 : Qt6Core!QEventLoop::exec+0x1c4
00000006`38aff3e0 00007ff7`7670f06a     : 00007ffb`57c33370 00000006`38aff540 00000000`00000000 00000000`00000000 : Qt6Core!QCoreApplication::exec+0x16a
00000006`38aff440 00007ff7`76a043c0     : 00000000`00000000 00007ffb`e0d87830 00000000`00000001 00000000`00000022 : QuarkMeta!main+0x6ba
00000006`38aff740 00007ff7`76a03dae     : 00000000`00000001 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta!qtEntryPoint+0x30
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!invoke_main+0x21
00000006`38aff7d0 00007ffb`e27f7374     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta!__scrt_common_main_seh+0x106
00000006`38aff810 00007ffb`e30bcc91     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : kernel32!BaseThreadInitThunk+0x14
00000006`38aff840 00000000`00000000     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : ntdll!RtlUserThreadStart+0x21


STACK_COMMAND: ~0s; .ecxr ; kb

IP_IN_PAGED_CODE: 
Qt6Gui!QStandardItemModel::flags+46
00007ffb`6f54bcd6 488b4008        mov     rax,qword ptr [rax+8]

SYMBOL_NAME:  Qt6Gui+38bcd6

MODULE_NAME: Qt6Gui

IMAGE_NAME:  Qt6Gui.dll

FAILURE_BUCKET_ID:  INVALID_POINTER_READ_c0000005_Qt6Gui.dll!Unknown

OS_VERSION:  10.0.19041.1

BUILDLAB_STR:  vb_release

OSPLATFORM_TYPE:  x64

OSNAME:  Windows 10

IMAGE_VERSION:  6.10.2.0

FAILURE_ID_HASH:  {673a7bfd-3e35-2a7b-f83f-919214cc16b7}

Followup:     MachineOwner
---------

0:000> k
 # Child-SP          RetAddr               Call Site
00 00000006`38afa298 00007ffb`e0800d00     ntdll!NtWaitForMultipleObjects+0x14
01 00000006`38afa2a0 00007ffb`e0800bfe     KERNELBASE!WaitForMultipleObjectsEx+0xf0
02 00000006`38afa590 00007ffb`e2851f5a     KERNELBASE!WaitForMultipleObjects+0xe
03 00000006`38afa5d0 00007ffb`e2851996     kernel32!WerpReportFaultInternal+0x58a
04 00000006`38afa6f0 00007ffb`e08dc6f9     kernel32!WerpReportFault+0xbe
05 00000006`38afa730 00007ffb`e31158d8     KERNELBASE!UnhandledExceptionFilter+0x3d9
06 00000006`38afa850 00007ffb`e30fce46     ntdll!RtlUserThreadStart$filt$0+0xa2
07 00000006`38afa890 00007ffb`e311296f     ntdll!_C_specific_handler+0x96
08 00000006`38afa900 00007ffb`e30c2554     ntdll!RtlpExecuteHandlerForException+0xf
09 00000006`38afa930 00007ffb`e311147e     ntdll!RtlDispatchException+0x244
0a 00000006`38afb040 00007ffb`6f54bcd6     ntdll!KiUserExceptionDispatch+0x2e
0b 00000006`38afb740 00007ffb`6fdc72f3     Qt6Gui!QStandardItemModel::flags+0x46
0c 00000006`38afb7b0 00007ffb`5793780b     Qt6Widgets!QAbstractItemView::setCurrentIndex+0x73
0d 00000006`38afb7f0 00007ffb`579355b7     Qt6Core!QMetaCallEvent::placeMetaCall+0x3b
0e 00000006`38afb830 00007ffb`6fb4a508     Qt6Core!QObject::event+0x187
0f 00000006`38afb930 00007ffb`6fba95f3     Qt6Widgets!QWidget::event+0xec8
10 00000006`38afba10 00007ffb`6fb03701     Qt6Widgets!QFrame::event+0x33
11 00000006`38afba40 00007ffb`6fb027fe     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
12 00000006`38afba70 00007ff7`769b9f8a     Qt6Widgets!QApplication::notify+0x156e
13 00000006`38afbf10 00007ffb`578e9c01     QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a [G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\QuarkApplication.cpp @ 152] 
14 00000006`38afc000 00007ffb`578ec423     Qt6Core!QCoreApplication::notifyInternal2+0x101
15 00000006`38afc070 00007ffb`6f585e5f     Qt6Core!QCoreApplicationPrivate::sendPostedEvents+0x203
16 00000006`38afc160 00007ffb`57a891c0     Qt6Gui!QWindowsGuiEventDispatcher::sendPostedEvents+0xf
17 00000006`38afc190 00007ffb`6f585dd9     Qt6Core!QEventDispatcherWin32::processEvents+0x90
18 00000006`38aff310 00007ffb`578f0604     Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
19 00000006`38aff340 00007ffb`578e812a     Qt6Core!QEventLoop::exec+0x1c4
1a 00000006`38aff3e0 00007ff7`7670f06a     Qt6Core!QCoreApplication::exec+0x16a
1b 00000006`38aff440 00007ff7`76a043c0     QuarkMeta!main+0x6ba [G:\C++\Project\Source_Code\QuarkMeta_Source\src\main.cpp @ 196] 
1c 00000006`38aff740 00007ff7`76a03dae     QuarkMeta!qtEntryPoint+0x30 [C:\Users\qt\work\qt\qtbase\src\entrypoint\qtentrypoint_win.cpp @ 59] 
1d (Inline Function) --------`--------     QuarkMeta!invoke_main+0x21 [D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 102] 
1e 00000006`38aff7d0 00007ffb`e27f7374     QuarkMeta!__scrt_common_main_seh+0x106 [D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288] 
1f 00000006`38aff810 00007ffb`e30bcc91     kernel32!BaseThreadInitThunk+0x14
20 00000006`38aff840 00000000`00000000     ntdll!RtlUserThreadStart+0x21
