# EQUIP ALL compiled ABI investigation

Exact old DLL SHA256: 6DBA991508E8FF8B48F31FEA8FDCF6880F060F5AF2CDBC0718A08B59D8BF9AD0.
Preferred base 10000000; dump loaded base 5B390000. MAP places the
inlined lookup in TryPickupOrdinary at RVA B78C0. Deferred parent is RVA
B7D20. ApplyPendingMask is inlined. Capstone decoded the actual PE bytes.

## Earliest proven defect

Old emitted code:

    100B7AB6 call eax                 ; FName constructor
    100B7AB8 mov ecx,[ebp-10h]        ; GameSystem
    100B7ABB mov eax,[ecx]            ; vtable
    100B7ABD mov eax,[eax+100DC7E4h]  ; WRONG: address of C++ constexpr
    100B7AC3 call eax

MSVC inline assembly resolved kGsVtableLookupOff as a symbol address,
not its value 20Ch. This is a PROVEN invalid factory lookup contract.
The correction uses literal 20Ch. It does not alter arguments or cleanup.
New emitted code at RVA B7A8D is `mov eax,[eax+20Ch]`, followed by
`call eax`. The compiled regression rejects the exact old DLL and passes
the new DLL.

## Native comparison and stack accounting

Native code comes from the archived decrypted text at
research/etw/capture-20261006-205508/Singularity.text-memory.bin,
base 00402000. Installed on-disk EXE text is packed and cannot be used
as decrypted native instructions. Native F03450 and 11B4B6E both load
the vtable and add literal 20Ch before their indirect call.

Let S be ESP before argument setup. Both mod and native mission caller:

    S-04  trailing factory argument = 1
    S-08  first argument after FName = 0 (duplicate caller uses 1)
    S-16  FName temporary, 8 bytes; ECX = S-16
    S-20  constructor argument 3 = 1
    S-24  constructor argument 2 = 1
    S-28  constructor argument 1 = character-data pointer
    S-32  CALL return address

004AFFD0 preserves ESI, forwards the text pointer and mode to 004AC030,
and returns with RET 0Ch. ESP is then S-16. Factory ECX is GameSystem.
Its CALL puts its return address at S-20. The factory receives FName at
[entry ESP+4..+11], then 0 at +12 and 1 at +16; RET 10h restores ESP=S.
The factory preserves ESI. The mod parent saves/restores EBX/ESI/EDI and
uses EBP-based locals around inline assembly. No extra ADD ESP is needed.
No evidence supports changing these arguments: F03450 uses 0,1 while
the duplicate-specific 11B4B6E path uses 1,1. Their deeper semantic names
are not independently established here.

Native F03436 reads catalog data, tests row+10h, takes row+0Ch and then
dereferences it if nonempty; otherwise uses 0179E4D0. The mod's
FStringPtrAtCatalogRowSecondary implements the same pointer expression:
character data, not the FString object address. Row/range checks precede
lookup; those checks do not prove future UObject lifetime. No new lifetime
claim is made, and the working identity/READY/acquisition policy is unchanged.

## Dump correlation

Both dumps name thread 19172, the deferred-acquisition thread. EXE base
00400000; mod base 5B390000. First exception is execute AV at 39503946,
EAX=39503946, ECX=13B33F00, EDX=022AFC78. Saved EIP=779CC6E1 is
already ntdll context; ESP=022AFFE4, EBP=022AFFEC. No original caller
stack is recoverable from that saved context. The bad vtable displacement
would read 5CE3A3DC for the verified vtable 019CDBF8 after relocation;
that address is not captured in the dump. Thus connection of its value
to 39503946 is INFERRED, not proven by captured memory.

Second exception: EIP=01580142, bytes 8B 0F (`mov ecx,[edi]`), EDI=1:
read AV at 1. ESP=022AFE64, EBP=30933B70, EAX=0, ECX=892F519E,
EDX=00030000. Raw stack contains mod RVA 48522D repeatedly plus game
addresses 01580568, 0157F491, 0156F4C6. These are raw candidates, not
a reliable unwound acquisition stack. Same PID/thread and later WER timing
support a secondary failure, but do not prove a common causal chain.
EDI=1 cannot be attributed to our argument pushes merely because they use 1.

## Verification and verdict

Compiled regression: old PE FAIL as expected; corrected PE PASS.
Production build /O2 PASS, machine 014C / optional magic 010B (PE32).
New DLL SHA256: E0380B57869A94BF7801F5A3991D312C8495D3894FC7273D9A0E38ABD8549440.

FIX READY FOR REVIEW for the proven emitted-call defect. NO-GO for
claiming the complete crash resolved without runtime evidence. No Steam
changes, launch, UI edits or PSVR2 request. LockerVrLoadout remains OFF.
If review requires exact crash causality, the missing datum is the value
read at the erroneous vtable address at first exception, with the original
exception context. The existing dumps cannot supply it. No speculative
second patch or exception-swallowing workaround is included.
