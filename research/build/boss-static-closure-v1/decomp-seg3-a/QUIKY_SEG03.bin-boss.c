/* Targeted boss-transition decompilation from QUIKY_SEG03.bin */
/* Addresses are segment-relative raw offsets. */

/* Known pooled-object layout (0x78-byte stride; roles are provisional where noted): */
/*   +0x02/+0x06  signed 16.16 world X/Y positions; +0x04/+0x08 are their integer words. */
/*   +0x12        logical sprite slot (0xffff means no standard BOB slot). */
/*   +0x14        ARE/object kind byte/word used by the generic kind scan. */
/*   +0x17        scheduler phase byte; generic update passes visit phases 0, 1, 2. */
/*   +0x18        far update callback (offset, segment). */
/*   +0x1c        callback segment word initialized by the generic allocator. */
/*   +0x1a        callback-side auxiliary word; allocator initializes it to 0xffff. */
/*   +0x2a        role-dependent cursor/link; Doktor uses it as projectile scan cursor. */
/*   +0x2c        role-dependent counter; Doktor uses it as damage/phase hit count. */
/*   +0x2e/+0x2f  role-dependent state/timer bytes; Doktor uses the pair for hit rearm. */
/*   +0x32        state-machine counter in shared transient/effect callbacks. */
/*   +0x34        role-dependent mode/state byte in player and boss records. */
/*   +0x36        role-dependent child/link word used by boss constructors. */
/*   +0x38/+0x3c/+0x3e/+0x40/+0x42/+0x44  boss movement, animation, and timer words. */
/*   +0x46/+0x48  additional world-specific phase/child-link words (W3/W5 evidence). */
/*   The generic allocator scans a 64-entry pool; level-specific boss constructors */
/*   then install custom callbacks, so bosses are not ARE dispatch entries. */
/*   Factory -> scheduler insertion is visible at 01F7:0E60 -> 01F7:1036. */

/* TARGET initialize_game_state at 0x0A43; resolved function entry 0000:0a43 */
/* CALLERS of 0000:0a43: */
/*   <none resolved> */

void initialize_game_state(void)

{
  undefined1 uVar1;
  undefined2 unaff_DS;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  
  func_0x0000ffff();
  func_0x0000ffff(0);
  *(undefined2 *)0x657a = 0;
  *(undefined2 *)0x657c = 0;
  *(undefined2 *)0x8196 = 0;
  *(undefined2 *)0x88bc = 0;
  uVar4 = 0x6586;
  uVar3 = 0x400;
  uVar2 = 0;
  func_0x0000ffff(0,0,0x400,0x6586);
  *(undefined2 *)0x365e = 0;
  while( true ) {
    uVar1 = func_0x0000ffff(0,0x100);
    *(undefined1 *)(*(int *)0x365e + 0x646c) = uVar1;
    if (*(int *)0x365e == 0xff) break;
    *(int *)0x365e = *(int *)0x365e + 1;
  }
  *(undefined2 *)0x6468 = 0;
  *(undefined2 *)0x365e = 0;
  while( true ) {
    *(undefined2 *)(*(int *)0x365e * 2 + 0x6d8e) = 0xffff;
    func_0x0000ffff(0,uVar2,uVar3,uVar4);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    uVar1 = func_0x0000ffff(0);
    *(undefined1 *)(*(int *)0x365e + 0x7974) = uVar1;
    if (*(int *)0x365e == 999) break;
    *(int *)0x365e = *(int *)0x365e + 1;
  }
  *(undefined2 *)0x365e = 1000;
  while( true ) {
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    uVar1 = func_0x0000ffff(0);
    *(undefined1 *)(*(int *)0x365e + 0x7974) = uVar1;
    if (*(int *)0x365e == 0x7ff) break;
    *(int *)0x365e = *(int *)0x365e + 1;
  }
  return;
}



/* TARGET are_object_factory at 0x0E06; resolved function entry 0000:0e06 */
/* CALLERS of 0000:0e06: */
/*   <none resolved> */

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



/* TARGET object_scheduler_insert at 0x1036; resolved function entry 0000:1036 */
/* CALLERS of 0000:1036: */
/*   <none resolved> */

undefined4 object_scheduler_insert(void)

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



/* TARGET boss_helper_runtime_1b77 at 0x1B77; resolved function entry 0000:1b77 */
/* CALLERS of 0000:1b77: */
/*   <none resolved> */

undefined2 boss_helper_runtime_1b77(void)

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



/* TARGET boss_helper_runtime_1c6e at 0x1C6E; resolved function entry 0000:1c6e */
/* CALLERS of 0000:1c6e: */
/*   <none resolved> */

undefined4 boss_helper_runtime_1c6e(void)

{
  long lVar1;
  uint in_AX;
  uint in_BX;
  undefined2 unaff_DS;
  
  lVar1 = (ulong)(in_AX >> 4) * (ulong)*(uint *)0x657e;
  return CONCAT22((int)((ulong)lVar1 >> 0x10),
                  *(undefined2 *)(*(int *)0x657a + (in_BX >> 4) * 2 + (int)lVar1));
}



/* TARGET compute_state_machine_bounds at 0x393C; resolved function entry 0000:393c */
/* CALLERS of 0000:393c: */
/*   <none resolved> */

undefined4 compute_state_machine_bounds(void)

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



/* TARGET shared_phase_child_constructor at 0x4B70; resolved function entry 0000:4b70 */
/* CALLERS of 0000:4b70: */
/*   <none resolved> */

void shared_phase_child_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x4c74;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined2 *)0x612e = 0xd;
  func_0x0000ffff(0);
  return;
}



/* TARGET w3_phase_helper_constructor at 0x6616; resolved function entry 0000:6616 */
/* CALLERS of 0000:6616: */
/*   <none resolved> */

void w3_phase_helper_constructor(void)

{
  char cVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x66e1;
  *(undefined2 *)(unaff_DI + 0x2a) = 0x14;
  cVar1 = *(char *)(unaff_DI + 0x29);
  *(char *)(unaff_DI + 0x28) = cVar1;
  *(undefined4 *)(unaff_DI + 10) = 0xfffeb000;
  if (-1 < cVar1) {
    *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  }
  *(undefined2 *)(unaff_DI + 0x30) = 0;
  *(undefined2 *)(unaff_DI + 0x32) = 0;
  return;
}



/* TARGET w3_phase_constructor_c955 at 0xC955; resolved function entry 0000:c955 */
/* CALLERS of 0000:c955: */
/*   <none resolved> */

void w3_phase_constructor_c955(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x389;
  *(undefined2 *)(unaff_DI + 0x18) = 0xcb11;
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
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0xf0000;
  return;
}



/* TARGET w3_phase_constructor_c9f8 at 0xC9F8; resolved function entry 0000:c9f8 */
/* CALLERS of 0000:c9f8: */
/*   <none resolved> */

void w3_phase_constructor_c9f8(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x387;
  *(undefined2 *)(unaff_DI + 0x18) = 0xcb11;
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
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0xa0000;
  return;
}



/* TARGET w3_phase_constructor_ca9b at 0xCA9B; resolved function entry 0000:ca9b */
/* CALLERS of 0000:ca9b: */
/*   <none resolved> */

void w3_phase_constructor_ca9b(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x388;
  *(undefined2 *)(unaff_DI + 0x18) = 0xcb11;
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



/* TARGET w5_phase_constructor_dc09 at 0xDC09; resolved function entry 0000:dc09 */
/* CALLERS of 0000:dc09: */
/*   <none resolved> */

void w5_phase_constructor_dc09(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x387;
  *(undefined2 *)(unaff_DI + 0x18) = 0xdd22;
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
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0xf0000;
  return;
}



/* TARGET w5_phase_constructor_dcac at 0xDCAC; resolved function entry 0000:dcac */
/* CALLERS of 0000:dcac: */
/*   <none resolved> */

void w5_phase_constructor_dcac(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x388;
  *(undefined2 *)(unaff_DI + 0x18) = 0xdd22;
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



/* TARGET w5_scrap_constructor_dfb3 at 0xDFB3; resolved function entry 0000:dfb3 */
/* CALLERS of 0000:dfb3: */
/*   <none resolved> */

void w5_scrap_constructor_dfb3(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000def2();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe0f5;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x4000;
  *(undefined4 *)(unaff_DI + 10) = 0xffffd000;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0x60000;
  return;
}



/* TARGET w5_scrap_constructor_e01d at 0xE01D; resolved function entry 0000:e01d */
/* CALLERS of 0000:e01d: */
/*   <none resolved> */

void w5_scrap_constructor_e01d(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000def2();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe2bf;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x8000;
  *(undefined4 *)(unaff_DI + 10) = 0x4000;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0x60000;
  return;
}



/* TARGET w5_effect_constructor_e39e at 0xE39E; resolved function entry 0000:e39e */
/* CALLERS of 0000:e39e: */
/*   <none resolved> */

void w5_effect_constructor_e39e(void)

{
  char cVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(char *)0x88ae == '\x06') {
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    cVar1 = *(char *)(iVar2 + 0x646c);
    if (cVar1 < -0x3f) {
      *(undefined2 *)(unaff_DI + 0x12) = 0x25f;
    }
    else if (cVar1 < '\x01') {
      *(undefined2 *)(unaff_DI + 0x12) = 0x260;
    }
    else if (cVar1 < 'A') {
      *(undefined2 *)(unaff_DI + 0x12) = 0x261;
    }
    else {
      *(undefined2 *)(unaff_DI + 0x12) = 0x262;
    }
  }
  else {
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    if (*(char *)(iVar2 + 0x646c) < '\x01') {
      func_0x0000ffff();
    }
    else {
      func_0x0000ffff();
    }
  }
  *(undefined2 *)(unaff_DI + 0x18) = 0xe44b;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar2 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  *(long *)(unaff_DI + 0xe) = (long)(int)*(char *)(iVar2 + 0x646c) * 0x80 + 0x18000;
  return;
}



/* TARGET w5_effect_constructor_e087 at 0xE087; resolved function entry 0000:e087 */
/* CALLERS of 0000:e087: */
/*   <none resolved> */

void w5_effect_constructor_e087(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000def2();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe1e0;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + 0x4000;
  *(undefined4 *)(unaff_DI + 10) = 0xffff9000;
  return;
}



/* TARGET w5_effect_constructor_e0be at 0xE0BE; resolved function entry 0000:e0be */
/* CALLERS of 0000:e0be: */
/*   <none resolved> */

void w5_effect_constructor_e0be(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000def2();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe0f5;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x3000;
  *(undefined4 *)(unaff_DI + 10) = 0xffffd000;
  return;
}



/* TARGET map_cell_descriptor_5d38 at 0x5D38; resolved function entry 0000:5d38 */
/* CALLERS of 0000:5d38: */
/*   <none resolved> */

void map_cell_descriptor_5d38(void)

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



/* TARGET map_cell_state_decay_5d60 at 0x5D60; resolved function entry 0000:5d60 */
/* CALLERS of 0000:5d60: */
/*   <none resolved> */

void map_cell_state_decay_5d60(void)

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



/* TARGET boss_random_byte_5c11 at 0x5C11; resolved function entry 0000:5c11 */
/* CALLERS of 0000:5c11: */
/*   <none resolved> */

int boss_random_byte_5c11(void)

{
  int iVar1;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  return (int)*(char *)(iVar1 + 0x646c);
}



/* TARGET map_tile_descriptor_query_5c27 at 0x5C27; resolved function entry 0000:5c27 */
/* CALLERS of 0000:5c27: */
/*   <none resolved> */

void map_tile_descriptor_query_5c27(void)

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



/* TARGET boss_map_helper_1bd1 at 0x1BD1; resolved function entry 0000:1bd1 */
/* CALLERS of 0000:1bd1: */
/*   <none resolved> */

void boss_map_helper_1bd1(void)

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



/* TARGET w1_post_boss_child_callback_4c74 at 0x4C74; resolved function entry 0000:4c74 */
/* CALLERS of 0000:4c74: */
/*   <none resolved> */

void w1_post_boss_child_callback_4c74(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
  if (0x1e < *(int *)(unaff_DI + 0x2a)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  return;
}



/* TARGET w3_phase_actor_callback_cb11 at 0xCB11; resolved function entry 0000:cb11 */
/* CALLERS of 0000:cb11: */
/*   <none resolved> */

void w3_phase_actor_callback_cb11(void)

{
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint uVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x40U < 0x1c1) &&
     ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x40U < 0x131)) {
    bVar3 = false;
  }
  else {
    bVar3 = true;
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  if (!bVar3) {
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto LAB_0000_cba5;
      func_0x0000ffff(0);
      uVar1 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto LAB_0000_cba5;
      func_0x0000ffff(0);
      uVar1 = extraout_DX_02;
    }
    if ((uVar1 & 0x70) == 0) {
      lVar2 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
      lVar2 = lVar2 + 6000;
      if (lVar2 < -0x30000) {
        lVar2 = -0x30000;
      }
      else if (0x35000 < lVar2) {
        lVar2 = 0x35000;
      }
      *(long *)(unaff_DI + 0xe) = lVar2;
      lVar2 = *(long *)(unaff_DI + 10);
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar2;
      lVar2 = lVar2 + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
      if (lVar2 < -0x16000) {
        lVar2 = -0x16000;
      }
      else if (0x16000 < lVar2) {
        lVar2 = 0x16000;
      }
      *(long *)(unaff_DI + 10) = lVar2;
      func_0x0000ffff(0);
      return;
    }
  }
LAB_0000_cba5:
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  return;
}



/* TARGET w5_phase_actor_callback_dd22 at 0xDD22; resolved function entry 0000:dd22 */
/* CALLERS of 0000:dd22: */
/*   <none resolved> */

void w5_phase_actor_callback_dd22(void)

{
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint uVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar3;
  
  if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x40U < 0x1c1) &&
     ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x40U < 0x131)) {
    bVar3 = false;
  }
  else {
    bVar3 = true;
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  if (!bVar3) {
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto LAB_0000_ddb6;
      func_0x0000ffff(0);
      uVar1 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto LAB_0000_ddb6;
      func_0x0000ffff(0);
      uVar1 = extraout_DX_02;
    }
    if ((uVar1 & 0x70) == 0) {
      lVar2 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
      lVar2 = lVar2 + 6000;
      if (lVar2 < -0x30000) {
        lVar2 = -0x30000;
      }
      else if (0x35000 < lVar2) {
        lVar2 = 0x35000;
      }
      *(long *)(unaff_DI + 0xe) = lVar2;
      lVar2 = *(long *)(unaff_DI + 10);
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar2;
      lVar2 = lVar2 + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
      if (lVar2 < -0x16000) {
        lVar2 = -0x16000;
      }
      else if (0x16000 < lVar2) {
        lVar2 = 0x16000;
      }
      *(long *)(unaff_DI + 10) = lVar2;
      func_0x0000ffff(0);
      return;
    }
  }
LAB_0000_ddb6:
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  return;
}



/* TARGET w5_effect_callback_e44b at 0xE44B; resolved function entry 0000:e44b */
/* CALLERS of 0000:e44b: */
/*   <none resolved> */

void w5_effect_callback_e44b(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((0x180 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x20U) ||
     (0xf0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x20U)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
  if (*(char *)0x88ae != '\x06') {
    func_0x0000ffff();
    return;
  }
  return;
}



/* TARGET w5_effect_callback_e1e0 at 0xE1E0; resolved function entry 0000:e1e0 */
/* CALLERS of 0000:e1e0: */
/*   <none resolved> */

void w5_effect_callback_e1e0(void)

{
  undefined2 uVar1;
  uint uVar2;
  int iVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    func_0x0000def2();
    iVar3 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar3 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      func_0x0000def2();
      iVar3 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar3 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      func_0x0000def2();
      goto LAB_0000_e290;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    uVar1 = func_0x0000def2();
    *(undefined2 *)(unaff_DI + 8) = uVar1;
  }
LAB_0000_e290:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar2 = *(int *)(unaff_DI + 0x2e) + 0xfU & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar2;
  iVar3 = (int)(*(char *)(uVar2 + 0x7974) >> 4);
  *(int *)(unaff_DI + 0x2c) = iVar3;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar3;
  func_0x0000ffff();
  return;
}



/* TARGET w5_random_helper_def2 at 0xDEF2; resolved function entry 0000:def2 */
/* CALLERS of 0000:def2: */
/*   0000:dfb3 UNCONDITIONAL_CALL in 0000:dfb3 */
/*   0000:e01d UNCONDITIONAL_CALL in 0000:e01d */
/*   0000:e087 UNCONDITIONAL_CALL in 0000:e087 */
/*   0000:e0be UNCONDITIONAL_CALL in 0000:e0be */
/*   0000:e204 UNCONDITIONAL_CALL in <unknown> */
/*   0000:e23f UNCONDITIONAL_CALL in <unknown> */
/*   0000:e275 UNCONDITIONAL_CALL in <unknown> */
/*   0000:e289 UNCONDITIONAL_CALL in <unknown> */

void w5_random_helper_def2(void)

{
  int iVar1;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)0x6468;
  *(int *)0x6468 = *(int *)0x6468 + 1;
  *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
  iVar1 = *(char *)(iVar1 + 0x646c) + 0x82;
  if (iVar1 < 0x2b) {
    func_0x0000ffff();
  }
  else if (iVar1 < 0x55) {
    func_0x0000ffff();
  }
  else if (iVar1 < 0x7f) {
    func_0x0000ffff();
  }
  else if (iVar1 < 0xa9) {
    func_0x0000ffff();
  }
  else if (iVar1 < 0xd3) {
    func_0x0000ffff();
  }
  else {
    func_0x0000ffff();
  }
  return;
}



/* TARGET object_pool_count_active at 0x0E66; resolved function entry 0000:0e66 */
/* CALLERS of 0000:0e66: */
/*   <none resolved> */

void object_pool_count_active(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x88c8 = 0;
  iVar2 = 0x40;
  uVar1 = *(undefined4 *)0x755e;
  iVar3 = (int)uVar1;
  do {
    if (*(int *)(iVar3 + 0x18) != 0) {
      *(int *)0x88c8 = *(int *)0x88c8 + 1;
    }
    iVar3 = iVar3 + *(int *)0x30ce;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}



/* TARGET object_update_pass_by_phase at 0x0E96; resolved function entry 0000:0e96 */
/* CALLERS of 0000:0e96: */
/*   <none resolved> */

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



/* TARGET object_update_pass_nonzero_state at 0x0FA2; resolved function entry 0000:0fa2 */
/* CALLERS of 0000:0fa2: */
/*   <none resolved> */

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



/* TARGET create_dedicated_are_effect at 0x1749; resolved function entry 0000:1749 */
/* CALLERS of 0000:1749: */
/*   <none resolved> */

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



/* TARGET create_are_type_65 at 0x178D; resolved function entry 0000:178d */
/* CALLERS of 0000:178d: */
/*   <none resolved> */

void create_are_type_65(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 0;
  func_0x0000ffff();
  return;
}



/* TARGET create_are_type_66 at 0x1798; resolved function entry 0000:1798 */
/* CALLERS of 0000:1798: */
/*   <none resolved> */

void create_are_type_66(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 8;
  func_0x0000ffff();
  return;
}



/* TARGET create_are_type_67 at 0x17A3; resolved function entry 0000:17a3 */
/* CALLERS of 0000:17a3: */
/*   <none resolved> */

void create_are_type_67(void)

{
  undefined2 unaff_DS;
  
  *(undefined1 *)0x36ee = 0x10;
  func_0x0000ffff();
  return;
}



/* TARGET instantiate_are_declaration at 0x1E04; resolved function entry 0000:1e04 */
/* CALLERS of 0000:1e04: */
/*   0000:1ec0 UNCONDITIONAL_JUMP in <unknown> */

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



/* TARGET player_transition_entry_a at 0x199D; resolved function entry 0000:199d */
/* CALLERS of 0000:199d: */
/*   <none resolved> */

void player_transition_entry_a(void)

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



/* TARGET player_transition_entry_b at 0x19E6; resolved function entry 0000:19e6 */
/* CALLERS of 0000:19e6: */
/*   <none resolved> */

void player_transition_entry_b(void)

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



/* TARGET player_transition_state_setup at 0x1A97; resolved function entry 0000:1a97 */
/* CALLERS of 0000:1a97: */
/*   <none resolved> */

void player_transition_state_setup(void)

{
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(*(int *)0x881a + 0x34) = 700;
  *(undefined2 *)0x8810 = 0xffff;
  return;
}



/* TARGET player_reentry_setup at 0x1AAA; resolved function entry 0000:1aaa */
/* CALLERS of 0000:1aaa: */
/*   <none resolved> */

void player_reentry_setup(void)

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
  *(undefined2 *)(iVar2 + 0x18) = 0x3f27;
  *(undefined1 *)(iVar2 + 0x29) = 1;
  *(undefined2 *)0x89ea = 0;
  func_0x0000ffff(0);
  return;
}



/* TARGET clear_player_transition_word at 0x1AE6; resolved function entry 0000:1ae6 */
/* CALLERS of 0000:1ae6: */
/*   <none resolved> */

void clear_player_transition_word(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ea = 0;
  func_0x0000ffff();
  return;
}



/* TARGET update_player_object at 0x3FF8; resolved function entry 0000:3ff8 */
/* CALLERS of 0000:3ff8: */
/*   <none resolved> */

void update_player_object(void)

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
      if ((extraout_DX & 0x70) != 0) goto LAB_0000_44dc;
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_01 & 0x70) != 0) goto LAB_0000_44dc;
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
LAB_0000_44dc:
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
        func_0x00003d02();
        func_0x00003df2();
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
      func_0x00003a1f();
      if (((bVar5) && ((char)unaff_DI[0x1c] == '\0')) && ((char)unaff_DI[0x1d] == '\0')) {
        unaff_DI[0x1f] = 0;
        goto LAB_0000_41cf;
      }
      func_0x00003df2();
      func_0x00003d02();
      if (((*unaff_DI & 0x22) == 0) ||
         ((bVar5 = false, (char)unaff_DI[0x1c] == '\0' &&
          (bVar5 = unaff_DI[0x20] == 0xd, 0xd < (int)unaff_DI[0x20])))) goto LAB_0000_4384;
    }
    else {
      if (*(char *)((int)unaff_DI + 0x37) < '\0') {
        func_0x00003986();
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
            func_0x00003986();
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
        if (((char)unaff_DI[0x1c] == '\0') && (func_0x00003d02(), (char)unaff_DI[0x1d] == '\0')) {
          lVar4 = *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28);
          if (*(long *)(unaff_DI + 0x30) <= *(long *)(unaff_DI + 7) + *(long *)(unaff_DI + 0x28)) {
            lVar4 = *(long *)(unaff_DI + 0x30);
          }
          *(long *)(unaff_DI + 7) = lVar4;
          puVar1 = unaff_DI + 3;
          *(long *)puVar1 = *(long *)puVar1 + *(long *)(unaff_DI + 7);
          bVar5 = *(long *)puVar1 == 0;
          func_0x00003a1f();
          if (bVar5) goto LAB_0000_4384;
        }
        goto LAB_0000_427f;
      }
    }
    func_0x00003971();
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
  func_0x000038ca();
  func_0x000038ec();
  func_0x00003ab9();
  func_0x0000ffff();
  func_0x00003a62();
  func_0x00003e41();
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



/* TARGET player_collision_transition_caller at 0x3A8A; resolved function entry 0000:3a8a */
/* CALLERS of 0000:3a8a: */
/*   <none resolved> */

void player_collision_transition_caller(void)

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



/* TARGET collision_transition_call_site at 0x3AB3; resolved function entry 0000:3ab3 */
/* CALLERS of 0000:3ab3: */
/*   <none resolved> */

void collision_transition_call_site(void)

{
  func_0x0000ffff();
  return;
}



/* TARGET object_overlap_transition_call_site at 0x1BC4; resolved function entry 0000:1bc4 */
/* CALLERS of 0000:1bc4: */
/*   <none resolved> */

undefined2 object_overlap_transition_call_site(void)

{
  func_0x0000ffff();
  return 2;
}



/* TARGET camera_boundary_transition_call_site at 0x43D0; resolved function entry 0000:43d0 */
/* CALLERS of 0000:43d0: */
/*   <none resolved> */

void camera_boundary_transition_call_site(void)

{
  int *unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*unaff_DI == 0)) {
    *(int *)0x4fee = *(int *)0x4fee + 1;
    if (*(int *)0x4fee == 0xd2) {
      func_0x0000ffff(0);
    }
  }
  else {
    *(undefined2 *)0x4fee = 0;
  }
  if ((*(char *)((int)unaff_DI + 0x37) == '\0') && (*(int *)0x89e6 == -1)) {
    func_0x0000ffff(0);
  }
  return;
}



/* TARGET player_transition_branch at 0x4416; resolved function entry 0000:4416 */
/* CALLERS of 0000:4416: */
/*   0000:4002 CONDITIONAL_JUMP in <unknown> */

void player_transition_branch(void)

{
  undefined2 uVar1;
  undefined2 extraout_var;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint uVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar3;
  
  if (*(int *)0x89ea == -1) {
    *(undefined4 *)(unaff_DI + 0xe) = 0xfffe0000;
    *(undefined2 *)0x8822 = 0;
    unaff_CS = 0;
    func_0x0000ffff();
  }
  func_0x0000ffff(unaff_CS);
  func_0x0000ffff(0);
  *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + 0x1800;
  if (0x1ffff < *(long *)(unaff_DI + 0xe)) {
    *(undefined4 *)(unaff_DI + 0xe) = 0x20000;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    func_0x0000ffff(0);
    if ((extraout_DX & 0x70) != 0) goto LAB_0000_44dc;
    func_0x0000ffff(0);
    uVar2 = extraout_DX_00;
  }
  else {
    func_0x0000ffff(0);
    if ((extraout_DX_01 & 0x70) != 0) goto LAB_0000_44dc;
    func_0x0000ffff(0);
    uVar2 = extraout_DX_02;
  }
  if ((uVar2 & 0x70) == 0) {
    bVar3 = false;
    uVar1 = func_0x0000ffff(0);
    if (!bVar3) {
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + CONCAT22(extraout_var,uVar1);
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x5000;
    }
  }
LAB_0000_44dc:
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



/* TARGET player_transition_countdown at 0x44DC; resolved function entry 0000:44dc */
/* CALLERS of 0000:44dc: */
/*   0000:44bd UNCONDITIONAL_JUMP in <unknown> */
/*   0000:44cc UNCONDITIONAL_JUMP in <unknown> */

void player_transition_countdown(void)

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



/* TARGET w1_post_boss_child_callback at 0x4A5E; resolved function entry 0000:4a5e */
/* CALLERS of 0000:4a5e: */
/*   <none resolved> */

void w1_post_boss_child_callback(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(int *)0x85d4 == 0xe) {
    if (*(char *)0x85da == '2') {
      *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
      if (*(int *)(unaff_DI + 0x2c) < 0xfb) {
        return;
      }
      if (*(char *)0x85da < '3') {
        *(undefined2 *)(unaff_DI + 0x2c) = 0;
        func_0x0000ffff();
        *(undefined2 *)0x89ec = 0xffff;
        return;
      }
    }
    *(undefined1 *)0x85da = 0;
  }
  *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
  if (600 < *(int *)(unaff_DI + 0x2c)) {
    *(undefined2 *)0x89e6 = 0xffff;
  }
  return;
}



/* TARGET w1_post_boss_animation_advance at 0x49F2; resolved function entry 0000:49f2 */
/* CALLERS of 0000:49f2: */
/*   <none resolved> */

void w1_post_boss_animation_advance(void)

{
  func_0x0000ffff();
  return;
}



/* TARGET shared_projectile_callback at 0x45AB; resolved function entry 0000:45ab */
/* CALLERS of 0000:45ab: */
/*   <none resolved> */

void shared_projectile_callback(void)

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
    *(undefined2 *)(unaff_DI + 0x18) = 0x470c;
  }
  if ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U) ||
     (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0x470c;
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



/* TARGET shared_post_boss_contact_constructor at 0x487F; resolved function entry 0000:487f */
/* CALLERS of 0000:487f: */
/*   <none resolved> */

void shared_post_boss_contact_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x489c;
  *(undefined1 *)(unaff_DI + 0x2a) = 0;
  *(undefined4 *)(unaff_DI + 0xe) = 0x11000;
  return;
}



/* TARGET shared_post_boss_contact_callback at 0x489C; resolved function entry 0000:489c */
/* CALLERS of 0000:489c: */
/*   <none resolved> */

void shared_post_boss_contact_callback(void)

{
  ulong *puVar1;
  uint uVar2;
  ulong uVar3;
  int in_CX;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  int in_BX;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  bool bVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  
  if ('\0' < *(char *)(unaff_DI + 0x2a)) {
    if ('\x01' < *(char *)(unaff_DI + 0x2a)) {
      *(undefined2 *)(unaff_DI + 0x18) = 0;
      return;
    }
    uVar5 = func_0x0000ffff();
    if ((in_CX <= *(int *)(unaff_DI + 4)) || (*(int *)(unaff_DI + 4) + 8 <= (int)uVar5)) {
      func_0x0000ffff(0);
      return;
    }
    uVar2 = *(int *)(unaff_DI + 8) - 0x23U & 0xfff0;
    if (((int)uVar2 < (int)((ulong)uVar5 >> 0x10)) && (in_BX < (int)(uVar2 + 0x23))) {
      *(undefined2 *)0x612e = 0xc;
      func_0x0000ffff(0);
      *(long *)0x881c = *(long *)0x881c + 5000;
      if ((*(int *)0x85d8 == 1) || ((*(int *)0x85d8 == 3 || (*(int *)0x85d8 == 5)))) {
        iVar6 = unaff_DI;
        func_0x0000ffff(0);
        *(undefined1 *)(unaff_DI + 0x17) = 1;
        *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(iVar6 + 4);
        *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar6 + 8);
        iVar7 = iVar6;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar6 + 0x17) = 2;
        *(undefined2 *)(iVar6 + 4) = *(undefined2 *)(iVar7 + 4);
        *(undefined2 *)(iVar6 + 8) = *(undefined2 *)(iVar7 + 8);
        *(undefined1 *)(iVar7 + 0x2a) = 2;
      }
      else {
        *(undefined1 *)(unaff_DI + 0x2a) = 2;
        *(undefined2 *)0x89e6 = 0xffff;
      }
    }
    goto w1_post_boss_animation_advance;
  }
  uVar3 = *(ulong *)(unaff_DI + 0xe);
  if (0x3000 < (long)uVar3) {
    uVar3 = uVar3 - 300;
  }
  puVar1 = (ulong *)(unaff_DI + 6);
  bVar4 = CARRY4(*puVar1,uVar3);
  *puVar1 = *puVar1 + uVar3;
  *(ulong *)(unaff_DI + 0xe) = uVar3;
  func_0x0000ffff();
  if (bVar4) {
    *(undefined1 *)(unaff_DI + 0x2a) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    func_0x0000ffff(0);
    if ((extraout_DX & 0x70) == 0) {
      func_0x0000ffff(0);
      uVar2 = extraout_DX_00;
joined_r0x00004921:
      if ((uVar2 & 0x70) == 0) goto w1_post_boss_animation_advance;
    }
  }
  else {
    func_0x0000ffff(0);
    if ((extraout_DX_01 & 0x70) == 0) {
      func_0x0000ffff(0);
      uVar2 = extraout_DX_02;
      goto joined_r0x00004921;
    }
  }
  *(undefined1 *)(unaff_DI + 0x2a) = 1;
w1_post_boss_animation_advance:
  func_0x0000ffff(0);
  return;
}



/* TARGET w1_transition_aux_constructor at 0x49FF; resolved function entry 0000:49ff */
/* CALLERS of 0000:49ff: */
/*   <none resolved> */

void w1_transition_aux_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x3a2;
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w1_post_boss_child_callback;
  if (*(int *)0x85d4 == 0xe) {
    *(undefined1 *)0x85da = 0x32;
  }
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(unaff_DI + 8);
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(unaff_DI + 4);
  *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(unaff_DI + 8);
  return;
}



/* TARGET transition_aux_constructor_common_a at 0x92B3; resolved function entry 0000:92b3 */
/* CALLERS of 0000:92b3: */
/*   <none resolved> */

void transition_aux_constructor_common_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x9313;
  *(undefined2 *)(unaff_DI + 0x2a) = 5;
  *(undefined2 *)(unaff_DI + 0x35) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffee000;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* TARGET transition_aux_constructor_a at 0x92F2; resolved function entry 0000:92f2 */
/* CALLERS of 0000:92f2: */
/*   <none resolved> */

void transition_aux_constructor_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  transition_aux_constructor_common_a();
  return;
}



/* TARGET transition_aux_constructor_common_b at 0x95C7; resolved function entry 0000:95c7 */
/* CALLERS of 0000:95c7: */
/*   <none resolved> */

void transition_aux_constructor_common_b(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x9627;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x35) = 0;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffff8000;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* TARGET transition_aux_constructor_b at 0x9614; resolved function entry 0000:9614 */
/* CALLERS of 0000:9614: */
/*   <none resolved> */

void transition_aux_constructor_b(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  transition_aux_constructor_common_b();
  *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
  return;
}



/* TARGET transition_aux_constructor_c at 0x991A; resolved function entry 0000:991a */
/* CALLERS of 0000:991a: */
/*   <none resolved> */

void transition_aux_constructor_c(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  func_0x000098db();
  return;
}



/* TARGET transition_aux_constructor_common_c at 0x98DB; resolved function entry 0000:98db */
/* CALLERS of 0000:98db: */
/*   0000:9924 UNCONDITIONAL_CALL in <unknown> */

void transition_aux_constructor_common_c(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0x993b;
  *(undefined2 *)(unaff_DI + 0x2a) = 10;
  *(undefined2 *)(unaff_DI + 0x35) = 0x1e;
  *(undefined2 *)(unaff_DI + 0x33) = 0;
  *(undefined1 *)(unaff_DI + 0x32) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffff0000;
  *(undefined1 *)(unaff_DI + 0x2c) = 0xff;
  *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
  *(undefined1 *)(unaff_DI + 0x2f) = 0xff;
  return;
}



/* TARGET transition_collision_helper_1c4d at 0x1C4D; resolved function entry 0000:1c4d */
/* CALLERS of 0000:1c4d: */
/*   <none resolved> */

void transition_collision_helper_1c4d(void)

{
  func_0x0000ffff();
  return;
}



/* TARGET w1_post_boss_contact_followup at 0x9627; resolved function entry 0000:9627 */
/* CALLERS of 0000:9627: */
/*   <none resolved> */

void w1_post_boss_contact_followup(void)

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
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x26;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_9668:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x26;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_9668;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x35) = *(int *)(unaff_DI + 0x35) + 1;
      if (*(int *)(unaff_DI + 0x35) < 0xa1) {
        *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
        if (100 < *(int *)(unaff_DI + 0x2a)) {
          iVar2 = *(int *)0x6468;
          *(int *)0x6468 = *(int *)0x6468 + 1;
          *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
          *(int *)(unaff_DI + 0x2a) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
          *(undefined1 *)(unaff_DI + 0x32) = 1;
        }
      }
      else {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(int *)(unaff_DI + 0x35) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar3 < -0x8000) {
        lVar3 = -0x8000;
      }
      else if (0x8000 < lVar3) {
        lVar3 = 0x8000;
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
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x8000) {
        lVar3 = -0x8000;
      }
      else if (0x8000 < lVar3) {
        lVar3 = 0x8000;
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
      if (*(int *)(unaff_DI + 0x33) < 0x4c) goto LAB_0000_98d5;
      *(undefined2 *)(unaff_DI + 0x33) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      func_0x0000ffff(0);
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar3 < -0x8000) {
        lVar3 = -0x8000;
      }
      else if (0x8000 < lVar3) {
        lVar3 = 0x8000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_98d5;
      }
      else if (0 < lVar3) goto LAB_0000_98d5;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_98d5;
    }
    lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
    if (lVar3 < -0x8000) {
      lVar3 = -0x8000;
    }
    else if (0x8000 < lVar3) {
      lVar3 = 0x8000;
    }
    *(long *)(unaff_DI + 10) = lVar3;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      if (-0x8000 < lVar3) goto LAB_0000_98d5;
    }
    else if (lVar3 < 0x8000) goto LAB_0000_98d5;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x3c;
  }
LAB_0000_98d5:
  func_0x0000ffff(0);
  return;
}



/* TARGET w1_transition_child_callback_a at 0x9313; resolved function entry 0000:9313 */
/* CALLERS of 0000:9313: */
/*   <none resolved> */

void w1_transition_child_callback_a(void)

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
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x26;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_9354:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x26;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_9354;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x35) = *(int *)(unaff_DI + 0x35) + 1;
      if (*(int *)(unaff_DI + 0x35) < 0x83) {
        *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
        if (0x50 < *(int *)(unaff_DI + 0x2a)) {
          iVar2 = *(int *)0x6468;
          *(int *)0x6468 = *(int *)0x6468 + 1;
          *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
          *(int *)(unaff_DI + 0x2a) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
          *(undefined1 *)(unaff_DI + 0x32) = 1;
        }
      }
      else {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(int *)(unaff_DI + 0x35) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
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
        *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
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
      if (*(int *)(unaff_DI + 0x33) < 0x4c) goto LAB_0000_95c1;
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
        if (lVar3 < 0) goto LAB_0000_95c1;
      }
      else if (0 < lVar3) goto LAB_0000_95c1;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_95c1;
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
      if (-0x12000 < lVar3) goto LAB_0000_95c1;
    }
    else if (lVar3 < 0x12000) goto LAB_0000_95c1;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x32;
  }
LAB_0000_95c1:
  func_0x0000ffff(0);
  return;
}



/* TARGET w1_transition_child_callback_b at 0x993B; resolved function entry 0000:993b */
/* CALLERS of 0000:993b: */
/*   <none resolved> */

void w1_transition_child_callback_b(void)

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
    *(undefined1 *)(unaff_DI + 0x2f) = 1;
  }
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar4 = *(int *)(unaff_DI + 4) == 0x26;
    func_0x0000ffff(0);
    if (bVar4) {
LAB_0000_997c:
      *(undefined1 *)(unaff_DI + 0x2f) = 1;
    }
  }
  else {
    bVar4 = *(int *)(unaff_DI + 4) == -0x26;
    func_0x0000ffff(0);
    if (bVar4) goto LAB_0000_997c;
  }
  if (*(char *)(unaff_DI + 0x32) < '\x01') {
    if (*(char *)(unaff_DI + 0x2f) < '\x01') {
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
      *(int *)(unaff_DI + 0x35) = *(int *)(unaff_DI + 0x35) + 1;
      if (*(int *)(unaff_DI + 0x35) < 0x79) {
        *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
        if (0x46 < *(int *)(unaff_DI + 0x2a)) {
          iVar2 = *(int *)0x6468;
          *(int *)0x6468 = *(int *)0x6468 + 1;
          *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
          *(int *)(unaff_DI + 0x2a) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
          *(undefined1 *)(unaff_DI + 0x32) = 1;
        }
      }
      else {
        iVar2 = *(int *)0x6468;
        *(int *)0x6468 = *(int *)0x6468 + 1;
        *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
        *(int *)(unaff_DI + 0x35) = (int)(char)(*(byte *)(iVar2 + 0x646c) >> 2);
        *(undefined1 *)(unaff_DI + 0x2f) = 1;
      }
    }
    else if (*(char *)(unaff_DI + 0x2c) < '\0') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
      if (lVar3 < -0x10000) {
        lVar3 = -0x10000;
      }
      else if (0x10000 < lVar3) {
        lVar3 = 0x10000;
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
        *(undefined2 *)(unaff_DI + 0x2d) = 0x14;
      }
    }
    else {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
      if (lVar3 < -0x10000) {
        lVar3 = -0x10000;
      }
      else if (0x10000 < lVar3) {
        lVar3 = 0x10000;
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
      if (*(int *)(unaff_DI + 0x33) < 0x4c) goto LAB_0000_9be8;
      *(undefined2 *)(unaff_DI + 0x33) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 3;
      func_0x0000ffff(0);
    }
    else if (*(char *)(unaff_DI + 0x32) != '\x03') {
      lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x800;
      if (lVar3 < -0x10000) {
        lVar3 = -0x10000;
      }
      else if (0x10000 < lVar3) {
        lVar3 = 0x10000;
      }
      *(long *)(unaff_DI + 10) = lVar3;
      *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        if (lVar3 < 0) goto LAB_0000_9be8;
      }
      else if (0 < lVar3) goto LAB_0000_9be8;
      *(undefined4 *)(unaff_DI + 10) = 0;
      *(undefined1 *)(unaff_DI + 0x32) = 2;
      func_0x0000ffff(0);
      goto LAB_0000_9be8;
    }
    lVar3 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x800;
    if (lVar3 < -0x10000) {
      lVar3 = -0x10000;
    }
    else if (0x10000 < lVar3) {
      lVar3 = 0x10000;
    }
    *(long *)(unaff_DI + 10) = lVar3;
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar3;
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      if (-0x10000 < lVar3) goto LAB_0000_9be8;
    }
    else if (lVar3 < 0x10000) goto LAB_0000_9be8;
    *(undefined1 *)(unaff_DI + 0x32) = 0;
  }
  else {
    *(undefined1 *)(unaff_DI + 0x32) = 0;
    *(undefined2 *)(unaff_DI + 0x2a) = 0x32;
  }
LAB_0000_9be8:
  func_0x0000ffff(0);
  return;
}



/* TARGET w1_short_lived_child_callback_a at 0xB84D; resolved function entry 0000:b84d */
/* CALLERS of 0000:b84d: */
/*   <none resolved> */

void w1_short_lived_child_callback_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x386;
  *(undefined2 *)(unaff_DI + 0x18) = 0xb87b;
  *(undefined2 *)(unaff_DI + 0x3a) = 0;
  *(undefined4 *)(unaff_DI + 0x3c) = 0x30000;
  *(undefined4 *)(unaff_DI + 10) = 0;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffeb000;
  return;
}



/* TARGET w1_short_lived_child_callback_b at 0xB87B; resolved function entry 0000:b87b */
/* CALLERS of 0000:b87b: */
/*   <none resolved> */

void w1_short_lived_child_callback_b(void)

{
  ulong *puVar1;
  long lVar2;
  uint extraout_DX;
  uint extraout_DX_00;
  uint extraout_DX_01;
  uint extraout_DX_02;
  uint uVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 uVar4;
  undefined2 unaff_DS;
  bool bVar5;
  
  if ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U) ||
     (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  bVar5 = false;
  if (0 < *(long *)(unaff_DI + 0xe)) {
    bVar5 = *(int *)(unaff_DI + 0x3a) == 0;
    uVar4 = unaff_CS;
    if (*(int *)(unaff_DI + 0x3a) == 1) goto LAB_0000_b943;
    func_0x0000ffff();
    if (!bVar5) {
      if (*(char *)(unaff_DI + 0x29) < '\x01') {
        func_0x0000ffff(0);
        if ((extraout_DX & 0x70) == 0) {
          func_0x0000ffff(0);
          uVar3 = extraout_DX_00;
joined_r0x0000b914:
          if ((uVar3 & 0x70) == 0) {
            bVar5 = false;
            unaff_CS = 0;
            goto LAB_0000_b932;
          }
        }
      }
      else {
        func_0x0000ffff(0);
        if ((extraout_DX_01 & 0x70) == 0) {
          func_0x0000ffff(0);
          uVar3 = extraout_DX_02;
          goto joined_r0x0000b914;
        }
      }
      *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
    }
    unaff_CS = 0;
    *(long *)(unaff_DI + 0xe) = -*(long *)(unaff_DI + 0xe);
    *(int *)(unaff_DI + 0x3a) = *(int *)(unaff_DI + 0x3a) + 1;
    puVar1 = (ulong *)(unaff_DI + 0x3c);
    bVar5 = *puVar1 < 0x5000;
    *puVar1 = *puVar1 - 0x5000;
  }
LAB_0000_b932:
  uVar4 = 0;
  func_0x0000ffff(unaff_CS);
  if (bVar5) {
    *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
  }
LAB_0000_b943:
  lVar2 = *(long *)(unaff_DI + 0xe);
  if (lVar2 < 1) {
    lVar2 = lVar2 + 15000;
    if (lVar2 < -0x30000) {
      lVar2 = -0x30000;
    }
    else if (*(long *)(unaff_DI + 0x3c) < lVar2) {
      lVar2 = *(long *)(unaff_DI + 0x3c);
    }
  }
  else {
    lVar2 = lVar2 + 17000;
    if (lVar2 < -0x30000) {
      lVar2 = -0x30000;
    }
    else if (*(long *)(unaff_DI + 0x3c) < lVar2) {
      lVar2 = *(long *)(unaff_DI + 0x3c);
    }
  }
  *(long *)(unaff_DI + 0xe) = lVar2;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
  func_0x0000ffff(uVar4);
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + *(char *)(unaff_DI + 0x29) * 4;
  return;
}



/* TARGET w1_main_boss_constructor at 0xB142; resolved function entry 0000:b142 */
/* CALLERS of 0000:b142: */
/*   <none resolved> */

void w1_main_boss_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined2 *)(unaff_DI + 0x18) = 0xb33b;
  *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined2 *)(unaff_DI + 0x38) = 0;
  *(undefined2 *)(unaff_DI + 0x44) = 0;
  *(undefined1 *)(unaff_DI + 0x34) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffff9000;
  *(undefined1 *)(unaff_DI + 0x40) = 0xff;
  *(undefined2 *)(unaff_DI + 0x42) = 0x14;
  *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x60000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x1f0000;
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 6);
  return;
}



/* TARGET w1_helper_constructor at 0xB1F0; resolved function entry 0000:b1f0 */
/* CALLERS of 0000:b1f0: */
/*   <none resolved> */

void w1_helper_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xb226;
  *(undefined2 *)(unaff_DI + 0x2e) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w1_doktor_constructor at 0xB20B; resolved function entry 0000:b20b */
/* CALLERS of 0000:b20b: */
/*   <none resolved> */

void w1_doktor_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xb25d;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET reset_projectile_table at 0x44FF; resolved function entry 0000:44ff */
/* CALLERS of 0000:44ff: */
/*   <none resolved> */

void reset_projectile_table(void)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x8808 = 4;
  *(undefined2 *)0x8806 = 0;
  puVar3 = (undefined4 *)0x87de;
  for (iVar2 = 10; iVar2 != 0; iVar2 = iVar2 + -1) {
    puVar1 = puVar3;
    puVar3 = puVar3 + 1;
    *puVar1 = 0;
  }
  return;
}



/* TARGET w1_cutscene_completion_write at 0x44F8; resolved function entry 0000:44f8 */
/* CALLERS of 0000:44f8: */
/*   <none resolved> */

void w1_cutscene_completion_write(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  return;
}



/* TARGET w1_transition_completion_write at 0x4996; resolved function entry 0000:4996 */
/* CALLERS of 0000:4996: */
/*   <none resolved> */

void w1_transition_completion_write(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89e6 = 0xffff;
  func_0x0000ffff();
  return;
}



/* TARGET w1_child_completion_write at 0x4A93; resolved function entry 0000:4a93 */
/* CALLERS of 0000:4a93: */
/*   <none resolved> */

void w1_child_completion_write(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  return;
}



/* TARGET w1_child_transition_write at 0x4AAC; resolved function entry 0000:4aac */
/* CALLERS of 0000:4aac: */
/*   <none resolved> */

void w1_child_transition_write(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89e6 = 0xffff;
  return;
}



/* TARGET w1_effect_transition_write at 0x92A9; resolved function entry 0000:92a9 */
/* CALLERS of 0000:92a9: */
/*   <none resolved> */

void w1_effect_transition_write(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89e6 = 0xffff;
  return;
}



/* TARGET w1in_completion_write_a at 0xA2A5; resolved function entry 0000:a2a5 */
/* CALLERS of 0000:a2a5: */
/*   <none resolved> */

void w1in_completion_write_a(void)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  long lVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  iVar2 = *(int *)(unaff_DI + 0x2a);
  if (*(char *)(unaff_DI + 0x28) == '\x01') {
    lVar4 = *(long *)(unaff_DI + 2) + 0x60000;
  }
  else {
    lVar4 = *(long *)(unaff_DI + 2) + -0x60000;
  }
  *(long *)(iVar2 + 2) = lVar4;
  *(long *)(iVar2 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  uVar3 = *(int *)(iVar2 + 0x2e) + 10U & 0x6ff;
  *(uint *)(iVar2 + 0x2e) = uVar3;
  *(int *)(iVar2 + 8) = *(int *)(iVar2 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
  *(undefined2 *)(iVar2 + 8) = *(undefined2 *)(iVar2 + 8);
  iVar2 = *(int *)(unaff_DI + 0x36);
  lVar4 = *(long *)(unaff_DI + 2);
  cVar1 = *(char *)(unaff_DI + 0x28);
  *(char *)(iVar2 + 0x28) = cVar1;
  if (cVar1 == '\x01') {
    lVar4 = lVar4 + 0x1f0000;
  }
  else {
    lVar4 = lVar4 + -0x1f0000;
  }
  *(long *)(iVar2 + 2) = lVar4;
  *(undefined4 *)(iVar2 + 6) = *(undefined4 *)(unaff_DI + 6);
  return;
}



/* TARGET w1in_completion_write_b at 0xA2FE; resolved function entry 0000:a2fe */
/* CALLERS of 0000:a2fe: */
/*   <none resolved> */

void w1in_completion_write_b(void)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  long lVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  iVar2 = *(int *)(unaff_DI + 0x2a);
  if (*(char *)(unaff_DI + 0x28) == '\x01') {
    lVar4 = *(long *)(unaff_DI + 2) + 0x60000;
  }
  else {
    lVar4 = *(long *)(unaff_DI + 2) + -0x60000;
  }
  *(long *)(iVar2 + 2) = lVar4;
  *(long *)(iVar2 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  uVar3 = *(int *)(iVar2 + 0x2e) + 10U & 0x6ff;
  *(uint *)(iVar2 + 0x2e) = uVar3;
  *(int *)(iVar2 + 8) = *(int *)(iVar2 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
  *(undefined2 *)(iVar2 + 8) = *(undefined2 *)(iVar2 + 8);
  iVar2 = *(int *)(unaff_DI + 0x36);
  lVar4 = *(long *)(unaff_DI + 2);
  cVar1 = *(char *)(unaff_DI + 0x28);
  *(char *)(iVar2 + 0x28) = cVar1;
  if (cVar1 == '\x01') {
    lVar4 = lVar4 + 0x1f0000;
  }
  else {
    lVar4 = lVar4 + -0x1f0000;
  }
  *(long *)(iVar2 + 2) = lVar4;
  *(undefined4 *)(iVar2 + 6) = *(undefined4 *)(unaff_DI + 6);
  return;
}



/* TARGET w1in_completion_write_c at 0xA82E; resolved function entry 0000:a82e */
/* CALLERS of 0000:a82e: */
/*   <none resolved> */

void w1in_completion_write_c(void)

{
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  func_0x0000ffff();
  return;
}



/* TARGET w1in_completion_write_d at 0xA874; resolved function entry 0000:a874 */
/* CALLERS of 0000:a874: */
/*   <none resolved> */

void w1in_completion_write_d(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(undefined2 *)0x89ec = 0xffff;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + 0x15000;
  func_0x0000ffff();
  return;
}



/* TARGET program_timer_helper at 0xF111; resolved function entry 0000:f111 */
/* CALLERS of 0000:f111: */
/*   <none resolved> */

void program_timer_helper(void)

{
  code *pcVar1;
  
  out(0x43,0x34);
  out(0x40,0);
  out(0x40,0);
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  return;
}



/* TARGET initialize_are_dispatch_table at 0x0B81; resolved function entry 0000:0b81 */
/* CALLERS of 0000:0b81: */
/*   <none resolved> */

void initialize_are_dispatch_table(void)

{
  undefined2 *puVar1;
  int iVar2;
  undefined2 *puVar3;
  undefined2 unaff_DS;
  
  puVar3 = (undefined2 *)0x85dc;
  for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
    puVar1 = puVar3;
    puVar3 = puVar3 + 1;
    *puVar1 = 0;
  }
  puVar3 = (undefined2 *)0x81d2;
  iVar2 = 0x100;
  do {
    *puVar3 = 0;
    puVar3 = puVar3 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined2 *)0x81d6 = 0x6da3;
  *(undefined1 *)0x81d8 = 1;
  *(undefined2 *)0x81da = 0x6db1;
  *(undefined1 *)0x81dc = 1;
  *(undefined2 *)0x81de = 0x689f;
  *(undefined1 *)0x81e0 = 1;
  *(undefined2 *)0x81e2 = 0x68ad;
  *(undefined1 *)0x81e4 = 1;
  *(undefined2 *)0x81e6 = 0x7b50;
  *(undefined1 *)0x81e8 = 1;
  *(undefined2 *)0x81ea = 0x7b5e;
  *(undefined1 *)0x81ec = 1;
  *(undefined2 *)0x81ee = 0x776b;
  *(undefined1 *)0x81f0 = 1;
  *(undefined2 *)0x81f2 = 0x7779;
  *(undefined1 *)0x81f4 = 1;
  *(undefined2 *)0x81f6 = 0x713d;
  *(undefined1 *)0x81f8 = 1;
  *(undefined2 *)0x81fa = 0x714b;
  *(undefined1 *)0x81fc = 1;
  *(undefined2 *)0x81fe = 0x6651;
  *(undefined1 *)0x8200 = 1;
  *(undefined2 *)0x8202 = 0x6699;
  *(undefined1 *)0x8204 = 1;
  *(undefined2 *)0x8226 = 0x7ed7;
  *(undefined1 *)0x8228 = 1;
  *(undefined2 *)0x822a = 0x7ee5;
  *(undefined1 *)0x822c = 1;
  *(undefined2 *)0x822e = 0x8451;
  *(undefined1 *)0x8230 = 1;
  *(undefined2 *)0x8232 = 0x845f;
  *(undefined1 *)0x8234 = 1;
  *(undefined2 *)0x8236 = 0x5050;
  *(undefined1 *)0x8238 = 1;
  *(undefined2 *)0x823a = 0x505e;
  *(undefined1 *)0x823c = 1;
  *(undefined2 *)0x823e = 0x5f07;
  *(undefined1 *)0x8240 = 1;
  *(undefined2 *)0x8242 = 0x5f15;
  *(undefined1 *)0x8244 = 1;
  *(undefined2 *)0x824e = 0x8b3d;
  *(undefined1 *)0x8250 = 1;
  *(undefined2 *)0x8252 = 0x8b50;
  *(undefined1 *)0x8254 = 1;
  *(undefined2 *)0x8256 = 0x8b63;
  *(undefined1 *)0x8258 = 1;
  *(undefined2 *)0x825a = 0x8b76;
  *(undefined1 *)0x825c = 1;
  *(undefined2 *)0x825e = 0x8b89;
  *(undefined1 *)0x8260 = 1;
  *(undefined2 *)0x8262 = 0x8b9c;
  *(undefined1 *)0x8264 = 1;
  *(undefined2 *)0x8266 = 0x8baf;
  *(undefined1 *)0x8268 = 1;
  *(undefined2 *)0x8272 = 0x9256;
  *(undefined1 *)0x8274 = 0;
  *(undefined2 *)0x8276 = 0x4727;
  *(undefined1 *)0x8278 = 1;
  *(undefined2 *)0x827a = 0x4727;
  *(undefined1 *)0x827c = 1;
  *(undefined2 *)0x827e = 0x4727;
  *(undefined1 *)0x8280 = 1;
  *(undefined2 *)0x8282 = 0x8c4e;
  *(undefined1 *)0x8284 = 1;
  *(undefined2 *)0x829e = 0x87d1;
  *(undefined1 *)0x82a0 = 1;
  *(undefined2 *)0x82a2 = 0x9bee;
  *(undefined1 *)0x82a4 = 1;
  *(undefined2 *)0x82a6 = 0x544c;
  *(undefined1 *)0x82a8 = 1;
  *(undefined2 *)0x82aa = 0x545a;
  *(undefined1 *)0x82ac = 1;
  *(undefined2 *)0x82c6 = 0x9cf5;
  *(undefined1 *)0x82c8 = 1;
  *(undefined2 *)0x82ca = 0x9d19;
  *(undefined1 *)0x82cc = 1;
  *(undefined2 *)0x82ce = 0x9d5e;
  *(undefined1 *)0x82d0 = 1;
  *(undefined2 *)0x82d2 = 0x9d82;
  *(undefined1 *)0x82d4 = 1;
  *(undefined2 *)0x838e = 0x8bc2;
  *(undefined1 *)0x8390 = 1;
  *(undefined2 *)0x8392 = 0x8be5;
  *(undefined1 *)0x8394 = 1;
  *(undefined2 *)0x8396 = 0x8c08;
  *(undefined1 *)0x8398 = 1;
  *(undefined2 *)0x839a = 0x8c2b;
  *(undefined1 *)0x839c = 1;
  *(undefined2 *)0x83b6 = 0x8c71;
  *(undefined1 *)0x83b8 = 1;
  *(undefined2 *)0x83ba = 0x8c8a;
  *(undefined1 *)0x83bc = 1;
  *(undefined2 *)0x83be = 0x8ca3;
  *(undefined1 *)0x83c0 = 1;
  *(undefined2 *)0x83c2 = 0x8cbc;
  *(undefined1 *)0x83c4 = 1;
  *(undefined2 *)0x83c6 = 0x8cd5;
  *(undefined1 *)0x83c8 = 1;
  *(undefined2 *)0x83ca = 0x8cee;
  *(undefined1 *)0x83cc = 1;
  *(undefined2 *)0x83ce = 0x8d07;
  *(undefined1 *)0x83d0 = 1;
  return;
}



/* TARGET completion_segment3_helper at 0x106A; resolved function entry 0000:106a */
/* CALLERS of 0000:106a: */
/*   <none resolved> */

void completion_segment3_helper(void)

{
  int *piVar1;
  undefined2 uVar2;
  uint uVar3;
  int *piVar4;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  uVar2 = *(undefined2 *)0x7560;
  uVar3 = *(uint *)0x7966;
  *(uint *)0x7966 = uVar3 + 0x200 & 0x200;
  uVar3 = uVar3 & 0x200;
  piVar4 = (int *)(uVar3 + 0x7566);
  while (*piVar4 != -1) {
    piVar1 = piVar4 + 2;
    piVar4 = piVar4 + 4;
    if (*(int *)(*piVar1 + 0x1a) == -1) {
      *(undefined2 *)(*piVar1 + 0x18) = 0;
    }
    else {
      func_0x0000ffff(0,uVar3);
    }
  }
  return;
}



/* TARGET completion_segment3_dispatch_helper at 0xF07B; resolved function entry 0000:f07b */
/* CALLERS of 0000:f07b: */
/*   <none resolved> */

void completion_segment3_dispatch_helper(void)

{
  code *pcVar1;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  return;
}



/* TARGET completion_segment3_render_helper at 0x0908; resolved function entry 0000:0908 */
/* CALLERS of 0000:0908: */
/*   <none resolved> */

void completion_segment3_render_helper(void)

{
  int iVar1;
  
  func_0x0000ffff();
  iVar1 = 0;
  while( true ) {
    func_0x0000ffff(0,iVar1);
    if (iVar1 == 999) break;
    iVar1 = iVar1 + 1;
  }
  return;
}



/* TARGET completion_segment3_map_helper at 0x0A15; resolved function entry 0000:0a15 */
/* CALLERS of 0000:0a15: */
/*   <none resolved> */

void completion_segment3_map_helper(void)

{
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  if (*(int *)0x796c != 0 || *(int *)0x796e != 0) {
    func_0x0000ffff(0,*(undefined2 *)0x7970,*(undefined2 *)0x796c,*(undefined2 *)0x796e);
    *(undefined2 *)0x796c = 0;
    *(undefined2 *)0x796e = 0;
  }
  return;
}



/* TARGET completion_segment3_release_helper at 0x08C9; resolved function entry 0000:08c9 */
/* CALLERS of 0000:08c9: */
/*   <none resolved> */

void completion_segment3_release_helper(void)

{
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  if (*(int *)0x657a != 0 || *(int *)0x657c != 0) {
    func_0x0000ffff(0,*(int *)0x657e * *(int *)0x6580 + *(int *)0x657e * 3,*(undefined2 *)0x657a,
                    *(undefined2 *)0x657c);
    *(undefined2 *)0x657a = 0;
    *(undefined2 *)0x657c = 0;
  }
  return;
}



/* TARGET completion_segment3_transition_helper at 0x321F; resolved function entry 0000:321f */
/* CALLERS of 0000:321f: */
/*   <none resolved> */

void completion_segment3_transition_helper(void)

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



/* TARGET w1_doktor_damage_callback at 0xB25D; resolved function entry 0000:b25d */
/* CALLERS of 0000:b25d: */
/*   <none resolved> */

void w1_doktor_damage_callback(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x2e) < '\x01') {
    if (*(int *)0x8806 != 0) {
      iVar1 = *(int *)(unaff_DI + 0x2a);
      if (*(int *)0x8808 < iVar1) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        iVar1 = 0;
      }
      iVar1 = iVar1 * 4;
      if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar1 + -0x7822)) &&
           (*(int *)(iVar1 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
          (*(int *)(iVar1 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
         (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar1 + -0x7820))) {
        *(undefined2 *)(iVar1 + -0x7822) = 0;
        *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
        iVar1 = unaff_DI;
        func_0x0000ffff();
        *(undefined1 *)(unaff_DI + 0x17) = 2;
        *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
        *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0xa0000;
        func_0x0000ffff(0);
        *(undefined2 *)0x612e = 0xd;
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar1 + 0x2e) = 1;
        unaff_DI = iVar1;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (4 < *(int *)(unaff_DI + 0x2c)) {
        *(undefined1 *)0x88ae = 2;
      }
    }
  }
  else {
    *(int *)(unaff_DI + 0x2f) = *(int *)(unaff_DI + 0x2f) + 1;
    if (100 < *(int *)(unaff_DI + 0x2f)) {
      *(undefined2 *)(unaff_DI + 0x2f) = 0;
      *(undefined1 *)(unaff_DI + 0x2e) = 0;
      unaff_CS = 0;
      func_0x0000ffff();
    }
  }
  func_0x0000ffff(unaff_CS);
  return;
}



/* TARGET w1_main_boss_callback at 0xB33B; resolved function entry 0000:b33b */
/* CALLERS of 0000:b33b: */
/*   <none resolved> */

void w1_main_boss_callback(void)

{
  int *piVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  
  if (*(char *)0x88ae < '\x02') {
    func_0x0000ffff();
    iVar4 = -0x32;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar4 = 0x32;
    }
    bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
    func_0x0000ffff(0);
    uVar7 = false;
    if (bVar6) {
      iVar4 = -0x32;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar4 = 0x32;
      }
      bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
      func_0x0000ffff(0);
      uVar7 = false;
      if (bVar6) {
        iVar4 = -0x32;
        if (-1 < *(char *)(unaff_DI + 0x28)) {
          iVar4 = 0x32;
        }
        uVar7 = *(int *)(unaff_DI + 4) + iVar4 == 0;
        func_0x0000ffff(0);
      }
    }
    if (!(bool)uVar7) {
      *(undefined1 *)(unaff_DI + 0x3e) = 1;
    }
    if (*(char *)(unaff_DI + 0x34) < '\x01') {
      if (*(char *)(unaff_DI + 0x3e) < '\x01') {
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
        uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x7ff;
        *(uint *)(unaff_DI + 0x2e) = uVar3;
        iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 4);
        *(int *)(unaff_DI + 0x2c) = iVar4;
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
        *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
        if (0xdc < *(int *)(unaff_DI + 0x38)) {
          *(undefined2 *)(unaff_DI + 0x38) = 0;
          *(undefined1 *)(unaff_DI + 0x34) = 1;
        }
      }
      else if (*(char *)(unaff_DI + 0x40) < '\0') {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
        if (lVar5 < -0x7000) {
          lVar5 = -0x7000;
        }
        else if (0x7000 < lVar5) {
          lVar5 = 0x7000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
          *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          if (*(int *)(unaff_DI + 0x12) == 0x385) {
            *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
          }
          else {
            *(undefined2 *)(unaff_DI + 0x12) = 0x385;
          }
          *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
          *(undefined2 *)(unaff_DI + 0x42) = 0x1e;
        }
      }
      else {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
        if (lVar5 < -0x7000) {
          lVar5 = -0x7000;
        }
        else if (0x7000 < lVar5) {
          lVar5 = 0x7000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
    }
    else {
      iVar4 = unaff_DI;
      func_0x0000ffff(0);
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar4 + 0x29);
      if (*(char *)(iVar4 + 0x28) == '\x01') {
        lVar5 = *(long *)(iVar4 + 2) + 0x1f0000;
      }
      else {
        lVar5 = *(long *)(iVar4 + 2) + -0x1f0000;
      }
      *(long *)(unaff_DI + 2) = lVar5;
      *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(iVar4 + 6);
      *(undefined1 *)(iVar4 + 0x34) = 0;
      unaff_DI = iVar4;
    }
    iVar4 = *(int *)(unaff_DI + 0x2a);
    if (*(char *)(unaff_DI + 0x28) == '\x01') {
      lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
    }
    else {
      lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
    }
    *(long *)(iVar4 + 2) = lVar5;
    *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
    uVar3 = *(int *)(iVar4 + 0x2e) + 10U & 0x6ff;
    *(uint *)(iVar4 + 0x2e) = uVar3;
    *(int *)(iVar4 + 8) = *(int *)(iVar4 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
    *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar4 + 8);
    iVar4 = *(int *)(unaff_DI + 0x36);
    lVar5 = *(long *)(unaff_DI + 2);
    cVar2 = *(char *)(unaff_DI + 0x28);
    *(char *)(iVar4 + 0x28) = cVar2;
    if (cVar2 == '\x01') {
      lVar5 = lVar5 + 0x1f0000;
    }
    else {
      lVar5 = lVar5 + -0x1f0000;
    }
    *(long *)(iVar4 + 2) = lVar5;
    *(undefined4 *)(iVar4 + 6) = *(undefined4 *)(unaff_DI + 6);
  }
  else if (*(char *)0x88ae < '\x03') {
    *(undefined2 *)(*(int *)(unaff_DI + 0x36) + 0x18) = 0;
    if (*(int *)(unaff_DI + 0x12) == 0x385) {
      *(undefined2 *)(unaff_DI + 0x12) = 900;
    }
    else {
      *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
    }
    *(undefined1 *)0x88ae = 3;
  }
  else {
    if (*(char *)0x88ae < '\x04') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
      uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x5ff;
      *(uint *)(unaff_DI + 0x2e) = uVar3;
      iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x2c) = iVar4;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      if (*(char *)(unaff_DI + 0x28) == '\x01') {
        lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
      }
      else {
        lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
      }
      *(long *)(iVar4 + 2) = lVar5;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
      uVar3 = *(int *)(iVar4 + 0x2e) + 10U & 0x6ff;
      *(uint *)(iVar4 + 0x2e) = uVar3;
      *(int *)(iVar4 + 8) = *(int *)(iVar4 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar4 + 8);
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x1a) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x38) = 0;
      iVar8 = unaff_DI;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 4) = *(int *)(iVar8 + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)(iVar8 + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3);
      unaff_CS = 0;
      unaff_DI = iVar8;
      func_0x0000ffff(0);
      *(undefined1 *)(iVar8 + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 4) = *(int *)(unaff_DI + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 8) = *(int *)(unaff_DI + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3);
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) {
        return;
      }
      *(undefined4 *)(unaff_DI + 0xe) = 0xffff0000;
      *(undefined1 *)0x88ae = 4;
    }
    if (*(char *)0x88ae < '\x05') {
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x29) {
        return;
      }
      lVar5 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + -0x1200;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      if (*(char *)(unaff_DI + 0x28) == '\x01') {
        lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
      }
      else {
        lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
      }
      *(long *)(iVar4 + 2) = lVar5;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
      if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U < 0x161) &&
         ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U < 0xd1)) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    *(undefined1 *)0x88ae = 5;
    iVar4 = unaff_DI;
    func_0x0000ffff(unaff_CS);
    *(undefined1 *)(unaff_DI + 0x17) = 1;
    *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(iVar4 + 4);
    *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar4 + 8);
  }
  return;
}



/* TARGET w1_main_constructor_call_site at 0xB11B; resolved function entry 0000:b11b */
/* CALLERS of 0000:b11b: */
/*   <none resolved> */

void w1_main_constructor_call_site(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  iVar1 = *(int *)0x81c4;
  *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x244;
  *(int *)(unaff_DI + 8) = iVar1 + 200;
  return;
}



/* TARGET w1_helper_constructor_dispatch at 0xB188; resolved function entry 0000:b188 */
/* CALLERS of 0000:b188: */
/*   <none resolved> */

void w1_helper_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x60000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(in_stack_00000000 + 0x36) = in_stack_00000000;
  *(undefined1 *)(in_stack_00000000 + 0x17) = 2;
  *(long *)(in_stack_00000000 + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(undefined4 *)(in_stack_00000000 + 6) = *(undefined4 *)(in_stack_00000000 + 6);
  return;
}



/* TARGET w1_doktor_constructor_dispatch at 0xB1BF; resolved function entry 0000:b1bf */
/* CALLERS of 0000:b1bf: */
/*   <none resolved> */

void w1_doktor_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(in_stack_00000000 + 6);
  return;
}



/* TARGET w2_doktor_damage_callback at 0xBB0E; resolved function entry 0000:bb0e */
/* CALLERS of 0000:bb0e: */
/*   <none resolved> */

void w2_doktor_damage_callback(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x2e) < '\x01') {
    if (*(int *)0x8806 != 0) {
      iVar1 = *(int *)(unaff_DI + 0x2a);
      if (*(int *)0x8808 < iVar1) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        iVar1 = 0;
      }
      iVar1 = iVar1 * 4;
      if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar1 + -0x7822)) &&
           (*(int *)(iVar1 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
          (*(int *)(iVar1 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
         (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar1 + -0x7820))) {
        *(undefined2 *)(iVar1 + -0x7822) = 0;
        *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
        iVar1 = unaff_DI;
        func_0x0000ffff();
        *(undefined1 *)(unaff_DI + 0x17) = 2;
        *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
        *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0xa0000;
        func_0x0000ffff(0);
        *(undefined2 *)0x612e = 0xd;
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar1 + 0x2e) = 1;
        unaff_DI = iVar1;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (5 < *(int *)(unaff_DI + 0x2c)) {
        *(undefined1 *)0x88ae = 2;
      }
    }
  }
  else {
    *(int *)(unaff_DI + 0x2f) = *(int *)(unaff_DI + 0x2f) + 1;
    if (100 < *(int *)(unaff_DI + 0x2f)) {
      *(undefined2 *)(unaff_DI + 0x2f) = 0;
      *(undefined1 *)(unaff_DI + 0x2e) = 0;
      unaff_CS = 0;
      func_0x0000ffff();
    }
  }
  func_0x0000ffff(unaff_CS);
  return;
}



/* TARGET w2_main_boss_callback at 0xBBEC; resolved function entry 0000:bbec */
/* CALLERS of 0000:bbec: */
/*   <none resolved> */

void w2_main_boss_callback(void)

{
  int *piVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  
  if (*(char *)0x88ae < '\x02') {
    func_0x0000ffff();
    iVar4 = -0x32;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar4 = 0x32;
    }
    bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
    func_0x0000ffff(0);
    uVar7 = false;
    if (bVar6) {
      iVar4 = -0x32;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar4 = 0x32;
      }
      bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
      func_0x0000ffff(0);
      uVar7 = false;
      if (bVar6) {
        iVar4 = -0x32;
        if (-1 < *(char *)(unaff_DI + 0x28)) {
          iVar4 = 0x32;
        }
        uVar7 = *(int *)(unaff_DI + 4) + iVar4 == 0;
        func_0x0000ffff(0);
      }
    }
    if (!(bool)uVar7) {
      *(undefined1 *)(unaff_DI + 0x3e) = 1;
    }
    if (*(char *)(unaff_DI + 0x34) < '\x01') {
      if (*(char *)(unaff_DI + 0x3e) < '\x01') {
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
        uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x7ff;
        *(uint *)(unaff_DI + 0x2e) = uVar3;
        iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 4);
        *(int *)(unaff_DI + 0x2c) = iVar4;
        *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
        *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
        if (0xaa < *(int *)(unaff_DI + 0x38)) {
          *(undefined2 *)(unaff_DI + 0x38) = 0;
          *(undefined1 *)(unaff_DI + 0x34) = 1;
        }
      }
      else if (*(char *)(unaff_DI + 0x40) < '\0') {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x1000;
        if (lVar5 < -0x9000) {
          lVar5 = -0x9000;
        }
        else if (0x9000 < lVar5) {
          lVar5 = 0x9000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
          *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          if (*(int *)(unaff_DI + 0x12) == 0xdc) {
            *(undefined2 *)(unaff_DI + 0x12) = 0x10e;
          }
          else {
            *(undefined2 *)(unaff_DI + 0x12) = 0xdc;
          }
          *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
      else {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x400;
        if (lVar5 < -0x9000) {
          lVar5 = -0x9000;
        }
        else if (0x9000 < lVar5) {
          lVar5 = 0x9000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
    }
    else {
      iVar4 = unaff_DI;
      func_0x0000ffff(0);
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar4 + 0x29);
      if (*(char *)(iVar4 + 0x28) == '\x01') {
        lVar5 = *(long *)(iVar4 + 2) + 0x70000;
      }
      else {
        lVar5 = *(long *)(iVar4 + 2) + -0x70000;
      }
      *(long *)(unaff_DI + 2) = lVar5;
      *(long *)(unaff_DI + 6) = *(long *)(iVar4 + 6) + 0x250000;
      *(undefined1 *)(iVar4 + 0x34) = 0;
      unaff_DI = iVar4;
    }
    iVar4 = *(int *)(unaff_DI + 0x2a);
    if (*(char *)(unaff_DI + 0x28) == '\x01') {
      lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
    }
    else {
      lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
    }
    *(long *)(iVar4 + 2) = lVar5;
    *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
    uVar3 = *(int *)(iVar4 + 0x2e) + 10U & 0x6ff;
    *(uint *)(iVar4 + 0x2e) = uVar3;
    *(int *)(iVar4 + 8) = *(int *)(iVar4 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
    *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar4 + 8);
    iVar4 = *(int *)(unaff_DI + 0x36);
    lVar5 = *(long *)(unaff_DI + 2);
    cVar2 = *(char *)(unaff_DI + 0x28);
    *(char *)(iVar4 + 0x28) = cVar2;
    if (cVar2 == '\x01') {
      lVar5 = lVar5 + 0x1f0000;
    }
    else {
      lVar5 = lVar5 + -0x1f0000;
    }
    *(long *)(iVar4 + 2) = lVar5;
    *(undefined4 *)(iVar4 + 6) = *(undefined4 *)(unaff_DI + 6);
  }
  else if (*(char *)0x88ae < '\x03') {
    *(undefined2 *)(*(int *)(unaff_DI + 0x36) + 0x18) = 0;
    if (*(int *)(unaff_DI + 0x12) == 0xdc) {
      *(undefined2 *)(unaff_DI + 0x12) = 0xdd;
    }
    else {
      *(undefined2 *)(unaff_DI + 0x12) = 0x10f;
    }
    *(undefined1 *)0x88ae = 3;
  }
  else {
    if (*(char *)0x88ae < '\x04') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
      uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x5ff;
      *(uint *)(unaff_DI + 0x2e) = uVar3;
      iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x2c) = iVar4;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      if (*(char *)(unaff_DI + 0x28) == '\x01') {
        lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
      }
      else {
        lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
      }
      *(long *)(iVar4 + 2) = lVar5;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
      uVar3 = *(int *)(iVar4 + 0x2e) + 10U & 0x6ff;
      *(uint *)(iVar4 + 0x2e) = uVar3;
      *(int *)(iVar4 + 8) = *(int *)(iVar4 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar4 + 8);
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x1a) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x38) = 0;
      iVar8 = unaff_DI;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 4) = *(int *)(iVar8 + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)(iVar8 + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3);
      unaff_CS = 0;
      unaff_DI = iVar8;
      func_0x0000ffff(0);
      *(undefined1 *)(iVar8 + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 4) = *(int *)(unaff_DI + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 8) = *(int *)(unaff_DI + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3);
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) {
        return;
      }
      *(undefined4 *)(unaff_DI + 0xe) = 0xffff0000;
      *(undefined1 *)0x88ae = 4;
    }
    if (*(char *)0x88ae < '\x05') {
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x29) {
        return;
      }
      lVar5 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + -0x1200;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      if (*(char *)(unaff_DI + 0x28) == '\x01') {
        lVar5 = *(long *)(unaff_DI + 2) + 0x60000;
      }
      else {
        lVar5 = *(long *)(unaff_DI + 2) + -0x60000;
      }
      *(long *)(iVar4 + 2) = lVar5;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
      if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U < 0x161) &&
         ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U < 0xd1)) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    *(undefined1 *)0x88ae = 5;
    iVar4 = unaff_DI;
    func_0x0000ffff(unaff_CS);
    *(undefined1 *)(unaff_DI + 0x17) = 1;
    *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(iVar4 + 4);
    *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar4 + 8);
  }
  return;
}



/* TARGET w2_main_constructor_call_site at 0xB9CC; resolved function entry 0000:b9cc */
/* CALLERS of 0000:b9cc: */
/*   <none resolved> */

void w2_main_constructor_call_site(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  iVar1 = *(int *)0x81c4;
  *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x1c2;
  *(int *)(unaff_DI + 8) = iVar1 + 0xdc;
  return;
}



/* TARGET w2_prop_constructor_dispatch at 0xBA39; resolved function entry 0000:ba39 */
/* CALLERS of 0000:ba39: */
/*   <none resolved> */

void w2_prop_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x60000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(in_stack_00000000 + 0x36) = in_stack_00000000;
  *(undefined1 *)(in_stack_00000000 + 0x17) = 2;
  *(long *)(in_stack_00000000 + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(undefined4 *)(in_stack_00000000 + 6) = *(undefined4 *)(in_stack_00000000 + 6);
  return;
}



/* TARGET w2_doktor_constructor_dispatch at 0xBA70; resolved function entry 0000:ba70 */
/* CALLERS of 0000:ba70: */
/*   <none resolved> */

void w2_doktor_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(in_stack_00000000 + 6);
  return;
}



/* TARGET w2_main_boss_constructor at 0xB9F3; resolved function entry 0000:b9f3 */
/* CALLERS of 0000:b9f3: */
/*   <none resolved> */

void w2_main_boss_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w2_main_boss_callback;
  *(undefined2 *)(unaff_DI + 0x12) = 0x10e;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined2 *)(unaff_DI + 0x38) = 0;
  *(undefined2 *)(unaff_DI + 0x44) = 0;
  *(undefined1 *)(unaff_DI + 0x34) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffff7000;
  *(undefined1 *)(unaff_DI + 0x40) = 0xff;
  *(undefined2 *)(unaff_DI + 0x42) = 0x14;
  *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x60000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x1f0000;
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 6);
  return;
}



/* TARGET w2_prop_constructor at 0xBAA1; resolved function entry 0000:baa1 */
/* CALLERS of 0000:baa1: */
/*   <none resolved> */

void w2_prop_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xbad7;
  *(undefined2 *)(unaff_DI + 0x2e) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w2_doktor_constructor at 0xBABC; resolved function entry 0000:babc */
/* CALLERS of 0000:babc: */
/*   <none resolved> */

void w2_doktor_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w2_doktor_damage_callback;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w2_helper_constructor_a at 0xC104; resolved function entry 0000:c104 */
/* CALLERS of 0000:c104: */
/*   <none resolved> */

void w2_helper_constructor_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0xed;
  *(undefined2 *)(unaff_DI + 0x18) = 0xc1a0;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffee000;
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0xf0000;
  return;
}



/* TARGET w2_helper_constructor_b at 0xC147; resolved function entry 0000:c147 */
/* CALLERS of 0000:c147: */
/*   <none resolved> */

void w2_helper_constructor_b(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0xee;
  *(undefined2 *)(unaff_DI + 0x18) = 0xc1a0;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffed000;
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0xa0000;
  return;
}



/* TARGET w2_helper_constructor_c at 0xC18A; resolved function entry 0000:c18a */
/* CALLERS of 0000:c18a: */
/*   <none resolved> */

void w2_helper_constructor_c(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0xef;
  *(undefined2 *)(unaff_DI + 0x18) = 0xc1a0;
  *(undefined4 *)(unaff_DI + 0xe) = 0xfffec000;
  return;
}



/* TARGET w2_effect_constructor_a at 0xE572; resolved function entry 0000:e572 */
/* CALLERS of 0000:e572: */
/*   <none resolved> */

void w2_effect_constructor_a(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe6a4;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x6000;
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0x60000;
  return;
}



/* TARGET w2_effect_constructor_b at 0xE5D8; resolved function entry 0000:e5d8 */
/* CALLERS of 0000:e5d8: */
/*   <none resolved> */

void w2_effect_constructor_b(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe836;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x6000;
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0x60000;
  return;
}



/* TARGET w2_effect_constructor_c at 0xE63E; resolved function entry 0000:e63e */
/* CALLERS of 0000:e63e: */
/*   <none resolved> */

void w2_effect_constructor_c(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe76d;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x7000;
  return;
}



/* TARGET w2_effect_constructor_d at 0xE671; resolved function entry 0000:e671 */
/* CALLERS of 0000:e671: */
/*   <none resolved> */

void w2_effect_constructor_d(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xe6a4;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x7000;
  return;
}



/* TARGET w2_boss_helper_callback at 0xC1A0; resolved function entry 0000:c1a0 */
/* CALLERS of 0000:c1a0: */
/*   <none resolved> */

void w2_boss_helper_callback(void)

{
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  long lVar1;
  uint extraout_DX;
  uint extraout_DX_00;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined1 in_CF;
  ulong uVar2;
  
  func_0x0000ffff();
  if ((bool)in_CF) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
    return;
  }
  func_0x0000ffff(0);
  if (!(bool)in_CF) {
    if (*(char *)(unaff_DI + 0x29) < '\x01') {
      func_0x0000ffff(0);
      if ((extraout_DX & 0x70) != 0) goto LAB_0000_c212;
      uVar2 = func_0x0000ffff(0);
      lVar1 = CONCAT22(extraout_var,(int)uVar2);
    }
    else {
      func_0x0000ffff(0);
      if ((extraout_DX_00 & 0x70) != 0) goto LAB_0000_c212;
      uVar2 = func_0x0000ffff(0);
      lVar1 = CONCAT22(extraout_var_00,(int)uVar2);
    }
    if ((uVar2 & 0x700000) == 0) {
      lVar1 = lVar1 + 15000;
      if (lVar1 < -0x30000) {
        lVar1 = -0x30000;
      }
      else if (0x30000 < lVar1) {
        lVar1 = 0x30000;
      }
      *(long *)(unaff_DI + 0xe) = lVar1;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar1;
      func_0x0000ffff(0);
      return;
    }
  }
LAB_0000_c212:
  *(undefined2 *)(unaff_DI + 0x18) = 0;
  return;
}



/* TARGET w2_boss_prop_callback at 0xBAD7; resolved function entry 0000:bad7 */
/* CALLERS of 0000:bad7: */
/*   <none resolved> */

void w2_boss_prop_callback(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((*(char *)0x88ae == '\x05') &&
     ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U ||
      (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)))) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  return;
}



/* TARGET w2_boss_effect_callback_a at 0xE6A4; resolved function entry 0000:e6a4 */
/* CALLERS of 0000:e6a4: */
/*   <none resolved> */

void w2_boss_effect_callback_a(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      goto LAB_0000_e73e;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
  }
LAB_0000_e73e:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 3);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w2_boss_effect_callback_b at 0xE836; resolved function entry 0000:e836 */
/* CALLERS of 0000:e836: */
/*   <none resolved> */

void w2_boss_effect_callback_b(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + *(long *)(unaff_DI + 0xe);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      goto LAB_0000_e8d0;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
  }
LAB_0000_e8d0:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 4);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w3_doktor_damage_callback at 0xC328; resolved function entry 0000:c328 */
/* CALLERS of 0000:c328: */
/*   <none resolved> */

void w3_doktor_damage_callback(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x2e) < '\x01') {
    if (*(int *)0x8806 != 0) {
      iVar1 = *(int *)(unaff_DI + 0x2a);
      if (*(int *)0x8808 < iVar1) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        iVar1 = 0;
      }
      iVar1 = iVar1 * 4;
      if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar1 + -0x7822)) &&
           (*(int *)(iVar1 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
          (*(int *)(iVar1 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
         (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar1 + -0x7820))) {
        *(undefined2 *)(iVar1 + -0x7822) = 0;
        *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
        iVar1 = unaff_DI;
        func_0x0000ffff();
        *(undefined1 *)(unaff_DI + 0x17) = 2;
        *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
        *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0xa0000;
        func_0x0000ffff(0);
        *(undefined2 *)0x612e = 0xd;
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar1 + 0x2e) = 1;
        unaff_DI = iVar1;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (4 < *(int *)(unaff_DI + 0x2c)) {
        *(char *)0x88ae = *(char *)0x88ae + '\x01';
        *(undefined2 *)(unaff_DI + 0x2c) = 0;
      }
    }
  }
  else {
    *(int *)(unaff_DI + 0x2f) = *(int *)(unaff_DI + 0x2f) + 1;
    if (100 < *(int *)(unaff_DI + 0x2f)) {
      *(undefined2 *)(unaff_DI + 0x2f) = 0;
      *(undefined1 *)(unaff_DI + 0x2e) = 0;
      unaff_CS = 0;
      func_0x0000ffff();
    }
  }
  func_0x0000ffff(unaff_CS);
  return;
}



/* TARGET w3_main_boss_callback at 0xC40B; resolved function entry 0000:c40b */
/* CALLERS of 0000:c40b: */
/*   <none resolved> */

void w3_main_boss_callback(void)

{
  int *piVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar6;
  int iVar7;
  
  if ('\x02' < *(char *)0x88ae) {
    if (*(char *)0x88ae < '\x04') {
      *(undefined2 *)(*(int *)(unaff_DI + 0x36) + 0x18) = 0;
      if (*(int *)(unaff_DI + 0x12) == 0x385) {
        *(undefined2 *)(unaff_DI + 0x12) = 900;
      }
      else {
        *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
      }
      *(undefined1 *)0x88ae = 4;
      return;
    }
    if (*(char *)0x88ae < '\x05') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
      uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x5ff;
      *(uint *)(unaff_DI + 0x2e) = uVar3;
      iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x2c) = iVar4;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x1a) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x38) = 0;
      iVar7 = unaff_DI;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 4) = *(int *)(iVar7 + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) =
           *(int *)(iVar7 + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x14;
      unaff_CS = 0;
      unaff_DI = iVar7;
      func_0x0000ffff(0);
      *(undefined1 *)(iVar7 + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar7 + 4) = *(int *)(unaff_DI + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar7 + 8) =
           *(int *)(unaff_DI + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x14;
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) {
        return;
      }
      *(undefined4 *)(unaff_DI + 0xe) = 0xffff0000;
      *(undefined1 *)0x88ae = 5;
    }
    if (*(char *)0x88ae < '\x06') {
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x29) {
        return;
      }
      lVar5 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + -0x1200;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
      if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U < 0x161) &&
         ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U < 0xd1)) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    *(undefined1 *)0x88ae = 6;
    iVar4 = unaff_DI;
    func_0x0000ffff(unaff_CS);
    *(undefined1 *)(unaff_DI + 0x17) = 1;
    *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(iVar4 + 4);
    *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar4 + 8);
    return;
  }
  func_0x0000ffff();
  if (*(char *)(unaff_DI + 0x29) < '\x01') {
    bVar6 = *(int *)(unaff_DI + 4) == 0x3c;
    func_0x0000ffff(0);
    if (bVar6) {
LAB_0000_c454:
      *(undefined1 *)(unaff_DI + 0x3e) = 1;
    }
  }
  else {
    bVar6 = *(int *)(unaff_DI + 4) == -0x3c;
    func_0x0000ffff(0);
    if (bVar6) goto LAB_0000_c454;
  }
  if (*(char *)(unaff_DI + 0x34) < '\x01') {
    if ('\0' < *(char *)(unaff_DI + 0x3e)) {
      if (*(char *)0x88ae < '\x02') {
        if (*(int *)(unaff_DI + 0x46) == 1) {
          *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
          if (100 < *(int *)(unaff_DI + 0x38)) {
            *(undefined2 *)(unaff_DI + 0x38) = 0;
            *(undefined2 *)(unaff_DI + 0x46) = 0;
            *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
            *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
            *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
            if (*(int *)(unaff_DI + 0x12) == 0x385) {
              *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
            }
            else {
              *(undefined2 *)(unaff_DI + 0x12) = 0x385;
            }
            *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
            *(undefined2 *)(unaff_DI + 0x42) = 0x28;
          }
        }
        else if (*(char *)(unaff_DI + 0x40) < '\0') {
          lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x400;
          if (lVar5 < -0x13000) {
            lVar5 = -0x13000;
          }
          else if (0x13000 < lVar5) {
            lVar5 = 0x13000;
          }
          *(long *)(unaff_DI + 10) = lVar5;
          *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
          piVar1 = (int *)(unaff_DI + 0x42);
          iVar4 = *piVar1;
          *piVar1 = *piVar1 + -1;
          if (SBORROW2(iVar4,1) != *piVar1 < 0) {
            *(undefined2 *)(unaff_DI + 0x46) = 1;
            iVar4 = unaff_DI;
            func_0x0000ffff(0);
            *(undefined1 *)(unaff_DI + 0x17) = 2;
            *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar4 + 0x29);
            if (*(char *)(iVar4 + 0x28) == '\x01') {
              lVar5 = *(long *)(iVar4 + 2) + 0x70000;
            }
            else {
              lVar5 = *(long *)(iVar4 + 2) + -0x70000;
            }
            *(long *)(unaff_DI + 2) = lVar5;
            *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(iVar4 + 6);
            unaff_DI = iVar4;
          }
        }
        else {
          lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x1000;
          if (lVar5 < -0x13000) {
            lVar5 = -0x13000;
          }
          else if (0x13000 < lVar5) {
            lVar5 = 0x13000;
          }
          *(long *)(unaff_DI + 10) = lVar5;
          *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
          piVar1 = (int *)(unaff_DI + 0x42);
          iVar4 = *piVar1;
          *piVar1 = *piVar1 + -1;
          if (SBORROW2(iVar4,1) != *piVar1 < 0) {
            *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
            *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
            *(undefined2 *)(unaff_DI + 0x42) = 0x14;
          }
        }
      }
      else if (*(char *)(unaff_DI + 0x40) < '\0') {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x2000;
        if (lVar5 < -0x15000) {
          lVar5 = -0x15000;
        }
        else if (0x15000 < lVar5) {
          lVar5 = 0x15000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
          *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          if (*(int *)(unaff_DI + 0x12) == 0x385) {
            *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
          }
          else {
            *(undefined2 *)(unaff_DI + 0x12) = 0x385;
          }
          *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
      else {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x2000;
        if (lVar5 < -0x15000) {
          lVar5 = -0x15000;
        }
        else if (0x15000 < lVar5) {
          lVar5 = 0x15000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
      goto LAB_0000_c747;
    }
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (*(char *)0x88ae < '\x02') goto LAB_0000_c747;
  }
  *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
  if (0x28 < *(int *)(unaff_DI + 0x38)) {
    iVar4 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 0x38) = (int)(char)(*(byte *)(iVar4 + 0x646c) >> 4);
    iVar4 = unaff_DI;
    func_0x0000ffff(0);
    *(undefined1 *)(unaff_DI + 0x17) = 2;
    *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar4 + 0x29);
    if (*(char *)(iVar4 + 0x28) == '\x01') {
      lVar5 = *(long *)(iVar4 + 2) + -0x1e0000;
    }
    else {
      lVar5 = *(long *)(iVar4 + 2) + 0x1e0000;
    }
    *(long *)(unaff_DI + 2) = lVar5;
    *(long *)(unaff_DI + 6) = *(long *)(iVar4 + 6) + -0x500000;
    *(undefined1 *)(iVar4 + 0x34) = 0;
    unaff_DI = iVar4;
  }
LAB_0000_c747:
  iVar4 = *(int *)(unaff_DI + 0x36);
  lVar5 = *(long *)(unaff_DI + 2);
  cVar2 = *(char *)(unaff_DI + 0x28);
  *(char *)(iVar4 + 0x28) = cVar2;
  if (cVar2 == '\x01') {
    lVar5 = lVar5 + 0x1f0000;
  }
  else {
    lVar5 = lVar5 + -0x1f0000;
  }
  *(long *)(iVar4 + 2) = lVar5;
  *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x190000;
  return;
}



/* TARGET w3_main_constructor_call_site at 0xC264; resolved function entry 0000:c264 */
/* CALLERS of 0000:c264: */
/*   <none resolved> */

void w3_main_constructor_call_site(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  iVar1 = *(int *)0x81c4;
  *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x244;
  *(int *)(unaff_DI + 8) = iVar1 + -0x26;
  return;
}



/* TARGET w3_doktor_constructor_dispatch at 0xC2D6; resolved function entry 0000:c2d6 */
/* CALLERS of 0000:c2d6: */
/*   <none resolved> */

void w3_doktor_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0x190000;
  return;
}



/* TARGET w3_main_boss_constructor at 0xC28A; resolved function entry 0000:c28a */
/* CALLERS of 0000:c28a: */
/*   <none resolved> */

void w3_main_boss_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w3_main_boss_callback;
  *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined2 *)(unaff_DI + 0x38) = 0;
  *(undefined2 *)(unaff_DI + 0x46) = 0;
  *(undefined2 *)(unaff_DI + 0x44) = 0;
  *(undefined1 *)(unaff_DI + 0x34) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffed000;
  *(undefined1 *)(unaff_DI + 0x40) = 0xff;
  *(undefined2 *)(unaff_DI + 0x42) = 0x14;
  *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x190000;
  return;
}



/* TARGET w3_doktor_constructor at 0xC30D; resolved function entry 0000:c30d */
/* CALLERS of 0000:c30d: */
/*   <none resolved> */

void w3_doktor_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w3_doktor_damage_callback;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w3_effect_constructor_a at 0xE9ED; resolved function entry 0000:e9ed */
/* CALLERS of 0000:e9ed: */
/*   <none resolved> */

void w3_effect_constructor_a(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xeb1f;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x6000;
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0x60000;
  return;
}



/* TARGET w3_effect_constructor_b at 0xEA53; resolved function entry 0000:ea53 */
/* CALLERS of 0000:ea53: */
/*   <none resolved> */

void w3_effect_constructor_b(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xecb1;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x6000;
  iVar1 = unaff_DI;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(iVar1 + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0x60000;
  return;
}



/* TARGET w3_effect_constructor_c at 0xEAB9; resolved function entry 0000:eab9 */
/* CALLERS of 0000:eab9: */
/*   <none resolved> */

void w3_effect_constructor_c(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xebe8;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x7000;
  return;
}



/* TARGET w3_effect_constructor_d at 0xEAEC; resolved function entry 0000:eaec */
/* CALLERS of 0000:eaec: */
/*   <none resolved> */

void w3_effect_constructor_d(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xeb1f;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x7000;
  return;
}



/* TARGET w3_boss_effect_callback_a at 0xEB1F; resolved function entry 0000:eb1f */
/* CALLERS of 0000:eb1f: */
/*   <none resolved> */

void w3_boss_effect_callback_a(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      goto LAB_0000_ebb9;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
  }
LAB_0000_ebb9:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 3);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w3_boss_effect_callback_b at 0xECB1; resolved function entry 0000:ecb1 */
/* CALLERS of 0000:ecb1: */
/*   <none resolved> */

void w3_boss_effect_callback_b(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      goto LAB_0000_ed4b;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
  }
LAB_0000_ed4b:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 4);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w3_boss_effect_callback_c at 0xEBE8; resolved function entry 0000:ebe8 */
/* CALLERS of 0000:ebe8: */
/*   <none resolved> */

void w3_boss_effect_callback_c(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      goto LAB_0000_ec82;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
  }
LAB_0000_ec82:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 0xfU & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 4);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w4_doktor_damage_callback at 0xCDA3; resolved function entry 0000:cda3 */
/* CALLERS of 0000:cda3: */
/*   <none resolved> */

void w4_doktor_damage_callback(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x2e) < '\x01') {
    if (*(int *)0x8806 != 0) {
      iVar1 = *(int *)(unaff_DI + 0x2a);
      if (*(int *)0x8808 < iVar1) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        iVar1 = 0;
      }
      iVar1 = iVar1 * 4;
      if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar1 + -0x7822)) &&
           (*(int *)(iVar1 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
          (*(int *)(iVar1 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
         (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar1 + -0x7820))) {
        *(undefined2 *)(iVar1 + -0x7822) = 0;
        *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
        iVar1 = unaff_DI;
        func_0x0000ffff();
        *(undefined1 *)(unaff_DI + 0x17) = 2;
        *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
        *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0xa0000;
        func_0x0000ffff(0);
        *(undefined2 *)0x612e = 0xd;
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar1 + 0x2e) = 1;
        unaff_DI = iVar1;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (5 < *(int *)(unaff_DI + 0x2c)) {
        *(undefined1 *)0x88ae = 2;
      }
    }
  }
  else {
    *(int *)(unaff_DI + 0x2f) = *(int *)(unaff_DI + 0x2f) + 1;
    if (100 < *(int *)(unaff_DI + 0x2f)) {
      *(undefined2 *)(unaff_DI + 0x2f) = 0;
      *(undefined1 *)(unaff_DI + 0x2e) = 0;
      unaff_CS = 0;
      func_0x0000ffff();
    }
  }
  func_0x0000ffff(unaff_CS);
  return;
}



/* TARGET w4_main_boss_callback at 0xCE81; resolved function entry 0000:ce81 */
/* CALLERS of 0000:ce81: */
/*   <none resolved> */

void w4_main_boss_callback(void)

{
  char cVar1;
  uint uVar2;
  long lVar3;
  int iVar4;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar5;
  undefined1 uVar6;
  int iVar7;
  
  if (*(char *)0x88ae < '\x02') {
    if (*(char *)(unaff_DI + 0x34) < '\x01') {
      func_0x0000ffff();
      iVar4 = -0x32;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar4 = 0x32;
      }
      bVar5 = *(int *)(unaff_DI + 4) + iVar4 == 0;
      func_0x0000ffff(0);
      uVar6 = false;
      if (bVar5) {
        iVar4 = -0x32;
        if (-1 < *(char *)(unaff_DI + 0x28)) {
          iVar4 = 0x32;
        }
        bVar5 = *(int *)(unaff_DI + 4) + iVar4 == 0;
        func_0x0000ffff(0);
        uVar6 = false;
        if (bVar5) {
          iVar4 = -0x32;
          if (-1 < *(char *)(unaff_DI + 0x28)) {
            iVar4 = 0x32;
          }
          uVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
          func_0x0000ffff(0);
        }
      }
      if (!(bool)uVar6) {
        *(undefined1 *)(unaff_DI + 0x3e) = 1;
      }
      if (*(int *)(unaff_DI + 8) < 0x1a4) {
        if ('\0' < *(char *)(unaff_DI + 0x3e)) {
          *(long *)(unaff_DI + 10) = -*(long *)(unaff_DI + 10);
          *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
          *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
          *(undefined1 *)(unaff_DI + 0x3e) = 0;
          if (*(int *)(unaff_DI + 0x12) == 0x385) {
            *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
          }
          else {
            *(undefined2 *)(unaff_DI + 0x12) = 0x385;
          }
        }
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
        lVar3 = *(long *)(unaff_DI + 0xe);
        if (lVar3 < 1) {
          lVar3 = lVar3 + 4000;
          if (lVar3 < -0x35000) {
            lVar3 = -0x35000;
          }
          else if (0x35000 < lVar3) {
            lVar3 = 0x35000;
          }
        }
        else {
          lVar3 = lVar3 + 5000;
          if (lVar3 < -0x35000) {
            lVar3 = -0x35000;
          }
          else if (0x35000 < lVar3) {
            lVar3 = 0x35000;
          }
        }
        *(long *)(unaff_DI + 0xe) = lVar3;
        *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar3;
        if (0xdc < *(int *)(unaff_DI + 0x38)) {
          *(undefined2 *)(unaff_DI + 0x38) = 0;
          *(undefined1 *)(unaff_DI + 0x34) = 1;
        }
      }
      else {
        *(long *)(unaff_DI + 0xe) = -*(long *)(unaff_DI + 0xe);
        *(undefined1 *)(unaff_DI + 0x34) = 1;
      }
    }
    else {
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0xa0000;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x34) = 0;
    }
    iVar4 = *(int *)(unaff_DI + 0x36);
    lVar3 = *(long *)(unaff_DI + 2);
    cVar1 = *(char *)(unaff_DI + 0x28);
    *(char *)(iVar4 + 0x28) = cVar1;
    if (cVar1 == '\x01') {
      lVar3 = lVar3 + 0x1f0000;
    }
    else {
      lVar3 = lVar3 + -0x1f0000;
    }
    *(long *)(iVar4 + 2) = lVar3;
    *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1c0000;
    iVar4 = *(int *)(unaff_DI + 0x2a);
    lVar3 = *(long *)(unaff_DI + 2);
    cVar1 = *(char *)(unaff_DI + 0x28);
    *(char *)(iVar4 + 0x28) = cVar1;
    if (cVar1 == '\x01') {
      lVar3 = lVar3 + 0x50000;
    }
    else {
      lVar3 = lVar3 + -0x50000;
    }
    *(long *)(iVar4 + 2) = lVar3;
    *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0xc0000;
  }
  else if (*(char *)0x88ae < '\x03') {
    *(undefined2 *)(*(int *)(unaff_DI + 0x36) + 0x18) = 0;
    if (*(int *)(unaff_DI + 0x12) == 0x385) {
      *(undefined2 *)(unaff_DI + 0x12) = 900;
    }
    else {
      *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
    }
    *(undefined1 *)0x88ae = 3;
  }
  else {
    if (*(char *)0x88ae < '\x04') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
      uVar2 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x5ff;
      *(uint *)(unaff_DI + 0x2e) = uVar2;
      iVar4 = (int)(*(char *)(uVar2 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x2c) = iVar4;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      lVar3 = *(long *)(unaff_DI + 2);
      cVar1 = *(char *)(unaff_DI + 0x28);
      *(char *)(iVar4 + 0x28) = cVar1;
      if (cVar1 == '\x01') {
        lVar3 = lVar3 + 0x50000;
      }
      else {
        lVar3 = lVar3 + -0x50000;
      }
      *(long *)(iVar4 + 2) = lVar3;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0xc0000;
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x1a) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x38) = 0;
      iVar7 = unaff_DI;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 4) = *(int *)(iVar7 + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) =
           *(int *)(iVar7 + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x1b;
      unaff_CS = 0;
      unaff_DI = iVar7;
      func_0x0000ffff(0);
      *(undefined1 *)(iVar7 + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar7 + 4) = *(int *)(unaff_DI + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar7 + 8) =
           *(int *)(unaff_DI + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x1b;
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) {
        return;
      }
      *(undefined4 *)(unaff_DI + 0xe) = 0xffff0000;
      *(undefined1 *)0x88ae = 4;
    }
    if (*(char *)0x88ae < '\x05') {
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x29) {
        return;
      }
      lVar3 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + -0x1200;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar3;
      iVar4 = *(int *)(unaff_DI + 0x2a);
      lVar3 = *(long *)(unaff_DI + 2);
      cVar1 = *(char *)(unaff_DI + 0x28);
      *(char *)(iVar4 + 0x28) = cVar1;
      if (cVar1 == '\x01') {
        lVar3 = lVar3 + 0x50000;
      }
      else {
        lVar3 = lVar3 + -0x50000;
      }
      *(long *)(iVar4 + 2) = lVar3;
      *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0xc0000;
      if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U < 0x161) &&
         ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U < 0xd1)) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    *(undefined1 *)0x88ae = 5;
    iVar4 = unaff_DI;
    func_0x0000ffff(unaff_CS);
    *(undefined1 *)(unaff_DI + 0x17) = 1;
    *(undefined2 *)(unaff_DI + 4) = *(undefined2 *)(iVar4 + 4);
    *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar4 + 8);
  }
  return;
}



/* TARGET w4_main_constructor_call_site at 0xCC41; resolved function entry 0000:cc41 */
/* CALLERS of 0000:cc41: */
/*   <none resolved> */

void w4_main_constructor_call_site(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  iVar1 = *(int *)0x81c4;
  *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 600;
  *(int *)(unaff_DI + 8) = iVar1 + 0x96;
  return;
}



/* TARGET w4_helper_constructor_dispatch at 0xCCB7; resolved function entry 0000:ccb7 */
/* CALLERS of 0000:ccb7: */
/*   <none resolved> */

void w4_helper_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0xc0000;
  func_0x0000ffff(0);
  *(int *)(in_stack_00000000 + 0x36) = in_stack_00000000;
  *(undefined1 *)(in_stack_00000000 + 0x17) = 2;
  *(long *)(in_stack_00000000 + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(long *)(in_stack_00000000 + 6) = *(long *)(in_stack_00000000 + 6) + -0x1c0000;
  return;
}



/* TARGET w4_doktor_constructor_dispatch at 0xCCEE; resolved function entry 0000:ccee */
/* CALLERS of 0000:ccee: */
/*   <none resolved> */

void w4_doktor_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0x1c0000;
  return;
}



/* TARGET w4_main_boss_constructor at 0xCC68; resolved function entry 0000:cc68 */
/* CALLERS of 0000:cc68: */
/*   <none resolved> */

void w4_main_boss_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w4_main_boss_callback;
  *(undefined2 *)(unaff_DI + 0x12) = 0x3b7;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined2 *)(unaff_DI + 0x38) = 0;
  *(undefined2 *)(unaff_DI + 0x44) = 0;
  *(undefined1 *)(unaff_DI + 0x34) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xffff9000;
  *(undefined4 *)(unaff_DI + 0xe) = 0xffff9000;
  *(undefined1 *)(unaff_DI + 0x40) = 0xff;
  *(undefined2 *)(unaff_DI + 0x42) = 0x14;
  *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0xc0000;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x1c0000;
  return;
}



/* TARGET w4_helper_constructor at 0xCD25; resolved function entry 0000:cd25 */
/* CALLERS of 0000:cd25: */
/*   <none resolved> */

void w4_helper_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xcd40;
  *(undefined2 *)(unaff_DI + 0x2e) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w4_doktor_constructor at 0xCD88; resolved function entry 0000:cd88 */
/* CALLERS of 0000:cd88: */
/*   <none resolved> */

void w4_doktor_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w4_doktor_damage_callback;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w4_boss_helper_callback at 0xCD40; resolved function entry 0000:cd40 */
/* CALLERS of 0000:cd40: */
/*   <none resolved> */

void w4_boss_helper_callback(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((*(char *)0x88ae == '\x05') &&
     ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U ||
      (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)))) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  func_0x0000ffff();
  func_0x0000ffff(0);
  return;
}



/* TARGET w5_doktor_damage_callback at 0xD55A; resolved function entry 0000:d55a */
/* CALLERS of 0000:d55a: */
/*   <none resolved> */

void w5_doktor_damage_callback(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  
  if (*(char *)(unaff_DI + 0x2e) < '\x01') {
    if (*(int *)0x8806 != 0) {
      iVar1 = *(int *)(unaff_DI + 0x2a);
      if (*(int *)0x8808 < iVar1) {
        *(undefined2 *)(unaff_DI + 0x2a) = 0;
        iVar1 = 0;
      }
      iVar1 = iVar1 * 4;
      if ((((*(int *)(unaff_DI + 4) + -0xf < *(int *)(iVar1 + -0x7822)) &&
           (*(int *)(iVar1 + -0x7822) < *(int *)(unaff_DI + 4) + 0xf)) &&
          (*(int *)(iVar1 + -0x7820) < *(int *)(unaff_DI + 8) + 5)) &&
         (*(int *)(unaff_DI + 8) + -0x19 < *(int *)(iVar1 + -0x7820))) {
        *(undefined2 *)(iVar1 + -0x7822) = 0;
        *(int *)(unaff_DI + 0x2c) = *(int *)(unaff_DI + 0x2c) + 1;
        iVar1 = unaff_DI;
        func_0x0000ffff();
        *(undefined1 *)(unaff_DI + 0x17) = 2;
        *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar1 + 2);
        *(long *)(unaff_DI + 6) = *(long *)(iVar1 + 6) + 0xa0000;
        func_0x0000ffff(0);
        *(undefined2 *)0x612e = 0xd;
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined1 *)(iVar1 + 0x2e) = 1;
        unaff_DI = iVar1;
      }
      *(int *)(unaff_DI + 0x2a) = *(int *)(unaff_DI + 0x2a) + 1;
      if (3 < *(int *)(unaff_DI + 0x2c)) {
        *(char *)0x88ae = *(char *)0x88ae + '\x01';
        *(undefined2 *)(unaff_DI + 0x2c) = 0;
      }
    }
  }
  else {
    *(int *)(unaff_DI + 0x2f) = *(int *)(unaff_DI + 0x2f) + 1;
    if (100 < *(int *)(unaff_DI + 0x2f)) {
      *(undefined2 *)(unaff_DI + 0x2f) = 0;
      *(undefined1 *)(unaff_DI + 0x2e) = 0;
      unaff_CS = 0;
      func_0x0000ffff();
    }
  }
  func_0x0000ffff(unaff_CS);
  return;
}



/* TARGET w5_main_boss_callback at 0xD63D; resolved function entry 0000:d63d */
/* CALLERS of 0000:d63d: */
/*   <none resolved> */

void w5_main_boss_callback(void)

{
  int *piVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_CS;
  undefined2 unaff_DS;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  
  if ('\x02' < *(char *)0x88ae) {
    if (*(char *)0x88ae < '\x04') {
      *(undefined2 *)(*(int *)(unaff_DI + 0x36) + 0x18) = 0;
      if (*(int *)(unaff_DI + 0x12) == 900) {
        *(undefined2 *)(unaff_DI + 0x12) = 900;
      }
      else {
        *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
      }
      *(undefined1 *)0x88ae = 4;
      return;
    }
    if (*(char *)0x88ae < '\x05') {
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) - *(int *)(unaff_DI + 0x2c);
      uVar3 = *(int *)(unaff_DI + 0x2e) + 0x20U & 0x5ff;
      *(uint *)(unaff_DI + 0x2e) = uVar3;
      iVar4 = (int)(*(char *)(uVar3 + 0x7974) >> 5);
      *(int *)(unaff_DI + 0x2c) = iVar4;
      *(int *)(unaff_DI + 8) = *(int *)(unaff_DI + 8) + iVar4;
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x1a) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x38) = 0;
      iVar8 = unaff_DI;
      func_0x0000ffff();
      *(undefined1 *)(unaff_DI + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 4) = *(int *)(iVar8 + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) =
           *(int *)(iVar8 + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x1e;
      unaff_CS = 0;
      unaff_DI = iVar8;
      func_0x0000ffff(0);
      *(undefined1 *)(iVar8 + 0x17) = 2;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 4) = *(int *)(unaff_DI + 4) + (char)(*(byte *)(iVar4 + 0x646c) >> 2) + -0x20;
      iVar4 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(iVar8 + 8) =
           *(int *)(unaff_DI + 8) + (int)(char)(*(byte *)(iVar4 + 0x646c) >> 3) + -0x1e;
      *(int *)(unaff_DI + 0x44) = *(int *)(unaff_DI + 0x44) + 1;
      if (*(int *)(unaff_DI + 0x44) < 0x10) {
        return;
      }
      *(undefined4 *)(unaff_DI + 0xe) = 0xffff0000;
      *(undefined1 *)0x88ae = 5;
    }
    if (*(char *)0x88ae < '\x06') {
      *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
      if (*(int *)(unaff_DI + 0x38) < 0x29) {
        return;
      }
      lVar5 = *(long *)(unaff_DI + 0xe);
      *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + -0x1200;
      *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar5;
      if (((*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U < 0x161) &&
         ((*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U < 0xd1)) {
        return;
      }
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    *(undefined1 *)0x88ae = 6;
    iVar4 = unaff_DI;
    func_0x0000ffff(unaff_CS);
    *(undefined1 *)(unaff_DI + 0x17) = 1;
    *(undefined2 *)(unaff_DI + 4) = 0x203;
    *(undefined2 *)(unaff_DI + 8) = *(undefined2 *)(iVar4 + 8);
    return;
  }
  func_0x0000ffff();
  iVar4 = -0x41;
  if (-1 < *(char *)(unaff_DI + 0x28)) {
    iVar4 = 0x41;
  }
  bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
  func_0x0000ffff(0);
  uVar7 = false;
  if (bVar6) {
    iVar4 = -0x41;
    if (-1 < *(char *)(unaff_DI + 0x28)) {
      iVar4 = 0x41;
    }
    bVar6 = *(int *)(unaff_DI + 4) + iVar4 == 0;
    func_0x0000ffff(0);
    uVar7 = false;
    if (bVar6) {
      iVar4 = -0x41;
      if (-1 < *(char *)(unaff_DI + 0x28)) {
        iVar4 = 0x41;
      }
      uVar7 = *(int *)(unaff_DI + 4) + iVar4 == 0;
      func_0x0000ffff(0);
    }
  }
  if (!(bool)uVar7) {
    *(undefined1 *)(unaff_DI + 0x3e) = 1;
  }
  if (*(char *)(unaff_DI + 0x34) < '\x01') {
    if ('\0' < *(char *)(unaff_DI + 0x3e)) {
      if (*(char *)0x88ae < '\x02') {
        if (*(char *)(unaff_DI + 0x40) < '\0') {
          lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x400;
          if (lVar5 < -0x13000) {
            lVar5 = -0x13000;
          }
          else if (0x13000 < lVar5) {
            lVar5 = 0x13000;
          }
          *(long *)(unaff_DI + 10) = lVar5;
          *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
          piVar1 = (int *)(unaff_DI + 0x42);
          iVar4 = *piVar1;
          *piVar1 = *piVar1 + -1;
          if (SBORROW2(iVar4,1) != *piVar1 < 0) {
            *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
            *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
            *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
            if (*(int *)(unaff_DI + 0x12) == 900) {
              *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
            }
            else {
              *(undefined2 *)(unaff_DI + 0x12) = 900;
            }
            iVar4 = *(int *)(unaff_DI + 0x2a);
            if (*(int *)(iVar4 + 0x12) == 0x3bb) {
              *(undefined2 *)(iVar4 + 0x12) = 0x389;
            }
            else {
              *(undefined2 *)(iVar4 + 0x12) = 0x3bb;
            }
            *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
            *(undefined2 *)(unaff_DI + 0x42) = 0x28;
          }
        }
        else {
          lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x1000;
          if (lVar5 < -0x13000) {
            lVar5 = -0x13000;
          }
          else if (0x13000 < lVar5) {
            lVar5 = 0x13000;
          }
          *(long *)(unaff_DI + 10) = lVar5;
          *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
          piVar1 = (int *)(unaff_DI + 0x42);
          iVar4 = *piVar1;
          *piVar1 = *piVar1 + -1;
          if (SBORROW2(iVar4,1) != *piVar1 < 0) {
            *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
            *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
            *(undefined2 *)(unaff_DI + 0x42) = 0x14;
          }
        }
      }
      else if (*(char *)(unaff_DI + 0x40) < '\0') {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * -0x2000;
        if (lVar5 < -0x16000) {
          lVar5 = -0x16000;
        }
        else if (0x16000 < lVar5) {
          lVar5 = 0x16000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x29) = -*(char *)(unaff_DI + 0x29);
          *(char *)(unaff_DI + 0x28) = -*(char *)(unaff_DI + 0x28);
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          if (*(int *)(unaff_DI + 0x12) == 900) {
            *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
          }
          else {
            *(undefined2 *)(unaff_DI + 0x12) = 900;
          }
          iVar4 = *(int *)(unaff_DI + 0x2a);
          if (*(int *)(iVar4 + 0x12) == 0x3bb) {
            *(undefined2 *)(iVar4 + 0x12) = 0x389;
          }
          else {
            *(undefined2 *)(iVar4 + 0x12) = 0x3bb;
          }
          *(long *)(unaff_DI + 10) = (long)(int)*(char *)(unaff_DI + 0x29) << 9;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
      else {
        lVar5 = *(long *)(unaff_DI + 10) + (long)(int)*(char *)(unaff_DI + 0x29) * 0x2000;
        if (lVar5 < -0x16000) {
          lVar5 = -0x16000;
        }
        else if (0x16000 < lVar5) {
          lVar5 = 0x16000;
        }
        *(long *)(unaff_DI + 10) = lVar5;
        *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + lVar5;
        piVar1 = (int *)(unaff_DI + 0x42);
        iVar4 = *piVar1;
        *piVar1 = *piVar1 + -1;
        if (SBORROW2(iVar4,1) != *piVar1 < 0) {
          *(char *)(unaff_DI + 0x40) = -*(char *)(unaff_DI + 0x40);
          *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
          *(undefined2 *)(unaff_DI + 0x42) = 0x14;
        }
      }
      goto LAB_0000_d96f;
    }
    *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + *(long *)(unaff_DI + 10);
    if (*(char *)0x88ae < '\x02') goto LAB_0000_d96f;
  }
  *(int *)(unaff_DI + 0x38) = *(int *)(unaff_DI + 0x38) + 1;
  if (0x3c < *(int *)(unaff_DI + 0x38)) {
    iVar4 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 0x38) = (int)(char)(*(byte *)(iVar4 + 0x646c) >> 4);
    iVar4 = unaff_DI;
    func_0x0000ffff(0);
    *(undefined1 *)(unaff_DI + 0x17) = 2;
    *(undefined1 *)(unaff_DI + 0x29) = *(undefined1 *)(iVar4 + 0x29);
    *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(iVar4 + 2);
    *(long *)(unaff_DI + 6) = *(long *)(iVar4 + 6) + -0x500000;
    *(undefined1 *)(iVar4 + 0x34) = 0;
    unaff_DI = iVar4;
  }
LAB_0000_d96f:
  iVar4 = *(int *)(unaff_DI + 0x36);
  lVar5 = *(long *)(unaff_DI + 2);
  cVar2 = *(char *)(unaff_DI + 0x28);
  *(char *)(iVar4 + 0x28) = cVar2;
  if (cVar2 == '\x01') {
    lVar5 = lVar5 + 0x1f0000;
  }
  else {
    lVar5 = lVar5 + -0x1f0000;
  }
  *(long *)(iVar4 + 2) = lVar5;
  *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  iVar4 = *(int *)(unaff_DI + 0x2a);
  lVar5 = *(long *)(unaff_DI + 2);
  cVar2 = *(char *)(unaff_DI + 0x28);
  *(char *)(iVar4 + 0x28) = cVar2;
  if (cVar2 == '\x01') {
    lVar5 = lVar5 + 0x40000;
  }
  else {
    lVar5 = lVar5 + -0x40000;
  }
  *(long *)(iVar4 + 2) = lVar5;
  *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + -0x370000;
  iVar4 = *(int *)(unaff_DI + 0x48);
  *(undefined4 *)(iVar4 + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(iVar4 + 6) = *(long *)(unaff_DI + 6) + 0x280000;
  uVar3 = *(int *)(iVar4 + 0x2e) + 0x14U & 0x7ff;
  *(uint *)(iVar4 + 0x2e) = uVar3;
  *(int *)(iVar4 + 8) = *(int *)(iVar4 + 8) + (int)(*(char *)(uVar3 + 0x7974) >> 3);
  *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar4 + 8);
  return;
}



/* TARGET w5_main_constructor_call_site at 0xD2D0; resolved function entry 0000:d2d0 */
/* CALLERS of 0000:d2d0: */
/*   <none resolved> */

void w5_main_constructor_call_site(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  func_0x0000ffff();
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  iVar1 = *(int *)0x81c4;
  *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 400;
  *(int *)(unaff_DI + 8) = iVar1 + 0x14;
  return;
}



/* TARGET w5_doktor_constructor_dispatch at 0xD379; resolved function entry 0000:d379 */
/* CALLERS of 0000:d379: */
/*   <none resolved> */

void w5_doktor_constructor_dispatch(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  int in_stack_00000000;
  
  func_0x0000ffff();
  *(int *)(in_stack_00000000 + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(in_stack_00000000 + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(in_stack_00000000 + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(in_stack_00000000 + 0x48) = in_stack_00000000;
  *(undefined1 *)(in_stack_00000000 + 0x17) = 1;
  *(undefined4 *)(in_stack_00000000 + 2) = *(undefined4 *)(in_stack_00000000 + 2);
  *(long *)(in_stack_00000000 + 6) = *(long *)(in_stack_00000000 + 6) + 0x280000;
  return;
}



/* TARGET w5_main_boss_constructor at 0xD2F6; resolved function entry 0000:d2f6 */
/* CALLERS of 0000:d2f6: */
/*   <none resolved> */

void w5_main_boss_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w5_main_boss_callback;
  *(undefined2 *)(unaff_DI + 0x12) = 0x3b6;
  *(undefined1 *)(unaff_DI + 0x28) = 0xff;
  *(undefined1 *)(unaff_DI + 0x29) = 0xff;
  *(undefined2 *)(unaff_DI + 0x38) = 0;
  *(undefined2 *)(unaff_DI + 0x46) = 0;
  *(undefined2 *)(unaff_DI + 0x44) = 0;
  *(undefined1 *)(unaff_DI + 0x34) = 0;
  *(undefined4 *)(unaff_DI + 10) = 0xfffed000;
  *(undefined1 *)(unaff_DI + 0x40) = 0xff;
  *(undefined2 *)(unaff_DI + 0x42) = 0x14;
  *(undefined1 *)(unaff_DI + 0x3e) = 0xff;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x40000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x370000;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x36) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + -0x1f0000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + -0x1d0000;
  func_0x0000ffff(0);
  *(int *)(unaff_DI + 0x48) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0x280000;
  return;
}



/* TARGET w5_effect_constructor_a at 0xD3E1; resolved function entry 0000:d3e1 */
/* CALLERS of 0000:d3e1: */
/*   <none resolved> */

void w5_effect_constructor_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x3bb;
  *(undefined2 *)(unaff_DI + 0x18) = 0xd3ee;
  return;
}



/* TARGET w5_effect_constructor_b at 0xD420; resolved function entry 0000:d420 */
/* CALLERS of 0000:d420: */
/*   <none resolved> */

void w5_effect_constructor_b(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = 0xd438;
  *(undefined4 *)(unaff_DI + 0xe) = 0x12000;
  return;
}



/* TARGET w5_effect_constructor_c at 0xD498; resolved function entry 0000:d498 */
/* CALLERS of 0000:d498: */
/*   <none resolved> */

void w5_effect_constructor_c(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x12) = 0x38a;
  *(undefined2 *)(unaff_DI + 0x18) = 0xd4d9;
  *(undefined4 *)(unaff_DI + 0xe) = 0x12000;
  func_0x0000ffff();
  *(int *)(unaff_DI + 0x2a) = unaff_DI;
  *(undefined1 *)(unaff_DI + 0x17) = 1;
  *(undefined4 *)(unaff_DI + 2) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(unaff_DI + 6) = *(undefined4 *)(unaff_DI + 6);
  return;
}



/* TARGET w5_doktor_constructor at 0xD53F; resolved function entry 0000:d53f */
/* CALLERS of 0000:d53f: */
/*   <none resolved> */

void w5_doktor_constructor(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  
  func_0x0000ffff();
  *(undefined2 *)(unaff_DI + 0x18) = (code *)w5_doktor_damage_callback;
  *(undefined2 *)(unaff_DI + 0x2a) = 0;
  *(undefined2 *)(unaff_DI + 0x2c) = 0;
  return;
}



/* TARGET w5_scrap_constructor_a at 0xDFB6; resolved function entry 0000:dfb6 */
/* CALLERS of 0000:dfb6: */
/*   <none resolved> */

void w5_scrap_constructor_a(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = 0xe0f5;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x4000;
  *(undefined4 *)(unaff_DI + 10) = 0xffffd000;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0x60000;
  return;
}



/* TARGET w5_scrap_constructor_b at 0xE020; resolved function entry 0000:e020 */
/* CALLERS of 0000:e020: */
/*   <none resolved> */

void w5_scrap_constructor_b(void)

{
  int iVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  
  *(undefined2 *)(unaff_DI + 0x18) = 0xe2bf;
  *(undefined1 *)(unaff_DI + 0x29) = 1;
  *(undefined1 *)(unaff_DI + 0x28) = 1;
  iVar1 = func_0x0000ffff();
  *(int *)(unaff_DI + 0x2e) = iVar1;
  *(long *)(unaff_DI + 0xe) = (long)iVar1 * 0x80 + -0x8000;
  *(undefined4 *)(unaff_DI + 10) = 0x4000;
  func_0x0000ffff(0);
  *(undefined1 *)(unaff_DI + 0x17) = 2;
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) + 0x50000;
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + 0x60000;
  return;
}



/* TARGET w5_boss_effect_callback_a at 0xD3EE; resolved function entry 0000:d3ee */
/* CALLERS of 0000:d3ee: */
/*   <none resolved> */

void w5_boss_effect_callback_a(void)

{
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if ((*(char *)0x88ae == '\x05') &&
     ((0x200 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x60U ||
      (0x170 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x60U)))) {
    *(undefined2 *)(unaff_DI + 0x18) = 0;
  }
  return;
}



/* TARGET w5_boss_effect_callback_b at 0xD4D9; resolved function entry 0000:d4d9 */
/* CALLERS of 0000:d4d9: */
/*   <none resolved> */

void w5_boss_effect_callback_b(void)

{
  int iVar1;
  long lVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  iVar1 = *(int *)(unaff_DI + 0x2a);
  *(undefined4 *)(iVar1 + 2) = *(undefined4 *)(unaff_DI + 2);
  *(undefined4 *)(iVar1 + 6) = *(undefined4 *)(unaff_DI + 6);
  if (*(char *)0x88ae == '\x04') {
    if ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U) ||
       (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)) {
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    lVar2 = *(long *)(unaff_DI + 0xe);
    *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + 3000;
    *(long *)(unaff_DI + 0xe) = lVar2;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar2;
  }
  return;
}



/* TARGET w5_boss_effect_callback_c at 0xD438; resolved function entry 0000:d438 */
/* CALLERS of 0000:d438: */
/*   <none resolved> */

void w5_boss_effect_callback_c(void)

{
  long lVar1;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  if (*(char *)0x88ae == '\x04') {
    if ((0x160 < (*(int *)(unaff_DI + 4) - *(int *)0x81c0) + 0x10U) ||
       (0xd0 < (*(int *)(unaff_DI + 8) - *(int *)0x81c4) + 0x10U)) {
      *(undefined2 *)(unaff_DI + 0x18) = 0;
    }
    lVar1 = *(long *)(unaff_DI + 0xe);
    *(long *)(unaff_DI + 0xe) = *(long *)(unaff_DI + 0xe) + 3000;
    *(long *)(unaff_DI + 0xe) = lVar1;
    *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) + lVar1;
  }
  func_0x0000ffff();
  func_0x0000ffff(0);
  return;
}



/* TARGET w5_scrap_effect_callback_a at 0xE0F5; resolved function entry 0000:e0f5 */
/* CALLERS of 0000:e0f5: */
/*   <none resolved> */

void w5_scrap_effect_callback_a(void)

{
  uint uVar1;
  int iVar2;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) - *(long *)(unaff_DI + 10);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    w5_random_helper_def2();
    func_0x0000df5d();
    iVar2 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      func_0x0000df5d();
      w5_random_helper_def2();
      iVar2 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar2 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + 200;
      w5_random_helper_def2();
      func_0x0000df5d();
      goto LAB_0000_e1b1;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    *(int *)(unaff_DI + 8) = *(int *)0x81c4;
    w5_random_helper_def2();
    func_0x0000df5d();
  }
LAB_0000_e1b1:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar1 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar1;
  iVar2 = (int)(*(char *)(uVar1 + 0x7974) >> 3);
  *(int *)(unaff_DI + 0x2c) = iVar2;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar2;
  func_0x0000ffff();
  return;
}



/* TARGET w5_scrap_effect_callback_b at 0xE2BF; resolved function entry 0000:e2bf */
/* CALLERS of 0000:e2bf: */
/*   <none resolved> */

void w5_scrap_effect_callback_b(void)

{
  undefined2 uVar1;
  uint uVar2;
  int iVar3;
  int unaff_DI;
  undefined2 unaff_ES;
  undefined2 unaff_DS;
  
  *(long *)(unaff_DI + 6) = *(long *)(unaff_DI + 6) - *(long *)(unaff_DI + 0xe);
  *(long *)(unaff_DI + 2) = *(long *)(unaff_DI + 2) - *(long *)(unaff_DI + 10);
  if (*(int *)(unaff_DI + 4) < *(int *)0x81c0) {
    *(int *)(unaff_DI + 4) = *(int *)0x81c0 + 0x140;
    w5_random_helper_def2();
    iVar3 = *(int *)0x6468;
    *(int *)0x6468 = *(int *)0x6468 + 1;
    *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
    *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar3 + 0x646c) >> 1) + 0x3c;
  }
  else {
    if (*(int *)0x81c0 + 0x140 < *(int *)(unaff_DI + 4)) {
      *(int *)(unaff_DI + 4) = *(int *)0x81c0;
      w5_random_helper_def2();
      iVar3 = *(int *)0x6468;
      *(int *)0x6468 = *(int *)0x6468 + 1;
      *(uint *)0x6468 = *(uint *)0x6468 & 0xff;
      *(int *)(unaff_DI + 8) = *(int *)0x81c4 + ((uint)(int)*(char *)(iVar3 + 0x646c) >> 1) + 0x3c;
    }
    if (*(int *)(unaff_DI + 8) < *(int *)0x81c4) {
      uVar1 = w5_random_helper_def2();
      *(undefined2 *)(unaff_DI + 8) = uVar1;
      goto LAB_0000_e36f;
    }
  }
  if (*(int *)0x81c4 + 200 < *(int *)(unaff_DI + 8)) {
    uVar1 = w5_random_helper_def2();
    *(undefined2 *)(unaff_DI + 8) = uVar1;
  }
LAB_0000_e36f:
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) - *(int *)(unaff_DI + 0x2c);
  uVar2 = *(int *)(unaff_DI + 0x2e) + 10U & 0x7ff;
  *(uint *)(unaff_DI + 0x2e) = uVar2;
  iVar3 = (int)(*(char *)(uVar2 + 0x7974) >> 4);
  *(int *)(unaff_DI + 0x2c) = iVar3;
  *(int *)(unaff_DI + 4) = *(int *)(unaff_DI + 4) + iVar3;
  func_0x0000ffff();
  return;
}



