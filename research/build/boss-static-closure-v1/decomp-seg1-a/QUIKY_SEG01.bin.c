/* Decompiled focused external-state closure from QUIKY_SEG01.bin */
/* Entries are address-qualified; containing functions are reported explicitly. */

/* requested 0x4EA0; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4EAA; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4F10; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x5010; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4BA4; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4BAE; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4BD8; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4C43; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



/* requested 0x4CB8; function player_external_4EA0 at 0x20128 */

void player_external_4EA0(void)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  int unaff_BP;
  undefined2 unaff_CS;
  undefined2 unaff_SS;
  undefined2 unaff_DS;
  undefined2 uVar4;
  undefined2 uVar5;
  
code_r0x00004ea0:
  if (*(int *)0x89e6 != 0) {
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(unaff_CS,0,1,9,0x4533,unaff_CS);
    *(undefined2 *)(unaff_BP + -0x406) = 0x14f;
    if (*(int *)0x85d4 == 0xe) {
      *(undefined2 *)0x89e0 = 0xffff;
    }
    if (*(char *)0x5044 == '\0') {
      uVar4 = 0x46;
      func_0x0000ffff(0,0x46);
    }
    else {
      uVar4 = *(undefined2 *)(unaff_BP + -0x406);
      func_0x0000ffff(0,uVar4);
    }
    func_0x0000ffff(0,uVar4);
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x000014e1();
    if (*(char *)0x85db == '\0') {
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0x10) {
        if (*(int *)0x85d6 == 0) {
          *(undefined2 *)0x85d4 = 1;
        }
        else {
          *(undefined2 *)0x85d4 = 2;
        }
      }
      else if (iVar2 == 0x11) {
        if (*(int *)0x85d6 == 3) {
          *(undefined2 *)0x85d4 = 4;
        }
        else {
          *(undefined2 *)0x85d4 = 5;
        }
      }
      else if (iVar2 == 0x12) {
        if (*(int *)0x85d6 == 6) {
          *(undefined2 *)0x85d4 = 7;
        }
        else {
          *(undefined2 *)0x85d4 = 8;
        }
      }
      else if (iVar2 == 0x13) {
        if (*(int *)0x85d6 == 9) {
          *(undefined2 *)0x85d4 = 10;
        }
        else {
          *(undefined2 *)0x85d4 = 0xb;
        }
      }
      else if (iVar2 == 0x14) {
        if (*(int *)0x85d6 == 0xc) {
          *(undefined2 *)0x85d4 = 0xd;
        }
        else {
          *(undefined2 *)0x85d4 = 0xe;
        }
      }
      else {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
      }
    }
    else {
      *(undefined2 *)0x85d6 = *(undefined2 *)0x85d4;
      iVar2 = *(int *)0x85d4;
      if ((iVar2 == 0) || (iVar2 == 1)) {
        *(undefined2 *)0x85d4 = 0x10;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x85d4 = 0x11;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x85d4 = 0x12;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x85d4 = 0x13;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x85d4 = 0x14;
      }
    }
    if (*(int *)0x89e0 != -1) {
      func_0x0000ffff(0);
      cVar3 = func_0x00003fad();
      if (cVar3 == '\0') {
        *(undefined2 *)0x89ec = 0xffff;
      }
      func_0x0000ffff(0,0x400,unaff_BP + -0x400);
      func_0x0000ffff(0);
      unaff_CS = 0;
      func_0x0000ffff(0);
      func_0x0000313d();
      *(undefined2 *)0x89e6 = 0;
    }
  }
LAB_0000_504f:
  do {
    if ((*(int *)0x89ec != 0) || (*(int *)0x89e0 == -1)) {
      func_0x0000ffff(unaff_CS,1,0x23);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
      return;
    }
    *(undefined2 *)0x85d2 = 0;
    if (*(char *)0x85da == '\x01') {
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      *(undefined1 *)(unaff_BP + -0x408) = 1;
    }
    if (*(char *)(unaff_BP + -0x407) != '\0') {
      iVar2 = *(int *)0x85d4;
      if (((iVar2 == 0) || (iVar2 == 1)) || (iVar2 == 0xf)) {
        *(undefined2 *)0x5042 = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        *(undefined2 *)0x5042 = 1;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        *(undefined2 *)0x5042 = 2;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        *(undefined2 *)0x5042 = 3;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        *(undefined2 *)0x5042 = 4;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        *(undefined2 *)0x5042 = 6;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        *(undefined2 *)0x5042 = 5;
      }
      *(undefined1 *)0x613f = 0;
      func_0x0000ffff(unaff_CS,1,1,7,0x4533,unaff_CS);
      *(undefined2 *)(unaff_BP + -0x406) = 0x14;
      *(undefined1 *)(unaff_BP + -0x407) = 0;
      unaff_CS = 0;
    }
    if (*(char *)(unaff_BP + -0x408) != '\0') {
      *(undefined1 *)(unaff_BP + -0x408) = 0;
      iVar2 = *(int *)0x85d4;
      if (iVar2 == 0xf) {
        func_0x0000ffff(unaff_CS,0,1,1,0x453b,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0) || (iVar2 == 1)) {
        func_0x0000ffff(unaff_CS,0,0,0,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 3) || (iVar2 == 4)) {
        func_0x0000ffff(unaff_CS,0,0,1,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 6) || (iVar2 == 7)) {
        func_0x0000ffff(unaff_CS,0,0,2,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 9) || (iVar2 == 10)) {
        func_0x0000ffff(unaff_CS,0,0,3,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((iVar2 == 0xc) || (iVar2 == 0xd)) {
        func_0x0000ffff(unaff_CS,0,0,4,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if ((((iVar2 == 2) || (iVar2 == 5)) || (iVar2 == 8)) ||
              ((iVar2 == 0xb || (iVar2 == 0xe)))) {
        func_0x0000ffff(unaff_CS,0,0,6,0x4533,unaff_CS);
        unaff_CS = 0;
      }
      else if (((iVar2 == 0x10) || (iVar2 == 0x11)) ||
              ((iVar2 == 0x12 || ((iVar2 == 0x13 || (iVar2 == 0x14)))))) {
        func_0x0000ffff(unaff_CS,0,0,5,0x4533,unaff_CS);
        unaff_CS = 0;
      }
    }
    *(undefined2 *)0x8196 = 0;
    *(undefined2 *)0x88bc = 0;
    func_0x0000ffff(unaff_CS);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    *(undefined2 *)0x89e4 = 0;
    *(undefined2 *)0x89ee = 0;
    if (*(char *)(unaff_BP + -0x403) != '\0') {
      func_0x0000ffff(0,0x23,unaff_BP + -0x400);
    }
    *(undefined2 *)0x8952 = 0xffff;
    do {
      while( true ) {
        while( true ) {
          do {
            if (*(int *)(unaff_BP + -0x406) != 0) {
              piVar1 = (int *)(unaff_BP + -0x406);
              *piVar1 = *piVar1 + -1;
              if (*piVar1 == 0) {
                func_0x0000ffff(0);
                *(undefined1 *)0x613f = *(undefined1 *)(unaff_BP + -0x409);
              }
            }
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            func_0x0000ffff(0);
            uVar4 = *(undefined2 *)0x817c;
            *(undefined2 *)0x817c = *(undefined2 *)0x817a;
            *(undefined2 *)0x817a = uVar4;
            *(undefined1 *)0x81d1 = 1;
            *(undefined2 *)0x819e = 0;
            do {
            } while (*(int *)0x819e == 0);
            func_0x0000ffff(0);
            unaff_CS = 0;
            func_0x0000ffff(0);
            if (*(int *)0x88ba == 0x19) {
              *(undefined2 *)0x89ee = 0xffff;
              goto LAB_0000_4968;
            }
            if ((*(int *)0x89ec == -1) || (*(int *)0x89e6 == -1)) goto LAB_0000_4968;
            if (*(int *)0x88b2 != 0) {
              *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
            }
            if (*(int *)0x88b4 != 0) {
              *(undefined2 *)0x880c = 99;
            }
            if (*(int *)0x88ba == 1) {
              *(undefined2 *)0x89f0 = 0xffff;
              goto LAB_0000_4968;
            }
          } while (*(char *)0x89f2 == '\0');
          if (*(int *)0x88ba != 2) break;
          *(undefined2 *)0x8822 = *(undefined2 *)0x8824;
        }
        if (*(int *)0x88ba == 3) {
          *(undefined2 *)0x89e6 = 0xffff;
          goto LAB_0000_4968;
        }
        if (*(int *)0x88ba != 4) break;
        *(undefined2 *)0x880c = 99;
      }
    } while (*(int *)0x88ba != 5);
    *(undefined2 *)0x89f4 = 0xffff;
LAB_0000_4968:
    *(undefined2 *)0x89ec = 0;
    *(undefined2 *)0x8952 = 0;
    *(undefined1 *)(unaff_BP + -0x403) = 1;
    if (*(int *)0x89f4 != -1) {
      if (*(int *)0x89ee == -1) {
        func_0x000001d6();
        *(undefined2 *)0x88ba = 0;
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89ea != 0) && (*(int *)0x880a != 0)) {
        func_0x0000ffff(0,0,0x23);
        *(undefined2 *)0x8810 = 0;
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x000044d0();
        func_0x0000ffff(0);
        *(undefined2 *)0x85d2 = 0;
        if (*(int *)0x880a == 0) {
          *(undefined2 *)0x89ec = 0xffff;
        }
        else {
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          iVar2 = *(int *)0x85d4;
          if (iVar2 == 2) {
            func_0x00003861();
          }
          else if (iVar2 == 5) {
            func_0x00003861();
          }
          else if (iVar2 == 8) {
            func_0x00003861();
          }
          else if (iVar2 == 0xb) {
            func_0x00003861();
          }
          else if (iVar2 == 0xe) {
            func_0x00003861();
          }
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000ffff(0);
          func_0x0000313d();
        }
        unaff_CS = 0;
        goto LAB_0000_504f;
      }
      if (((*(int *)0x89ea != 0) && (*(int *)0x880a == 0)) ||
         ((*(int *)0x89f0 != 0 && (*(int *)0x880a == 1)))) {
        *(undefined2 *)0x8810 = 0;
        *(undefined2 *)0x88ba = 0;
        *(undefined2 *)0x880a = 0;
        func_0x0000ffff(0,1,1,8,0x4533,0);
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff(0,1,0x23);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        func_0x00000c2c();
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if ((*(int *)0x89f0 != 0) && (*(int *)0x89ea == 0)) {
        *(undefined2 *)0x8810 = 0;
        func_0x000044d0();
        *(undefined1 *)(unaff_BP + -0x407) = 1;
        func_0x0000ffff();
        func_0x0000ffff(0,1);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        func_0x0000ffff(0);
        unaff_CS = 0;
        func_0x0000ffff(0);
        *(undefined2 *)0x89ec = 0xffff;
        goto LAB_0000_504f;
      }
      if (1 < *(byte *)0x85da) {
        cVar3 = *(char *)0x85da;
        if (cVar3 == '\x02') {
          func_0x0000ffff(0,0);
          *(undefined2 *)0x60d2 = 0x127;
          *(undefined2 *)0x60d4 = 0x118;
          *(undefined2 *)0x60d6 = 0x15c;
          func_0x000044f0();
          func_0x0000ffff(0,1);
          func_0x0000ffff(0,2);
          func_0x0000ffff(0,3);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x04') {
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,5);
          func_0x0000ffff(0,6);
          func_0x0000ffff(0,7);
          func_0x0000ffff(0,8);
          func_0x0000ffff(0,9);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\x06') {
          *(undefined2 *)0x60d6 = 0x15d;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0);
          *(undefined2 *)0x60d2 = 0xb4;
          *(undefined2 *)0x60d4 = 300;
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0xe);
          *(undefined2 *)0x60d2 = 0x152;
          *(undefined2 *)0x60d4 = 0x123;
          func_0x000044f0();
          func_0x0000ffff(0,0xf);
          *(undefined2 *)0x60d6 = 0;
        }
        else if (cVar3 == '\a') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        else if (cVar3 == '2') {
          func_0x000044f0();
          func_0x0000ffff(0);
          func_0x0000ffff(0,0x11);
          func_0x0000ffff(0,0x12);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x13);
          func_0x0000ffff(0,0x14);
          func_0x0000ffff(0,0x15);
          *(int *)0x60d2 = *(int *)0x60d2 + -0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x16);
          *(int *)0x60d2 = *(int *)0x60d2 + 0x28;
          func_0x000044f0();
          func_0x0000ffff(0,0x17);
          func_0x0000ffff(0,0x18);
          *(undefined2 *)0x60d6 = 0x15b;
        }
        else if (cVar3 == '4') {
          *(undefined2 *)0x89e0 = 0xffff;
        }
        unaff_CS = 0;
        *(char *)0x85da = *(char *)0x85da + '\x01';
        *(undefined1 *)(unaff_BP + -0x403) = 0;
        goto LAB_0000_504f;
      }
      goto code_r0x00004ea0;
    }
    *(undefined2 *)0x8810 = 0;
    func_0x0000ffff(0);
    *(undefined2 *)0x89f4 = 0;
    *(undefined1 *)(unaff_BP + -0x407) = 1;
    func_0x0000ffff(0,1,0x23);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,3);
    func_0x0000ffff(0,1000);
    func_0x0000ffff(0,0);
    func_0x0000ffff(0,0xe);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0xd,1);
    func_0x0000ffff(0,0,0x4541,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x20,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0,0,0x4575,0,0x9a30);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    do {
      if (0x14 < *(uint *)0x85d4) {
        *(undefined2 *)0x85d4 = 0;
      }
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0xe);
      func_0x0000ffff(0,0x10,0x1b);
      func_0x0000ffff(0,0x19,*(int *)0x85d4 * 0x15 + 0x10);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,0);
      func_0x0000ffff(0,1,1);
      func_0x0000ffff(0,0,0x20,0x9a30);
      func_0x0000ffff(0);
      func_0x0000ffff(0);
      uVar5 = 1;
      uVar4 = 1;
      func_0x0000ffff(0,1,1);
      *(undefined2 *)(unaff_BP + -0x40c) = *(undefined2 *)0x88bc;
      func_0x0000ffff(0);
      do {
      } while (*(int *)0x88bc == *(int *)(unaff_BP + -0x40c));
      func_0x0000ffff(0,uVar4,uVar5);
      if (*(int *)0x88bc == 2) {
        *(int *)0x85d4 = *(int *)0x85d4 + 1;
        if (*(int *)0x85d4 == 0xf) {
          *(undefined2 *)0x85d4 = 0x10;
        }
      }
      else if ((*(int *)0x88bc == 1) &&
              (*(int *)0x85d4 = *(int *)0x85d4 + -1, *(int *)0x85d4 == 0xf)) {
        *(undefined2 *)0x85d4 = 0xe;
      }
    } while (*(int *)0x88bc != 0x20);
    iVar2 = *(int *)0x85d4;
    if ((((iVar2 == 0x10) || (iVar2 == 0x11)) || (iVar2 == 0x12)) ||
       ((iVar2 == 0x13 || (iVar2 == 0x14)))) {
      *(undefined1 *)0x85db = 1;
    }
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    func_0x0000ffff(0);
    cVar3 = func_0x00003fad();
    if (cVar3 == '\0') {
      *(undefined2 *)0x89ec = 0xffff;
    }
    func_0x0000ffff(0,0x400,unaff_BP + -0x400);
    *(undefined2 *)0x85d2 = 0;
    func_0x0000ffff(0);
    unaff_CS = 0;
    func_0x0000ffff(0);
    func_0x0000313d();
    *(undefined2 *)0x89f0 = 0;
    *(undefined2 *)0x89e0 = 0;
    *(undefined2 *)0x89e6 = 0;
  } while( true );
}



