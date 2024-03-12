
## This portion tells us the general failure occured due to dereferencing a pointer to NULL
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000<br>
<br>
## These lines show what function/file the failure happened in:
In this case it happened in faulty_write which is a part of the faulty module
```
pc : faulty_write+0x14/0x20 [faulty]<br>
faulty_write+0x14/0x20 [faulty]<br>
```
<br>
## This line additionally tells us about the hexadecimal offset fromthe beggining of the function where the issue occurred. in this case it is 0x14.<br>

```
faulty_write+0x14/0x20 [faulty]<br>
```
## Full print below
<br>

```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000<br>
Mem abort info:<br>
  ESR = 0x96000045<br>
  EC = 0x25: DABT (current EL), IL = 32 bits<br>
  SET = 0, FnV = 0<br>
  EA = 0, S1PTW = 0<br>
  FSC = 0x05: level 1 translation fault<br>
Data abort info:<br>
  ISV = 0, ISS = 0x00000045<br>
  CM = 0, WnR = 1<br>
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000422d0000<br>
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000<br>
Internal error: Oops: 96000045 [#1] SMP<br>
Modules linked in: hello(O) scull(O) faulty(O)<br>
CPU: 0 PID: 159 Comm: sh Tainted: G           O      5.15.18 #1<br>
Hardware name: linux,dummy-virt (DT)<br>
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)<br>
pc : faulty_write+0x14/0x20 [faulty]<br>
lr : vfs_write+0xa8/0x2b0<br>
sp : ffffffc008c83d80<br>
x29: ffffffc008c83d80 x28: ffffff80020d8000 x27: 0000000000000000<br>
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000<br>
x23: 0000000040001000 x22: 0000000000000012 x21: 0000005582d72a70<br>
x20: 0000005582d72a70 x19: ffffff800207d000 x18: 0000000000000000<br>
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000<br>
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000<br>
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000<br>
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000<br>
x5 : 0000000000000001 x4 : ffffffc0006f0000 x3 : ffffffc008c83df0<br>
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000<br>
Call trace:<br>
 faulty_write+0x14/0x20 [faulty]<br>
 ksys_write+0x68/0x100<br>
 __arm64_sys_write+0x20/0x30<br>
 invoke_syscall+0x54/0x130<br>
 el0_svc_common.constprop.0+0x44/0xf0<br>
 do_el0_svc+0x40/0xa0<br>
 el0_svc+0x20/0x60<br>
 el0t_64_sync_handler+0xe8/0xf0<br>
 el0t_64_sync+0x1a0/0x1a4<br>
Code: d2800001 d2800000 d503233f d50323bf (b900003f)<br>
---[ end trace c561a1b4985f2d20 ]---<br>
```
<br>
<br>

## Running objdump and getting relevant data

```
./buildroot/output/host/bin/aarch64-linux-objdump -S ./buildroot/output/target/lib/modules/5.15.18/extra/faulty.ko
```
See that below offset for 0x14 is the store command which is trying to store into address 0 which is what caused the issue.<br>

```
./buildroot/output/target/lib/modules/5.15.18/extra/faulty.ko:     file format elf64-littleaarch64

Disassembly of section .text:
0000000000000000 <faulty_write>:
   0:   d503245f        bti     c
   4:   d2800001        mov     x1, #0x0                        // #0
   8:   d2800000        mov     x0, #0x0                        // #0
   c:   d503233f        paciasp
  10:   d50323bf        autiasp
  14:   b900003f        str     wzr, [x1]
  18:   d65f03c0        ret
  1c:   d503201f        nop
```
