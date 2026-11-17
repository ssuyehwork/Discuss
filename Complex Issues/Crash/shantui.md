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
.
>>>>>>>>>>>>> Waiting for Debugger Extensions Gallery to Initialize completed, duration 0.875 seconds
   ----> Repository : UserExtensions, Enabled: true, Packages count: 0
   ----> Repository : LocalInstalled, Enabled: true, Packages count: 46

Microsoft (R) Windows Debugger Version 10.0.29547.1002 AMD64
Copyright (c) Microsoft Corporation. All rights reserved.


Loading Dump File [C:\Users\fachu\AppData\Local\CrashDumps\QuarkMeta.exe.4860.dmp]
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
Debug session time: Tue Nov 10 13:48:47.000 2026 (UTC + 7:00)
System Uptime: not available
Process Uptime: 0 days 0:11:45.000
................................................................
.....................................................
Loading unloaded module list
...
This dump file has an exception of interest stored in it.
The stored exception information can be accessed via .ecxr
(12fc.33d8): Access violation - code c0000005 (first/second chance not available)
ReadVirtual() failed in GetXStateConfiguration() first read attempt (error == 0.)
Invalid mini-dump Miscellaneous information in InitializeConfiguration()
For analysis of this file, run !analyze -v
ntdll!NtWaitForMultipleObjects+0x14:
00007ffb`e310e0f4 c3              ret
0:000> .ecxr
rax=00000277865a61c0 rbx=00000090724faa60 rcx=0000000000000004
rdx=0000000000000000 rsi=00000277866b7760 rdi=00000090724fa9e8
rip=00007ffb6f54bd34 rsp=00000090724fa930 rbp=00000090724faa20
 r8=0000000000000000  r9=00000000000000ff r10=0000000000000000
r11=000000000000020b r12=00000090724fb290 r13=0000027786628930
r14=000002778667f6b0 r15=0000027786592630
iopl=0         nv up ei pl nz na pe nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010206
Qt6Gui!QStandardItemModel::flags+0xa4:
00007ffb`6f54bd34 3b4a40          cmp     ecx,dword ptr [rdx+40h] ds:00000000`00000040=????????
0:000> !analyze -v
................................................................
.....................................................
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
    Value: 906

    Key  : Analysis.Elapsed.mSec
    Value: 5423

    Key  : Analysis.IO.Other.Mb
    Value: 0

    Key  : Analysis.IO.Read.Mb
    Value: 58

    Key  : Analysis.IO.Write.Mb
    Value: 56

    Key  : Analysis.Init.CPU.mSec
    Value: 781

    Key  : Analysis.Init.Elapsed.mSec
    Value: 10691

    Key  : Analysis.Memory.CommitPeak.Mb
    Value: 174

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
    Value: 0x7ffb6f54bd34

    Key  : Failure.Exception.IP.Module
    Value: Qt6Gui

    Key  : Failure.Exception.IP.Offset
    Value: 0x38bd34

    Key  : Failure.Hash
    Value: {673a7bfd-3e35-2a7b-f83f-919214cc16b7}

    Key  : Failure.ProblemClass.Primary
    Value: INVALID_POINTER_READ

    Key  : Faulting.IP.Type
    Value: Paged

    Key  : Timeline.Process.Start.DeltaSec
    Value: 705

    Key  : WER.OS.Branch
    Value: vb_release

    Key  : WER.OS.Version
    Value: 10.0.19041.1


FILE_IN_CAB:  QuarkMeta.exe.4860.dmp

NTGLOBALFLAG:  0

APPLICATION_VERIFIER_FLAGS:  0

CONTEXT:  (.ecxr)
rax=00000277865a61c0 rbx=00000090724faa60 rcx=0000000000000004
rdx=0000000000000000 rsi=00000277866b7760 rdi=00000090724fa9e8
rip=00007ffb6f54bd34 rsp=00000090724fa930 rbp=00000090724faa20
 r8=0000000000000000  r9=00000000000000ff r10=0000000000000000
r11=000000000000020b r12=00000090724fb290 r13=0000027786628930
r14=000002778667f6b0 r15=0000027786592630
iopl=0         nv up ei pl nz na pe nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010206
Qt6Gui!QStandardItemModel::flags+0xa4:
00007ffb`6f54bd34 3b4a40          cmp     ecx,dword ptr [rdx+40h] ds:00000000`00000040=????????
Resetting default scope

EXCEPTION_RECORD:  (.exr -1)
ExceptionAddress: 00007ffb6f54bd34 (Qt6Gui!QStandardItemModel::flags+0x00000000000000a4)
   ExceptionCode: c0000005 (Access violation)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 0000000000000000
   Parameter[1]: 0000000000000040
Attempt to read from address 0000000000000040

PROCESS_NAME:  QuarkMeta.exe

READ_ADDRESS:  0000000000000040 

ERROR_CODE: (NTSTATUS) 0xc0000005 - 0x%p            0x%p                    %s

EXCEPTION_CODE_STR:  c0000005

EXCEPTION_PARAMETER1:  0000000000000000

EXCEPTION_PARAMETER2:  0000000000000040

STACK_TEXT:  
00000090`724fa930 00007ffb`6fdbe6f0     : 00000277`86628938 00000277`8658c3f0 00000277`00000000 00007ffb`6fb4b254 : Qt6Gui!QStandardItemModel::flags+0xa4
00000090`724fa9a0 00007ffb`6fb49b4f     : 00000277`885c3a38 00000277`86628930 00000000`00000000 00000277`86628938 : Qt6Widgets!QAbstractItemView::focusInEvent+0x260
00000090`724faa50 00007ffb`6fba95f3     : 00000090`724fb2f0 00007ffb`6fdb7ffb 00000277`86628938 00007ffa`dc711bac : Qt6Widgets!QWidget::event+0x50f
00000090`724fab30 00007ffb`6fba49b3     : 00000277`851752f0 00000277`86628930 00007ffa`dca3dd70 00000000`00000000 : Qt6Widgets!QFrame::event+0x33
00000090`724fab60 00007ffb`6fb03701     : 00000277`851752f0 00000090`724fb290 00000090`724fb290 00000277`86628930 : Qt6Widgets!QAbstractScrollArea::event+0x493
00000090`724fac10 00007ffb`6fb027fe     : 00000277`851752f0 00000090`724fad40 00000090`724fb200 00000090`724fb290 : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000090`724fac40 00007ff6`363f9f7a     : 00000090`724ff7c8 00000277`86628930 00000090`724fb290 00000277`00000000 : Qt6Widgets!QApplication::notify+0x156e
00000090`724fb0e0 00007ffa`dc709c01     : 00000277`86628930 00000000`00000001 00000090`724fb290 00000277`86628930 : QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a
00000090`724fb1d0 00007ffb`6fb074ce     : 00000277`86628930 00000277`86628930 00000277`86628930 00007ffa`dc74e314 : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000090`724fb240 00007ffb`6fb5a605     : 00000277`86628930 00000277`86628930 00000277`86628930 00000000`00000000 : Qt6Widgets!QApplicationPrivate::setFocusWidget+0x1ce
00000090`724fb300 00007ff6`362b24f7     : 00000000`00000010 00000277`8891e2b0 00000278`8b6b3410 00000000`00000010 : Qt6Widgets!QWidget::setFocus+0x2d5
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QWidget::setFocus+0x13
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QuarkMeta::LibraryPanel::loadLibrary::__l40::<lambda_1>::operator()+0xdb
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QtPrivate::FunctorCall<std::integer_sequence<unsigned __int64>,QtPrivate::List<>,void,`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call::__l2::<lambda_1>::operator()+0xdb
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QtPrivate::FunctorCallBase::call_internal+0xdb
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QtPrivate::FunctorCall<std::integer_sequence<unsigned __int64>,QtPrivate::List<>,void,`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call+0xdb
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!QtPrivate::FunctorCallable<`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call+0xdb
00000090`724fb3b0 00007ffa`dc74b60f     : 00000000`00000000 00000278`8b6b3410 00000090`00000001 00000278`923f3996 : QuarkMeta!QtPrivate::QCallableObject<`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1>,QtPrivate::List<>,void>::impl+0xf7
00000090`724fb430 00007ffa`dc74e314     : 00000277`8891d290 00000277`00000003 00000000`00000003 00000278`92319101 : Qt6Core!QObject::qt_static_metacall+0x15bf
00000090`724fb570 00007ffa`dc763aff     : 00000277`8891d290 00000090`724fb669 00000090`724fbd20 00000000`00000890 : Qt6Core!QMetaObject::activate+0x84
00000090`724fb5a0 00007ffa`dc755498     : 00000277`00000000 00000277`8380a680 00000001`00000051 00000277`83800000 : Qt6Core!QSignalMapper::tr+0x61f
00000090`724fb5d0 00007ffb`6fb03701     : 00000277`851752f0 00000277`83691b40 00000090`724fbd20 00000277`8891d290 : Qt6Core!QObject::event+0x68
00000090`724fb6d0 00007ffb`6fb027fe     : 00000277`851752f0 00000090`724fb800 00000090`724fbd00 00000000`00000000 : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000090`724fb700 00007ff6`363f9f7a     : 00000090`724ff7c8 00000277`8891d290 00000090`724fbd20 00000278`00000000 : Qt6Widgets!QApplication::notify+0x156e
00000090`724fbba0 00007ffa`dc709c01     : 00000277`8891d290 00000000`00000001 00000090`724fbd20 00000277`8891d290 : QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a
00000090`724fbc90 00007ffa`dc8a9057     : 00000277`8942a4d0 ffffffff`fffffffe 00000278`91e47210 00000278`8b4f5c80 : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000090`724fbd00 00007ffb`6fb03701     : 00000277`58000011 00000277`851752f0 00000277`83691b40 00000278`923d8660 : Qt6Core!QEventDispatcherWin32::event+0x157
00000090`724fbd50 00007ffb`6fb027fe     : 00000277`851752f0 00000090`724fbe80 00000278`923d8600 00007ffb`dc93b050 : Qt6Widgets!QApplicationPrivate::notify_helper+0x151
00000090`724fbd80 00007ff6`363f9f7a     : 00000090`724ff7c8 00000277`851ad340 00000278`923d8660 00000000`00000000 : Qt6Widgets!QApplication::notify+0x156e
00000090`724fc220 00007ffa`dc709c01     : 00000277`851ad340 00000000`00000001 00000278`923d8660 00000277`851ad340 : QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a
00000090`724fc310 00007ffa`dc70c423     : 00000000`00000000 00000277`83d8d8b0 00000277`83629a20 00000000`00000000 : Qt6Core!QCoreApplication::notifyInternal2+0x101
00000090`724fc380 00007ffb`6f585e5f     : 00000277`851ad340 00000277`851ad340 00000000`00000003 00000278`923d8660 : Qt6Core!QCoreApplicationPrivate::sendPostedEvents+0x203
00000090`724fc470 00007ffa`dc8a91c0     : 00000000`000000a4 00000090`724fc5a0 00000277`887c8220 00000000`0000000f : Qt6Gui!QWindowsGuiEventDispatcher::sendPostedEvents+0xf
00000090`724fc4a0 00007ffb`6f585dd9     : 00000090`724ff710 00000000`00000000 00000277`887c8220 00000277`851ad340 : Qt6Core!QEventDispatcherWin32::processEvents+0x90
00000090`724ff620 00007ffa`dc710604     : 00000000`000000a4 00000277`887c8220 00000277`851752f0 00000000`00000000 : Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
00000090`724ff650 00007ffa`dc70812a     : 00000090`724ff710 00000277`83629a20 00000277`83629a20 00000090`724ff850 : Qt6Core!QEventLoop::exec+0x1c4
00000090`724ff6f0 00007ff6`3614f06a     : 00007ffa`dca53370 00000090`724ff850 00000000`00000000 00000000`00000000 : Qt6Core!QCoreApplication::exec+0x16a
00000090`724ff750 00007ff6`364443b0     : 00000000`00000000 00007ffb`e0d87830 00000000`00000001 00000000`00000022 : QuarkMeta!main+0x6ba
00000090`724ffa50 00007ff6`36443d9e     : 00000000`00000001 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta!qtEntryPoint+0x30
(Inline Function) --------`--------     : --------`-------- --------`-------- --------`-------- --------`-------- : QuarkMeta!invoke_main+0x21
00000090`724ffae0 00007ffb`e27f7374     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : QuarkMeta!__scrt_common_main_seh+0x106
00000090`724ffb20 00007ffb`e30bcc91     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : kernel32!BaseThreadInitThunk+0x14
00000090`724ffb50 00000000`00000000     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : ntdll!RtlUserThreadStart+0x21


STACK_COMMAND: ~0s; .ecxr ; kb

IP_IN_PAGED_CODE: 
Qt6Gui!QStandardItemModel::flags+a4
00007ffb`6f54bd34 3b4a40          cmp     ecx,dword ptr [rdx+40h]

SYMBOL_NAME:  Qt6Gui+38bd34

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
00 00000090`724f9458 00007ffb`e0800d00     ntdll!NtWaitForMultipleObjects+0x14
01 00000090`724f9460 00007ffb`e0800bfe     KERNELBASE!WaitForMultipleObjectsEx+0xf0
02 00000090`724f9750 00007ffb`e2851f5a     KERNELBASE!WaitForMultipleObjects+0xe
03 00000090`724f9790 00007ffb`e2851996     kernel32!WerpReportFaultInternal+0x58a
04 00000090`724f98b0 00007ffb`e08dc6f9     kernel32!WerpReportFault+0xbe
05 00000090`724f98f0 00007ffb`e31158d8     KERNELBASE!UnhandledExceptionFilter+0x3d9
06 00000090`724f9a10 00007ffb`e30fce46     ntdll!RtlUserThreadStart$filt$0+0xa2
07 00000090`724f9a50 00007ffb`e311296f     ntdll!_C_specific_handler+0x96
08 00000090`724f9ac0 00007ffb`e30c2554     ntdll!RtlpExecuteHandlerForException+0xf
09 00000090`724f9af0 00007ffb`e311147e     ntdll!RtlDispatchException+0x244
0a 00000090`724fa200 00007ffb`6f54bd34     ntdll!KiUserExceptionDispatch+0x2e
0b 00000090`724fa930 00007ffb`6fdbe6f0     Qt6Gui!QStandardItemModel::flags+0xa4
0c 00000090`724fa9a0 00007ffb`6fb49b4f     Qt6Widgets!QAbstractItemView::focusInEvent+0x260
0d 00000090`724faa50 00007ffb`6fba95f3     Qt6Widgets!QWidget::event+0x50f
0e 00000090`724fab30 00007ffb`6fba49b3     Qt6Widgets!QFrame::event+0x33
0f 00000090`724fab60 00007ffb`6fb03701     Qt6Widgets!QAbstractScrollArea::event+0x493
10 00000090`724fac10 00007ffb`6fb027fe     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
11 00000090`724fac40 00007ff6`363f9f7a     Qt6Widgets!QApplication::notify+0x156e
12 00000090`724fb0e0 00007ffa`dc709c01     QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a [G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\QuarkApplication.cpp @ 152] 
13 00000090`724fb1d0 00007ffb`6fb074ce     Qt6Core!QCoreApplication::notifyInternal2+0x101
14 00000090`724fb240 00007ffb`6fb5a605     Qt6Widgets!QApplicationPrivate::setFocusWidget+0x1ce
15 00000090`724fb300 00007ff6`362b24f7     Qt6Widgets!QWidget::setFocus+0x2d5
16 (Inline Function) --------`--------     QuarkMeta!QWidget::setFocus+0x13 [C:\Qt\6.10.2\msvc2022_64\include\QtWidgets\qwidget.h @ 426] 
17 (Inline Function) --------`--------     QuarkMeta!QuarkMeta::LibraryPanel::loadLibrary::__l40::<lambda_1>::operator()+0xdb [G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\LibraryPanel.cpp @ 307] 
18 (Inline Function) --------`--------     QuarkMeta!QtPrivate::FunctorCall<std::integer_sequence<unsigned __int64>,QtPrivate::List<>,void,`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call::__l2::<lambda_1>::operator()+0xdb [C:\Qt\6.10.2\msvc2022_64\include\QtCore\qobjectdefs_impl.h @ 116] 
19 (Inline Function) --------`--------     QuarkMeta!QtPrivate::FunctorCallBase::call_internal+0xdb [C:\Qt\6.10.2\msvc2022_64\include\QtCore\qobjectdefs_impl.h @ 65] 
1a (Inline Function) --------`--------     QuarkMeta!QtPrivate::FunctorCall<std::integer_sequence<unsigned __int64>,QtPrivate::List<>,void,`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call+0xdb [C:\Qt\6.10.2\msvc2022_64\include\QtCore\qobjectdefs_impl.h @ 115] 
1b (Inline Function) --------`--------     QuarkMeta!QtPrivate::FunctorCallable<`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1> >::call+0xdb [C:\Qt\6.10.2\msvc2022_64\include\QtCore\qobjectdefs_impl.h @ 337] 
1c 00000090`724fb3b0 00007ffa`dc74b60f     QuarkMeta!QtPrivate::QCallableObject<`QuarkMeta::LibraryPanel::loadLibrary'::`40'::<lambda_1>,QtPrivate::List<>,void>::impl+0xf7 [C:\Qt\6.10.2\msvc2022_64\include\QtCore\qobjectdefs_impl.h @ 547] 
1d 00000090`724fb430 00007ffa`dc74e314     Qt6Core!QObject::qt_static_metacall+0x15bf
1e 00000090`724fb570 00007ffa`dc763aff     Qt6Core!QMetaObject::activate+0x84
1f 00000090`724fb5a0 00007ffa`dc755498     Qt6Core!QSignalMapper::tr+0x61f
20 00000090`724fb5d0 00007ffb`6fb03701     Qt6Core!QObject::event+0x68
21 00000090`724fb6d0 00007ffb`6fb027fe     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
22 00000090`724fb700 00007ff6`363f9f7a     Qt6Widgets!QApplication::notify+0x156e
23 00000090`724fbba0 00007ffa`dc709c01     QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a [G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\QuarkApplication.cpp @ 152] 
24 00000090`724fbc90 00007ffa`dc8a9057     Qt6Core!QCoreApplication::notifyInternal2+0x101
25 00000090`724fbd00 00007ffb`6fb03701     Qt6Core!QEventDispatcherWin32::event+0x157
26 00000090`724fbd50 00007ffb`6fb027fe     Qt6Widgets!QApplicationPrivate::notify_helper+0x151
27 00000090`724fbd80 00007ff6`363f9f7a     Qt6Widgets!QApplication::notify+0x156e
28 00000090`724fc220 00007ffa`dc709c01     QuarkMeta!QuarkMeta::QuarkApplication::notify+0x63a [G:\C++\Project\Source_Code\QuarkMeta_Source\src\ui\QuarkApplication.cpp @ 152] 
29 00000090`724fc310 00007ffa`dc70c423     Qt6Core!QCoreApplication::notifyInternal2+0x101
2a 00000090`724fc380 00007ffb`6f585e5f     Qt6Core!QCoreApplicationPrivate::sendPostedEvents+0x203
2b 00000090`724fc470 00007ffa`dc8a91c0     Qt6Gui!QWindowsGuiEventDispatcher::sendPostedEvents+0xf
2c 00000090`724fc4a0 00007ffb`6f585dd9     Qt6Core!QEventDispatcherWin32::processEvents+0x90
2d 00000090`724ff620 00007ffa`dc710604     Qt6Gui!QWindowsGuiEventDispatcher::processEvents+0x19
2e 00000090`724ff650 00007ffa`dc70812a     Qt6Core!QEventLoop::exec+0x1c4
2f 00000090`724ff6f0 00007ff6`3614f06a     Qt6Core!QCoreApplication::exec+0x16a
30 00000090`724ff750 00007ff6`364443b0     QuarkMeta!main+0x6ba [G:\C++\Project\Source_Code\QuarkMeta_Source\src\main.cpp @ 196] 
31 00000090`724ffa50 00007ff6`36443d9e     QuarkMeta!qtEntryPoint+0x30 [C:\Users\qt\work\qt\qtbase\src\entrypoint\qtentrypoint_win.cpp @ 59] 
32 (Inline Function) --------`--------     QuarkMeta!invoke_main+0x21 [D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 102] 
33 00000090`724ffae0 00007ffb`e27f7374     QuarkMeta!__scrt_common_main_seh+0x106 [D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288] 
34 00000090`724ffb20 00007ffb`e30bcc91     kernel32!BaseThreadInitThunk+0x14
35 00000090`724ffb50 00000000`00000000     ntdll!RtlUserThreadStart+0x21
