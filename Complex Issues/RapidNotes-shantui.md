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

>>>>>>>>>>>>> Waiting for Debugger Extensions Gallery to Initialize completed, duration 0.562 seconds
   ----> Repository : UserExtensions, Enabled: true, Packages count: 0
   ----> Repository : LocalInstalled, Enabled: true, Packages count: 46

Microsoft (R) Windows Debugger Version 10.0.29547.1002 AMD64
Copyright (c) Microsoft Corporation. All rights reserved.


Loading Dump File [G:\C++\Project\Source_Code\RapidNotes_Build\RapidNotes\crash_20261111_210451.dmp]
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
Debug session time: Wed Nov 11 21:04:52.000 2026 (UTC + 7:00)
System Uptime: not available
Process Uptime: 0 days 0:04:58.000
................................................................
.................................................
This dump file has an exception of interest stored in it.
The stored exception information can be accessed via .ecxr
(53f8.4564): Access violation - code c0000005 (first/second chance not available)
ReadVirtual() failed in GetXStateConfiguration() first read attempt (error == 0.)
Invalid mini-dump Miscellaneous information in InitializeConfiguration()
For analysis of this file, run !analyze -v
ntdll!NtGetContextThread+0x14:
00007ff8`09def3f4 c3              ret
0:000> .ecxr
rax=0000000000000009 rbx=000000000000000a rcx=000000000000000a
rdx=000000000000000a rsi=0000000000000009 rdi=0000017bb4da39e0
rip=0000000000000000 rsp=00000030760f94f8 rbp=0000017bb4da39e0
 r8=0000017bb4da39e0  r9=00000030760f9600 r10=0000000000000009
r11=00007ff7e4a10000 r12=00000030760f9870 r13=0000000000000000
r14=0000000000000000 r15=00007ff7e4a10000
iopl=0         nv up ei pl zr na pe nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010246
00000000`00000000 ??              ???
*** WARNING: Unable to verify timestamp for RapidNotes.exe
0:000> !analyze -v
................................................................
.................................................
*******************************************************************************
*                                                                             *
*                        Exception Analysis                                   *
*                                                                             *
*******************************************************************************

*** WARNING: Unable to verify timestamp for RapidNotes.exe

KEY_VALUES_STRING: 1

    Key  : AV.Type
    Value: Execute

    Key  : Analysis.CPU.mSec
    Value: 656

    Key  : Analysis.Elapsed.mSec
    Value: 1077

    Key  : Analysis.IO.Other.Mb
    Value: 0

    Key  : Analysis.IO.Read.Mb
    Value: 1

    Key  : Analysis.IO.Write.Mb
    Value: 0

    Key  : Analysis.Init.CPU.mSec
    Value: 328

    Key  : Analysis.Init.Elapsed.mSec
    Value: 12069

    Key  : Analysis.Memory.CommitPeak.Mb
    Value: 78

    Key  : Analysis.Version.DbgEng
    Value: 10.0.29547.1002

    Key  : Analysis.Version.Description
    Value: 10.2602.27.2 amd64fre

    Key  : Analysis.Version.Ext
    Value: 1.2602.27.2

    Key  : Failure.Bucket
    Value: SOFTWARE_NX_FAULT_NULL_INVALID_POINTER_EXECUTE_c0000005_RapidNotes.exe!Unknown

    Key  : Failure.Exception.Code
    Value: 0xc0000005

    Key  : Failure.Hash
    Value: {3a61ccef-38f5-cf02-d756-1655e8d8c5db}

    Key  : Failure.ProblemClass.Primary
    Value: SOFTWARE_NX_FAULT

    Key  : Faulting.IP.Type
    Value: Null

    Key  : Timeline.Process.Start.DeltaSec
    Value: 298

    Key  : WER.OS.Branch
    Value: vb_release

    Key  : WER.OS.Version
    Value: 10.0.19041.1

    Key  : WER.Process.Version
    Value: 64.2.0.0


FILE_IN_CAB:  crash_20261111_210451.dmp

CONTEXT:  (.ecxr)
rax=0000000000000009 rbx=000000000000000a rcx=000000000000000a
rdx=000000000000000a rsi=0000000000000009 rdi=0000017bb4da39e0
rip=0000000000000000 rsp=00000030760f94f8 rbp=0000017bb4da39e0
 r8=0000017bb4da39e0  r9=00000030760f9600 r10=0000000000000009
r11=00007ff7e4a10000 r12=00000030760f9870 r13=0000000000000000
r14=0000000000000000 r15=00007ff7e4a10000
iopl=0         nv up ei pl zr na pe nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00010246
00000000`00000000 ??              ???
Resetting default scope

EXCEPTION_RECORD:  (.exr -1)
ExceptionAddress: 0000000000000000
   ExceptionCode: c0000005 (Access violation)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 0000000000000008
   Parameter[1]: 0000000000000000
Attempt to execute non-executable address 0000000000000000

PROCESS_NAME:  RapidNotes.exe

EXECUTE_ADDRESS: 0

ERROR_CODE: (NTSTATUS) 0xc0000005 - 0x%p            0x%p                    %s

EXCEPTION_CODE_STR:  c0000005

EXCEPTION_PARAMETER1:  0000000000000008

EXCEPTION_PARAMETER2:  0000000000000000

STACK_TEXT:  
00000030`760f94f8 00007ff7`e4d243a5     : 0000017c`2897e060 00000000`00000021 0000017b`b235032c 0000017b`b2350150 : 0x0
00000030`760f9500 0000017c`2897e060     : 00000000`00000021 0000017b`b235032c 0000017b`b2350150 00000030`760f9d30 : RapidNotes+0x3143a5
00000030`760f9508 00000000`00000021     : 0000017b`b235032c 0000017b`b2350150 00000030`760f9d30 00007ff7`e4ccfff5 : 0x0000017c`2897e060
00000030`760f9510 0000017b`b235032c     : 0000017b`b2350150 00000030`760f9d30 00007ff7`e4ccfff5 0000017b`b4da39e0 : 0x21
00000030`760f9518 0000017b`b2350150     : 00000030`760f9d30 00007ff7`e4ccfff5 0000017b`b4da39e0 00000000`000000ab : 0x0000017b`b235032c
00000030`760f9520 00000030`760f9d30     : 00007ff7`e4ccfff5 0000017b`b4da39e0 00000000`000000ab 0000017b`b2350324 : 0x0000017b`b2350150
00000030`760f9528 00007ff7`e4ccfff5     : 0000017b`b4da39e0 00000000`000000ab 0000017b`b2350324 00000000`00000021 : 0x00000030`760f9d30
00000030`760f9530 0000017b`b4da39e0     : 00000000`000000ab 0000017b`b2350324 00000000`00000021 00007ff7`e4ec690b : RapidNotes+0x2bfff5
00000030`760f9538 00000000`000000ab     : 0000017b`b2350324 00000000`00000021 00007ff7`e4ec690b 00007ff7`e4d25274 : 0x0000017b`b4da39e0
00000030`760f9540 0000017b`b2350324     : 00000000`00000021 00007ff7`e4ec690b 00007ff7`e4d25274 00000030`760f9d30 : 0xab
00000030`760f9548 00000000`00000021     : 00007ff7`e4ec690b 00007ff7`e4d25274 00000030`760f9d30 0000017b`b2451f80 : 0x0000017b`b2350324
00000030`760f9550 00007ff7`e4ec690b     : 00007ff7`e4d25274 00000030`760f9d30 0000017b`b2451f80 00000030`760f95f9 : 0x21
00000030`760f9558 00007ff7`e4d25274     : 00000030`760f9d30 0000017b`b2451f80 00000030`760f95f9 0000017b`b2450000 : RapidNotes+0x4b690b
00000030`760f9560 00000030`760f9d30     : 0000017b`b2451f80 00000030`760f95f9 0000017b`b2450000 00000030`760f9888 : RapidNotes+0x315274
00000030`760f9568 0000017b`b2451f80     : 00000030`760f95f9 0000017b`b2450000 00000030`760f9888 00007ff7`e4d38936 : 0x00000030`760f9d30
00000030`760f9570 00000030`760f95f9     : 0000017b`b2450000 00000030`760f9888 00007ff7`e4d38936 00000030`760f9d30 : 0x0000017b`b2451f80
00000030`760f9578 0000017b`b2450000     : 00000030`760f9888 00007ff7`e4d38936 00000030`760f9d30 00000000`00000001 : 0x00000030`760f95f9
00000030`760f9580 00000030`760f9888     : 00007ff7`e4d38936 00000030`760f9d30 00000000`00000001 0000017b`b4da39e0 : 0x0000017b`b2450000
00000030`760f9588 00007ff7`e4d38936     : 00000030`760f9d30 00000000`00000001 0000017b`b4da39e0 00000000`00000001 : 0x00000030`760f9888
00000030`760f9590 00000030`760f9d30     : 00000000`00000001 0000017b`b4da39e0 00000000`00000001 00000000`00000000 : RapidNotes+0x328936
00000030`760f9598 00000000`00000001     : 0000017b`b4da39e0 00000000`00000001 00000000`00000000 00000000`8d008d00 : 0x00000030`760f9d30
00000030`760f95a0 0000017b`b4da39e0     : 00000000`00000001 00000000`00000000 00000000`8d008d00 00000000`00000060 : 0x1
00000030`760f95a8 00000000`00000001     : 00000000`00000000 00000000`8d008d00 00000000`00000060 00007ff8`09d7b86b : 0x0000017b`b4da39e0
00000030`760f95b0 00000000`00000000     : 00000000`8d008d00 00000000`00000060 00007ff8`09d7b86b 00007ff7`e4a10000 : 0x1


STACK_COMMAND: ~0s; .ecxr ; kb

SYMBOL_NAME:  RapidNotes+3143a5

MODULE_NAME: RapidNotes

IMAGE_NAME:  RapidNotes.exe

FAILURE_BUCKET_ID:  SOFTWARE_NX_FAULT_NULL_INVALID_POINTER_EXECUTE_c0000005_RapidNotes.exe!Unknown

OS_VERSION:  10.0.19041.1

BUILDLAB_STR:  vb_release

OSPLATFORM_TYPE:  x64

OSNAME:  Windows 10

FAILURE_ID_HASH:  {3a61ccef-38f5-cf02-d756-1655e8d8c5db}

Followup:     MachineOwner
---------

0:000> k
 # Child-SP          RetAddr               Call Site
00 00000030`760f64f8 00000030`00000000     ntdll!NtGetContextThread+0x14
01 00000030`760f6500 00000002`760f0100     0x00000030`00000000
02 00000030`760f6508 0000003e`0000003e     0x00000002`760f0100
03 00000030`760f6510 00000000`0000001f     0x0000003e`0000003e
04 00000030`760f6518 00000000`0208003e     0x1f
05 00000030`760f6520 00000030`760f6660     0x208003e
06 00000030`760f6528 00000000`00000000     0x00000030`760f6660
