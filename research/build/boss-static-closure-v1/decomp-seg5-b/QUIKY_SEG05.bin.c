/* Decompiled focused external-state closure from QUIKY_SEG05.bin */
/* Entries are address-qualified; containing functions are reported explicitly. */

/* requested 0x05CD; function player_external_05CD at 0x1485 */

/* WARNING: Possible PIC construction at 0x000002a6: Changing call to branch */

void player_external_05CD(int param_1)

{
  code *pcVar1;
  uint in_AX;
  uint uVar2;
  undefined2 unaff_SS;
  bool bVar3;
  int in_stack_00000000;
  
  if (((in_AX < 0xfe00) && ((undefined1 *)(in_AX + 0x200) < &stack0x0000)) &&
     (uVar2 = -((int)(in_AX + 0x200) - (int)&stack0x0000), *(uint *)0xa <= uVar2)) {
    if (uVar2 < *(uint *)0xe) {
      *(uint *)0xe = uVar2;
    }
    return;
  }
  uRamffff35f0 = 0xca;
  if (in_stack_00000000 != 0 || param_1 != 0) {
    bVar3 = (bool)verr();
    if (bVar3) {
      param_1 = *(int *)0x0;
    }
    else {
      param_1 = -1;
      in_stack_00000000 = -1;
    }
  }
  iRamffff35f2 = in_stack_00000000;
  iRamffff35f4 = param_1;
  if (iRamffff35f6 == 0) {
    if (in_stack_00000000 != 0 || param_1 != 0) {
      func_0x00000301();
      func_0x00000301();
      func_0x00000301();
      pcVar1 = (code *)swi(0x21);
      (*pcVar1)();
    }
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)();
  }
  if ((int)((ulong)uRamffff35ec >> 0x10) == 0 && (int)uRamffff35ec == 0) {
    return;
  }
  uRamffff35ec = 0;
  uRamffff35f8 = 0;
  return;
}



/* requested 0x1B7E; function player_external_1B7E at 0x7038 */

void player_external_1B7E(undefined2 param_1,undefined1 param_2,int param_3,undefined1 *param_4)

{
  undefined1 *puVar1;
  undefined1 *puVar2;
  
  puVar2 = (undefined1 *)param_4;
  for (; param_3 != 0; param_3 = param_3 + -1) {
    puVar1 = puVar2;
    puVar2 = puVar2 + 1;
    *puVar1 = param_2;
  }
  return;
}



/* requested 0x0271; function player_external_0271 at 0x625 */

/* WARNING: Possible PIC construction at 0x000002a6: Changing call to branch */
/* WARNING: Removing unreachable block (ram,0x00000284) */
/* WARNING: Removing unreachable block (ram,0x00000290) */
/* WARNING: Removing unreachable block (ram,0x00000289) */
/* WARNING: Removing unreachable block (ram,0x000002b2) */

void player_external_0271(void)

{
  code *pcVar1;
  undefined2 in_AX;
  
  uRamffff35f2 = 0;
  uRamffff35f4 = 0;
  uRamffff35f0 = in_AX;
  if (iRamffff35f6 == 0) {
    uRamffff35f4 = 0;
    uRamffff35f2 = 0;
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)();
  }
  if ((int)((ulong)uRamffff35ec >> 0x10) == 0 && (int)uRamffff35ec == 0) {
    return;
  }
  uRamffff35ec = 0;
  uRamffff35f8 = 0;
  return;
}



