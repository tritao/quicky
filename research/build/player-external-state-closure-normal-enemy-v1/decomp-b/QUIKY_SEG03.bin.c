/* Decompiled focused external-state closure from QUIKY_SEG03.bin */
/* Entries are address-qualified; containing functions are reported explicitly. */

/* requested 0x0E06; function are_object_factory at 0x3590 */

/* Scans the 64-entry pooled-object array and initializes a free object; the normal ARE path returns
   ES:DI and type 0x2B is initialized by the caller. */

void are_object_factory(void)

{
  undefined2 in_AX;
  int iVar1;
  int iVar2;
  undefined2 uVar3;
  undefined2 unaff_DS;
  
  iVar1 = 0x40;
  uVar3 = (undefined2)((ulong)*(undefined4 *)0x755e >> 0x10);
  iVar2 = (int)*(undefined4 *)0x755e;
  do {
    if (*(int *)(iVar2 + 0x18) == 0) {
      *(undefined2 *)(iVar2 + 0x18) = in_AX;
      *(undefined2 *)(iVar2 + 0x1c) = 0x1997;
      *(undefined1 *)(iVar2 + 0x28) = 1;
      *(undefined1 *)(iVar2 + 0x17) = 1;
      *(undefined2 *)(iVar2 + 0x12) = 0xffff;
      *(undefined2 *)(iVar2 + 0x1a) = 0xffff;
      *(undefined2 *)(iVar2 + 0x14) = 0;
      func_0x0000ffff();
      return;
    }
    iVar2 = iVar2 + *(int *)0x30ce;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  out(0x3c8,0);
  out(0x3c9,0x3f);
  out(0x3c9,0);
  out(0x3c9,0);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* requested 0x0B56; function player_external_0B56 at 0x2902 */

void player_external_0B56(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x7966 = 0;
  *(undefined2 *)0x7566 = 0xffff;
  *(undefined2 *)0x7766 = 0xffff;
  func_0x0000ffff();
  *(int *)0x881a = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  return;
}



/* requested 0x0E96; function object_update_pass_by_phase at 0x3734 */

/* Runs pooled-object callbacks in phase order using object byte +0x17 values 0, 1, and 2. */

void object_update_pass_by_phase(void)

{
  code *pcVar1;
  undefined2 in_CX;
  undefined2 in_DX;
  uint uVar2;
  undefined2 unaff_BP;
  code *pcVar3;
  code *pcVar4;
  undefined2 uVar5;
  undefined2 unaff_DS;
  code *pcVar6;
  code *pcStack_2;
  
  uVar5 = *(undefined2 *)0x7560;
  *(undefined2 *)0x88c8 = 0;
  uVar2 = *(uint *)0x7966;
  *(uint *)0x7966 = uVar2 + 0x200 & 0x200;
  uVar2 = uVar2 & 0x200;
  pcVar3 = (code *)(uVar2 + 0x7566);
  pcStack_2 = pcVar3;
  while( true ) {
    pcVar1 = pcVar3;
    pcVar4 = pcStack_2;
    if (*(code **)pcVar1 == (code *)&SUB_0000_ffff) break;
    pcVar4 = pcVar3 + 4;
    pcVar3 = pcVar3 + 8;
    if (*(char *)(*(int *)pcVar4 + 0x17) == '\0') {
      (**(code **)pcVar1)(unaff_DS,uVar5,*(int *)pcVar4,pcVar3,unaff_BP,&pcStack_2);
      unaff_DS = 0;
      func_0x0000ffff();
      *(int *)0x88c8 = *(int *)0x88c8 + 1;
    }
  }
  while( true ) {
    pcVar1 = pcVar4;
    if (*(code **)pcVar1 == (code *)&SUB_0000_ffff) break;
    pcVar3 = pcVar4 + 4;
    pcVar4 = pcVar4 + 8;
    if (*(char *)(*(int *)pcVar3 + 0x17) == '\x01') {
      (**(code **)pcVar1)(unaff_DS,uVar5,*(int *)pcVar3,pcVar4,unaff_BP,&pcStack_2);
      unaff_DS = 0;
      func_0x0000ffff();
      *(int *)0x88c8 = *(int *)0x88c8 + 1;
    }
  }
  while (pcVar3 = *(code **)pcStack_2, pcVar3 != (code *)&SUB_0000_ffff) {
    pcVar4 = pcStack_2 + 4;
    pcStack_2 = pcStack_2 + 8;
    if (*(char *)(*(int *)pcVar4 + 0x17) == '\x02') {
      pcVar6 = pcStack_2;
      pcStack_2 = pcVar3;
      (*pcVar3)(unaff_DS,uVar5,*(int *)pcVar4,pcVar6,unaff_BP,&stack0x0000,uVar2,in_DX,in_CX);
      unaff_DS = 0;
      func_0x0000ffff();
      pcStack_2 = pcVar6;
      *(int *)0x88c8 = *(int *)0x88c8 + 1;
    }
  }
  pcStack_2 = (code *)0xf38;
  func_0x00000fdc();
  return;
}



/* requested 0x0F3C; function find_object_kind_0x64 at 0x3900 */

/* Scans the object list for an object whose +0x14 kind field equals 0x64; ownership semantics
   remain unresolved. */

void find_object_kind_0x64(void)

{
  int *piVar1;
  int *piVar2;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  piVar2 = (int *)*(undefined2 *)0x36e0;
  do {
    if (*piVar2 == -1) {
      return;
    }
    piVar1 = piVar2 + 2;
    piVar2 = piVar2 + 4;
  } while (*(int *)(*piVar1 + 0x14) != 100);
  *(undefined2 *)0x36e0 = piVar2;
  return;
}



/* requested 0x0FA2; function object_update_pass_nonzero_state at 0x4002 */

/* Runs callbacks for list entries with a non-null callback pointer; list/object state semantics
   remain unresolved. */

void object_update_pass_nonzero_state(void)

{
  code *pcVar1;
  undefined2 in_CX;
  undefined2 in_DX;
  undefined2 in_BX;
  undefined2 unaff_BP;
  int *piVar2;
  undefined2 uVar3;
  undefined2 unaff_DS;
  
  uVar3 = *(undefined2 *)0x7560;
  piVar2 = (int *)((*(uint *)0x7966 & 0x200) + 0x7566);
  while (*piVar2 != -1) {
    pcVar1 = (code *)piVar2[1];
    if (pcVar1 == (code *)0x0) {
      piVar2 = piVar2 + 4;
    }
    else {
      (*pcVar1)(unaff_DS,uVar3,piVar2[2],piVar2,unaff_BP,&stack0x0000,in_BX,in_DX,in_CX,pcVar1);
      piVar2 = piVar2 + 4;
    }
  }
  return;
}



/* requested 0x0FDC; function player_external_0FDC at 0x4060 */

void player_external_0FDC(void)

{
  undefined2 in_CX;
  undefined2 in_DX;
  undefined2 in_BX;
  undefined2 unaff_BP;
  int *piVar1;
  undefined2 uVar2;
  undefined2 unaff_CS;
  undefined2 uVar3;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x8174 = 0;
  uVar2 = *(undefined2 *)0x7560;
  piVar1 = (int *)((*(uint *)0x7966 & 0x200) + 0x7566);
  while (*piVar1 != -1) {
    if (piVar1[1] == 0) {
      piVar1 = piVar1 + 4;
    }
    else {
      uVar3 = unaff_CS;
      if ((*(uint *)(piVar1[2] + 0x12) & 0x8000) == 0) {
        uVar3 = 0;
        func_0x0000ffff(unaff_CS,unaff_DS,uVar2,piVar1[2],piVar1,unaff_BP,&stack0x0000,in_BX,in_DX,
                        in_CX,piVar1[1]);
      }
      piVar1 = piVar1 + 4;
      unaff_CS = uVar3;
    }
  }
  return;
}



/* requested 0x1036; function register_object_scheduler_entry at 0x4150 */

/* Appends the live callback and object offset to the active scheduler bank and writes its
   terminator. */

undefined4 register_object_scheduler_entry(void)

{
  int *piVar1;
  int iVar2;
  undefined2 in_AX;
  undefined2 in_DX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  piVar1 = (int *)(unaff_DI + 0x18);
  *piVar1 = *piVar1;
  if (*piVar1 == 0) {
    return CONCAT22(in_DX,in_AX);
  }
  iVar2 = *(int *)0x7966;
  *(undefined2 *)(iVar2 + 0x7568) = *(undefined2 *)(unaff_DI + 0x1c);
  *(undefined2 *)(iVar2 + 0x7566) = *(undefined2 *)(unaff_DI + 0x18);
  *(int *)(iVar2 + 0x756a) = unaff_DI;
  *(int *)0x7966 = *(int *)0x7966 + 8;
  *(undefined2 *)(iVar2 + 0x756e) = 0xffff;
  return CONCAT22(in_DX,in_AX);
}



/* requested 0x0517; function player_external_0517 at 0x1303 */

void player_external_0517(void)

{
  undefined2 uVar1;
  int iVar2;
  int in_AX;
  byte bVar3;
  int unaff_BP;
  int iVar4;
  undefined2 uVar5;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  bool in_ZF;
  
  if (!in_ZF) {
    if (in_AX == 1) {
      *(undefined1 *)(unaff_BP + -0x27) = 1;
    }
    else if (in_AX == 2) {
      *(undefined1 *)(unaff_BP + -0x26) = 1;
    }
    else {
      *(undefined1 *)(unaff_BP + -0x25) = 1;
    }
  }
  uVar5 = (undefined2)((ulong)*(undefined4 *)0x6d8a >> 0x10);
  iVar4 = (int)*(undefined4 *)0x6d8a + *(int *)(unaff_BP + -0x2a) * *(int *)0x30d2;
  bVar3 = (byte)*(undefined2 *)(unaff_BP + 10) & 3;
  *(uint *)(unaff_BP + -0x24) = 0x1111 << bVar3 | 0x1111U >> 0x10 - bVar3;
  uVar1 = *(undefined2 *)0x6572;
  out(0x3c4,2);
  iVar2 = *(int *)(iVar4 + 0x18);
  *(int *)(unaff_BP + -0x12) = iVar2;
  iVar4 = *(int *)(iVar4 + 0x1a);
  *(int *)(unaff_BP + -0x10) = iVar4;
  if (iVar4 != 0 || iVar2 != 0) {
    (*(code *)*(undefined2 *)(unaff_BP + -0x12))();
  }
  return;
}



/* requested 0x04DF; function player_external_04DF at 0x1247 */

void player_external_04DF(void)

{
  byte *pbVar1;
  undefined2 uVar2;
  int iVar3;
  int in_AX;
  uint uVar4;
  byte bVar5;
  int in_CX;
  int unaff_BP;
  int unaff_DI;
  int iVar6;
  undefined2 uVar7;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  int in_stack_00000000;
  
  pbVar1 = (byte *)(unaff_BP + unaff_DI + -0x7438);
  bVar5 = (byte)in_CX & 7;
  *pbVar1 = *pbVar1 >> bVar5 | *pbVar1 << 8 - bVar5;
  *(int *)(unaff_BP + 8) = *(int *)0x81ac + ((in_AX - in_CX) - in_stack_00000000);
  *(undefined1 *)(unaff_BP + -0x25) = 0;
  *(undefined1 *)(unaff_BP + -0x26) = 0;
  *(undefined1 *)(unaff_BP + -0x27) = 0;
  uVar4 = *(uint *)(unaff_BP + 10) & 3;
  if (uVar4 != 0) {
    if (uVar4 == 1) {
      *(undefined1 *)(unaff_BP + -0x27) = 1;
    }
    else if (uVar4 == 2) {
      *(undefined1 *)(unaff_BP + -0x26) = 1;
    }
    else {
      *(undefined1 *)(unaff_BP + -0x25) = 1;
    }
  }
  uVar7 = (undefined2)((ulong)*(undefined4 *)0x6d8a >> 0x10);
  iVar6 = (int)*(undefined4 *)0x6d8a + *(int *)(unaff_BP + -0x2a) * *(int *)0x30d2;
  bVar5 = (byte)*(undefined2 *)(unaff_BP + 10) & 3;
  *(uint *)(unaff_BP + -0x24) = 0x1111 << bVar5 | 0x1111U >> 0x10 - bVar5;
  uVar2 = *(undefined2 *)0x6572;
  out(0x3c4,2);
  iVar3 = *(int *)(iVar6 + 0x18);
  *(int *)(unaff_BP + -0x12) = iVar3;
  iVar6 = *(int *)(iVar6 + 0x1a);
  *(int *)(unaff_BP + -0x10) = iVar6;
  if (iVar6 != 0 || iVar3 != 0) {
    (*(code *)*(undefined2 *)(unaff_BP + -0x12))();
  }
  return;
}



/* requested 0x1C6E; function map_word_probe_16px at 0x7278 */

/* Computes a 16-pixel MAP address, returns the raw word, and tests bit 0x4000. */

undefined4 map_word_probe_16px(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)(*(int *)0x657a + (in_BX >> 4) * 2 + (int)lVar1));
}



/* requested 0x1C92; function player_external_1C92 at 0x7314 */

undefined4 player_external_1C92(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)(*(int *)0x657a + (in_BX >> 4) * 2 + (int)lVar1));
}



/* requested 0x1CDA; function stream_are_regions at 0x7386 */

/* Streams ARE declarations for newly visible 64-pixel regions using camera coordinates and the
   reference grid. */

uint stream_are_regions(void)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  int *piVar6;
  undefined2 unaff_DS;
  undefined2 uVar7;
  
  uVar1 = *(uint *)0x81c0 & 0xffc0;
  uVar3 = *(uint *)0x3712;
  if (*(uint *)0x3710 != uVar1) {
    if ((int)*(uint *)0x3710 < (int)uVar1) {
      uVar1 = uVar1 + 0x180;
    }
    if (0x3f < (int)uVar1) {
      uVar1 = uVar1 - 0x40;
    }
    uVar5 = uVar3;
    if (0x3f < (int)uVar3) {
      uVar5 = uVar3 - 0x40;
    }
    *(uint *)0x3714 = uVar1;
    *(uint *)0x3716 = uVar5;
    iVar4 = 6;
    uVar7 = (undefined2)((ulong)*(undefined4 *)0x796c >> 0x10);
    iVar2 = (uVar5 >> 6) * *(int *)0x7968;
    piVar6 = (int *)((int)*(undefined4 *)0x796c + (uVar1 >> 6) * 2 + iVar2);
    do {
      if ((iVar2 <= *(int *)0x7972) && (*piVar6 != -1)) {
        iVar2 = instantiate_are_declaration(iVar4,uVar3);
      }
      piVar6 = (int *)((int)piVar6 + *(int *)0x7968);
      *(int *)0x3716 = *(int *)0x3716 + 0x40;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  uVar1 = *(uint *)0x81c4 & 0xffc0;
  uVar3 = *(uint *)0x3710;
  if (*(uint *)0x3712 != uVar1) {
    if ((int)*(uint *)0x3712 < (int)uVar1) {
      uVar1 = uVar1 + 0x100;
    }
    if (0x3f < (int)uVar1) {
      uVar1 = uVar1 - 0x40;
    }
    uVar5 = uVar3;
    if (0x3f < (int)uVar3) {
      uVar5 = uVar3 - 0x40;
    }
    *(uint *)0x3716 = uVar1;
    *(uint *)0x3714 = uVar5;
    iVar4 = 8;
    uVar7 = (undefined2)((ulong)*(undefined4 *)0x796c >> 0x10);
    iVar2 = (uVar1 >> 6) * *(int *)0x7968;
    piVar6 = (int *)((int)*(undefined4 *)0x796c + (uVar5 >> 6) * 2 + iVar2);
    do {
      if ((iVar2 <= *(int *)0x7972) && (*piVar6 != -1)) {
        iVar2 = instantiate_are_declaration();
      }
      piVar6 = piVar6 + 1;
      *(int *)0x3714 = *(int *)0x3714 + 0x40;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    *(uint *)0x3712 = *(uint *)0x81c4 & 0xffc0;
  }
  *(uint *)0x3710 = *(uint *)0x81c0 & 0xffc0;
  return uVar3;
}



/* requested 0x1E04; function instantiate_are_declaration at 0x7684 */

/* Runtime-confirmed six-byte ARE record walker: type, local X, local Y; marks records processed and
   creates objects at region origin plus local coordinates. */

void instantiate_are_declaration(void)

{
  uint uVar1;
  int iVar2;
  char cVar3;
  uint *in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 uVar4;
  undefined2 unaff_DS;
  undefined2 in_FS;
  int iVar5;
  
  for (; uVar1 = *in_BX, uVar1 != 0xffff; in_BX = in_BX + 3) {
    uVar4 = unaff_CS;
    if ((char)(uVar1 >> 8) != '\x01') {
      cVar3 = (char)uVar1;
      *in_BX = CONCAT11(1,cVar3);
      if (cVar3 == 'f') {
        uVar4 = 0;
        func_0x0000ffff(unaff_CS);
      }
      else if (cVar3 == 'e') {
        uVar4 = 0;
        func_0x0000ffff(unaff_CS);
      }
      else if (cVar3 == 'g') {
        uVar4 = 0;
        func_0x0000ffff(unaff_CS);
      }
      else {
        uVar4 = 0;
        iVar5 = unaff_DI;
        func_0x0000ffff(unaff_CS);
        *(undefined1 *)(unaff_DI + 0x17) = *(undefined1 *)((uVar1 & 0xff) * 4 + -0x7e2c);
        *(undefined2 *)(unaff_DI + 0x1a) = in_BX;
        uVar1 = in_BX[2];
        iVar2 = *(int *)0x3716;
        *(long *)(unaff_DI + 6) = (ulong)(uVar1 + iVar2) << 0x10;
        *(long *)(unaff_DI + 2) = CONCAT22(uVar1 + iVar2,in_BX[1] + *(int *)0x3714) << 0x10;
        unaff_DI = iVar5;
      }
    }
    unaff_CS = uVar4;
  }
  return;
}



/* requested 0x321F; function player_external_321F at 0x12831 */

void player_external_321F(void)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  func_0x0000ffff(0);
  uVar2 = *(undefined4 *)0x81be;
  uVar3 = *(undefined4 *)0x81a6;
  func_0x0000ffff(0);
  *(long *)0x81be = *(long *)0x81be + 0x2000000;
  *(long *)0x81a6 = *(long *)0x81a6 + 0x2000000;
  if ((int)((uint)((ulong)*(undefined4 *)0x81be >> 0x13) + 0x2c) <= *(int *)0x657e) {
    iVar4 = 0x200;
    do {
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar1 = *(undefined2 *)0x817c;
      *(undefined2 *)0x817c = *(undefined2 *)0x817a;
      *(undefined2 *)0x817a = uVar1;
      *(undefined1 *)0x81d1 = 1;
      func_0x0000ffff(0);
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    func_0x0000ffff(0);
    *(undefined4 *)0x81be = uVar2;
    *(undefined4 *)0x81a6 = uVar3;
    return;
  }
  func_0x0000ffff(0);
  *(long *)0x81be = *(long *)0x81be + -0x4000000;
  *(long *)0x81a6 = *(long *)0x81a6 + -0x4000000;
  iVar4 = 0x200;
  do {
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    uVar1 = *(undefined2 *)0x817c;
    *(undefined2 *)0x817c = *(undefined2 *)0x817a;
    *(undefined2 *)0x817a = uVar1;
    *(undefined1 *)0x81d1 = 1;
    func_0x0000ffff(0);
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  func_0x0000ffff(0);
  *(undefined4 *)0x81be = uVar2;
  *(undefined4 *)0x81a6 = uVar3;
  return;
}



/* requested 0x1749; function create_dedicated_are_effect at 0x5961 */

/* Shared creator used by types 0x65/0x66/0x67 after selecting subtype 0x00/0x08/0x10. */

void create_dedicated_are_effect(void)

{
  byte bVar1;
  int iVar2;
  ulong in_EDX;
  undefined2 in_BX;
  undefined2 unaff_DS;
  
  if (*(int *)0x895e != 0x80) {
    iVar2 = *(int *)0x895e;
    *(int *)0x895e = *(int *)0x895e + 1;
    iVar2 = *(char *)(iVar2 + -0x76a0) * 8;
    *(ulong *)(iVar2 + 0x6586) = in_EDX >> 0x10 & 0xfffffff0 | in_EDX << 0x10;
    *(undefined2 *)(iVar2 + 0x658a) = in_BX;
    bVar1 = func_0x0000ffff();
    *(byte *)(iVar2 + 0x658c) = bVar1 & 7;
    *(undefined1 *)(iVar2 + 0x658d) = *(undefined1 *)0x36ee;
  }
  return;
}



/* requested 0x178D; function create_are_type_65 at 0x6029 */

/* Dedicated ARE type 0x65 wrapper. */

void create_are_type_65(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 0;
  func_0x0000ffff();
  return;
}



/* requested 0x1798; function create_are_type_66 at 0x6040 */

/* Dedicated ARE type 0x66 wrapper. */

void create_are_type_66(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 8;
  func_0x0000ffff();
  return;
}



/* requested 0x17A3; function create_are_type_67 at 0x6051 */

/* Dedicated ARE type 0x67 wrapper. */

void create_are_type_67(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 0x10;
  func_0x0000ffff();
  return;
}



/* requested 0x1892; function player_external_1892 at 0x6290 */

uint player_external_1892(undefined2 param_1,undefined2 param_2,int *param_3)

{
  char cVar1;
  int *piVar2;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  undefined2 in_stack_00000000;
  uint in_stack_00000012;
  
  do {
    func_0x0000ffff(unaff_CS);
    *(char *)0x36ef = *(char *)0x36ef + '\x01';
    if (*(char *)0x36ef == '\x01') {
      if (0x6985 < (int)(param_3 + 4)) {
        *(undefined2 *)0x36f0 = 0;
        return in_stack_00000012;
      }
      *(undefined2 *)0x36f0 = param_3 + -0x32bf;
      return in_stack_00000012;
    }
    while( true ) {
      do {
        piVar2 = param_3;
        param_3 = piVar2 + 4;
        if (0x6985 < (int)param_3) {
          *(undefined2 *)0x36f0 = 0;
          return in_stack_00000012;
        }
        in_stack_00000012 = 0;
      } while (*param_3 == 0);
      in_stack_00000012 = (*param_3 - *(int *)0x81c0) + 0x80;
      if ((in_stack_00000012 < 0x201) &&
         (in_stack_00000012 = (piVar2[5] - *(int *)0x81c4) + 0x80, in_stack_00000012 < 0x181))
      break;
      *(int *)0x895e = *(int *)0x895e + -1;
      *(undefined1 *)(*(int *)0x895e + -0x76a0) = (char)((uint)(piVar2 + -0x32bf) >> 3);
      *(undefined1 *)(piVar2[6] + 1) = 0;
      *param_3 = 0;
    }
    cVar1 = (char)piVar2[7] + '\x01';
    if (cVar1 == '\b') {
      cVar1 = '\0';
    }
    *(char *)(piVar2 + 7) = cVar1;
    unaff_CS = 0;
    unaff_DS = in_stack_00000000;
  } while( true );
}



/* requested 0x5C11; function player_external_5C11 at 0x23569 */

int player_external_5C11(void)

{
  int iVar1;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  return (int)*(char *)(iVar1 + 0x646c);
}



/* requested 0x05A0; function seg3_target_05a0 at 0x1440 */

/* NE relocation target in segment 3; semantics not assigned yet. */

undefined2 seg3_target_05a0(undefined2 param_1,int param_2)

{
  int iVar1;
  undefined2 uVar2;
  undefined2 unaff_DS;
  undefined2 uStack_6;
  
  func_0x0000ffff();
  func_0x0000ffff(0,param_2);
  uStack_6 = 0;
  while( true ) {
    uVar2 = (undefined2)((ulong)*(undefined4 *)0x6d8a >> 0x10);
    iVar1 = (int)*(undefined4 *)0x6d8a + uStack_6 * 0x2c;
    if (*(int *)(iVar1 + 0x10) == 0 && *(int *)(iVar1 + 0x12) == 0) {
      *(int *)(param_2 * 2 + 0x6d8e) = uStack_6;
      uStack_6 = 399;
    }
    if (uStack_6 == 399) break;
    uStack_6 = uStack_6 + 1;
  }
  return *(undefined2 *)(param_2 * 2 + 0x6d8e);
}



/* requested 0x1DCA; function object_camera_visibility_gate at 0x7626 */

/* Returns carry set when ES:DI+04/+08 falls outside the camera window derived from DS:81C0/81C4;
   uses a 0x80 margin and 0x240/0x1B0 extents. */

void object_camera_visibility_gate(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (((uint)((0x80 - *(int *)0x81c0) + *(int *)(unaff_DI + 4)) < 0x241) &&
     ((uint)((0x80 - *(int *)0x81c4) + *(int *)(unaff_DI + 8)) < 0x1b1)) {
    return;
  }
  return;
}



/* requested 0x1DEE; function deactivate_object_outside_camera at 0x7662 */

/* Clears ES:DI+18 and the byte at FS:[ES:DI+1A+1] after the camera gate rejects an object. */

void deactivate_object_outside_camera(void)

{
  undefined2 uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  uVar1 = *(undefined2 *)0x796e;
  iVar2 = *(int *)(unaff_DI + 0x1a);
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  *(undefined1 *)(iVar2 + 1) = 0;
  return;
}



/* requested 0x1C4D; function check_object_map_contact at 0x7245 */

/* Forms the directional type-0x33 MAP probe and forwards it to the 16-pixel raw MAP-word helper. */

void check_object_map_contact(void)

{
  func_0x0000ffff();
  return;
}



/* requested 0x1BD1; function player_external_1BD1 at 0x7121 */

void player_external_1BD1(void)

{
  uint uVar1;
  uint uVar2;
  int in_CX;
  int in_DX;
  uint uVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  uVar2 = *(int *)(unaff_DI + 8) + in_CX;
  uVar3 = *(int *)(unaff_DI + 4) + in_DX;
  uVar1 = *(uint *)(*(int *)0x6582 +
                    (*(uint *)(*(int *)0x657a + (uVar3 >> 4) * 2 + (uVar2 >> 4) * *(int *)0x657e) &
                    0x1ff) * *(int *)0x30d4 + 2);
  if ((uVar1 & 0xf) != 0) {
    if ((uVar2 & 8) == 0) {
      if ((uVar3 & 8) == 0) {
        if ((uVar1 & 8) != 0) {
          return;
        }
      }
      else if ((uVar1 & 4) != 0) {
        return;
      }
    }
    else if ((uVar3 & 8) == 0) {
      if ((uVar1 & 1) != 0) {
        return;
      }
    }
    else if ((uVar1 & 2) != 0) {
      return;
    }
  }
  return;
}



/* requested 0x1B07; function apply_tile_transition_1B07 at 0x6919 */

/* Transition-state helper: clears response bits, zeroes timer, sets mode FF, copies +0x64 into
   velocity Y, and clears +0x3B/+0x3A. */

void apply_tile_transition_1B07(void)

{
  int iVar1;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)0x881a;
  *(uint *)0x8950 = *(uint *)0x8950 & 0xffcf;
  *(undefined1 *)(iVar1 + 0x3b) = 0;
  *(undefined2 *)(iVar1 + 0x3e) = 1000;
  *(undefined1 *)(iVar1 + 0x37) = 0xff;
  *(undefined1 *)(iVar1 + 0x3a) = 0;
  *(undefined4 *)(iVar1 + 0xe) = *(undefined4 *)(iVar1 + 100);
  if ((*(uint *)0x8950 & 4) == 0) {
    func_0x0000ffff();
  }
  return;
}



/* requested 0x1B5D; function apply_player_displacement at 0x7005 */

/* Type-0x34 action helper: updates player state and applies the observed fixed-point displacement
   before the effect sink. */

void apply_player_displacement(void)

{
  long *plVar1;
  int iVar2;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  iVar2 = *(int *)0x881a;
  func_0x0000ffff();
  *(undefined1 *)(iVar2 + 0x2b) = 0xff;
  plVar1 = (long *)(iVar2 + 0xe);
  *plVar1 = *plVar1 + -0x1b000;
  return;
}



/* requested 0x0442; function player_external_0442 at 0x1090 */

void player_external_0442(undefined2 param_1,uint param_2)

{
  code *pcVar1;
  int iVar2;
  undefined2 uVar3;
  undefined2 unaff_DS;
  int iStack_12;
  undefined2 uStack_10;
  undefined2 uStack_e;
  undefined2 uStack_6;
  
  uStack_6 = 0x44d;
  func_0x0000ffff();
  if ((param_2 < 1000) && (iVar2 = *(int *)(param_2 * 2 + 0x6d8e), iVar2 != -1)) {
    uStack_e = *(undefined2 *)((int)*(undefined4 *)0x6d8a + iVar2 * 0x2c);
    uStack_10 = *(undefined2 *)((int)*(undefined4 *)0x6d8a + iVar2 * 0x2c + 2);
    uVar3 = (undefined2)((ulong)*(undefined4 *)0x6d8a >> 0x10);
    iVar2 = (int)*(undefined4 *)0x6d8a + iVar2 * *(int *)0x30d2;
    out(0x3c4,2);
    pcVar1 = (code *)*(int *)(iVar2 + 0x18);
    iStack_12 = *(int *)(iVar2 + 0x1a);
    if (iStack_12 != 0 || pcVar1 != (code *)0x0) {
      (*pcVar1)(0);
    }
  }
  return;
}



/* requested 0x0598; function player_external_0517 at 0x1303 */

void player_external_0517(void)

{
  undefined2 uVar1;
  int iVar2;
  int in_AX;
  byte bVar3;
  int unaff_BP;
  int iVar4;
  undefined2 uVar5;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  bool in_ZF;
  
  if (!in_ZF) {
    if (in_AX == 1) {
      *(undefined1 *)(unaff_BP + -0x27) = 1;
    }
    else if (in_AX == 2) {
      *(undefined1 *)(unaff_BP + -0x26) = 1;
    }
    else {
      *(undefined1 *)(unaff_BP + -0x25) = 1;
    }
  }
  uVar5 = (undefined2)((ulong)*(undefined4 *)0x6d8a >> 0x10);
  iVar4 = (int)*(undefined4 *)0x6d8a + *(int *)(unaff_BP + -0x2a) * *(int *)0x30d2;
  bVar3 = (byte)*(undefined2 *)(unaff_BP + 10) & 3;
  *(uint *)(unaff_BP + -0x24) = 0x1111 << bVar3 | 0x1111U >> 0x10 - bVar3;
  uVar1 = *(undefined2 *)0x6572;
  out(0x3c4,2);
  iVar2 = *(int *)(iVar4 + 0x18);
  *(int *)(unaff_BP + -0x12) = iVar2;
  iVar4 = *(int *)(iVar4 + 0x1a);
  *(int *)(unaff_BP + -0x10) = iVar4;
  if (iVar4 != 0 || iVar2 != 0) {
    (*(code *)*(undefined2 *)(unaff_BP + -0x12))();
  }
  return;
}



/* requested 0x386F; function player_external_386F at 0x14447 */

void player_external_386F(void)

{
  undefined2 in_AX;
  undefined2 in_CX;
  undefined2 in_BX;
  undefined2 unaff_DS;
  undefined2 uStack_12;
  undefined2 uStack_10;
  undefined2 uStack_e;
  undefined2 uStack_c;
  undefined2 uStack_a;
  undefined2 uStack_8;
  
  uStack_a = (undefined2)*(undefined4 *)0x81aa;
  uStack_8 = (undefined2)((ulong)*(undefined4 *)0x81aa >> 0x10);
  uStack_e = (undefined2)*(undefined4 *)0x81be;
  uStack_c = (undefined2)((ulong)*(undefined4 *)0x81be >> 0x10);
  uStack_12 = (undefined2)*(undefined4 *)0x81c2;
  uStack_10 = (undefined2)((ulong)*(undefined4 *)0x81c2 >> 0x10);
  *(undefined4 *)0x81be = 0;
  *(undefined4 *)0x81c2 = 0;
  *(undefined4 *)0x81a6 = 0;
  *(undefined4 *)0x81aa = 0;
  *(undefined2 *)0x817c = 0;
  func_0x0000ffff();
  *(undefined4 *)0x81c2 = CONCAT22(in_BX,in_CX);
  *(undefined4 *)0x81be = CONCAT22(uStack_12,in_AX);
  *(undefined4 *)0x81aa = CONCAT22(uStack_e,uStack_10);
  *(undefined4 *)0x81a6 = CONCAT22(uStack_a,uStack_c);
  *(undefined2 *)0x817c = uStack_8;
  return;
}



/* requested 0x3971; function player_external_3971 at 0x14705 */

void player_external_3971(void)

{
  func_0x0000ffff();
  return;
}



/* requested 0x3986; function player_external_3986 at 0x14726 */

void player_external_3986(void)

{
  func_0x0000ffff();
  return;
}



/* requested 0x39FE; function query_player_collision_state at 0x14846 */

/* Returns the persistent-player X/Y and collision-class byte used by the type-0x34 proximity test.
    */

undefined2 query_player_collision_state(void)

{
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(int *)0x89ea == 0) {
    return *(undefined2 *)(*(int *)0x881a + 4);
  }
  return 0;
}



/* requested 0x3D02; function apply_descriptor_vertical_correction at 0x15618 */

/* Near leaf: gates on +0x3B, reads descriptor at (x,y), retries at y-8 when DX&30 is clear, halves
   velocity Y, computes target Y, and returns at 3D44/3DE4/3DF1. */

int apply_descriptor_vertical_correction(void)

{
  int iVar1;
  byte bVar2;
  int in_AX;
  uint uVar3;
  ulong uVar4;
  uint extraout_DX;
  uint extraout_DX_00;
  uint uVar5;
  int iVar6;
  int unaff_DI;
  undefined2 unaff_ES;
  
  if (*(char *)(unaff_DI + 0x3b) != '\0') {
    *(undefined1 *)(unaff_DI + 0x3a) = 0;
    iVar1 = *(int *)(unaff_DI + 8);
    func_0x0000ffff();
    uVar5 = extraout_DX;
    if ((extraout_DX & 0x30) == 0) {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -8;
      func_0x0000ffff(0);
      uVar5 = extraout_DX_00;
      if ((extraout_DX_00 & 0x30) == 0) {
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 8;
        return iVar1;
      }
    }
    if ((uVar5 & 0x20) == 0) {
      if (*(char *)(unaff_DI + 0x37) == '\0') {
        uVar4 = (ulong)-*(long *)(unaff_DI + 10) >> 1;
        if ((uVar4 & 0x40000000) != 0) {
          uVar4 = uVar4 | 0x80000000;
        }
        *(ulong *)(unaff_DI + 0xe) = uVar4;
      }
      *(undefined1 *)(unaff_DI + 0x3a) = 1;
      uVar3 = 0xf - (*(uint *)(unaff_DI + 4) & 0xf);
    }
    else {
      if (*(char *)(unaff_DI + 0x37) == '\0') {
        uVar4 = *(ulong *)(unaff_DI + 10) >> 1;
        if ((uVar4 & 0x40000000) != 0) {
          uVar4 = uVar4 | 0x80000000;
        }
        *(ulong *)(unaff_DI + 0xe) = uVar4;
      }
      *(undefined1 *)(unaff_DI + 0x3a) = 0xff;
      uVar3 = *(uint *)(unaff_DI + 4) & 0xf;
    }
    iVar6 = (*(uint *)(unaff_DI + 8) & 0xfff0) + (uVar3 >> 1);
    if ((uVar5 & 0x40) == 0) {
      iVar6 = iVar6 + 8;
    }
    bVar2 = (byte)((uint)iVar1 >> 8);
    if (iVar1 < iVar6) {
      *(undefined1 *)(unaff_DI + 0x3a) = 0;
      return (uint)bVar2 << 8;
    }
    if (iVar1 != iVar6) {
      *(int *)(unaff_DI + 8) = iVar6;
    }
    in_AX = (int)CONCAT31((uint3)bVar2,1);
  }
  return in_AX;
}



/* requested 0x3DF2; function snap_player_y_on_side_contact at 0x15858 */

/* Near leaf: requires +0x3B!=0 and +0x3A==0, probes x-5/x+5 through 5C27, and writes object y high
   word &= FFF8 when either probe reports occupancy. It does not snap X. */

void snap_player_y_on_side_contact(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  bool bVar1;
  
  if ((*(char *)(unaff_DI + 0x3b) != '\0') && (*(char *)(unaff_DI + 0x3a) == '\0')) {
    bVar1 = *(int *)(unaff_DI + 4) == 5;
    func_0x0000ffff();
    if (bVar1) {
      bVar1 = *(int *)(unaff_DI + 4) == -5;
      func_0x0000ffff(0);
      if (bVar1) {
        return;
      }
    }
    *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff8;
  }
  return;
}



/* requested 0x1B77; function save_collision_probe_context at 0x7031 */

/* Saves the four incoming collision registers before the type-0x33 MAP contact chain. */

undefined2 save_collision_probe_context(void)

{
  undefined2 in_AX;
  int in_CX;
  undefined2 in_DX;
  int in_BX;
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined4 uVar2;
  
  *(undefined2 *)0x36f4 = in_AX;
  *(int *)0x36f6 = in_BX;
  *(int *)0x36f8 = in_CX;
  *(undefined2 *)0x36fa = in_DX;
  uVar2 = func_0x0000ffff();
  iVar1 = *(int *)(unaff_DI + 4) + *(int *)0x36f4;
  if ((((iVar1 < in_CX) && ((int)uVar2 < iVar1 + *(int *)0x36f8)) &&
      (iVar1 = *(int *)(unaff_DI + 8) + *(int *)0x36f6, iVar1 < (int)((ulong)uVar2 >> 0x10))) &&
     (in_BX < iVar1 + *(int *)0x36fa)) {
    if (*(int *)0x8810 == 0) {
      func_0x0000ffff(0);
      return 2;
    }
    return 1;
  }
  return 0;
}



/* requested 0x393C; function compute_player_collision_bounds at 0x14652 */

/* Returns four bounds from the object pointed to by DS:881A: position fields plus +2C/+30/+2E/+32,
   or four zeroes when DS:89EA is nonzero. */

undefined4 compute_player_collision_bounds(void)

{
  int iVar1;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)0x881a;
  if (*(int *)0x89ea == 0) {
    return CONCAT22(*(int *)(iVar1 + 8) + *(int *)(iVar1 + 0x32),
                    *(int *)(iVar1 + 4) + *(int *)(iVar1 + 0x2c));
  }
  return 0;
}



/* requested 0x3A8A; function dispatch_special_tile_contact at 0x14986 */

/* Far leaf: when mode +0x37 is positive, reads the tile ID at (x,y) and dispatches IDs 0B/0C/0D to
   transition handlers. */

void dispatch_special_tile_contact(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  if ('\0' < *(char *)(unaff_DI + 0x37)) {
    iVar1 = func_0x0000ffff();
    if (((iVar1 != 0xd) && (iVar1 != 0xc)) && (iVar1 != 0xb)) {
      return;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
  }
  return;
}



/* requested 0x38CA; function player_external_38CA at 0x14538 */

void player_external_38CA(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((*(char *)(unaff_DI + 0x37) == '\0') && (*(int *)0x88b6 == 1)) {
    *(undefined4 *)(unaff_DI + 0x5c) = 0x30000;
    return;
  }
  return;
}



/* requested 0x38EC; function player_external_38EC at 0x14572 */

void player_external_38EC(void)

{
  uint *unaff_DI;
  undefined2 unaff_ES;
  
  if ((*unaff_DI & 0x10) == 0) {
    *(undefined1 *)(unaff_DI + 0x1e) = 0;
  }
  else if ((char)unaff_DI[0x1e] == '\0') {
    *(undefined1 *)(unaff_DI + 0x1e) = 0xff;
    func_0x0000ffff();
    *(undefined1 *)((int)unaff_DI + 0x29) = (char)unaff_DI[0x14];
    *(undefined1 *)((int)unaff_DI + 0x17) = 1;
    *(undefined4 *)(unaff_DI + 3) = *(undefined4 *)(unaff_DI + 3);
    *(undefined4 *)(unaff_DI + 1) = *(undefined4 *)(unaff_DI + 1);
  }
  return;
}



/* requested 0x3FF8; function update_player at 0x16376 */

/* Player callback. Ordinary path calls hazard_right, hazard_plus5, transition_tile_probe, then
   state-dependent descriptor leaves. */

void update_player(void)

{
  uint *puVar1;
  uint uVar2;
  undefined2 uVar3;
  long lVar4;
  undefined2 extraout_var;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar5;
  
  func_0x0000ffff();
  bVar5 = false;
  if (*(int *)0x89ea != 0) {
    if (*(int *)0x89ea == -1) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0xfffe;
      *(undefined2 *)0x8822 = 0;
      func_0x0000ffff(0);
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(long *)(unaff_DI + 7) = *(long *)(unaff_DI + 7) + 0x1800;
    if (0x1ffff < *(long *)(unaff_DI + 7)) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 2;
    }
    if (*(char *)((int)unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_02;
    }
    if ((uVar2 & 0x70) == 0) {
      bVar5 = false;
      uVar3 = func_0x0000ffff(0);
      if (!bVar5) {
        *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + CONCAT22(extraout_var,uVar3);
        *(long *)(unaff_DI + 1) = *(long *)(unaff_DI + 1) + -0x5000;
      }
    }
player_update_transition_motion:
    *(int *)0x89ea = *(int *)0x89ea + -1;
    if (*(int *)0x89ea < -0x50) {
      if (*(int *)0x89ea < -0x15d) {
        *(undefined2 *)0x89ec = 0xffff;
      }
    }
    else if (-0x15 < *(int *)0x89ea) {
      return;
    }
    return;
  }
  func_0x0000ffff(0);
  if ((bVar5) || (func_0x0000ffff(0), bVar5)) {
LAB_0000_41c1:
    unaff_DI[0x1f] = 999;
LAB_0000_41cf:
    *(undefined1 *)((int)unaff_DI + 0x37) = 1;
    (unaff_DI + 7)[0] = 0;
    (unaff_DI + 7)[1] = 0;
    func_0x0000ffff(0);
  }
  else {
    func_0x0000ffff(0);
    unaff_DI[9] = unaff_DI[9] & 0xfff;
    *(undefined4 *)(unaff_DI + 0x22) = *(undefined4 *)(unaff_DI + 3);
    *(undefined4 *)(unaff_DI + 0x24) = *(undefined4 *)(unaff_DI + 1);
    if (*(long *)0x8812 != 0) {
      *(undefined1 *)(unaff_DI + 0x1c) = 0xff;
      *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + *(long *)0x8812 + 1;
      *(undefined4 *)0x8812 = 0;
    }
    uVar2 = *(uint *)0x656c;
    if (*(char *)0x85da < '\x01') {
      uVar2 = func_0x0000ffff(0);
    }
    *(undefined1 *)0x4ff0 = (char)uVar2;
    if (*(char *)((int)unaff_DI + 0x39) != '\0') {
      *(undefined1 *)((int)unaff_DI + 0x39) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 1;
      unaff_DI[0x1f] = 0;
      uVar2 = uVar2 | 0x22;
    }
    if (*(int *)0x89e6 != 0) {
      uVar2 = 0;
    }
    *unaff_DI = uVar2;
    if ((uVar2 & 0x22) == 0) {
      unaff_DI[0x20] = 0;
    }
    unaff_DI[0x20] = unaff_DI[0x20] + 1;
    if (((char)unaff_DI[0x1c] != '\0') && (*(char *)((int)unaff_DI + 0x37) != '\0')) {
LAB_0000_427f:
      if ((char)unaff_DI[0x1c] == '\0') {
        apply_descriptor_vertical_correction();
        snap_player_y_on_side_contact();
      }
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0;
      if (*(int *)0x4fee < 0xd2) {
        func_0x0000ffff(0);
      }
      *(undefined1 *)(unaff_DI + 0x1b) = 1;
      goto LAB_0000_4384;
    }
    if (((uVar2 & 1) == 0) || (*(char *)((int)unaff_DI + 0x37) != '\0')) {
      *(undefined2 *)0x4fec = 0;
      uVar2 = uVar2 & 0xfffe;
      unaff_DI[0x17] = -unaff_DI[0x39];
      if ((char)unaff_DI[0x15] != '\0') {
        if (*(long *)0x4fe8 != 0) goto LAB_0000_40e2;
        *(undefined1 *)(unaff_DI + 0x15) = 0;
      }
LAB_0000_4159:
      if (0 < *(long *)0x4fe2) {
        *(long *)0x4fe8 = *(long *)0x4fe8 + -0x1000;
        if (*(long *)0x4fe8 < -0x18000) {
          *(undefined4 *)0x4fe8 = 0xfffe8000;
        }
        if ((*(long *)0x4fe2 < 0x100000) &&
           (*(long *)0x4fe8 = *(long *)0x4fe8 + 0x2000, 0 < *(long *)0x4fe8)) {
          *(undefined4 *)0x4fe8 = 0;
        }
        *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
      }
    }
    else {
      uVar2 = uVar2 & 0xfff3;
      unaff_DI[0x17] = -(unaff_DI[0x39] >> 1);
      *(int *)0x4fec = *(int *)0x4fec + 1;
      if (*(int *)0x4fec < 0x3c) goto LAB_0000_4159;
      *(int *)0x4fec = *(int *)0x4fec + -1;
LAB_0000_40e2:
      *(long *)0x4fe8 = *(long *)0x4fe8 + 0x1000;
      if (0x18000 < *(long *)0x4fe8) {
        *(undefined4 *)0x4fe8 = 0x18000;
      }
      if ((0x200000 < *(long *)0x4fe2) &&
         (*(long *)0x4fe8 = *(long *)0x4fe8 + -0x2000, *(long *)0x4fe8 < 0)) {
        *(undefined4 *)0x4fe8 = 0;
      }
      *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
    }
    *unaff_DI = uVar2;
    bVar5 = *(char *)((int)unaff_DI + 0x37) == '\0';
    if (bVar5) {
      player_probe_side_clear();
      if (((bVar5) && ((char)unaff_DI[0x1c] == '\0')) && ((char)unaff_DI[0x1d] == '\0')) {
        unaff_DI[0x1f] = 0;
        goto LAB_0000_41cf;
      }
      snap_player_y_on_side_contact();
      apply_descriptor_vertical_correction();
      if (((*unaff_DI & 0x22) == 0) ||
         ((bVar5 = false, (char)unaff_DI[0x1c] == '\0' &&
          (bVar5 = unaff_DI[0x20] == 0xd, 0xd < (int)unaff_DI[0x20])))) goto LAB_0000_4384;
    }
    else {
      if (*(char *)((int)unaff_DI + 0x37) < '\0') {
        player_external_3986();
        if (bVar5) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x2c);
          if (((*(char *)((int)unaff_DI + 0x2b) == '\0') && ((*unaff_DI & 0x22) == 0)) &&
             (lVar4 < -0x1ffff)) {
            lVar4 = -0x20000;
          }
          if (lVar4 < 0) {
            *(long *)(unaff_DI + 7) = lVar4;
            puVar1 = unaff_DI + 3;
            *(long *)puVar1 = *(long *)puVar1 + lVar4;
            bVar5 = *(long *)puVar1 == 0;
            player_external_3986();
            if (bVar5) goto LAB_0000_4384;
          }
        }
        goto LAB_0000_41c1;
      }
      unaff_DI[0x1f] = unaff_DI[0x1f] + 1;
      bVar5 = unaff_DI[2] == 5;
      func_0x0000ffff(0);
      if (bVar5) {
        bVar5 = unaff_DI[2] == 0xfffb;
        func_0x0000ffff(0);
        if ((bVar5) && (func_0x0000ffff(0), !bVar5)) {
          unaff_DI[4] = unaff_DI[4] & 0xfff0;
        }
      }
      if ((((*unaff_DI & 0x22) == 0) || (0x13 < (int)unaff_DI[0x20])) ||
         (bVar5 = unaff_DI[0x1f] == 10, 9 < (int)unaff_DI[0x1f])) {
        *(undefined1 *)((int)unaff_DI + 0x2b) = 0;
        if (((char)unaff_DI[0x1c] == '\0') &&
           (apply_descriptor_vertical_correction(), (char)unaff_DI[0x1d] == '\0')) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28);
          if (*(long *)(unaff_DI + 0x30) <= *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28)) {
            lVar4 = *(long *)(unaff_DI + 0x30);
          }
          *(long *)(unaff_DI + 7) = lVar4;
          puVar1 = unaff_DI + 3;
          *(long *)puVar1 = *(long *)puVar1 + *(long *)(unaff_DI + 7);
          bVar5 = *(long *)puVar1 == 0;
          player_probe_side_clear();
          if (bVar5) goto LAB_0000_4384;
        }
        goto LAB_0000_427f;
      }
    }
    player_external_3971();
    if (bVar5) {
      *(undefined2 *)0x612e = 0;
      func_0x0000ffff(0);
      unaff_DI[0x1f] = 1000;
      *(undefined1 *)((int)unaff_DI + 0x3b) = 0;
      *(undefined1 *)(unaff_DI + 0x1d) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0xff;
      *(undefined4 *)(unaff_DI + 7) = *(undefined4 *)(unaff_DI + 0x32);
      func_0x0000ffff(0);
    }
  }
LAB_0000_4384:
  player_external_38CA();
  player_external_38EC();
  player_collision_probe_3ab9();
  func_0x0000ffff();
  player_collision_probe_3a62();
  player_collision_probe_3e41();
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x1c) = 0;
  if (unaff_DI[0x1a] != 0) {
    uVar2 = unaff_DI[0x1a] - 1;
    if (uVar2 == 0) {
      *(undefined2 *)0x8810 = 0;
    }
    if ((uVar2 & 2) != 0) {
      unaff_DI[9] = unaff_DI[9] | 0x8000;
    }
    unaff_DI[0x1a] = uVar2;
  }
  if (*(int *)0x81cc <= (int)(unaff_DI[4] - *(int *)0x81c4)) {
    func_0x0000ffff();
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*unaff_DI == 0)) {
    *(int *)0x4fee = *(int *)0x4fee + 1;
    if (*(int *)0x4fee == 0xd2) {
      func_0x0000ffff();
    }
  }
  else {
    *(undefined2 *)0x4fee = 0;
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*(int *)0x89e6 == -1)) {
    func_0x0000ffff();
  }
  return;
}



/* requested 0x41C1; function update_player at 0x16376 */

/* Player callback. Ordinary path calls hazard_right, hazard_plus5, transition_tile_probe, then
   state-dependent descriptor leaves. */

void update_player(void)

{
  uint *puVar1;
  uint uVar2;
  undefined2 uVar3;
  long lVar4;
  undefined2 extraout_var;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar5;
  
  func_0x0000ffff();
  bVar5 = false;
  if (*(int *)0x89ea != 0) {
    if (*(int *)0x89ea == -1) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0xfffe;
      *(undefined2 *)0x8822 = 0;
      func_0x0000ffff(0);
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(long *)(unaff_DI + 7) = *(long *)(unaff_DI + 7) + 0x1800;
    if (0x1ffff < *(long *)(unaff_DI + 7)) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 2;
    }
    if (*(char *)((int)unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_02;
    }
    if ((uVar2 & 0x70) == 0) {
      bVar5 = false;
      uVar3 = func_0x0000ffff(0);
      if (!bVar5) {
        *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + CONCAT22(extraout_var,uVar3);
        *(long *)(unaff_DI + 1) = *(long *)(unaff_DI + 1) + -0x5000;
      }
    }
player_update_transition_motion:
    *(int *)0x89ea = *(int *)0x89ea + -1;
    if (*(int *)0x89ea < -0x50) {
      if (*(int *)0x89ea < -0x15d) {
        *(undefined2 *)0x89ec = 0xffff;
      }
    }
    else if (-0x15 < *(int *)0x89ea) {
      return;
    }
    return;
  }
  func_0x0000ffff(0);
  if ((bVar5) || (func_0x0000ffff(0), bVar5)) {
LAB_0000_41c1:
    unaff_DI[0x1f] = 999;
LAB_0000_41cf:
    *(undefined1 *)((int)unaff_DI + 0x37) = 1;
    (unaff_DI + 7)[0] = 0;
    (unaff_DI + 7)[1] = 0;
    func_0x0000ffff(0);
  }
  else {
    func_0x0000ffff(0);
    unaff_DI[9] = unaff_DI[9] & 0xfff;
    *(undefined4 *)(unaff_DI + 0x22) = *(undefined4 *)(unaff_DI + 3);
    *(undefined4 *)(unaff_DI + 0x24) = *(undefined4 *)(unaff_DI + 1);
    if (*(long *)0x8812 != 0) {
      *(undefined1 *)(unaff_DI + 0x1c) = 0xff;
      *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + *(long *)0x8812 + 1;
      *(undefined4 *)0x8812 = 0;
    }
    uVar2 = *(uint *)0x656c;
    if (*(char *)0x85da < '\x01') {
      uVar2 = func_0x0000ffff(0);
    }
    *(undefined1 *)0x4ff0 = (char)uVar2;
    if (*(char *)((int)unaff_DI + 0x39) != '\0') {
      *(undefined1 *)((int)unaff_DI + 0x39) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 1;
      unaff_DI[0x1f] = 0;
      uVar2 = uVar2 | 0x22;
    }
    if (*(int *)0x89e6 != 0) {
      uVar2 = 0;
    }
    *unaff_DI = uVar2;
    if ((uVar2 & 0x22) == 0) {
      unaff_DI[0x20] = 0;
    }
    unaff_DI[0x20] = unaff_DI[0x20] + 1;
    if (((char)unaff_DI[0x1c] != '\0') && (*(char *)((int)unaff_DI + 0x37) != '\0')) {
LAB_0000_427f:
      if ((char)unaff_DI[0x1c] == '\0') {
        apply_descriptor_vertical_correction();
        snap_player_y_on_side_contact();
      }
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0;
      if (*(int *)0x4fee < 0xd2) {
        func_0x0000ffff(0);
      }
      *(undefined1 *)(unaff_DI + 0x1b) = 1;
      goto LAB_0000_4384;
    }
    if (((uVar2 & 1) == 0) || (*(char *)((int)unaff_DI + 0x37) != '\0')) {
      *(undefined2 *)0x4fec = 0;
      uVar2 = uVar2 & 0xfffe;
      unaff_DI[0x17] = -unaff_DI[0x39];
      if ((char)unaff_DI[0x15] != '\0') {
        if (*(long *)0x4fe8 != 0) goto LAB_0000_40e2;
        *(undefined1 *)(unaff_DI + 0x15) = 0;
      }
LAB_0000_4159:
      if (0 < *(long *)0x4fe2) {
        *(long *)0x4fe8 = *(long *)0x4fe8 + -0x1000;
        if (*(long *)0x4fe8 < -0x18000) {
          *(undefined4 *)0x4fe8 = 0xfffe8000;
        }
        if ((*(long *)0x4fe2 < 0x100000) &&
           (*(long *)0x4fe8 = *(long *)0x4fe8 + 0x2000, 0 < *(long *)0x4fe8)) {
          *(undefined4 *)0x4fe8 = 0;
        }
        *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
      }
    }
    else {
      uVar2 = uVar2 & 0xfff3;
      unaff_DI[0x17] = -(unaff_DI[0x39] >> 1);
      *(int *)0x4fec = *(int *)0x4fec + 1;
      if (*(int *)0x4fec < 0x3c) goto LAB_0000_4159;
      *(int *)0x4fec = *(int *)0x4fec + -1;
LAB_0000_40e2:
      *(long *)0x4fe8 = *(long *)0x4fe8 + 0x1000;
      if (0x18000 < *(long *)0x4fe8) {
        *(undefined4 *)0x4fe8 = 0x18000;
      }
      if ((0x200000 < *(long *)0x4fe2) &&
         (*(long *)0x4fe8 = *(long *)0x4fe8 + -0x2000, *(long *)0x4fe8 < 0)) {
        *(undefined4 *)0x4fe8 = 0;
      }
      *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
    }
    *unaff_DI = uVar2;
    bVar5 = *(char *)((int)unaff_DI + 0x37) == '\0';
    if (bVar5) {
      player_probe_side_clear();
      if (((bVar5) && ((char)unaff_DI[0x1c] == '\0')) && ((char)unaff_DI[0x1d] == '\0')) {
        unaff_DI[0x1f] = 0;
        goto LAB_0000_41cf;
      }
      snap_player_y_on_side_contact();
      apply_descriptor_vertical_correction();
      if (((*unaff_DI & 0x22) == 0) ||
         ((bVar5 = false, (char)unaff_DI[0x1c] == '\0' &&
          (bVar5 = unaff_DI[0x20] == 0xd, 0xd < (int)unaff_DI[0x20])))) goto LAB_0000_4384;
    }
    else {
      if (*(char *)((int)unaff_DI + 0x37) < '\0') {
        player_external_3986();
        if (bVar5) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x2c);
          if (((*(char *)((int)unaff_DI + 0x2b) == '\0') && ((*unaff_DI & 0x22) == 0)) &&
             (lVar4 < -0x1ffff)) {
            lVar4 = -0x20000;
          }
          if (lVar4 < 0) {
            *(long *)(unaff_DI + 7) = lVar4;
            puVar1 = unaff_DI + 3;
            *(long *)puVar1 = *(long *)puVar1 + lVar4;
            bVar5 = *(long *)puVar1 == 0;
            player_external_3986();
            if (bVar5) goto LAB_0000_4384;
          }
        }
        goto LAB_0000_41c1;
      }
      unaff_DI[0x1f] = unaff_DI[0x1f] + 1;
      bVar5 = unaff_DI[2] == 5;
      func_0x0000ffff(0);
      if (bVar5) {
        bVar5 = unaff_DI[2] == 0xfffb;
        func_0x0000ffff(0);
        if ((bVar5) && (func_0x0000ffff(0), !bVar5)) {
          unaff_DI[4] = unaff_DI[4] & 0xfff0;
        }
      }
      if ((((*unaff_DI & 0x22) == 0) || (0x13 < (int)unaff_DI[0x20])) ||
         (bVar5 = unaff_DI[0x1f] == 10, 9 < (int)unaff_DI[0x1f])) {
        *(undefined1 *)((int)unaff_DI + 0x2b) = 0;
        if (((char)unaff_DI[0x1c] == '\0') &&
           (apply_descriptor_vertical_correction(), (char)unaff_DI[0x1d] == '\0')) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28);
          if (*(long *)(unaff_DI + 0x30) <= *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28)) {
            lVar4 = *(long *)(unaff_DI + 0x30);
          }
          *(long *)(unaff_DI + 7) = lVar4;
          puVar1 = unaff_DI + 3;
          *(long *)puVar1 = *(long *)puVar1 + *(long *)(unaff_DI + 7);
          bVar5 = *(long *)puVar1 == 0;
          player_probe_side_clear();
          if (bVar5) goto LAB_0000_4384;
        }
        goto LAB_0000_427f;
      }
    }
    player_external_3971();
    if (bVar5) {
      *(undefined2 *)0x612e = 0;
      func_0x0000ffff(0);
      unaff_DI[0x1f] = 1000;
      *(undefined1 *)((int)unaff_DI + 0x3b) = 0;
      *(undefined1 *)(unaff_DI + 0x1d) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0xff;
      *(undefined4 *)(unaff_DI + 7) = *(undefined4 *)(unaff_DI + 0x32);
      func_0x0000ffff(0);
    }
  }
LAB_0000_4384:
  player_external_38CA();
  player_external_38EC();
  player_collision_probe_3ab9();
  func_0x0000ffff();
  player_collision_probe_3a62();
  player_collision_probe_3e41();
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x1c) = 0;
  if (unaff_DI[0x1a] != 0) {
    uVar2 = unaff_DI[0x1a] - 1;
    if (uVar2 == 0) {
      *(undefined2 *)0x8810 = 0;
    }
    if ((uVar2 & 2) != 0) {
      unaff_DI[9] = unaff_DI[9] | 0x8000;
    }
    unaff_DI[0x1a] = uVar2;
  }
  if (*(int *)0x81cc <= (int)(unaff_DI[4] - *(int *)0x81c4)) {
    func_0x0000ffff();
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*unaff_DI == 0)) {
    *(int *)0x4fee = *(int *)0x4fee + 1;
    if (*(int *)0x4fee == 0xd2) {
      func_0x0000ffff();
    }
  }
  else {
    *(undefined2 *)0x4fee = 0;
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*(int *)0x89e6 == -1)) {
    func_0x0000ffff();
  }
  return;
}



/* requested 0x41CF; function update_player at 0x16376 */

/* Player callback. Ordinary path calls hazard_right, hazard_plus5, transition_tile_probe, then
   state-dependent descriptor leaves. */

void update_player(void)

{
  uint *puVar1;
  uint uVar2;
  undefined2 uVar3;
  long lVar4;
  undefined2 extraout_var;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar5;
  
  func_0x0000ffff();
  bVar5 = false;
  if (*(int *)0x89ea != 0) {
    if (*(int *)0x89ea == -1) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0xfffe;
      *(undefined2 *)0x8822 = 0;
      func_0x0000ffff(0);
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(long *)(unaff_DI + 7) = *(long *)(unaff_DI + 7) + 0x1800;
    if (0x1ffff < *(long *)(unaff_DI + 7)) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 2;
    }
    if (*(char *)((int)unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_02;
    }
    if ((uVar2 & 0x70) == 0) {
      bVar5 = false;
      uVar3 = func_0x0000ffff(0);
      if (!bVar5) {
        *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + CONCAT22(extraout_var,uVar3);
        *(long *)(unaff_DI + 1) = *(long *)(unaff_DI + 1) + -0x5000;
      }
    }
player_update_transition_motion:
    *(int *)0x89ea = *(int *)0x89ea + -1;
    if (*(int *)0x89ea < -0x50) {
      if (*(int *)0x89ea < -0x15d) {
        *(undefined2 *)0x89ec = 0xffff;
      }
    }
    else if (-0x15 < *(int *)0x89ea) {
      return;
    }
    return;
  }
  func_0x0000ffff(0);
  if ((bVar5) || (func_0x0000ffff(0), bVar5)) {
LAB_0000_41c1:
    unaff_DI[0x1f] = 999;
LAB_0000_41cf:
    *(undefined1 *)((int)unaff_DI + 0x37) = 1;
    (unaff_DI + 7)[0] = 0;
    (unaff_DI + 7)[1] = 0;
    func_0x0000ffff(0);
  }
  else {
    func_0x0000ffff(0);
    unaff_DI[9] = unaff_DI[9] & 0xfff;
    *(undefined4 *)(unaff_DI + 0x22) = *(undefined4 *)(unaff_DI + 3);
    *(undefined4 *)(unaff_DI + 0x24) = *(undefined4 *)(unaff_DI + 1);
    if (*(long *)0x8812 != 0) {
      *(undefined1 *)(unaff_DI + 0x1c) = 0xff;
      *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + *(long *)0x8812 + 1;
      *(undefined4 *)0x8812 = 0;
    }
    uVar2 = *(uint *)0x656c;
    if (*(char *)0x85da < '\x01') {
      uVar2 = func_0x0000ffff(0);
    }
    *(undefined1 *)0x4ff0 = (char)uVar2;
    if (*(char *)((int)unaff_DI + 0x39) != '\0') {
      *(undefined1 *)((int)unaff_DI + 0x39) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 1;
      unaff_DI[0x1f] = 0;
      uVar2 = uVar2 | 0x22;
    }
    if (*(int *)0x89e6 != 0) {
      uVar2 = 0;
    }
    *unaff_DI = uVar2;
    if ((uVar2 & 0x22) == 0) {
      unaff_DI[0x20] = 0;
    }
    unaff_DI[0x20] = unaff_DI[0x20] + 1;
    if (((char)unaff_DI[0x1c] != '\0') && (*(char *)((int)unaff_DI + 0x37) != '\0')) {
LAB_0000_427f:
      if ((char)unaff_DI[0x1c] == '\0') {
        apply_descriptor_vertical_correction();
        snap_player_y_on_side_contact();
      }
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0;
      if (*(int *)0x4fee < 0xd2) {
        func_0x0000ffff(0);
      }
      *(undefined1 *)(unaff_DI + 0x1b) = 1;
      goto LAB_0000_4384;
    }
    if (((uVar2 & 1) == 0) || (*(char *)((int)unaff_DI + 0x37) != '\0')) {
      *(undefined2 *)0x4fec = 0;
      uVar2 = uVar2 & 0xfffe;
      unaff_DI[0x17] = -unaff_DI[0x39];
      if ((char)unaff_DI[0x15] != '\0') {
        if (*(long *)0x4fe8 != 0) goto LAB_0000_40e2;
        *(undefined1 *)(unaff_DI + 0x15) = 0;
      }
LAB_0000_4159:
      if (0 < *(long *)0x4fe2) {
        *(long *)0x4fe8 = *(long *)0x4fe8 + -0x1000;
        if (*(long *)0x4fe8 < -0x18000) {
          *(undefined4 *)0x4fe8 = 0xfffe8000;
        }
        if ((*(long *)0x4fe2 < 0x100000) &&
           (*(long *)0x4fe8 = *(long *)0x4fe8 + 0x2000, 0 < *(long *)0x4fe8)) {
          *(undefined4 *)0x4fe8 = 0;
        }
        *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
      }
    }
    else {
      uVar2 = uVar2 & 0xfff3;
      unaff_DI[0x17] = -(unaff_DI[0x39] >> 1);
      *(int *)0x4fec = *(int *)0x4fec + 1;
      if (*(int *)0x4fec < 0x3c) goto LAB_0000_4159;
      *(int *)0x4fec = *(int *)0x4fec + -1;
LAB_0000_40e2:
      *(long *)0x4fe8 = *(long *)0x4fe8 + 0x1000;
      if (0x18000 < *(long *)0x4fe8) {
        *(undefined4 *)0x4fe8 = 0x18000;
      }
      if ((0x200000 < *(long *)0x4fe2) &&
         (*(long *)0x4fe8 = *(long *)0x4fe8 + -0x2000, *(long *)0x4fe8 < 0)) {
        *(undefined4 *)0x4fe8 = 0;
      }
      *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
    }
    *unaff_DI = uVar2;
    bVar5 = *(char *)((int)unaff_DI + 0x37) == '\0';
    if (bVar5) {
      player_probe_side_clear();
      if (((bVar5) && ((char)unaff_DI[0x1c] == '\0')) && ((char)unaff_DI[0x1d] == '\0')) {
        unaff_DI[0x1f] = 0;
        goto LAB_0000_41cf;
      }
      snap_player_y_on_side_contact();
      apply_descriptor_vertical_correction();
      if (((*unaff_DI & 0x22) == 0) ||
         ((bVar5 = false, (char)unaff_DI[0x1c] == '\0' &&
          (bVar5 = unaff_DI[0x20] == 0xd, 0xd < (int)unaff_DI[0x20])))) goto LAB_0000_4384;
    }
    else {
      if (*(char *)((int)unaff_DI + 0x37) < '\0') {
        player_external_3986();
        if (bVar5) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x2c);
          if (((*(char *)((int)unaff_DI + 0x2b) == '\0') && ((*unaff_DI & 0x22) == 0)) &&
             (lVar4 < -0x1ffff)) {
            lVar4 = -0x20000;
          }
          if (lVar4 < 0) {
            *(long *)(unaff_DI + 7) = lVar4;
            puVar1 = unaff_DI + 3;
            *(long *)puVar1 = *(long *)puVar1 + lVar4;
            bVar5 = *(long *)puVar1 == 0;
            player_external_3986();
            if (bVar5) goto LAB_0000_4384;
          }
        }
        goto LAB_0000_41c1;
      }
      unaff_DI[0x1f] = unaff_DI[0x1f] + 1;
      bVar5 = unaff_DI[2] == 5;
      func_0x0000ffff(0);
      if (bVar5) {
        bVar5 = unaff_DI[2] == 0xfffb;
        func_0x0000ffff(0);
        if ((bVar5) && (func_0x0000ffff(0), !bVar5)) {
          unaff_DI[4] = unaff_DI[4] & 0xfff0;
        }
      }
      if ((((*unaff_DI & 0x22) == 0) || (0x13 < (int)unaff_DI[0x20])) ||
         (bVar5 = unaff_DI[0x1f] == 10, 9 < (int)unaff_DI[0x1f])) {
        *(undefined1 *)((int)unaff_DI + 0x2b) = 0;
        if (((char)unaff_DI[0x1c] == '\0') &&
           (apply_descriptor_vertical_correction(), (char)unaff_DI[0x1d] == '\0')) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28);
          if (*(long *)(unaff_DI + 0x30) <= *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28)) {
            lVar4 = *(long *)(unaff_DI + 0x30);
          }
          *(long *)(unaff_DI + 7) = lVar4;
          puVar1 = unaff_DI + 3;
          *(long *)puVar1 = *(long *)puVar1 + *(long *)(unaff_DI + 7);
          bVar5 = *(long *)puVar1 == 0;
          player_probe_side_clear();
          if (bVar5) goto LAB_0000_4384;
        }
        goto LAB_0000_427f;
      }
    }
    player_external_3971();
    if (bVar5) {
      *(undefined2 *)0x612e = 0;
      func_0x0000ffff(0);
      unaff_DI[0x1f] = 1000;
      *(undefined1 *)((int)unaff_DI + 0x3b) = 0;
      *(undefined1 *)(unaff_DI + 0x1d) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0xff;
      *(undefined4 *)(unaff_DI + 7) = *(undefined4 *)(unaff_DI + 0x32);
      func_0x0000ffff(0);
    }
  }
LAB_0000_4384:
  player_external_38CA();
  player_external_38EC();
  player_collision_probe_3ab9();
  func_0x0000ffff();
  player_collision_probe_3a62();
  player_collision_probe_3e41();
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x1c) = 0;
  if (unaff_DI[0x1a] != 0) {
    uVar2 = unaff_DI[0x1a] - 1;
    if (uVar2 == 0) {
      *(undefined2 *)0x8810 = 0;
    }
    if ((uVar2 & 2) != 0) {
      unaff_DI[9] = unaff_DI[9] | 0x8000;
    }
    unaff_DI[0x1a] = uVar2;
  }
  if (*(int *)0x81cc <= (int)(unaff_DI[4] - *(int *)0x81c4)) {
    func_0x0000ffff();
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*unaff_DI == 0)) {
    *(int *)0x4fee = *(int *)0x4fee + 1;
    if (*(int *)0x4fee == 0xd2) {
      func_0x0000ffff();
    }
  }
  else {
    *(undefined2 *)0x4fee = 0;
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*(int *)0x89e6 == -1)) {
    func_0x0000ffff();
  }
  return;
}



/* requested 0x4416; function update_player at 0x16376 */

/* Player callback. Ordinary path calls hazard_right, hazard_plus5, transition_tile_probe, then
   state-dependent descriptor leaves. */

void update_player(void)

{
  uint *puVar1;
  uint uVar2;
  undefined2 uVar3;
  long lVar4;
  undefined2 extraout_var;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar5;
  
  func_0x0000ffff();
  bVar5 = false;
  if (*(int *)0x89ea != 0) {
    if (*(int *)0x89ea == -1) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0xfffe;
      *(undefined2 *)0x8822 = 0;
      func_0x0000ffff(0);
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(long *)(unaff_DI + 7) = *(long *)(unaff_DI + 7) + 0x1800;
    if (0x1ffff < *(long *)(unaff_DI + 7)) {
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 2;
    }
    if (*(char *)((int)unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto player_update_transition_motion;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_02;
    }
    if ((uVar2 & 0x70) == 0) {
      bVar5 = false;
      uVar3 = func_0x0000ffff(0);
      if (!bVar5) {
        *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + CONCAT22(extraout_var,uVar3);
        *(long *)(unaff_DI + 1) = *(long *)(unaff_DI + 1) + -0x5000;
      }
    }
player_update_transition_motion:
    *(int *)0x89ea = *(int *)0x89ea + -1;
    if (*(int *)0x89ea < -0x50) {
      if (*(int *)0x89ea < -0x15d) {
        *(undefined2 *)0x89ec = 0xffff;
      }
    }
    else if (-0x15 < *(int *)0x89ea) {
      return;
    }
    return;
  }
  func_0x0000ffff(0);
  if ((bVar5) || (func_0x0000ffff(0), bVar5)) {
LAB_0000_41c1:
    unaff_DI[0x1f] = 999;
LAB_0000_41cf:
    *(undefined1 *)((int)unaff_DI + 0x37) = 1;
    (unaff_DI + 7)[0] = 0;
    (unaff_DI + 7)[1] = 0;
    func_0x0000ffff(0);
  }
  else {
    func_0x0000ffff(0);
    unaff_DI[9] = unaff_DI[9] & 0xfff;
    *(undefined4 *)(unaff_DI + 0x22) = *(undefined4 *)(unaff_DI + 3);
    *(undefined4 *)(unaff_DI + 0x24) = *(undefined4 *)(unaff_DI + 1);
    if (*(long *)0x8812 != 0) {
      *(undefined1 *)(unaff_DI + 0x1c) = 0xff;
      *(long *)(unaff_DI + 3) = *(long *)(unaff_DI + 3) + *(long *)0x8812 + 1;
      *(undefined4 *)0x8812 = 0;
    }
    uVar2 = *(uint *)0x656c;
    if (*(char *)0x85da < '\x01') {
      uVar2 = func_0x0000ffff(0);
    }
    *(undefined1 *)0x4ff0 = (char)uVar2;
    if (*(char *)((int)unaff_DI + 0x39) != '\0') {
      *(undefined1 *)((int)unaff_DI + 0x39) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 1;
      unaff_DI[0x1f] = 0;
      uVar2 = uVar2 | 0x22;
    }
    if (*(int *)0x89e6 != 0) {
      uVar2 = 0;
    }
    *unaff_DI = uVar2;
    if ((uVar2 & 0x22) == 0) {
      unaff_DI[0x20] = 0;
    }
    unaff_DI[0x20] = unaff_DI[0x20] + 1;
    if (((char)unaff_DI[0x1c] != '\0') && (*(char *)((int)unaff_DI + 0x37) != '\0')) {
LAB_0000_427f:
      if ((char)unaff_DI[0x1c] == '\0') {
        apply_descriptor_vertical_correction();
        snap_player_y_on_side_contact();
      }
      (unaff_DI + 7)[0] = 0;
      (unaff_DI + 7)[1] = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0;
      if (*(int *)0x4fee < 0xd2) {
        func_0x0000ffff(0);
      }
      *(undefined1 *)(unaff_DI + 0x1b) = 1;
      goto LAB_0000_4384;
    }
    if (((uVar2 & 1) == 0) || (*(char *)((int)unaff_DI + 0x37) != '\0')) {
      *(undefined2 *)0x4fec = 0;
      uVar2 = uVar2 & 0xfffe;
      unaff_DI[0x17] = -unaff_DI[0x39];
      if ((char)unaff_DI[0x15] != '\0') {
        if (*(long *)0x4fe8 != 0) goto LAB_0000_40e2;
        *(undefined1 *)(unaff_DI + 0x15) = 0;
      }
LAB_0000_4159:
      if (0 < *(long *)0x4fe2) {
        *(long *)0x4fe8 = *(long *)0x4fe8 + -0x1000;
        if (*(long *)0x4fe8 < -0x18000) {
          *(undefined4 *)0x4fe8 = 0xfffe8000;
        }
        if ((*(long *)0x4fe2 < 0x100000) &&
           (*(long *)0x4fe8 = *(long *)0x4fe8 + 0x2000, 0 < *(long *)0x4fe8)) {
          *(undefined4 *)0x4fe8 = 0;
        }
        *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
      }
    }
    else {
      uVar2 = uVar2 & 0xfff3;
      unaff_DI[0x17] = -(unaff_DI[0x39] >> 1);
      *(int *)0x4fec = *(int *)0x4fec + 1;
      if (*(int *)0x4fec < 0x3c) goto LAB_0000_4159;
      *(int *)0x4fec = *(int *)0x4fec + -1;
LAB_0000_40e2:
      *(long *)0x4fe8 = *(long *)0x4fe8 + 0x1000;
      if (0x18000 < *(long *)0x4fe8) {
        *(undefined4 *)0x4fe8 = 0x18000;
      }
      if ((0x200000 < *(long *)0x4fe2) &&
         (*(long *)0x4fe8 = *(long *)0x4fe8 + -0x2000, *(long *)0x4fe8 < 0)) {
        *(undefined4 *)0x4fe8 = 0;
      }
      *(long *)0x4fe2 = *(long *)0x4fe2 + *(long *)0x4fe8;
    }
    *unaff_DI = uVar2;
    bVar5 = *(char *)((int)unaff_DI + 0x37) == '\0';
    if (bVar5) {
      player_probe_side_clear();
      if (((bVar5) && ((char)unaff_DI[0x1c] == '\0')) && ((char)unaff_DI[0x1d] == '\0')) {
        unaff_DI[0x1f] = 0;
        goto LAB_0000_41cf;
      }
      snap_player_y_on_side_contact();
      apply_descriptor_vertical_correction();
      if (((*unaff_DI & 0x22) == 0) ||
         ((bVar5 = false, (char)unaff_DI[0x1c] == '\0' &&
          (bVar5 = unaff_DI[0x20] == 0xd, 0xd < (int)unaff_DI[0x20])))) goto LAB_0000_4384;
    }
    else {
      if (*(char *)((int)unaff_DI + 0x37) < '\0') {
        player_external_3986();
        if (bVar5) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x2c);
          if (((*(char *)((int)unaff_DI + 0x2b) == '\0') && ((*unaff_DI & 0x22) == 0)) &&
             (lVar4 < -0x1ffff)) {
            lVar4 = -0x20000;
          }
          if (lVar4 < 0) {
            *(long *)(unaff_DI + 7) = lVar4;
            puVar1 = unaff_DI + 3;
            *(long *)puVar1 = *(long *)puVar1 + lVar4;
            bVar5 = *(long *)puVar1 == 0;
            player_external_3986();
            if (bVar5) goto LAB_0000_4384;
          }
        }
        goto LAB_0000_41c1;
      }
      unaff_DI[0x1f] = unaff_DI[0x1f] + 1;
      bVar5 = unaff_DI[2] == 5;
      func_0x0000ffff(0);
      if (bVar5) {
        bVar5 = unaff_DI[2] == 0xfffb;
        func_0x0000ffff(0);
        if ((bVar5) && (func_0x0000ffff(0), !bVar5)) {
          unaff_DI[4] = unaff_DI[4] & 0xfff0;
        }
      }
      if ((((*unaff_DI & 0x22) == 0) || (0x13 < (int)unaff_DI[0x20])) ||
         (bVar5 = unaff_DI[0x1f] == 10, 9 < (int)unaff_DI[0x1f])) {
        *(undefined1 *)((int)unaff_DI + 0x2b) = 0;
        if (((char)unaff_DI[0x1c] == '\0') &&
           (apply_descriptor_vertical_correction(), (char)unaff_DI[0x1d] == '\0')) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28);
          if (*(long *)(unaff_DI + 0x30) <= *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28)) {
            lVar4 = *(long *)(unaff_DI + 0x30);
          }
          *(long *)(unaff_DI + 7) = lVar4;
          puVar1 = unaff_DI + 3;
          *(long *)puVar1 = *(long *)puVar1 + *(long *)(unaff_DI + 7);
          bVar5 = *(long *)puVar1 == 0;
          player_probe_side_clear();
          if (bVar5) goto LAB_0000_4384;
        }
        goto LAB_0000_427f;
      }
    }
    player_external_3971();
    if (bVar5) {
      *(undefined2 *)0x612e = 0;
      func_0x0000ffff(0);
      unaff_DI[0x1f] = 1000;
      *(undefined1 *)((int)unaff_DI + 0x3b) = 0;
      *(undefined1 *)(unaff_DI + 0x1d) = 0;
      *(undefined1 *)((int)unaff_DI + 0x37) = 0xff;
      *(undefined4 *)(unaff_DI + 7) = *(undefined4 *)(unaff_DI + 0x32);
      func_0x0000ffff(0);
    }
  }
LAB_0000_4384:
  player_external_38CA();
  player_external_38EC();
  player_collision_probe_3ab9();
  func_0x0000ffff();
  player_collision_probe_3a62();
  player_collision_probe_3e41();
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x1c) = 0;
  if (unaff_DI[0x1a] != 0) {
    uVar2 = unaff_DI[0x1a] - 1;
    if (uVar2 == 0) {
      *(undefined2 *)0x8810 = 0;
    }
    if ((uVar2 & 2) != 0) {
      unaff_DI[9] = unaff_DI[9] | 0x8000;
    }
    unaff_DI[0x1a] = uVar2;
  }
  if (*(int *)0x81cc <= (int)(unaff_DI[4] - *(int *)0x81c4)) {
    func_0x0000ffff();
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*unaff_DI == 0)) {
    *(int *)0x4fee = *(int *)0x4fee + 1;
    if (*(int *)0x4fee == 0xd2) {
      func_0x0000ffff();
    }
  }
  else {
    *(undefined2 *)0x4fee = 0;
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*(int *)0x89e6 == -1)) {
    func_0x0000ffff();
  }
  return;
}



/* requested 0x44DC; function player_update_transition_motion at 0x17628 */

/* Transition motion helper reached from the callback's nonzero DS:89EA path. */

void player_update_transition_motion(void)

{
  undefined2 unaff_DS;
  
  *(int *)0x89ea = *(int *)0x89ea + -1;
  if (*(int *)0x89ea < -0x50) {
    if (*(int *)0x89ea < -0x15d) {
      *(undefined2 *)0x89ec = 0xffff;
    }
  }
  else if (-0x15 < *(int *)0x89ea) {
    return;
  }
  return;
}



/* requested 0x44FE; function player_update_transition_motion at 0x17628 */

/* Transition motion helper reached from the callback's nonzero DS:89EA path. */

void player_update_transition_motion(void)

{
  undefined2 unaff_DS;
  
  *(int *)0x89ea = *(int *)0x89ea + -1;
  if (*(int *)0x89ea < -0x50) {
    if (*(int *)0x89ea < -0x15d) {
      *(undefined2 *)0x89ec = 0xffff;
    }
  }
  else if (-0x15 < *(int *)0x89ea) {
    return;
  }
  return;
}



/* requested 0x4519; function spawn_contact_effect_entry at 0x17689 */

/* Adds a contact-effect entry and publishes its ring coordinates. */

void spawn_contact_effect_entry(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((*"}-\x14u" < '\x01') && (*(int *)0x880c < 1)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
    return;
  }
  if (*(int *)0x8808 <= *(int *)0x8806) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
    return;
  }
  if (*"}-\x14u" < '\x01') {
    *(int *)0x880c = *(int *)0x880c + -1;
  }
  *(int *)0x8806 = *(int *)0x8806 + 1;
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_contact_effect_entry;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  *(undefined4 *)(unaff_DI + 0x2e) = 0x30000;
  *(undefined4 *)(unaff_DI + 10) = 0;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffeb000;
  *(undefined2 *)0x612e = 8;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -0xf;
  iVar1 = 0;
  while (*(int *)(iVar1 + -0x7822) + *(int *)(iVar1 + -0x7820) != 0) {
    iVar1 = iVar1 + 4;
  }
  *(int *)(unaff_DI + 0x2a) = iVar1;
  *(undefined2 *)(iVar1 + -0x7822) = 1;
  return;
}



/* requested 0x45AB; function update_contact_effect_entry at 0x17835 */

/* Updates an active contact-effect entry and its ring coordinates. */

void update_contact_effect_entry(void)

{
  ulong *puVar1;
  int iVar2;
  long lVar3;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint uVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 uVar5;
  undefined2 unaff_DS;
  bool bVar6;
  
  if (*(int *)(*(int *)(unaff_DI + 0x2a) + -0x7822) == 0) {
    *(undefined2 *)(unaff_DI + 0x18) = (code *)remove_contact_effect_entry;
  }
  if ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U) ||
     (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)) {
    *(undefined2 *)(unaff_DI + 0x18) = (code *)remove_contact_effect_entry;
    return;
  }
  bVar6 = false;
  if (0 < *(long *)(unaff_DI + 0xe)) {
    bVar6 = *(uint *)(unaff_DI + 0x2c) < 2;
    uVar5 = unaff_CS;
    if (*(uint *)(unaff_DI + 0x2c) == 2) goto LAB_0000_4681;
    func_0x0000ffff();
    if (!bVar6) {
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        func_0x0000ffff(0);
        if ((extraout_DX & 0x70) == 0) {
          func_0x0000ffff(0);
          uVar4 = extraout_DX_00;
joined_r0x00004656:
          if ((uVar4 & 0x70) == 0) {
            bVar6 = false;
            unaff_CS = 0;
            goto LAB_0000_4670;
          }
        }
      }
      else {
        func_0x0000ffff(0);
        if ((extraout_DX_01 & 0x70) == 0) {
          func_0x0000ffff(0);
          uVar4 = extraout_DX_02;
          goto joined_r0x00004656;
        }
      }
    }
    unaff_CS = 0;
    *(long *)(unaff_DI + 0xe) = -*(long *)(unaff_DI + 0xe);
    *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
    puVar1 = (ulong *)(unaff_DI + 0x2e);
    bVar6 = *puVar1 < 0x5000;
    *puVar1 = *puVar1 - 0x5000;
  }
LAB_0000_4670:
  uVar5 = 0;
  func_0x0000ffff(unaff_CS);
  if (bVar6) {
    *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
  }
LAB_0000_4681:
  lVar3 = *(long *)(unaff_DI + 0xe);
  if (lVar3 < 1) {
    lVar3 = lVar3 + 12000;
    if (lVar3 < -0x30000) {
      lVar3 = -0x30000;
    }
    else if (*(long *)(unaff_DI + 0x2e) < lVar3) {
      lVar3 = *(long *)(unaff_DI + 0x2e);
    }
  }
  else {
    lVar3 = lVar3 + 14000;
    if (lVar3 < -0x30000) {
      lVar3 = -0x30000;
    }
    else if (*(long *)(unaff_DI + 0x2e) < lVar3) {
      lVar3 = *(long *)(unaff_DI + 0x2e);
    }
  }
  *(long *)(unaff_DI + 0xe) = lVar3;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar3;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + *(char *)(unaff_DI + 0x29) * 4;
  iVar2 = *(int *)(unaff_DI + 0x2a);
  *(undefined2 *)(iVar2 + -0x7822) = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(iVar2 + -0x7820) = *(undefined2 *)(unaff_DI + 8);
  func_0x0000ffff(uVar5);
  return;
}



/* requested 0x470C; function remove_contact_effect_entry at 0x18188 */

/* Removes a contact-effect entry and advances the ring state. */

void remove_contact_effect_entry(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(int *)0x8806 = *(int *)0x8806 + -1;
  iVar1 = *(int *)(unaff_DI + 0x2a);
  *(undefined2 *)(iVar1 + -0x7822) = 0;
  *(undefined2 *)(iVar1 + -0x7820) = 0;
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  return;
}



/* requested 0x5937; function player_external_5937 at 0x22839 */

int player_external_5937(void)

{
  uint uVar1;
  int in_AX;
  uint uVar2;
  int iVar3;
  undefined4 in_EDX;
  ulong uVar4;
  undefined4 uVar5;
  undefined2 unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 uVar7;
  undefined2 uVar8;
  undefined2 unaff_DS;
  undefined2 uVar6;
  
  uVar6 = (undefined2)((ulong)in_EDX >> 0x10);
  if (*(char *)0x85da != '\0') {
    return in_AX;
  }
  uVar2 = *(uint *)0x60d8;
  if (*(uint *)0x60da != uVar2) {
    uVar1 = *(uint *)0x60da;
    *(uint *)0x60da = uVar2;
    uVar2 = uVar2 ^ uVar1;
    if ((uVar2 & 1) != 0) {
      unaff_CS = 0;
      func_0x0000ffff();
    }
    uVar7 = unaff_CS;
    if ((uVar2 & 2) != 0) {
      uVar7 = 0;
      func_0x0000ffff(unaff_CS,unaff_DI,unaff_ES);
    }
    uVar8 = uVar7;
    if ((uVar2 & 4) != 0) {
      uVar8 = 0;
      func_0x0000ffff(uVar7,unaff_DI,unaff_ES);
    }
    uVar7 = uVar8;
    if ((uVar2 & 8) != 0) {
      uVar7 = 0;
      func_0x0000ffff(uVar8,unaff_DI,unaff_ES);
    }
    uVar8 = uVar7;
    if ((uVar2 & 0x10) != 0) {
      uVar8 = 0;
      func_0x0000ffff(uVar7,unaff_DI,unaff_ES);
    }
    uVar7 = uVar8;
    if ((uVar2 & 0x20) != 0) {
      uVar7 = 0;
      func_0x0000ffff(uVar8,unaff_DI,unaff_ES);
    }
    unaff_CS = uVar7;
    if ((uVar2 & 0x40) != 0) {
      unaff_CS = 0;
      func_0x0000ffff(uVar7,unaff_DI,unaff_ES,uVar2);
    }
  }
  uVar4 = *(ulong *)0x881c;
  if (uVar4 != *(ulong *)0x4ff2) {
    *(ulong *)0x4ff2 = uVar4;
    uVar4 = uVar4 % 100000;
    func_0x0000ffff(unaff_CS,uVar4,unaff_DI,unaff_ES);
    uVar4 = uVar4 % 10000;
    func_0x0000ffff(0,uVar4,unaff_DI,unaff_ES);
    uVar4 = uVar4 % 1000;
    func_0x0000ffff(0,uVar4,unaff_DI,unaff_ES);
    uVar4 = uVar4 % 100;
    func_0x0000ffff(0,uVar4,unaff_DI,unaff_ES);
    func_0x0000ffff(0,uVar4 % 10,unaff_DI,unaff_ES);
    uVar6 = 0;
    func_0x0000ffff(0,0,unaff_DI,unaff_ES);
  }
  iVar3 = *(int *)0x4ff8;
  uVar5 = CONCAT22(uVar6,iVar3);
  if (iVar3 != *(int *)0x8822) {
    if (iVar3 < *(int *)0x8822) {
      *(int *)0x4ff8 = iVar3 + 1;
      uVar5 = CONCAT22(uVar6,(iVar3 + 1) * 8);
      func_0x0000ffff();
    }
    else {
      uVar5 = CONCAT22(uVar6,iVar3 << 3);
      func_0x0000ffff();
      *(int *)0x4ff8 = *(int *)0x4ff8 + -1;
    }
  }
  iVar3 = *(int *)0x880a;
  if (iVar3 < 0) {
    iVar3 = 0;
  }
  if (iVar3 != *(int *)0x4ffa) {
    *(int *)0x4ffa = iVar3;
    func_0x0000ffff();
    func_0x0000ffff(0,uVar5,unaff_DI,unaff_ES);
  }
  func_0x0000ffff();
  iVar3 = *(int *)0x880c;
  if (iVar3 != *(int *)0x4ff6) {
    func_0x0000ffff(0,(ulong)(long)iVar3 % 10,unaff_DI,unaff_ES);
    iVar3 = 0;
    func_0x0000ffff(0,0,unaff_DI,unaff_ES);
  }
  return iVar3;
}



/* requested 0x5C27; function probe_descriptor_quadrant at 0x23591 */

/* Far leaf: AX=y, BX=x; reads descriptor record +2 and tests the low-nibble bit selected by AX/BX
   bit 3. Returns the test in flags. */

void probe_descriptor_quadrant(void)

{
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  if ((*(uint *)(*(int *)0x6582 +
                 (*(uint *)(*(int *)0x657a + (in_BX >> 3 & 0xfffe) + (in_AX >> 4) * *(int *)0x657e)
                 & 0x1ff) * *(int *)0x30d4 + 2) & 0xf) == 0) {
    return;
  }
  if ((in_AX & 8) != 0) {
    if ((in_BX & 8) != 0) {
      return;
    }
    return;
  }
  if ((in_BX & 8) != 0) {
    return;
  }
  return;
}



/* requested 0x5CC3; function read_descriptor_word at 0x23747 */

/* Far leaf: AX=y, BX=x; returns descriptor record +2 in DX. Directly consumed by
   descriptor_response_resolver. */

void read_descriptor_word(void)

{
  return;
}



/* requested 0x5D38; function load_animation_descriptor at 0x23864 */

/* Loads a descriptor/table entry into object animation state and selects the current slot/action.
    */

void load_animation_descriptor(void)

{
  undefined2 uVar1;
  int iVar2;
  undefined2 *unaff_SI;
  int *piVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  piVar3 = unaff_SI + 1;
  uVar1 = *unaff_SI;
  *(undefined2 *)(unaff_DI + 0x1e) = uVar1;
  *(undefined2 *)(unaff_DI + 0x20) = uVar1;
  *(undefined2 *)(unaff_DI + 0x22) = piVar3;
  *(undefined2 *)(unaff_DI + 0x24) = piVar3;
  iVar2 = *piVar3;
  if (*(char *)(unaff_DI + 0x28) == -1) {
    iVar2 = iVar2 + 0x32;
  }
  *(int *)(unaff_DI + 0x12) = iVar2;
  return;
}



/* requested 0x5D60; function advance_animation_descriptor at 0x23904 */

/* Decrements the active descriptor timer or advances the descriptor cursor when it expires. */

void advance_animation_descriptor(void)

{
  int iVar1;
  int *piVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(int *)(unaff_DI + 0x20) != 0) {
    *(int *)(unaff_DI + 0x20) = *(int *)(unaff_DI + 0x20) + -1;
    return;
  }
  *(int *)(unaff_DI + 0x24) = *(int *)(unaff_DI + 0x24) + 2;
  for (piVar2 = (int *)*(undefined2 *)(unaff_DI + 0x24); iVar1 = *piVar2, iVar1 < 0;
      piVar2 = piVar2 + iVar1) {
    *(int *)(unaff_DI + 0x24) = *(int *)(unaff_DI + 0x24) + iVar1 * 2;
  }
  if (*(char *)(unaff_DI + 0x28) == -1) {
    iVar1 = iVar1 + 0x32;
  }
  *(int *)(unaff_DI + 0x12) = iVar1;
  *(undefined2 *)(unaff_DI + 0x20) = *(undefined2 *)(unaff_DI + 0x1e);
  return;
}



/* requested 0x5DC3; function player_external_5DC3 at 0x24003 */

undefined4 player_external_5DC3(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)(*(int *)0x657a + (in_BX >> 4) * 2 + (int)lVar1));
}



/* requested 0x6328; function player_external_6328 at 0x25384 */

void player_external_6328(void)

{
  int *piVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  piVar1 = (int *)(unaff_DI + 0x32);
  *piVar1 = *piVar1 + -1;
  if (-1 < *piVar1) {
    return;
  }
  *(undefined2 *)(unaff_DI + 0x32) = 8;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = *(int *)(unaff_DI + 0x2e) + 1;
  if (*(int *)(unaff_DI + 0x2e) == 4) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  return;
}



/* requested 0x16CE; function map_effect_tile_rewrite at 0x5838 */

/* Rewrites one loaded MAP cell as (word & 0xfe00) | (DX & 0x01ff), unless DX bit 0x8000 requests
   the non-MAP path; called by tile-effect state updates. */

void map_effect_tile_rewrite(void)

{
  uint in_AX;
  uint in_CX;
  uint in_DX;
  uint in_BX;
  uint *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((in_DX & 0x8000) == 0) {
    unaff_ES = (undefined2)((ulong)*(undefined4 *)0x657a >> 0x10);
    unaff_DI = (uint *)((int)*(undefined4 *)0x657a + (in_BX >> 4) * *(int *)0x657e +
                       (in_AX >> 4) * 2);
    *unaff_DI = *unaff_DI & 0xfe00;
    *unaff_DI = *unaff_DI | in_DX & 0x1ff;
  }
  func_0x0000ffff();
  unaff_DI[0x16] = 3;
  unaff_DI[9] = 0xffff;
  *(ulong *)(unaff_DI + 1) = (ulong)in_AX << 0x10;
  *(long *)(unaff_DI + 3) = CONCAT22(in_AX,in_BX) << 0x10;
  unaff_DI[0x17] = in_CX;
  return;
}



/* requested 0x10B5; function player_external_10B5 at 0x4277 */

void player_external_10B5(void)

{
  int *piVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  
  piVar1 = (int *)(unaff_DI + 0x2c);
  *piVar1 = *piVar1 + -1;
  if (*piVar1 == 0) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
    return;
  }
  func_0x0000ffff();
  if (!(bool)in_CF) {
    *(undefined2 *)0x36e8 = 0;
    *(int *)0x8184 = (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + *(int *)0x81ac;
    *(undefined2 *)0x8188 = 0x10;
    *(undefined2 *)0x8186 = 0x10;
    *(int *)0x8182 = (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + *(int *)0x81a8;
    *(int *)0x8192 = *(int *)0x81ac + *(int *)0x81cc;
    func_0x0000ffff(0);
    *(undefined2 *)0x8192 = *(undefined2 *)0x81ac;
    func_0x0000ffff(0);
    if ((*(uint *)0x81a2 & 0x8000) == 0) {
      *(undefined2 *)0x8188 = *(undefined2 *)0x8190;
      *(undefined2 *)0x8186 = *(undefined2 *)0x818e;
      *(int *)0x36e8 = 0x10 - *(int *)0x8188;
      *(undefined2 *)0x8182 = *(undefined2 *)0x818a;
      *(undefined2 *)0x8184 = *(undefined2 *)0x818c;
    }
    *(undefined2 *)0x8192 = *(undefined2 *)0x81cc;
    func_0x0000ffff(0);
    *(undefined2 *)0x81a0 = *(undefined2 *)0x81a2;
    func_0x000011b4();
    *(undefined2 *)0x81a0 = 0;
    func_0x000011b4();
    *(undefined2 *)0x81a0 = *(undefined2 *)0x81a4;
    func_0x000011b4();
    *(undefined2 *)0x81a0 = 0;
    func_0x000011b4();
  }
  return;
}



/* requested 0x1693; function player_external_1693 at 0x5779 */

void player_external_1693(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
    return;
  }
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0 + -0x10) {
    return;
  }
  if (*(int *)0x81c4 + 0xaf < *(int *)(unaff_DI + 8)) {
    return;
  }
  if (*(int *)(unaff_DI + 8) < *(int *)0x81c4 + -0xf) {
    return;
  }
  return;
}



/* requested 0x199D; function player_external_199D at 0x6557 */

void player_external_199D(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x8950 = 0;
  *(undefined2 *)0x89ea = 0xffff;
  func_0x0000ffff();
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffe0000;
  *(undefined4 *)(unaff_DI + 0x4c) = 0x2000;
  *(undefined4 *)(unaff_DI + 0x50) = 0x2000;
  *(undefined4 *)(unaff_DI + 0x5c) = 0x18000;
  *(undefined4 *)(unaff_DI + 0x60) = 0x40000;
  *(int *)0x880a = *(int *)0x880a + -1;
  *(undefined2 *)0x8822 = 0;
  return;
}



/* requested 0x19E6; function apply_transition_reset_19E6 at 0x6630 */

/* Transition/reset helper reached from the 0B/0C/0D tile path; resets player motion and shared
   transition globals. */

void apply_transition_reset_19E6(void)

{
  int *piVar1;
  int iVar2;
  int in_BX;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  iVar2 = *(int *)0x881a;
  if (*(int *)(iVar2 + 0x34) == 0) {
    *(undefined2 *)0x612e = 1;
    func_0x0000ffff();
    piVar1 = (int *)0x8822;
    *piVar1 = *piVar1 + -1;
    if (*piVar1 == 0) {
      *(int *)0x880a = *(int *)0x880a + -1;
      *(undefined2 *)0x8950 = 0;
      *(undefined4 *)(iVar2 + 0xe) = 0xfffe0000;
      *(undefined4 *)(iVar2 + 0x4c) = 0x2000;
      *(undefined4 *)(iVar2 + 0x50) = 0x2000;
      *(undefined4 *)(iVar2 + 0x5c) = 0x18000;
      *(undefined4 *)(iVar2 + 0x60) = 0x40000;
      *(undefined2 *)0x89ea = 0xffff;
      func_0x0000ffff(0);
      *(undefined4 *)(iVar2 + 0xe) = 0xfffe0000;
      *(undefined1 *)(iVar2 + 0x3b) = 0;
      *(undefined2 *)(iVar2 + 0x3e) = 1000;
      *(undefined1 *)(iVar2 + 0x37) = 0xff;
      *(undefined1 *)(iVar2 + 0x3a) = 0;
      *(undefined1 *)(iVar2 + 0x2b) = 0;
      *(uint *)0x8950 = *(uint *)0x8950 & 0xffcf;
    }
    else {
      *(undefined2 *)(iVar2 + 0x34) = 0xd2;
    }
    if (-1 < *(int *)(iVar2 + 4) - in_BX) {
      *(undefined4 *)(iVar2 + 10) = 0x18000;
      return;
    }
    *(undefined4 *)(iVar2 + 10) = 0xfffe8000;
  }
  return;
}



/* requested 0x1AAA; function player_external_1AAA at 0x6826 */

void player_external_1AAA(void)

{
  undefined2 uVar1;
  int iVar2;
  int iVar3;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  uVar1 = *(undefined2 *)0x7560;
  iVar2 = *(int *)0x881a;
  iVar3 = *(int *)0x85d2;
  *(long *)(iVar2 + 2) = (ulong)*(uint *)(iVar3 * 4 + -0x77d8) << 0x10;
  *(long *)(iVar2 + 6) = (ulong)*(uint *)(iVar3 * 4 + -0x77d6) << 0x10;
  *(undefined2 *)(iVar2 + 0x18) = (code *)initialize_player;
  *(undefined1 *)(iVar2 + 0x29) = 1;
  *(undefined2 *)0x89ea = 0;
  func_0x0000ffff(0);
  return;
}



/* requested 0x1AE6; function player_external_1AAA at 0x6826 */

void player_external_1AAA(void)

{
  undefined2 uVar1;
  int iVar2;
  int iVar3;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  uVar1 = *(undefined2 *)0x7560;
  iVar2 = *(int *)0x881a;
  iVar3 = *(int *)0x85d2;
  *(long *)(iVar2 + 2) = (ulong)*(uint *)(iVar3 * 4 + -0x77d8) << 0x10;
  *(long *)(iVar2 + 6) = (ulong)*(uint *)(iVar3 * 4 + -0x77d6) << 0x10;
  *(undefined2 *)(iVar2 + 0x18) = (code *)initialize_player;
  *(undefined1 *)(iVar2 + 0x29) = 1;
  *(undefined2 *)0x89ea = 0;
  func_0x0000ffff(0);
  return;
}



/* requested 0x1AF5; function player_external_1AF5 at 0x6901 */

void player_external_1AF5(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
  *(undefined2 *)0x85d2 = 0;
  func_0x0000ffff();
  return;
}



/* requested 0x3376; function map_tile_id_at_pixel at 0x13174 */

/* Far leaf: AX=y pixels, BX=x pixels; reads MAP[(y>>4)*row_stride+(x>>4)*2] and returns raw_cell &
   0x01ff. */

ulong map_tile_id_at_pixel(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)((in_BX >> 4) * 2 + (int)lVar1 + *(int *)0x657a)) & 0xffff01ff;
}



/* requested 0x6370; function probe_contact_tile_offset at 0x25456 */

/* Hazard/effect probe using x+DS:5003 and y or y-+0x72; recognizes tile IDs 5..A and spawns a
   transient object. */

void probe_contact_tile_offset(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x37) < '\0') {
    *(int *)0x4ffe = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x72);
    unaff_CS = 0;
    iVar1 = func_0x0000ffff();
    *(undefined1 *)0x5000 = 0;
    *(undefined2 *)0x5001 = 500;
    if (((iVar1 == 8) || (*(undefined2 *)0x5001 = 0x1f9, iVar1 == 9)) ||
       (*(undefined2 *)0x5001 = 0x1fe, iVar1 == 10)) goto LAB_0000_6420;
  }
  if (*(char *)(unaff_DI + 0x37) != '\0') {
    return;
  }
  *(undefined2 *)0x4ffe = *(undefined2 *)(unaff_DI + 8);
  iVar1 = func_0x0000ffff(unaff_CS);
  *(undefined1 *)0x5000 = 0;
  *(undefined2 *)0x5001 = 500;
  if (((iVar1 != 8) && (*(undefined2 *)0x5001 = 0x1f9, iVar1 != 9)) &&
     (*(undefined2 *)0x5001 = 0x1fe, iVar1 != 10)) {
    *(undefined1 *)0x5000 = 0xff;
    *(undefined2 *)0x5001 = 0x1f2;
    if (((iVar1 != 5) && (*(undefined2 *)0x5001 = 0x1f7, iVar1 != 6)) &&
       (*(undefined2 *)0x5001 = 0x1fc, iVar1 != 7)) {
      return;
    }
  }
LAB_0000_6420:
  *(undefined2 *)0x612e = 7;
  func_0x0000ffff(0);
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x38) = *(undefined1 *)0x5000;
  *(undefined2 *)(unaff_DI + 0x2a) = *(undefined2 *)0x5001;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + (ulong)*(uint *)0x5003 * 0x10000;
  *(long *)(unaff_DI + 6) = (ulong)*(uint *)0x4ffe << 0x10;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x2e) = 0;
  *(undefined2 *)(unaff_DI + 0x36) = 0;
  if (-1 < *(char *)(iVar1 + 0x37)) {
    return;
  }
  return;
}



/* requested 0x6484; function probe_contact_plus5 at 0x25732 */

/* Far wrapper: stores hazard probe offset +5 at DS:5003 and calls hazard_offset_probe. */

void probe_contact_plus5(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x5003 = 5;
  probe_contact_tile_offset();
  return;
}



/* requested 0x648E; function probe_contact_right at 0x25742 */

/* Far hazard/effect probe at x+5. Uses tile IDs 5..A and spawns a transient object; this is
   separate from descriptor geometry. */

void probe_contact_right(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x37) < '\0') {
    *(int *)0x4ffe = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x72);
    unaff_CS = 0;
    iVar1 = func_0x0000ffff();
    *(undefined1 *)0x5000 = 0;
    *(undefined2 *)0x5001 = 500;
    if (((iVar1 == 8) || (*(undefined2 *)0x5001 = 0x1f9, iVar1 == 9)) ||
       (*(undefined2 *)0x5001 = 0x1fe, iVar1 == 10)) goto LAB_0000_653c;
  }
  if (*(char *)(unaff_DI + 0x37) != '\0') {
    return;
  }
  *(undefined2 *)0x4ffe = *(undefined2 *)(unaff_DI + 8);
  iVar1 = func_0x0000ffff(unaff_CS);
  *(undefined1 *)0x5000 = 0;
  *(undefined2 *)0x5001 = 500;
  if (((iVar1 != 8) && (*(undefined2 *)0x5001 = 0x1f9, iVar1 != 9)) &&
     (*(undefined2 *)0x5001 = 0x1fe, iVar1 != 10)) {
    *(undefined1 *)0x5000 = 0xff;
    *(undefined2 *)0x5001 = 0x1f2;
    if (((iVar1 != 5) && (*(undefined2 *)0x5001 = 0x1f7, iVar1 != 6)) &&
       (*(undefined2 *)0x5001 = 0x1fc, iVar1 != 7)) {
      return;
    }
  }
LAB_0000_653c:
  *(undefined2 *)0x612e = 7;
  func_0x0000ffff(0);
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x38) = *(undefined1 *)0x5000;
  *(undefined2 *)(unaff_DI + 0x2a) = *(undefined2 *)0x5001;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + 0x50000;
  *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)0x4ffe;
  *(undefined2 *)(unaff_DI + 6) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x2e) = 0;
  *(undefined2 *)(unaff_DI + 0x36) = 0;
  if (-1 < *(char *)(iVar1 + 0x37)) {
    return;
  }
  return;
}



/* requested 0x684A; function player_external_684A at 0x26698 */

void player_external_684A(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x68c0;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x40) = 0;
  *(undefined4 *)(unaff_DI + 0x36) = *(undefined4 *)(unaff_DI + 6);
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x68C0; function player_external_68C0 at 0x26816 */

void player_external_68C0(void)

{
  int *piVar1;
  ulong *puVar2;
  uint uVar3;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  int iVar4;
  long lVar5;
  ulong uVar6;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar7;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  func_0x0000ffff(0);
  if ((bool)in_CF) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(int *)(unaff_DI + 0x32) < 1) {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar5 < -0x15000) {
        lVar5 = -0x15000;
      }
      else if (0x15000 < lVar5) {
        lVar5 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar5 < -0x15000) {
        lVar5 = -0x15000;
      }
      else if (0x15000 < lVar5) {
        lVar5 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
    iVar4 = *(int *)(*(int *)0x881a + 4);
    if (*(char *)(unaff_DI + 0x29) < '\0') {
      if ((iVar4 + 0x28 <= *(int *)(unaff_DI + 4)) || (*(int *)(unaff_DI + 4) <= iVar4 + 0x23))
      goto LAB_0000_6d01;
    }
    else if ((*(int *)(unaff_DI + 4) <= iVar4 + -0x28) || (iVar4 + -0x23 <= *(int *)(unaff_DI + 4)))
    goto LAB_0000_6d01;
    if ((*(int *)(unaff_DI + 8) < *(int *)(*(int *)0x881a + 8)) &&
       (*(int *)0x81c4 < *(int *)(unaff_DI + 8))) {
      *(undefined2 *)(unaff_DI + 0x32) = 1;
      func_0x0000ffff(0);
    }
    goto LAB_0000_6d01;
  }
  if (*(int *)(unaff_DI + 0x32) < 2) {
    *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x40);
    uVar3 = *(int *)(unaff_DI + 0x3e) + 0x20U & 0x3ff;
    *(uint *)(unaff_DI + 0x3e) = uVar3;
    iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
    *(int *)(unaff_DI + 0x40) = iVar4;
    *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -5000;
    *(long *)(unaff_DI + 2) =
         *(long *)(unaff_DI + 2) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x2000;
    *(int *)(unaff_DI + 0x34) = *(int *)(unaff_DI + 0x34) + 1;
    if (*(int *)(unaff_DI + 0x34) < 0x33) goto LAB_0000_6d01;
    *(undefined2 *)(unaff_DI + 0x34) = 0;
    *(undefined2 *)(unaff_DI + 0x32) = 2;
  }
  if (4 < *(int *)(unaff_DI + 0x32)) {
    if (*(int *)(unaff_DI + 0x32) < 6) {
      *(int *)(unaff_DI + 0x34) = *(int *)(unaff_DI + 0x34) + 1;
      if (*(int *)(unaff_DI + 0x34) < 0x6f) goto LAB_0000_6d01;
      *(undefined2 *)(unaff_DI + 0x34) = 0;
      *(undefined2 *)(unaff_DI + 0x32) = 6;
      func_0x0000ffff(0);
    }
    if (*(char *)(unaff_DI + 0x32) == '\a') {
LAB_0000_6c5e:
      uVar6 = *(long *)(unaff_DI + 0xe) - 4000;
      if ((long)uVar6 < -0x40000) {
        uVar6 = 0xfffc0000;
      }
      else if (0x40000 < (long)uVar6) {
        uVar6 = 0x40000;
      }
      *(ulong *)(unaff_DI + 0xe) = uVar6;
      puVar2 = (ulong *)(unaff_DI + 6);
      bVar7 = CARRY4(*puVar2,uVar6);
      *puVar2 = *puVar2 + uVar6;
      func_0x0000ffff(0);
      if (bVar7) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
      }
      *(long *)(unaff_DI + 2) =
           *(long *)(unaff_DI + 2) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x20000;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(undefined1 *)(unaff_DI + 0x32) = 8;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x46;
      }
    }
    else {
      if (*(char *)(unaff_DI + 0x32) != '\b') {
        *(undefined1 *)(unaff_DI + 0x32) = 7;
        *(undefined4 *)(unaff_DI + 0xe) = 0xfffffe0c;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x46;
        goto LAB_0000_6c5e;
      }
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
    }
    if (*(long *)(unaff_DI + 6) < *(long *)(unaff_DI + 0x36)) {
      *(undefined2 *)(unaff_DI + 0x32) = 0;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      *(undefined4 *)(unaff_DI + 10) = *(undefined4 *)(unaff_DI + 0x3a);
    }
    goto LAB_0000_6d01;
  }
  *(undefined4 *)(unaff_DI + 0x3a) = *(undefined4 *)(unaff_DI + 10);
  if (*(char *)(unaff_DI + 0x32) == '\x03') {
LAB_0000_6b02:
    lVar5 = *(long *)(unaff_DI + 0xe) + 20000;
    if (lVar5 < -0x50000) {
      lVar5 = -0x50000;
    }
    else if (0x50000 < lVar5) {
      lVar5 = 0x50000;
    }
    *(long *)(unaff_DI + 0xe) = lVar5;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
    uVar6 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x10000;
    if ((long)uVar6 < -0x15000) {
      uVar6 = 0xfffeb000;
    }
    else if (0x15000 < (long)uVar6) {
      uVar6 = 0x15000;
    }
    *(ulong *)(unaff_DI + 10) = uVar6;
    puVar2 = (ulong *)(unaff_DI + 2);
    bVar7 = CARRY4(*puVar2,uVar6);
    *puVar2 = *puVar2 + uVar6;
    piVar1 = (int *)(unaff_DI + 0x2d);
    iVar4 = *piVar1;
    *piVar1 = *piVar1 + -1;
    if (SBORROW2(iVar4,1) != *piVar1 < 0) {
      *(undefined1 *)(unaff_DI + 0x32) = 4;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
    }
  }
  else {
    if (*(char *)(unaff_DI + 0x32) != '\x04') {
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      *(undefined4 *)(unaff_DI + 0xe) = 500;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      goto LAB_0000_6b02;
    }
    puVar2 = (ulong *)(unaff_DI + 6);
    bVar7 = CARRY4(*puVar2,*(ulong *)(unaff_DI + 0xe));
    *puVar2 = *puVar2 + *(ulong *)(unaff_DI + 0xe);
  }
  func_0x0000ffff(0);
  if (!bVar7) {
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) == 0) {
        func_0x0000ffff(0);
        uVar3 = extraout_DX_00;
joined_r0x00006bf8:
        if ((uVar3 & 0x70) == 0) goto LAB_0000_6d01;
      }
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) == 0) {
        func_0x0000ffff(0);
        uVar3 = extraout_DX_02;
        goto joined_r0x00006bf8;
      }
    }
  }
  *(undefined2 *)(unaff_DI + 0x32) = 5;
  func_0x0000ffff(0);
LAB_0000_6d01:
  if (*(int *)0x8806 != 0) {
    iVar4 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar4) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar4 = 0;
    }
    iVar4 = iVar4 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar4 + -0x7822)) &&
         (*(int *)(iVar4 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
        (*(int *)(iVar4 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar4 + -0x7820))) {
      *(undefined2 *)(iVar4 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = 0x4ab3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x689F; function player_external_689F at 0x26783 */

void player_external_689F(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_684A();
  return;
}



/* requested 0x68AD; function player_external_68AD at 0x26797 */

void player_external_68AD(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_684A();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x4AB3; function player_external_4AB3 at 0x19123 */

void player_external_4AB3(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  int iVar1;
  int iVar2;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x4c5d;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined2 *)0x612e = 0xd;
  func_0x0000ffff(0);
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(iVar1 + 6);
  *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar1 + 0x29);
  iVar2 = iVar1;
  func_0x0000ffff(0);
  *(undefined1 *)(iVar1 + 0x17) = 1;
  *(undefined4 *)(iVar1 + 2) = *(undefined4 *)(iVar2 + 2);
  *(undefined4 *)(iVar1 + 6) = *(undefined4 *)(iVar2 + 6);
  *(undefined1 *)(iVar1 + 0x29) = *(undefined1 *)(iVar2 + 0x29);
  iVar1 = iVar2;
  func_0x0000ffff(0);
  *(undefined1 *)(iVar2 + 0x17) = 1;
  *(undefined4 *)(iVar2 + 2) = *(undefined4 *)(iVar1 + 2);
  *(undefined4 *)(iVar2 + 6) = *(undefined4 *)(iVar1 + 6);
  *(undefined1 *)(iVar2 + 0x29) = *(undefined1 *)(iVar1 + 0x29);
  return;
}



/* requested 0x4BA0; function player_external_4BA0 at 0x19360 */

void player_external_4BA0(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  int iVar1;
  int iVar2;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x4c5d;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined2 *)0x612e = 2;
  func_0x0000ffff(0);
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(iVar1 + 6);
  *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar1 + 0x29);
  iVar2 = iVar1;
  func_0x0000ffff(0);
  *(undefined1 *)(iVar1 + 0x17) = 1;
  *(undefined4 *)(iVar1 + 2) = *(undefined4 *)(iVar2 + 2);
  *(undefined4 *)(iVar1 + 6) = *(undefined4 *)(iVar2 + 6);
  *(undefined1 *)(iVar1 + 0x29) = *(undefined1 *)(iVar2 + 0x29);
  iVar1 = iVar2;
  func_0x0000ffff(0);
  *(undefined1 *)(iVar2 + 0x17) = 1;
  *(undefined4 *)(iVar2 + 2) = *(undefined4 *)(iVar1 + 2);
  *(undefined4 *)(iVar2 + 6) = *(undefined4 *)(iVar1 + 6);
  *(undefined1 *)(iVar2 + 0x29) = *(undefined1 *)(iVar1 + 0x29);
  return;
}



/* requested 0x4C5D; function player_external_4C5D at 0x19549 */

void player_external_4C5D(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
  if (0x28 < *(int *)(unaff_DI + 0x2a)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  return;
}



/* requested 0x4C8B; function player_external_4C8B at 0x19595 */

void player_external_4C8B(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x4dce;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0x19;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  *(long *)(unaff_DI + 0xe) = (long)(int)*(char *)(iVar1 + 0x646c) * -0x100 + -0x14000;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  lVar2 = (long)(int)*(char *)(iVar1 + 0x646c) * 0x400 + -0x6000;
  *(long *)(unaff_DI + 10) = lVar2;
  if (lVar2 < 0) {
    *(undefined1 *)(unaff_DI + 0x28) = 0xff;
    *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x28) = 1;
    *(undefined1 *)(unaff_DI + 0x29) = 1;
  }
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 6);
  *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(unaff_DI + 0x29);
  return;
}



/* requested 0x4D44; function player_external_4D44 at 0x19780 */

void player_external_4D44(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x4dce;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0x19;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  *(long *)(unaff_DI + 0xe) = (long)(int)*(char *)(iVar1 + 0x646c) * -0x100 + -0x14000;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  lVar2 = (long)(int)*(char *)(iVar1 + 0x646c) * 0x400 + -0x6000;
  *(long *)(unaff_DI + 10) = lVar2;
  if (lVar2 < 0) {
    *(undefined1 *)(unaff_DI + 0x28) = 0xff;
    *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x28) = 1;
    *(undefined1 *)(unaff_DI + 0x29) = 1;
  }
  return;
}



/* requested 0x4DCE; function player_external_4DCE at 0x19918 */

void player_external_4DCE(void)

{
  int *piVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined1 in_CF;
  
  func_0x0000ffff();
  if (!(bool)in_CF) {
    if (*(int *)(unaff_DI + 0x32) == 1) {
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + 0x18000;
      *(undefined2 *)(unaff_DI + 0x32) = 2;
    }
    else {
      if (*(int *)(unaff_DI + 0x32) == 2) {
        lVar2 = *(long *)(unaff_DI + 0xe);
        *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
        lVar2 = lVar2 + -6000;
        if (lVar2 < -0x34000) {
          lVar2 = -0x34000;
        }
        else if (0x34000 < lVar2) {
          lVar2 = 0x34000;
        }
        *(long *)(unaff_DI + 0xe) = lVar2;
      }
      else {
        piVar1 = (int *)(unaff_DI + 0x34);
        *piVar1 = *piVar1 + -1;
        if (*piVar1 == 0) {
          *(undefined2 *)(unaff_DI + 0x32) = 1;
        }
        lVar2 = *(long *)(unaff_DI + 0xe);
        *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
        lVar2 = lVar2 + 3000;
        if (lVar2 < -0x14000) {
          lVar2 = -0x14000;
        }
        else if (0x14000 < lVar2) {
          lVar2 = 0x14000;
        }
        *(long *)(unaff_DI + 0xe) = lVar2;
      }
      lVar2 = *(long *)(unaff_DI + 10);
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar2;
      lVar2 = lVar2 + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
      if (lVar2 < -0x20000) {
        lVar2 = -0x20000;
      }
      else if (0x20000 < lVar2) {
        lVar2 = 0x20000;
      }
      *(long *)(unaff_DI + 10) = lVar2;
    }
    func_0x0000ffff(0);
    return;
  }
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  return;
}



/* requested 0x4EC9; function player_external_4EC9 at 0x20169 */

void player_external_4EC9(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4DCE;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0x19;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  *(long *)(unaff_DI + 0xe) = (long)(int)*(char *)(iVar1 + 0x646c) * -0x100 + -0x14000;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  lVar2 = (long)(int)*(char *)(iVar1 + 0x646c) * 0x400 + -0x6000;
  *(long *)(unaff_DI + 10) = lVar2;
  if (lVar2 < 0) {
    *(undefined1 *)(unaff_DI + 0x28) = 0xff;
    *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x28) = 1;
    *(undefined1 *)(unaff_DI + 0x29) = 1;
  }
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 6);
  *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(unaff_DI + 0x29);
  return;
}



/* requested 0x4F82; function player_external_4F82 at 0x20354 */

void player_external_4F82(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4DCE;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0x19;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  *(long *)(unaff_DI + 0xe) = (long)(int)*(char *)(iVar1 + 0x646c) * -0x100 + -0x14000;
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  lVar2 = (long)(int)*(char *)(iVar1 + 0x646c) * 0x400 + -0x6000;
  *(long *)(unaff_DI + 10) = lVar2;
  if (lVar2 < 0) {
    *(undefined1 *)(unaff_DI + 0x28) = 0xff;
    *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x28) = 1;
    *(undefined1 *)(unaff_DI + 0x29) = 1;
  }
  return;
}



/* requested 0x9DC7; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  func_0x0000a075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  func_0x0000a0b2();
  return;
}



/* requested 0xA075; function player_external_A075 at 0x41077 */

void player_external_A075(void)

{
  int iVar1;
  char in_CL;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x58) == '\0') {
    iVar1 = func_0x0000ffff();
    if ((((-1 < in_CL) && (*(int *)(unaff_DI + 4) < iVar1)) &&
        (iVar1 < *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e))) &&
       ((*(int *)(unaff_DI + 8) <= in_BX && (in_BX < *(int *)(unaff_DI + 8) + 0xc)))) {
      *(undefined2 *)0x5006 = 0xffff;
    }
  }
  else {
    *(char *)(unaff_DI + 0x58) = *(char *)(unaff_DI + 0x58) + -1;
  }
  return;
}



/* requested 0xA0B2; function player_external_A0B2 at 0x41138 */

void player_external_A0B2(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(int *)0x5006 != 0) {
    *(long *)0x8816 =
         (ulong)(uint)(*(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2a)) * 0x10000 + 1;
    *(long *)0x8812 = 1 - (*(long *)(*(int *)0x881a + 6) - *(long *)(unaff_DI + 6));
    *(undefined1 *)(unaff_DI + 0x5a) = 0xff;
    *(undefined1 *)(unaff_DI + 0x59) = 0;
    return;
  }
  if (*(char *)(unaff_DI + 0x5a) == -1) {
    *(undefined1 *)(unaff_DI + 0x5a) = 0;
    *(undefined1 *)(unaff_DI + 0x58) = 0x14;
  }
  return;
}



/* requested 0xA06F; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  player_external_A075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  player_external_A0B2();
  return;
}



/* requested 0xA03D; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  player_external_A075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  player_external_A0B2();
  return;
}



/* requested 0xA051; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  player_external_A075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  player_external_A0B2();
  return;
}



/* requested 0x9F35; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  player_external_A075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  player_external_A0B2();
  return;
}



/* requested 0x9F4A; function player_external_9DC7 at 0x40391 */

void player_external_9DC7(void)

{
  undefined2 uVar1;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if ((((*(int *)(unaff_DI + 4) < 0) || (*(int *)(unaff_DI + 8) < 0)) ||
      (*(int *)0x657e * 8 - *(int *)(unaff_DI + 4) < 0)) ||
     (*(int *)0x6580 * 0x10 - *(int *)(unaff_DI + 8) < 0)) goto LAB_0000_a06f;
  *(undefined2 *)0x5006 = 0;
  player_external_A075();
  uVar1 = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 0x2c) = *(undefined2 *)(unaff_DI + 8);
  *(undefined2 *)(unaff_DI + 0x2a) = uVar1;
  if (*(char *)(unaff_DI + 0x59) == '\x01') goto LAB_0000_a06b;
  if (*(char *)(unaff_DI + 0x4a) == '\0') {
    if ((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x100U < 0x341) {
      if (*(int *)(unaff_DI + 0x54) == 0) {
        if (*(long *)(unaff_DI + 0xe) == 0) {
          if (*(int *)(unaff_DI + 0x52) == 0) {
            bVar3 = *(int *)(unaff_DI + 8) == -0x10;
            func_0x0000ffff();
            if (bVar3) {
              bVar3 = *(int *)(unaff_DI + 8) == 0x10;
              func_0x0000ffff(0);
              if ((!bVar3) || (*(char *)(unaff_DI + 0x4c) == '\x01')) {
                *(undefined4 *)(unaff_DI + 0xe) = 0x28000;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 1;
                *(undefined1 *)(unaff_DI + 0x4c) = 1;
                goto LAB_0000_a06b;
              }
            }
            *(undefined4 *)(unaff_DI + 0xe) = 0xfffd8000;
            *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -1;
            *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
          }
          else {
            *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
          }
        }
        else {
          uVar2 = *(uint *)(unaff_DI + 8);
          *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
          if (((*(uint *)(unaff_DI + 8) ^ uVar2) & 0xfff0) != 0) {
            bVar3 = *(char *)(unaff_DI + 0x4c) == '\x01';
            if (bVar3) {
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
              }
            }
            else {
              bVar3 = (*(uint *)(unaff_DI + 8) & 0xfff0) == 0xfff0;
              func_0x0000ffff();
              if (!bVar3) {
                *(undefined4 *)(unaff_DI + 0xe) = 0;
                *(undefined2 *)(unaff_DI + 0x54) = 0x46;
                *(uint *)(unaff_DI + 8) = *(uint *)(unaff_DI + 8) & 0xfff0;
                *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
              }
            }
          }
        }
      }
      else {
        *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
      }
      goto LAB_0000_a06b;
    }
LAB_0000_a06f:
    func_0x0000ffff();
    return;
  }
  if (0x2b0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x100U) goto LAB_0000_a06f;
  if (*(int *)(unaff_DI + 0x54) != 0) {
    *(int *)(unaff_DI + 0x54) = *(int *)(unaff_DI + 0x54) + -1;
    goto LAB_0000_a06b;
  }
  if (*(long *)(unaff_DI + 10) != 0) {
    uVar2 = *(uint *)(unaff_DI + 4);
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (((*(uint *)(unaff_DI + 4) ^ uVar2) & 0xfff0) != 0) {
      if (*(char *)(unaff_DI + 0x4e) == '\x01') {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) + *(int *)(unaff_DI + 0x3e) == 0x10;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
        }
      }
      else {
        bVar3 = (*(uint *)(unaff_DI + 4) & 0xfff0) == 0xfff0;
        func_0x0000ffff();
        if (!bVar3) {
          *(undefined4 *)(unaff_DI + 10) = 0;
          *(undefined2 *)(unaff_DI + 0x54) = 0x46;
          *(uint *)(unaff_DI + 4) = *(uint *)(unaff_DI + 4) & 0xfff0;
          *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
        }
      }
    }
    goto LAB_0000_a06b;
  }
  if (*(int *)(unaff_DI + 0x52) != 0) {
    *(int *)(unaff_DI + 0x52) = *(int *)(unaff_DI + 0x52) + -1;
    goto LAB_0000_a06b;
  }
  if (*(char *)(unaff_DI + 0x50) == -1) {
    *(undefined1 *)(unaff_DI + 0x50) = 0;
    bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
    func_0x0000ffff();
    if (!bVar3) {
LAB_0000_9f35:
      *(undefined4 *)(unaff_DI + 10) = 0xfffd8000;
      *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + -1;
      *(undefined1 *)(unaff_DI + 0x4e) = 0xff;
      goto LAB_0000_a06b;
    }
  }
  else {
    bVar3 = *(int *)(unaff_DI + 4) == 0x10;
    func_0x0000ffff();
    if (bVar3) {
      bVar3 = *(int *)(unaff_DI + 4) + *(int *)(unaff_DI + 0x3e) == 0;
      func_0x0000ffff(0);
      if ((!bVar3) || (*(char *)(unaff_DI + 0x4e) == -1)) goto LAB_0000_9f35;
    }
  }
  *(undefined4 *)(unaff_DI + 10) = 0x28000;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
LAB_0000_a06b:
  player_external_A0B2();
  return;
}



/* requested 0x9C70; function player_external_9C70 at 0x40048 */

void player_external_9C70(void)

{
  uint uVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined4 *)(unaff_DI + 10) = 0;
  *(undefined4 *)(unaff_DI + 0xe) = 0;
  *(undefined1 *)(unaff_DI + 0x50) = 0xff;
  *(undefined1 *)(unaff_DI + 0x4c) = 0xff;
  *(undefined1 *)(unaff_DI + 0x5a) = 0;
  *(undefined1 *)(unaff_DI + 0x4e) = 1;
  *(undefined2 *)(unaff_DI + 0x54) = 0;
  *(ulong *)(unaff_DI + 2) = *(ulong *)(unaff_DI + 2) & 0xfff00000;
  *(ulong *)(unaff_DI + 6) = *(ulong *)(unaff_DI + 6) & 0xfff00000;
  *(undefined4 *)(unaff_DI + 0x2e) = 0x4000;
  *(undefined4 *)(unaff_DI + 0x32) = 0x4000;
  *(undefined4 *)(unaff_DI + 0x36) = 0x2000;
  *(undefined4 *)(unaff_DI + 0x3a) = 0x2000;
  *(undefined1 *)(unaff_DI + 0x59) = 1;
  uVar1 = func_0x0000ffff();
  if ((uVar1 & 0x200) != 0) {
    *(undefined1 *)(unaff_DI + 0x59) = 0;
  }
  *(undefined2 *)(unaff_DI + 0x12) = 300;
  return;
}



/* requested 0x9CF5; function player_external_9CF5 at 0x40181 */

void player_external_9CF5(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_9C70();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_9DC7;
  *(int *)(unaff_DI + 0x12) = *(int *)(unaff_DI + 0x12) + 1;
  *(undefined2 *)(unaff_DI + 0x3e) = 0x20;
  *(undefined1 *)(unaff_DI + 0x4a) = 0xff;
  *(undefined1 *)(unaff_DI + 0x4b) = 0;
  *(undefined2 *)(unaff_DI + 0x52) = 0x14;
  return;
}



/* requested 0x9D19; function player_external_9D19 at 0x40217 */

void player_external_9D19(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_9C70();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_9DC7;
  *(undefined2 *)(unaff_DI + 0x3e) = 0x30;
  *(undefined1 *)(unaff_DI + 0x4a) = 0xff;
  *(undefined1 *)(unaff_DI + 0x4b) = 0;
  *(undefined2 *)(unaff_DI + 0x52) = 0x14;
  return;
}



/* requested 0x9D5E; function player_external_9D5E at 0x40286 */

void player_external_9D5E(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_9C70();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_9DC7;
  *(int *)(unaff_DI + 0x12) = *(int *)(unaff_DI + 0x12) + 1;
  *(undefined2 *)(unaff_DI + 0x3e) = 0x20;
  *(undefined1 *)(unaff_DI + 0x4a) = 0;
  *(undefined1 *)(unaff_DI + 0x4b) = 0xff;
  *(undefined2 *)(unaff_DI + 0x52) = 0x14;
  return;
}



/* requested 0x9D82; function player_external_9D82 at 0x40322 */

void player_external_9D82(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_9C70();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_9DC7;
  *(undefined2 *)(unaff_DI + 0x3e) = 0x30;
  *(undefined1 *)(unaff_DI + 0x4a) = 0;
  *(undefined1 *)(unaff_DI + 0x4b) = 0xff;
  *(undefined2 *)(unaff_DI + 0x52) = 0x14;
  return;
}



/* requested 0x5DA1; function player_external_5DA1 at 0x23969 */

undefined4 player_external_5DA1(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)(*(int *)0x657a + (in_BX >> 4) * 2 + (int)lVar1));
}



/* requested 0x9BEE; function init_are_type_34 at 0x39918 */

/* Observed initializer callback for ARE type 0x34; runtime changes its slot to 0x9C0C after the
   first update. */

void init_are_type_34(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_are_type_34;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 0x10;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
  func_0x0000ffff();
  return;
}



/* requested 0x9C0C; function update_are_type_34 at 0x39948 */

/* Observed steady callback for ARE type 0x34; input-trace samples show stable state fields and a
   persistent object callback. */

void update_are_type_34(void)

{
  undefined2 unaff_DS;
  bool bVar1;
  
  bVar1 = *(byte *)0x85da < 0x32;
  if ((char)*(byte *)0x85da < '2') {
    func_0x0000ffff();
    if (bVar1) {
      func_0x0000ffff(0);
      return;
    }
    func_0x0000ffff(0);
    test_type34_proximity();
  }
  return;
}



/* requested 0x4727; function update_falling_leaves_types_29_2b at 0x18215 */

/* Dispatch-table callback shared by ARE types 0x29, 0x2A, and confirmed falling-leaves type 0x2B.
    */

void update_falling_leaves_types_29_2b(void)

{
  char cVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x47e7;
  iVar2 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  cVar1 = *(char *)(iVar2 + 0x646c);
  *(undefined4 *)(unaff_DI + 0xe) = 0x13000;
  *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + (long)(int)cVar1 * -0x80;
  *(undefined4 *)(unaff_DI + 0x2a) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(unaff_DI + 0x2e) = *(undefined4 *)(unaff_DI + 6);
  *(undefined2 *)(unaff_DI + 0x32) = 0xc;
  return;
}



/* requested 0x474D; function update_falling_leaves_types_29_2b at 0x18215 */

/* Dispatch-table callback shared by ARE types 0x29, 0x2A, and confirmed falling-leaves type 0x2B.
    */

void update_falling_leaves_types_29_2b(void)

{
  char cVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x47e7;
  iVar2 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  cVar1 = *(char *)(iVar2 + 0x646c);
  *(undefined4 *)(unaff_DI + 0xe) = 0x13000;
  *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + (long)(int)cVar1 * -0x80;
  *(undefined4 *)(unaff_DI + 0x2a) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(unaff_DI + 0x2e) = *(undefined4 *)(unaff_DI + 6);
  *(undefined2 *)(unaff_DI + 0x32) = 0xc;
  return;
}



/* requested 0x47E7; function player_external_47E7 at 0x18407 */

void player_external_47E7(void)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  long lVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  if ((bool)in_CF) {
    piVar1 = (int *)(unaff_DI + 0x32);
    *piVar1 = *piVar1 + -1;
    if (*piVar1 == 0) {
      *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 0x2a);
      *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 0x2e);
      *(undefined2 *)(unaff_DI + 0x32) = 0xc;
      *(uint *)(unaff_DI + 0x12) = *(uint *)(unaff_DI + 0x12) & 0x7fff;
      iVar3 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      cVar2 = *(char *)(iVar3 + 0x646c);
      *(undefined4 *)(unaff_DI + 0xe) = 0x13000;
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + (long)(int)cVar2 * -0x80;
    }
    else {
      *(uint *)(unaff_DI + 0x12) = *(uint *)(unaff_DI + 0x12) ^ 0x8000;
    }
  }
  else {
    lVar4 = *(long *)(unaff_DI + 0xe);
    if (0x4000 < lVar4) {
      lVar4 = lVar4 + -300;
    }
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar4;
    *(long *)(unaff_DI + 0xe) = lVar4;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x8BC2; function player_external_8BC2 at 0x35778 */

void player_external_8BC2(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25f;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 1;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + -2;
  *(undefined2 *)(unaff_DI + 0x2c) = 1;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  return;
}



/* requested 0x8BE5; function player_external_8BE5 at 0x35813 */

void player_external_8BE5(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x260;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 5;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 10;
  *(undefined2 *)(unaff_DI + 0x2c) = 2;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  return;
}



/* requested 0x8C08; function player_external_8C08 at 0x35848 */

void player_external_8C08(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x261;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 5;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 10;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 3;
  return;
}



/* requested 0x8C2B; function player_external_8C2B at 0x35883 */

void player_external_8C2B(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x262;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 3;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 7;
  *(undefined2 *)(unaff_DI + 0x2c) = 4;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  return;
}



/* requested 0x8C4E; function init_are_type_2c at 0x35918 */

/* Observed initializer callback for ARE type 0x2C; runtime changes its slot to 0x8D20 after the
   first update. */

void init_are_type_2c(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x2c6;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + 3;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 7;
  *(undefined2 *)(unaff_DI + 0x2c) = 5;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  return;
}



/* requested 0x8C71; function player_external_8C71 at 0x35953 */

void player_external_8C71(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 600;
  *(undefined2 *)(unaff_DI + 0x2a) = 1;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8C8A; function player_external_8C8A at 0x35978 */

void player_external_8C8A(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x259;
  *(undefined2 *)(unaff_DI + 0x2a) = 2;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8CA3; function player_external_8CA3 at 0x36003 */

void player_external_8CA3(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25a;
  *(undefined2 *)(unaff_DI + 0x2a) = 4;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8CBC; function player_external_8CBC at 0x36028 */

void player_external_8CBC(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25b;
  *(undefined2 *)(unaff_DI + 0x2a) = 8;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8CD5; function player_external_8CD5 at 0x36053 */

void player_external_8CD5(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25c;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x10;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8CEE; function player_external_8CEE at 0x36078 */

void player_external_8CEE(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25d;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x20;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8D07; function player_external_8D07 at 0x36103 */

void player_external_8D07(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = (code *)update_collectible_effect;
  *(undefined2 *)(unaff_DI + 0x12) = 0x25e;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x40;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* requested 0x8D20; function update_collectible_effect at 0x36128 */

/* Updates collectible-effect objects: runs the visibility gate, then delegates to the player-bounds
   state routine. */

void update_collectible_effect(void)

{
  undefined1 in_CF;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  update_collectible_state();
  return;
}



/* requested 0x8D31; function update_collectible_state at 0x36145 */

/* Collectible state routine: tests strict player-bounds overlap, applies object state, and writes
   pending effect IDs. */

void update_collectible_state(void)

{
  uint uVar1;
  int in_CX;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined4 uVar2;
  
  uVar2 = func_0x0000ffff();
  if ((*(int *)(unaff_DI + 4) < in_CX) && ((int)uVar2 < *(int *)(unaff_DI + 4) + 0x10)) {
    uVar1 = *(uint *)(unaff_DI + 8) & 0xfff0;
    if (((int)uVar1 < (int)((ulong)uVar2 >> 0x10)) && (in_BX < (int)(uVar1 + 0x10))) {
      if (*(char *)(unaff_DI + 0x2c) == '\0') {
        *(long *)0x881c = *(long *)0x881c + 100;
        *(uint *)0x60d8 = *(uint *)0x60d8 | *(uint *)(unaff_DI + 0x2a);
        *(undefined2 *)0x612e = 0xb;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x01') {
        *(int *)0x880c = *(int *)0x880c + 10;
        *(long *)0x881c = *(long *)0x881c + 0x32;
        *(undefined2 *)0x612e = 9;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x02') {
        *(long *)0x881c = *(long *)0x881c + 0xfa;
        if (*(int *)0x8824 != 5) {
          *(int *)0x8824 = *(int *)0x8824 + 1;
        }
        *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        *(undefined2 *)0x612e = 9;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x03') {
        *(undefined2 *)0x612e = 10;
        func_0x0000ffff(0);
        *(long *)0x881c = *(long *)0x881c + 100;
        if (*(int *)0x8822 != *(int *)0x8824) {
          *(int *)0x8822 = *(int *)0x8822 + 1;
        }
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x04') {
        if (*(int *)0x85d4 < 0x10) {
          func_0x0000ffff(0);
        }
        *(long *)0x881c = *(long *)0x881c + 0x96;
        *(undefined2 *)0x612e = 0xc;
        func_0x0000ffff(0);
      }
      else {
        *(long *)0x881c = *(long *)0x881c + 500;
        *(undefined2 *)0x612e = 0xc;
        func_0x0000ffff(0);
        if (*(int *)0x880a < 9) {
          *(int *)0x880a = *(int *)0x880a + 1;
        }
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    return;
  }
  return;
}



/* requested 0x8E42; function update_collectible_state at 0x36145 */

/* Collectible state routine: tests strict player-bounds overlap, applies object state, and writes
   pending effect IDs. */

void update_collectible_state(void)

{
  uint uVar1;
  int in_CX;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined4 uVar2;
  
  uVar2 = func_0x0000ffff();
  if ((*(int *)(unaff_DI + 4) < in_CX) && ((int)uVar2 < *(int *)(unaff_DI + 4) + 0x10)) {
    uVar1 = *(uint *)(unaff_DI + 8) & 0xfff0;
    if (((int)uVar1 < (int)((ulong)uVar2 >> 0x10)) && (in_BX < (int)(uVar1 + 0x10))) {
      if (*(char *)(unaff_DI + 0x2c) == '\0') {
        *(long *)0x881c = *(long *)0x881c + 100;
        *(uint *)0x60d8 = *(uint *)0x60d8 | *(uint *)(unaff_DI + 0x2a);
        *(undefined2 *)0x612e = 0xb;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x01') {
        *(int *)0x880c = *(int *)0x880c + 10;
        *(long *)0x881c = *(long *)0x881c + 0x32;
        *(undefined2 *)0x612e = 9;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x02') {
        *(long *)0x881c = *(long *)0x881c + 0xfa;
        if (*(int *)0x8824 != 5) {
          *(int *)0x8824 = *(int *)0x8824 + 1;
        }
        *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        *(undefined2 *)0x612e = 9;
        func_0x0000ffff(0);
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x03') {
        *(undefined2 *)0x612e = 10;
        func_0x0000ffff(0);
        *(long *)0x881c = *(long *)0x881c + 100;
        if (*(int *)0x8822 != *(int *)0x8824) {
          *(int *)0x8822 = *(int *)0x8822 + 1;
        }
      }
      else if (*(char *)(unaff_DI + 0x2c) == '\x04') {
        if (*(int *)0x85d4 < 0x10) {
          func_0x0000ffff(0);
        }
        *(long *)0x881c = *(long *)0x881c + 0x96;
        *(undefined2 *)0x612e = 0xc;
        func_0x0000ffff(0);
      }
      else {
        *(long *)0x881c = *(long *)0x881c + 500;
        *(undefined2 *)0x612e = 0xc;
        func_0x0000ffff(0);
        if (*(int *)0x880a < 9) {
          *(int *)0x880a = *(int *)0x880a + 1;
        }
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    return;
  }
  return;
}



/* requested 0x8E4B; function update_tile_effect_state_machine at 0x36427 */

/* Dispatches the shared tile-effect object state machine through object +0x32; state branches call
   the MAP lookup and create transient effects. */

uint update_tile_effect_state_machine(void)

{
  uint in_AX;
  int iVar1;
  uint uVar2;
  int in_CX;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  undefined4 uVar4;
  
  bVar3 = false;
  if (*(int *)(unaff_DI + 0x32) == 0) {
    func_0x0000ffff();
    if (bVar3) {
      uVar2 = func_0x0000ffff(0);
      return uVar2;
    }
    uVar4 = func_0x0000ffff(0);
    uVar2 = (uint)uVar4;
    if ((((*(int *)(unaff_DI + 4) < in_CX) && ((int)uVar2 < *(int *)(unaff_DI + 4) + 0x50)) &&
        (uVar2 = *(uint *)(unaff_DI + 8) & 0xfff0, (int)uVar2 < (int)((ulong)uVar4 >> 0x10))) &&
       (uVar2 = uVar2 + 0x40, in_BX < (int)uVar2)) {
      *(undefined2 *)(unaff_DI + 0x32) = 1;
    }
  }
  else {
    *(int *)(unaff_DI + 0x32) = *(int *)(unaff_DI + 0x32) + 1;
    if (*(int *)(unaff_DI + 0x32) == 4) {
      iVar1 = func_0x0000ffff();
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(undefined2 *)(unaff_DI + 4),*(undefined2 *)(unaff_DI + 8));
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x20,*(undefined2 *)(unaff_DI + 8));
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x30,*(undefined2 *)(unaff_DI + 8));
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      uVar2 = *(int *)(unaff_DI + 4) + 0x40;
      iVar1 = func_0x0000ffff(0,uVar2,*(undefined2 *)(unaff_DI + 8));
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        uVar2 = func_0x0000ffff(0);
      }
    }
    else if (*(int *)(unaff_DI + 0x32) == 6) {
      iVar1 = func_0x0000ffff();
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x10,*(int *)(unaff_DI + 8) + 0x10);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x20,*(int *)(unaff_DI + 8) + 0x10);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x30,*(int *)(unaff_DI + 8) + 0x10);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      uVar2 = *(int *)(unaff_DI + 4) + 0x40;
      iVar1 = func_0x0000ffff(0,uVar2,*(int *)(unaff_DI + 8) + 0x10);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        uVar2 = func_0x0000ffff(0);
      }
    }
    else {
      if (*(int *)(unaff_DI + 0x32) != 8) {
        if (*(int *)(unaff_DI + 0x32) == 10) {
          iVar1 = func_0x0000ffff();
          if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
            func_0x0000ffff(0);
          }
          iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x10,*(int *)(unaff_DI + 8) + 0x30);
          if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
            func_0x0000ffff(0);
          }
          iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x20,*(int *)(unaff_DI + 8) + 0x30);
          if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
            func_0x0000ffff(0);
          }
          iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x30,*(int *)(unaff_DI + 8) + 0x30);
          if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
            func_0x0000ffff(0);
          }
          iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x40,*(int *)(unaff_DI + 8) + 0x30);
          if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
            func_0x0000ffff(0);
          }
          *(undefined2 *)(unaff_DI + 0x18) = 0;
          iVar1 = *(int *)(unaff_DI + 8);
          uVar2 = *(int *)(unaff_DI + 4) + 0x19;
          *(uint *)0x8828 = uVar2;
          *(int *)0x882a = iVar1 + 0x46;
          return uVar2;
        }
        return in_AX;
      }
      iVar1 = func_0x0000ffff();
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x10,*(int *)(unaff_DI + 8) + 0x20);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x20,*(int *)(unaff_DI + 8) + 0x20);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      iVar1 = func_0x0000ffff(0,*(int *)(unaff_DI + 4) + 0x30,*(int *)(unaff_DI + 8) + 0x20);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        func_0x0000ffff(0);
      }
      uVar2 = *(int *)(unaff_DI + 4) + 0x40;
      iVar1 = func_0x0000ffff(0,uVar2,*(int *)(unaff_DI + 8) + 0x20);
      if (*(int *)(iVar1 * 2 + 0x6986) != 0) {
        uVar2 = func_0x0000ffff(0);
      }
    }
  }
  return uVar2;
}



/* requested 0x9256; function update_are_type_28 at 0x37462 */

/* Dispatch-table callback for normal ARE type 0x28, whose object class is zero. */

void update_are_type_28(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = 0x9269;
  *(undefined2 *)(unaff_DI + 0x12) = 0xffff;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  return;
}



/* requested 0x9269; function player_external_9269 at 0x37481 */

void player_external_9269(void)

{
  uint uVar1;
  int in_CX;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  undefined4 uVar2;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  uVar2 = func_0x0000ffff(0);
  if ((*(int *)(unaff_DI + 4) < in_CX) && ((int)uVar2 < *(int *)(unaff_DI + 4) + 0x10)) {
    uVar1 = *(uint *)(unaff_DI + 8) & 0xfff0;
    if (((int)uVar1 < (int)((ulong)uVar2 >> 0x10)) &&
       ((in_BX < (int)(uVar1 + 0x10) && (*(char *)(*(int *)0x881a + 0x37) == '\0')))) {
      *(undefined2 *)0x89e6 = 0xffff;
    }
    return;
  }
  return;
}



/* requested 0x92A9; function player_external_9269 at 0x37481 */

void player_external_9269(void)

{
  uint uVar1;
  int in_CX;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  undefined4 uVar2;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  uVar2 = func_0x0000ffff(0);
  if ((*(int *)(unaff_DI + 4) < in_CX) && ((int)uVar2 < *(int *)(unaff_DI + 4) + 0x10)) {
    uVar1 = *(uint *)(unaff_DI + 8) & 0xfff0;
    if (((int)uVar1 < (int)((ulong)uVar2 >> 0x10)) &&
       ((in_BX < (int)(uVar1 + 0x10) && (*(char *)(*(int *)0x881a + 0x37) == '\0')))) {
      *(undefined2 *)0x89e6 = 0xffff;
    }
    return;
  }
  return;
}



/* requested 0x6D5F; function player_external_6D5F at 0x27999 */

void player_external_6D5F(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x6dc4;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x6DA3; function player_external_6DA3 at 0x28067 */

void player_external_6DA3(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_6D5F();
  return;
}



/* requested 0x6DB1; function player_external_6DB1 at 0x28081 */

void player_external_6DB1(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_6D5F();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x6DC4; function player_external_6DC4 at 0x28100 */

void player_external_6DC4(void)

{
  int *piVar1;
  int iVar2;
  long lVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar4;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  func_0x0000ffff(0);
  if ((bool)in_CF) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x26;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_6e23:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x26;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_6e23;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(uint *)(unaff_DI + 0x2a) = (uint)(int)*(char *)(iVar2 + 0x646c) >> 1;
        *(undefined1 *)(unaff_DI + 0x32) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
  }
  else if (*(char *)(unaff_DI + 0x2f) < '\x01') {
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x33) = *(int *)(unaff_DI + 0x33) + 1;
      if (*(int *)(unaff_DI + 0x33) < 0x4c) goto LAB_0000_707b;
      *(undefined2 *)(unaff_DI + 0x33) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      func_0x0000ffff(0);
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      func_0x0000ffff(0);
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_707b;
      }
      else if (0 < lVar3) goto LAB_0000_707b;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_707b;
    }
    lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
    if (lVar3 < -0x15000) {
      lVar3 = -0x15000;
    }
    else if (0x15000 < lVar3) {
      lVar3 = 0x15000;
    }
    *(long *)(unaff_DI + 10) = lVar3;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      if (-0x15000 < lVar3) goto LAB_0000_707b;
    }
    else if (lVar3 < 0x15000) goto LAB_0000_707b;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    func_0x0000ffff(0);
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x78;
  }
LAB_0000_707b:
  if (*(int *)0x8806 != 0) {
    iVar2 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar2) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar2 = 0;
    }
    iVar2 = iVar2 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0x19 < *(int *)(iVar2 + -0x7822)) &&
         (*(int *)(iVar2 + -0x7822) < *(int *)(unaff_DI + 4) + 0x19)) &&
        (*(int *)(iVar2 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0xf < *(int *)(iVar2 + -0x7820))) {
      *(undefined2 *)(iVar2 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4AB3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x70D9; function player_external_70D9 at 0x28889 */

void player_external_70D9(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x715e;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined4 *)(unaff_DI + 10) = 0xffffa000;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffd0000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined1 *)(unaff_DI + 0x3d) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(undefined2 *)(unaff_DI + 0x3e) = 0;
  *(undefined2 *)(unaff_DI + 0x35) = 0;
  *(undefined2 *)(unaff_DI + 0x37) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x713D; function player_external_713D at 0x28989 */

void player_external_713D(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_70D9();
  return;
}



/* requested 0x714B; function player_external_714B at 0x29003 */

void player_external_714B(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_70D9();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x715E; function player_external_715E at 0x29022 */

void player_external_715E(void)

{
  int *piVar1;
  ulong *puVar2;
  uint uVar3;
  undefined2 uVar4;
  undefined2 extraout_var;
  ulong uVar5;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint extraout_DX_03;
  uint extraout_DX_04;
  int iVar6;
  long lVar7;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar8;
  undefined1 uVar9;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  func_0x0000ffff(0);
  if ((bool)in_CF) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    func_0x0000ffff(0);
    if ((extraout_DX & 0x70) != 0) goto LAB_0000_71e5;
    func_0x0000ffff(0);
    uVar3 = extraout_DX_00;
joined_r0x000071e1:
    if ((uVar3 & 0x70) != 0) goto LAB_0000_71e5;
  }
  else {
    func_0x0000ffff(0);
    if ((extraout_DX_01 & 0x70) == 0) {
      func_0x0000ffff(0);
      uVar3 = extraout_DX_02;
      goto joined_r0x000071e1;
    }
LAB_0000_71e5:
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if ('\0' < *(char *)(unaff_DI + 0x3d)) {
    if (*(char *)(unaff_DI + 0x3d) < '\x02') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x37);
      uVar3 = *(int *)(unaff_DI + 0x35) + 0x20U & 0x3ff;
      *(uint *)(unaff_DI + 0x35) = uVar3;
      iVar6 = (int)(*(char *)(uVar3 + 0x7974) >> 6);
      *(int *)(unaff_DI + 0x37) = iVar6;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar6;
      *(int *)(unaff_DI + 0x33) = *(int *)(unaff_DI + 0x33) + 1;
      if (0x32 < *(int *)(unaff_DI + 0x33)) {
        *(undefined2 *)(unaff_DI + 0x33) = 0;
        *(undefined1 *)(unaff_DI + 0x3d) = 2;
        *(undefined4 *)(unaff_DI + 0x39) = *(undefined4 *)(unaff_DI + 10);
        *(long *)(unaff_DI + 10) =
             *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x10000;
      }
      goto LAB_0000_76bf;
    }
    if (*(char *)(unaff_DI + 0x3d) < '\x03') {
      lVar7 = *(long *)(unaff_DI + 0xe);
      if (-1 < lVar7) {
        *(undefined1 *)(unaff_DI + 0x3d) = 3;
        uVar4 = func_0x0000ffff(0);
        lVar7 = CONCAT22(extraout_var,uVar4);
      }
      uVar5 = lVar7 + 4000;
      if ((long)uVar5 < -0x30000) {
        uVar5 = 0xfffd0000;
      }
      else if (0x30000 < (long)uVar5) {
        uVar5 = 0x30000;
      }
    }
    else {
      if ('\x03' < *(char *)(unaff_DI + 0x3d)) {
        *(undefined4 *)(unaff_DI + 10) = *(undefined4 *)(unaff_DI + 0x39);
        *(undefined4 *)(unaff_DI + 0xe) = 0xfffd0000;
        *(undefined1 *)(unaff_DI + 0x3d) = 0;
        *(undefined1 *)(unaff_DI + 0x32) = 0;
        func_0x0000ffff(0);
        goto LAB_0000_749d;
      }
      uVar5 = *(long *)(unaff_DI + 0xe) + 6000;
      if ((long)uVar5 < -0x30000) {
        uVar5 = 0xfffd0000;
      }
      else if (0x30000 < (long)uVar5) {
        uVar5 = 0x30000;
      }
    }
    *(ulong *)(unaff_DI + 0xe) = uVar5;
    puVar2 = (ulong *)(unaff_DI + 6);
    bVar8 = CARRY4(*puVar2,uVar5);
    *puVar2 = *puVar2 + uVar5;
    func_0x0000ffff(0);
    if (bVar8) {
      *(undefined1 *)(unaff_DI + 0x3d) = 4;
    }
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      uVar3 = extraout_DX_03;
    }
    else {
      func_0x0000ffff(0);
      uVar3 = extraout_DX_04;
    }
    if ((uVar3 & 0x70) != 0) {
      *(undefined1 *)(unaff_DI + 0x3d) = 4;
    }
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    goto LAB_0000_76bf;
  }
  if ('\0' < *(char *)(unaff_DI + 0x32)) {
    if ('\0' < *(char *)(unaff_DI + 0x2f)) {
      *(undefined1 *)(unaff_DI + 0x32) = 0;
      *(undefined2 *)(unaff_DI + 0x2a) = 0x82;
      goto LAB_0000_76bf;
    }
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x3e) = *(int *)(unaff_DI + 0x3e) + 1;
      if (0x2d < *(int *)(unaff_DI + 0x3e)) {
        *(undefined2 *)(unaff_DI + 0x3e) = 0;
        *(undefined1 *)(unaff_DI + 0x32) = 3;
      }
      goto LAB_0000_76bf;
    }
    if (*(char *)(unaff_DI + 0x32) == '\x03') {
      uVar5 = *(long *)(unaff_DI + 0xe) + 4000;
      if ((long)uVar5 < -0x15000) {
        uVar5 = 0xfffeb000;
      }
      else if (0x15000 < (long)uVar5) {
        uVar5 = 0x15000;
      }
      *(ulong *)(unaff_DI + 0xe) = uVar5;
      puVar2 = (ulong *)(unaff_DI + 6);
      bVar8 = CARRY4(*puVar2,uVar5);
      *puVar2 = *puVar2 + uVar5;
      func_0x0000ffff(0);
      if (bVar8) {
        *(undefined4 *)(unaff_DI + 0xe) = 0xfffd0000;
        *(undefined1 *)(unaff_DI + 0x32) = 4;
        func_0x0000ffff(0);
      }
      goto LAB_0000_76bf;
    }
    if (*(char *)(unaff_DI + 0x32) == '\x04') {
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        uVar9 = *(int *)(unaff_DI + 4) == 4;
        func_0x0000ffff(0);
      }
      else {
        uVar9 = *(int *)(unaff_DI + 4) == -4;
        func_0x0000ffff(0);
      }
      if (!(bool)uVar9) {
        lVar7 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x200;
        if (lVar7 < -0x6000) {
          lVar7 = -0x6000;
        }
        else if (0x6000 < lVar7) {
          lVar7 = 0x6000;
        }
        *(long *)(unaff_DI + 10) = lVar7;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar7;
        if (*(char *)(unaff_DI + 0x29) < '\x01') {
          if (-0x6000 < lVar7) goto LAB_0000_76bf;
        }
        else if (lVar7 < 0x6000) goto LAB_0000_76bf;
        *(undefined1 *)(unaff_DI + 0x32) = 0;
        goto LAB_0000_76bf;
      }
    }
    else {
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        uVar9 = *(int *)(unaff_DI + 4) == 4;
        func_0x0000ffff(0);
      }
      else {
        uVar9 = *(int *)(unaff_DI + 4) == -4;
        func_0x0000ffff(0);
      }
      if (!(bool)uVar9) {
        lVar7 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x100;
        if (lVar7 < -0x6000) {
          lVar7 = -0x6000;
        }
        else if (0x6000 < lVar7) {
          lVar7 = 0x6000;
        }
        *(long *)(unaff_DI + 10) = lVar7;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar7;
        if (*(char *)(unaff_DI + 0x29) < '\x01') {
          if (lVar7 < 0) goto LAB_0000_76bf;
        }
        else if (0 < lVar7) goto LAB_0000_76bf;
        *(undefined4 *)(unaff_DI + 10) = 0;
        *(undefined1 *)(unaff_DI + 0x32) = 2;
        func_0x0000ffff(0);
        goto LAB_0000_76bf;
      }
    }
    goto LAB_0000_7315;
  }
  if ('\0' < *(char *)(unaff_DI + 0x2f)) {
    if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x19) {
        func_0x0000ffff(0);
      }
      lVar7 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x200;
      if (lVar7 < -0x6000) {
        lVar7 = -0x6000;
      }
      else if (0x6000 < lVar7) {
        lVar7 = 0x6000;
      }
      *(long *)(unaff_DI + 10) = lVar7;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar7;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar6 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar6,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 5;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x19;
      }
    }
    else {
      lVar7 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar7 < -0x6000) {
        lVar7 = -0x6000;
      }
      else if (0x6000 < lVar7) {
        lVar7 = 0x6000;
      }
      *(long *)(unaff_DI + 10) = lVar7;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar7;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar6 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar6,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x19;
      }
    }
    goto LAB_0000_76bf;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar8 = *(int *)(unaff_DI + 4) == 0xe;
    func_0x0000ffff(0);
    if (bVar8) {
LAB_0000_7315:
      *(undefined1 *)(unaff_DI + 0x3d) = 1;
      *(undefined4 *)(unaff_DI + 0x39) = *(undefined4 *)(unaff_DI + 10);
      func_0x0000ffff(0);
      goto LAB_0000_76bf;
    }
  }
  else {
    bVar8 = *(int *)(unaff_DI + 4) == -0xe;
    func_0x0000ffff(0);
    if (bVar8) goto LAB_0000_7315;
  }
LAB_0000_749d:
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
  *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
  if (0x96 < *(int *)(unaff_DI + 0x2a)) {
    iVar6 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(uint *)(unaff_DI + 0x2a) = (uint)(int)*(char *)(iVar6 + 0x646c) >> 1;
    *(undefined1 *)(unaff_DI + 0x32) = 1;
  }
LAB_0000_76bf:
  if (*(int *)0x8806 != 0) {
    iVar6 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 < iVar6) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar6 = 0;
    }
    iVar6 = iVar6 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar6 + -0x7822)) &&
         (*(int *)(iVar6 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
        (*(int *)(iVar6 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x23 < *(int *)(iVar6 + -0x7820))) {
      *(undefined2 *)(iVar6 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4AB3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x771D; function player_external_771D at 0x30493 */

void player_external_771D(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x778c;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffffb000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x776B; function player_external_776B at 0x30571 */

void player_external_776B(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_771D();
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  return;
}



/* requested 0x7779; function player_external_7779 at 0x30585 */

void player_external_7779(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_771D();
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x778C; function player_external_778C at 0x30604 */

void player_external_778C(void)

{
  int *piVar1;
  int iVar2;
  long lVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar4;
  undefined1 uVar5;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  iVar2 = -0x19;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar2 = 0x19;
  }
  bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
  func_0x0000ffff(0);
  uVar5 = false;
  if (bVar4) {
    iVar2 = -0x19;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar2 = 0x19;
    }
    bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
    func_0x0000ffff(0);
    uVar5 = false;
    if (bVar4) {
      iVar2 = -0x19;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar2 = 0x19;
      }
      uVar5 = *(int *)(unaff_DI + 4) + iVar2 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar5) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x19;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_7847:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x19;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_7847;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(uint *)(unaff_DI + 0x2a) = (uint)(int)*(char *)(iVar2 + 0x646c) >> 1;
        *(undefined1 *)(unaff_DI + 0x32) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x400;
      if (lVar3 < -0x6000) {
        lVar3 = -0x6000;
      }
      else if (0x6000 < lVar3) {
        lVar3 = 0x6000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 5;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x6000) {
        lVar3 = -0x6000;
      }
      else if (0x6000 < lVar3) {
        lVar3 = 0x6000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
  }
  else if (*(char *)(unaff_DI + 0x2f) < '\x01') {
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x33) = *(int *)(unaff_DI + 0x33) + 1;
      if (*(int *)(unaff_DI + 0x33) < 0x2e) goto LAB_0000_7a85;
      *(undefined2 *)(unaff_DI + 0x33) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      func_0x0000ffff(0);
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x100;
      if (lVar3 < -0x5000) {
        lVar3 = -0x5000;
      }
      else if (0x5000 < lVar3) {
        lVar3 = 0x5000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_7a85;
      }
      else if (0 < lVar3) goto LAB_0000_7a85;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_7a85;
    }
    lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x200;
    if (lVar3 < -0x5000) {
      lVar3 = -0x5000;
    }
    else if (0x5000 < lVar3) {
      lVar3 = 0x5000;
    }
    *(long *)(unaff_DI + 10) = lVar3;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      if (-0x5000 < lVar3) goto LAB_0000_7a85;
    }
    else if (lVar3 < 0x5000) goto LAB_0000_7a85;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x78;
  }
LAB_0000_7a85:
  if (*(int *)0x8806 != 0) {
    iVar2 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar2) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar2 = 0;
    }
    iVar2 = iVar2 * 4;
    if ((((*(int *)(unaff_DI + 4) + -10 < *(int *)(iVar2 + -0x7822)) &&
         (*(int *)(iVar2 + -0x7822) < *(int *)(unaff_DI + 4) + 10)) &&
        (*(int *)(iVar2 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x23 < *(int *)(iVar2 + -0x7820))) {
      *(undefined2 *)(iVar2 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4BA0;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x7AE3; function player_external_7AE3 at 0x31459 */

void player_external_7AE3(void)

{
  int iVar1;
  uint uVar2;
  undefined2 uVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  
  iVar1 = func_0x0000ffff();
  if (iVar1 < 0x81) {
    func_0x0000ffff(0);
  }
  else {
    func_0x0000ffff(0);
  }
  *(undefined2 *)(unaff_DI + 0x18) = 0x7b71;
  uVar2 = func_0x0000ffff(0);
  *(uint *)(unaff_DI + 0x2a) = uVar2 >> 2;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x10;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  uVar3 = func_0x0000ffff(0);
  *(undefined2 *)(unaff_DI + 0x34) = uVar3;
  *(undefined4 *)(unaff_DI + 10) = 0xffff9000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x39) = 0x14;
  *(undefined1 *)(unaff_DI + 0x36) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x19;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x7B50; function player_external_7B50 at 0x31568 */

void player_external_7B50(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_7AE3();
  return;
}



/* requested 0x7B5E; function player_external_7B5E at 0x31582 */

void player_external_7B5E(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_7AE3();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x7B71; function player_external_7B71 at 0x31601 */

void player_external_7B71(void)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  long lVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar5;
  undefined1 uVar6;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  iVar3 = -*(int *)(unaff_DI + 0x39);
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar3 = *(int *)(unaff_DI + 0x39);
  }
  bVar5 = *(int *)(unaff_DI + 4) + iVar3 == 0;
  func_0x0000ffff(0);
  uVar6 = false;
  if (bVar5) {
    iVar3 = -*(int *)(unaff_DI + 0x39);
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar3 = *(int *)(unaff_DI + 0x39);
    }
    bVar5 = *(int *)(unaff_DI + 4) + iVar3 == 0;
    func_0x0000ffff(0);
    uVar6 = false;
    if (bVar5) {
      iVar3 = -*(int *)(unaff_DI + 0x39);
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar3 = *(int *)(unaff_DI + 0x39);
      }
      uVar6 = *(int *)(unaff_DI + 4) + iVar3 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar6) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x36) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x32);
      uVar2 = *(int *)(unaff_DI + 0x34) + 10U & 0x3ff;
      *(uint *)(unaff_DI + 0x34) = uVar2;
      iVar3 = (int)(*(char *)(uVar2 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x32) = iVar3;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar3;
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0xb4 < *(int *)(unaff_DI + 0x2a)) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        *(undefined1 *)(unaff_DI + 0x36) = 1;
        *(undefined2 *)(unaff_DI + 0x39) = 0x28;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x19) {
        func_0x0000ffff(0);
      }
      lVar4 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x400;
      if (lVar4 < -0x8000) {
        lVar4 = -0x8000;
      }
      else if (0x8000 < lVar4) {
        lVar4 = 0x8000;
      }
      *(long *)(unaff_DI + 10) = lVar4;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar4;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar3 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar3,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 5;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x19;
      }
    }
    else {
      lVar4 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar4 < -0x8000) {
        lVar4 = -0x8000;
      }
      else if (0x8000 < lVar4) {
        lVar4 = 0x8000;
      }
      *(long *)(unaff_DI + 10) = lVar4;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar4;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar3 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar3,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x19;
      }
    }
  }
  else if (*(char *)(unaff_DI + 0x2f) < '\x01') {
    if (*(char *)(unaff_DI + 0x36) != '\x02') {
      if (*(char *)(unaff_DI + 0x36) == '\x03') {
        lVar4 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x100000;
        if (lVar4 < 0x8000) {
          lVar4 = 0x8000;
        }
        else if (-0x8000 < lVar4) {
          lVar4 = -0x8000;
        }
        *(long *)(unaff_DI + 10) = lVar4;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar4;
        piVar1 = (int *)(unaff_DI + 0x37);
        iVar3 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar3,1) != *piVar1 < 0) {
          *(undefined1 *)(unaff_DI + 0x36) = 0;
        }
        goto LAB_0000_7e1a;
      }
      *(undefined1 *)(unaff_DI + 0x36) = 2;
      *(undefined2 *)(unaff_DI + 0x37) = 0x32;
    }
    lVar4 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x8000;
    if (lVar4 < -0x15000) {
      lVar4 = -0x15000;
    }
    else if (0x15000 < lVar4) {
      lVar4 = 0x15000;
    }
    *(long *)(unaff_DI + 10) = lVar4;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar4;
    piVar1 = (int *)(unaff_DI + 0x37);
    iVar3 = *piVar1;
    *piVar1 = *piVar1 + -1;
    if (SBORROW2(iVar3,1) != *piVar1 < 0) {
      *(undefined2 *)(unaff_DI + 0x37) = 0x1e;
      *(undefined1 *)(unaff_DI + 0x36) = 3;
    }
  }
  else {
    *(undefined1 *)(unaff_DI + 0x36) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0;
    *(undefined2 *)(unaff_DI + 0x39) = 0x14;
  }
LAB_0000_7e1a:
  if (*(int *)0x8806 != 0) {
    iVar3 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar3) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar3 = 0;
    }
    iVar3 = iVar3 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0x11 < *(int *)(iVar3 + -0x7822)) &&
         (*(int *)(iVar3 + -0x7822) < *(int *)(unaff_DI + 4) + 0x12)) &&
        (*(int *)(iVar3 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x14 < *(int *)(iVar3 + -0x7820))) {
      *(undefined2 *)(iVar3 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4BA0;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x6651; function player_external_6651 at 0x26193 */

void player_external_6651(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x66e1;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x14;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  iVar1 = func_0x0000ffff(0);
  *(long *)(unaff_DI + 10) = *(long *)(unaff_DI + 10) - (long)(iVar1 << 2);
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  return;
}



/* requested 0x6699; function player_external_6699 at 0x26265 */

void player_external_6699(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x66e1;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x14;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(undefined4 *)(unaff_DI + 10) = 0x15000;
  iVar1 = func_0x0000ffff(0);
  *(long *)(unaff_DI + 10) = *(long *)(unaff_DI + 10) + (long)(iVar1 << 2);
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  return;
}



/* requested 0x66E1; function player_external_66E1 at 0x26337 */

void player_external_66E1(void)

{
  uint uVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar2;
  undefined1 uVar3;
  
  bVar2 = false;
  uVar3 = *"}-\x14u" == '\0';
  if ((bool)uVar3) {
    unaff_CS = 0;
    func_0x0000ffff();
    if (bVar2) {
      func_0x0000ffff(0);
      return;
    }
  }
  else {
    uVar1 = (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x40;
    uVar3 = uVar1 == 0x1c0;
    if ((0x1c0 < uVar1) ||
       (uVar1 = (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x40, uVar3 = uVar1 == 0x130,
       0x130 < uVar1)) {
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
  }
  func_0x0000ffff(unaff_CS);
  func_0x0000ffff(0);
  if ((bool)uVar3) {
    *(undefined2 *)(unaff_DI + 0x18) = 0x6757;
  }
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
  func_0x0000ffff(0);
  return;
}



/* requested 0x6757; function player_external_6757 at 0x26455 */

void player_external_6757(void)

{
  uint uVar1;
  long lVar2;
  int iVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar4;
  undefined1 uVar5;
  
  bVar4 = false;
  uVar5 = *"}-\x14u" == '\0';
  if ((bool)uVar5) {
    unaff_CS = 0;
    func_0x0000ffff();
    if (bVar4) {
      func_0x0000ffff(0);
      return;
    }
  }
  else {
    uVar1 = (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x40;
    uVar5 = uVar1 == 0x1c0;
    if ((0x1c0 < uVar1) ||
       (uVar1 = (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x40, uVar5 = uVar1 == 0x130,
       0x130 < uVar1)) {
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
  }
  func_0x0000ffff(unaff_CS);
  func_0x0000ffff(0);
  if (!(bool)uVar5) {
    *(undefined2 *)(unaff_DI + 0x18) = 0x6838;
    func_0x0000ffff(0);
  }
  lVar2 = *(long *)(unaff_DI + 10);
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar2;
  lVar2 = lVar2 >> 1;
  if (lVar2 < 1) {
    lVar2 = -lVar2;
  }
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
  if (*(int *)0x8806 != 0) {
    iVar3 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar3) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar3 = 0;
    }
    iVar3 = iVar3 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar3 + -0x7822)) &&
         (*(int *)(iVar3 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
        (*(int *)(iVar3 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar3 + -0x7820))) {
      *(undefined2 *)(iVar3 + -0x7822) = 0;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x6838; function player_external_6838 at 0x26680 */

void player_external_6838(void)

{
  int *piVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  piVar1 = (int *)(unaff_DI + 0x2a);
  *piVar1 = *piVar1 + -1;
  if (*piVar1 == 0) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  return;
}



/* requested 0x7E78; function player_external_7E78 at 0x32376 */

void player_external_7E78(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x7ef8;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined4 *)(unaff_DI + 10) = 0xfffe9000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x40) = 0;
  *(undefined4 *)(unaff_DI + 0x36) = *(undefined4 *)(unaff_DI + 6);
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x7ED7; function player_external_7ED7 at 0x32471 */

void player_external_7ED7(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_7E78();
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  return;
}



/* requested 0x7EE5; function player_external_7EE5 at 0x32485 */

void player_external_7EE5(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  player_external_7E78();
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x7EF8; function player_external_7EF8 at 0x32504 */

void player_external_7EF8(void)

{
  int *piVar1;
  ulong *puVar2;
  uint uVar3;
  int iVar4;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  long lVar5;
  ulong uVar6;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar7;
  undefined1 uVar8;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  iVar4 = -0x20;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar4 = 0x20;
  }
  bVar7 = *(int *)(unaff_DI + 4) + iVar4 == 0;
  func_0x0000ffff(0);
  uVar8 = false;
  if (bVar7) {
    iVar4 = -0x20;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar4 = 0x20;
    }
    bVar7 = *(int *)(unaff_DI + 4) + iVar4 == 0;
    func_0x0000ffff(0);
    uVar8 = false;
    if (bVar7) {
      iVar4 = -0x20;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar4 = 0x20;
      }
      uVar8 = *(int *)(unaff_DI + 4) + iVar4 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar8) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(int *)(unaff_DI + 0x32) < 1) {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0xaa < *(int *)(unaff_DI + 0x2a)) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar5 < -0x17000) {
        lVar5 = -0x17000;
      }
      else if (0x17000 < lVar5) {
        lVar5 = 0x17000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar5 < -0x17000) {
        lVar5 = -0x17000;
      }
      else if (0x17000 < lVar5) {
        lVar5 = 0x17000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
    iVar4 = *(int *)(*(int *)0x881a + 4);
    if (*(char *)(unaff_DI + 0x29) < '\0') {
      if ((iVar4 + 0x28 <= *(int *)(unaff_DI + 4)) || (*(int *)(unaff_DI + 4) <= iVar4 + 0x23))
      goto LAB_0000_83af;
    }
    else if ((*(int *)(unaff_DI + 4) <= iVar4 + -0x28) || (iVar4 + -0x23 <= *(int *)(unaff_DI + 4)))
    goto LAB_0000_83af;
    if ((*(int *)(unaff_DI + 8) < *(int *)(*(int *)0x881a + 8)) &&
       (*(int *)0x81c4 < *(int *)(unaff_DI + 8))) {
      *(undefined2 *)(unaff_DI + 0x32) = 1;
      func_0x0000ffff(0);
    }
    goto LAB_0000_83af;
  }
  if (*(int *)(unaff_DI + 0x32) < 2) {
    *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x40);
    uVar3 = *(int *)(unaff_DI + 0x3e) + 0x20U & 0x3ff;
    *(uint *)(unaff_DI + 0x3e) = uVar3;
    iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
    *(int *)(unaff_DI + 0x40) = iVar4;
    *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -5000;
    *(long *)(unaff_DI + 2) =
         *(long *)(unaff_DI + 2) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x2000;
    *(int *)(unaff_DI + 0x34) = *(int *)(unaff_DI + 0x34) + 1;
    if (*(int *)(unaff_DI + 0x34) < 0x33) goto LAB_0000_83af;
    *(undefined2 *)(unaff_DI + 0x34) = 0;
    *(undefined2 *)(unaff_DI + 0x32) = 2;
  }
  if (4 < *(int *)(unaff_DI + 0x32)) {
    if (*(int *)(unaff_DI + 0x32) < 6) {
      *(int *)(unaff_DI + 0x34) = *(int *)(unaff_DI + 0x34) + 1;
      if (*(int *)(unaff_DI + 0x34) < 0x6f) goto LAB_0000_83af;
      *(undefined2 *)(unaff_DI + 0x34) = 0;
      *(undefined2 *)(unaff_DI + 0x32) = 6;
      func_0x0000ffff(0);
    }
    if (*(char *)(unaff_DI + 0x32) == '\a') {
LAB_0000_82fa:
      uVar6 = *(long *)(unaff_DI + 0xe) - 4000;
      if ((long)uVar6 < -0x40000) {
        uVar6 = 0xfffc0000;
      }
      else if (0x40000 < (long)uVar6) {
        uVar6 = 0x40000;
      }
      *(ulong *)(unaff_DI + 0xe) = uVar6;
      puVar2 = (ulong *)(unaff_DI + 6);
      bVar7 = CARRY4(*puVar2,uVar6);
      *puVar2 = *puVar2 + uVar6;
      func_0x0000ffff(0);
      if (bVar7) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
      }
      *(long *)(unaff_DI + 2) =
           *(long *)(unaff_DI + 2) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x20000;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(undefined1 *)(unaff_DI + 0x32) = 8;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x46;
      }
    }
    else {
      if (*(char *)(unaff_DI + 0x32) != '\b') {
        *(undefined1 *)(unaff_DI + 0x32) = 7;
        *(undefined4 *)(unaff_DI + 0xe) = 0xfffffe0c;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x46;
        goto LAB_0000_82fa;
      }
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
    }
    if (*(long *)(unaff_DI + 6) < *(long *)(unaff_DI + 0x36)) {
      *(undefined2 *)(unaff_DI + 0x32) = 0;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      *(undefined4 *)(unaff_DI + 10) = *(undefined4 *)(unaff_DI + 0x3a);
      uVar8 = *(undefined1 *)(unaff_DI + 0x44);
      *(undefined1 *)(unaff_DI + 0x29) = uVar8;
      *(undefined1 *)(unaff_DI + 0x28) = uVar8;
      *(undefined2 *)(unaff_DI + 0x2a) = 0x28;
    }
    goto LAB_0000_83af;
  }
  *(undefined4 *)(unaff_DI + 0x3a) = *(undefined4 *)(unaff_DI + 10);
  *(undefined1 *)(unaff_DI + 0x44) = *(undefined1 *)(unaff_DI + 0x29);
  if (*(char *)(unaff_DI + 0x32) == '\x03') {
LAB_0000_819e:
    lVar5 = *(long *)(unaff_DI + 0xe) + 20000;
    if (lVar5 < -0x50000) {
      lVar5 = -0x50000;
    }
    else if (0x50000 < lVar5) {
      lVar5 = 0x50000;
    }
    *(long *)(unaff_DI + 0xe) = lVar5;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
    uVar6 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x10000;
    if ((long)uVar6 < -0x17000) {
      uVar6 = 0xfffe9000;
    }
    else if (0x17000 < (long)uVar6) {
      uVar6 = 0x17000;
    }
    *(ulong *)(unaff_DI + 10) = uVar6;
    puVar2 = (ulong *)(unaff_DI + 2);
    bVar7 = CARRY4(*puVar2,uVar6);
    *puVar2 = *puVar2 + uVar6;
    piVar1 = (int *)(unaff_DI + 0x2d);
    iVar4 = *piVar1;
    *piVar1 = *piVar1 + -1;
    if (SBORROW2(iVar4,1) != *piVar1 < 0) {
      *(undefined1 *)(unaff_DI + 0x32) = 4;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
    }
  }
  else {
    if (*(char *)(unaff_DI + 0x32) != '\x04') {
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      *(undefined4 *)(unaff_DI + 0xe) = 500;
      *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      goto LAB_0000_819e;
    }
    puVar2 = (ulong *)(unaff_DI + 6);
    bVar7 = CARRY4(*puVar2,*(ulong *)(unaff_DI + 0xe));
    *puVar2 = *puVar2 + *(ulong *)(unaff_DI + 0xe);
  }
  func_0x0000ffff(0);
  if (!bVar7) {
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) == 0) {
        func_0x0000ffff(0);
        uVar3 = extraout_DX_00;
joined_r0x00008294:
        if ((uVar3 & 0x70) == 0) goto LAB_0000_83af;
      }
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) == 0) {
        func_0x0000ffff(0);
        uVar3 = extraout_DX_02;
        goto joined_r0x00008294;
      }
    }
  }
  *(undefined2 *)(unaff_DI + 0x32) = 5;
  func_0x0000ffff(0);
LAB_0000_83af:
  if (*(int *)0x8806 != 0) {
    iVar4 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar4) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar4 = 0;
    }
    iVar4 = iVar4 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar4 + -0x7822)) &&
         (*(int *)(iVar4 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
        (*(int *)(iVar4 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar4 + -0x7820))) {
      *(undefined2 *)(iVar4 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4AB3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x840D; function player_external_840D at 0x33805 */

void player_external_840D(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x8472;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffee000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x8451; function player_external_8451 at 0x33873 */

void player_external_8451(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_840D();
  return;
}



/* requested 0x845F; function player_external_845F at 0x33887 */

void player_external_845F(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_840D();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x8472; function player_external_8472 at 0x33906 */

void player_external_8472(void)

{
  int *piVar1;
  int iVar2;
  long lVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar4;
  undefined1 uVar5;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  iVar2 = -0x23;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar2 = 0x23;
  }
  bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
  func_0x0000ffff(0);
  uVar5 = false;
  if (bVar4) {
    iVar2 = -0x23;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar2 = 0x23;
    }
    bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
    func_0x0000ffff(0);
    uVar5 = false;
    if (bVar4) {
      iVar2 = -0x23;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar2 = 0x23;
      }
      uVar5 = *(int *)(unaff_DI + 4) + iVar2 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar5) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x21;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_852d:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x21;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_852d;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(uint *)(unaff_DI + 0x2a) = (uint)(int)*(char *)(iVar2 + 0x646c) >> 1;
        *(undefined1 *)(unaff_DI + 0x32) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar3 < -0x12000) {
        lVar3 = -0x12000;
      }
      else if (0x12000 < lVar3) {
        lVar3 = 0x12000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 5;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x12000) {
        lVar3 = -0x12000;
      }
      else if (0x12000 < lVar3) {
        lVar3 = 0x12000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
  }
  else if (*(char *)(unaff_DI + 0x2f) < '\x01') {
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x33) = *(int *)(unaff_DI + 0x33) + 1;
      if (*(int *)(unaff_DI + 0x33) < 0x4c) goto LAB_0000_8773;
      *(undefined2 *)(unaff_DI + 0x33) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      func_0x0000ffff(0);
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar3 < -0x12000) {
        lVar3 = -0x12000;
      }
      else if (0x12000 < lVar3) {
        lVar3 = 0x12000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_8773;
      }
      else if (0 < lVar3) goto LAB_0000_8773;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_8773;
    }
    lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
    if (lVar3 < -0x12000) {
      lVar3 = -0x12000;
    }
    else if (0x12000 < lVar3) {
      lVar3 = 0x12000;
    }
    *(long *)(unaff_DI + 10) = lVar3;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      if (-0x12000 < lVar3) goto LAB_0000_8773;
    }
    else if (lVar3 < 0x12000) goto LAB_0000_8773;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x78;
  }
LAB_0000_8773:
  if (*(int *)0x8806 != 0) {
    iVar2 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar2) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar2 = 0;
    }
    iVar2 = iVar2 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0x11 < *(int *)(iVar2 + -0x7822)) &&
         (*(int *)(iVar2 + -0x7822) < *(int *)(unaff_DI + 4) + 0x12)) &&
        (*(int *)(iVar2 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x14 < *(int *)(iVar2 + -0x7820))) {
      *(undefined2 *)(iVar2 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4AB3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x500C; function player_external_500C at 0x20492 */

void player_external_500C(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x5071;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x5050; function player_external_5050 at 0x20560 */

void player_external_5050(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_500C();
  return;
}



/* requested 0x505E; function player_external_505E at 0x20574 */

void player_external_505E(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_500C();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x5071; function player_external_5071 at 0x20593 */

void player_external_5071(void)

{
  int *piVar1;
  int iVar2;
  long lVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  undefined1 in_CF;
  bool bVar4;
  undefined1 uVar5;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    func_0x0000ffff(0);
    return;
  }
  func_0x0000ffff(0);
  iVar2 = -0x28;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar2 = 0x28;
  }
  bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
  func_0x0000ffff(0);
  uVar5 = false;
  if (bVar4) {
    iVar2 = -0x28;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar2 = 0x28;
    }
    bVar4 = *(int *)(unaff_DI + 4) + iVar2 == 0;
    func_0x0000ffff(0);
    uVar5 = false;
    if (bVar4) {
      iVar2 = -0x28;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar2 = 0x28;
      }
      uVar5 = *(int *)(unaff_DI + 4) + iVar2 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar5) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x26;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_512c:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x26;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_512c;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(uint *)(unaff_DI + 0x2a) = (uint)(int)*(char *)(iVar2 + 0x646c) >> 1;
        *(undefined1 *)(unaff_DI + 0x32) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar2 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar2,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
  }
  else if (*(char *)(unaff_DI + 0x2f) < '\x01') {
    iVar2 = unaff_DI;
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x33) = *(int *)(unaff_DI + 0x33) + 1;
      if (*(int *)(unaff_DI + 0x33) < 0x2e) goto LAB_0000_5399;
      func_0x0000ffff(0);
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar2 + 0x29);
      *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar2 + 2);
      *(long *)(unaff_DI + 6) = *(long *)(iVar2 + 6) + -0xc0000;
      *(undefined2 *)(iVar2 + 0x33) = 0;
      *(undefined1 *)(iVar2 + 0x32) = 3;
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar3 < -0x15000) {
        lVar3 = -0x15000;
      }
      else if (0x15000 < lVar3) {
        lVar3 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_5399;
      }
      else if (0 < lVar3) goto LAB_0000_5399;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      goto LAB_0000_5399;
    }
    lVar3 = *(long *)(iVar2 + 10) + (long)(int)*(char *)(iVar2 + 0x29) * 0x800;
    if (lVar3 < -0x15000) {
      lVar3 = -0x15000;
    }
    else if (0x15000 < lVar3) {
      lVar3 = 0x15000;
    }
    *(long *)(iVar2 + 10) = lVar3;
    *(long *)(iVar2 + 2) = *(long *)(iVar2 + 2) + lVar3;
    unaff_DI = iVar2;
    if (*(char *)(iVar2 + 0x29) < '\x01') {
      if (-0x15000 < lVar3) goto LAB_0000_5399;
    }
    else if (lVar3 < 0x15000) goto LAB_0000_5399;
    *(undefined1 *)(iVar2 + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x78;
  }
LAB_0000_5399:
  if (*(int *)0x8806 != 0) {
    iVar2 = *(int *)(unaff_DI + 0x30);
    if (*(int *)0x8808 <= iVar2) {
      *(undefined2 *)(unaff_DI + 0x30) = 0;
      iVar2 = 0;
    }
    iVar2 = iVar2 * 4;
    if ((((*(int *)(unaff_DI + 4) + -0x14 < *(int *)(iVar2 + -0x7822)) &&
         (*(int *)(iVar2 + -0x7822) < *(int *)(unaff_DI + 4) + 0x14)) &&
        (*(int *)(iVar2 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
       (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar2 + -0x7820))) {
      *(undefined2 *)(iVar2 + -0x7822) = 0;
      *(undefined2 *)(unaff_DI + 0x18) = (code *)player_external_4AB3;
    }
    *(int *)(unaff_DI + 0x30) = *(int *)(unaff_DI + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



/* requested 0x5EAC; function player_external_5EAC at 0x24236 */

void player_external_5EAC(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x5f28;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x34) = 0;
  *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + 0x20;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined2 *)(unaff_DI + 0x40) = 0;
  *(undefined2 *)(unaff_DI + 0x42) = 0;
  *(undefined4 *)(unaff_DI + 0x36) = *(undefined4 *)(unaff_DI + 6);
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* requested 0x5F07; function player_external_5F07 at 0x24327 */

void player_external_5F07(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  player_external_5EAC();
  return;
}



/* requested 0x5F15; function player_external_5F15 at 0x24341 */

void player_external_5F15(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  player_external_5EAC();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* requested 0x5F28; function player_external_5F28 at 0x24360 */

void player_external_5F28(void)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  long lVar5;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar6;
  undefined1 uVar7;
  
  func_0x0000ffff();
  iVar3 = -0x14;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar3 = 0x14;
  }
  bVar6 = *(int *)(unaff_DI + 4) + iVar3 == 0;
  func_0x0000ffff(0);
  uVar7 = false;
  if (bVar6) {
    iVar3 = -0x14;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar3 = 0x14;
    }
    bVar6 = *(int *)(unaff_DI + 4) + iVar3 == 0;
    func_0x0000ffff(0);
    uVar7 = false;
    if (bVar6) {
      iVar3 = -0x14;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar3 = 0x14;
      }
      uVar7 = *(int *)(unaff_DI + 4) + iVar3 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar7) {
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  bVar6 = *(int *)(unaff_DI + 0x32) == 0;
  iVar3 = unaff_DI;
  if (*(int *)(unaff_DI + 0x32) < 1) {
    func_0x0000ffff(0);
    if (bVar6) {
      func_0x0000ffff(0);
      return;
    }
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      if (*(int *)(unaff_DI + 0x42) == 1) {
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x40);
        uVar2 = *(int *)(unaff_DI + 0x3e) + 0x20U & 0x7ff;
        *(uint *)(unaff_DI + 0x3e) = uVar2;
        iVar4 = (int)(*(char *)(uVar2 + 0x7974) >> 4);
        *(int *)(unaff_DI + 0x40) = iVar4;
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (0x96 < *(int *)(unaff_DI + 0x2a)) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      if (*(int *)(unaff_DI + 0x2d) == 0x14) {
        func_0x0000ffff(0);
      }
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar5 < -0x15000) {
        lVar5 = -0x15000;
      }
      else if (0x15000 < lVar5) {
        lVar5 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x3c;
      }
    }
    else {
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar5 < -0x15000) {
        lVar5 = -0x15000;
      }
      else if (0x15000 < lVar5) {
        lVar5 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      piVar1 = (int *)(unaff_DI + 0x2d);
      iVar4 = *piVar1;
      *piVar1 = *piVar1 + -1;
      if (SBORROW2(iVar4,1) != *piVar1 < 0) {
        *(char *)(unaff_DI + 0x2c) = -*(char *)(unaff_DI + 0x2c);
        *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
        *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
      }
    }
    iVar4 = *(int *)(*(int *)0x881a + 4);
    if (*(char *)(unaff_DI + 0x29) < '\0') {
      if ((iVar4 + 0x14 <= *(int *)(unaff_DI + 4)) || (*(int *)(unaff_DI + 4) <= iVar4 + 0xf))
      goto LAB_0000_62ae;
    }
    else if ((*(int *)(unaff_DI + 4) <= iVar4 + -0x14) || (iVar4 + -0xf <= *(int *)(unaff_DI + 4)))
    goto LAB_0000_62ae;
    if ((*(int *)(unaff_DI + 8) < *(int *)(*(int *)0x881a + 8)) &&
       (*(int *)0x81c4 < *(int *)(unaff_DI + 8))) {
      *(undefined2 *)(unaff_DI + 0x32) = 1;
    }
  }
  else if (*(int *)(unaff_DI + 0x32) < 4) {
    *(undefined4 *)(unaff_DI + 0x3a) = *(undefined4 *)(unaff_DI + 10);
    if (*(char *)(unaff_DI + 0x32) == '\x02') {
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) goto LAB_0000_62ae;
      func_0x0000ffff(0);
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar3 + 0x29);
      *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar3 + 2);
      *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(iVar3 + 6);
      *(undefined2 *)(iVar3 + 0x44) = 0;
      *(undefined1 *)(iVar3 + 0x32) = 3;
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar5 < -0x15000) {
        lVar5 = -0x15000;
      }
      else if (0x15000 < lVar5) {
        lVar5 = 0x15000;
      }
      *(long *)(unaff_DI + 10) = lVar5;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar5 < 0) goto LAB_0000_62ae;
      }
      else if (0 < lVar5) goto LAB_0000_62ae;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      goto LAB_0000_62ae;
    }
    lVar5 = *(long *)(iVar3 + 10) + (long)(int)*(char *)(iVar3 + 0x29) * 0x800;
    if (lVar5 < -0x15000) {
      lVar5 = -0x15000;
    }
    else if (0x15000 < lVar5) {
      lVar5 = 0x15000;
    }
    *(long *)(iVar3 + 10) = lVar5;
    *(long *)(iVar3 + 2) = *(long *)(iVar3 + 2) + lVar5;
    if (*(char *)(iVar3 + 0x29) < '\x01') {
      if (-0x15000 < lVar5) goto LAB_0000_62ae;
    }
    else if (lVar5 < 0x15000) goto LAB_0000_62ae;
    *(undefined1 *)(iVar3 + 0x32) = 0;
  }
  else {
    *(undefined2 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
    *(undefined4 *)(unaff_DI + 10) = *(undefined4 *)(unaff_DI + 0x3a);
  }
LAB_0000_62ae:
  if (*(int *)0x8806 != 0) {
    iVar4 = *(int *)(iVar3 + 0x30);
    if (*(int *)0x8808 <= iVar4) {
      *(undefined2 *)(iVar3 + 0x30) = 0;
      iVar4 = 0;
    }
    iVar4 = iVar4 * 4;
    if ((((*(int *)(iVar3 + 4) + -0xf < *(int *)(iVar4 + -0x7822)) &&
         (*(int *)(iVar4 + -0x7822) < *(int *)(iVar3 + 4) + 0xf)) &&
        (*(int *)(iVar4 + -0x7820) < *(int *)(iVar3 + 8) + 5)) &&
       (*(int *)(iVar3 + 8) + -0x19 < *(int *)(iVar4 + -0x7820))) {
      *(undefined2 *)(iVar4 + -0x7822) = 0;
      *(int *)(iVar3 + 0x42) = *(int *)(iVar3 + 0x42) + 1;
      if (*(int *)(iVar3 + 0x42) == 2) {
        *(undefined2 *)(iVar3 + 0x18) = (code *)player_external_4AB3;
      }
      else {
        *(undefined2 *)0x612e = 0xd;
        func_0x0000ffff(0);
      }
    }
    *(int *)(iVar3 + 0x30) = *(int *)(iVar3 + 0x30) + 1;
  }
  func_0x0000ffff(0);
  return;
}



