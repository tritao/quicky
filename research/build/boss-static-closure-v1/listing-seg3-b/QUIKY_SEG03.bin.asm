; ---- B142 count=192 ----
0000:b142  MOV byte ptr ES:[DI + 0x17],0x1
0000:b147  MOV word ptr ES:[DI + 0x18],0xb33b
0000:b14d  MOV word ptr ES:[DI + 0x12],0x3b7
0000:b153  MOV byte ptr ES:[DI + 0x28],0xff
0000:b158  MOV byte ptr ES:[DI + 0x29],0xff
0000:b15d  MOV word ptr ES:[DI + 0x38],0x0
0000:b163  MOV word ptr ES:[DI + 0x44],0x0
0000:b169  MOV byte ptr ES:[DI + 0x34],0x0
0000:b16e  MOV dword ptr ES:[DI + 0xa],0xffff9000
0000:b177  MOV byte ptr ES:[DI + 0x40],0xff
0000:b17c  MOV word ptr ES:[DI + 0x42],0x14
0000:b182  MOV byte ptr ES:[DI + 0x3e],0xff
0000:b187  PUSH DI
0000:b188  MOV AX,0xb1f0
0000:b18b  XOR DX,DX
0000:b18d  CALLF 0x0000:ffff
0000:b192  POP SI
0000:b193  MOV word ptr ES:[SI + 0x2a],DI
0000:b197  MOV byte ptr ES:[DI + 0x17],0x2
0000:b19c  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b1a1  SUB EAX,0x60000
0000:b1a7  MOV dword ptr ES:[DI + 0x2],EAX
0000:b1ac  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b1b1  SUB EAX,0x1d0000
0000:b1b7  MOV dword ptr ES:[DI + 0x6],EAX
0000:b1bc  MOV DI,SI
0000:b1be  PUSH DI
0000:b1bf  MOV AX,0xb20b
0000:b1c2  XOR DX,DX
0000:b1c4  CALLF 0x0000:ffff
0000:b1c9  POP SI
0000:b1ca  MOV word ptr ES:[SI + 0x36],DI
0000:b1ce  MOV byte ptr ES:[DI + 0x17],0x2
0000:b1d3  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b1d8  SUB EAX,0x1f0000
0000:b1de  MOV dword ptr ES:[DI + 0x2],EAX
0000:b1e3  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b1e8  MOV dword ptr ES:[DI + 0x6],EAX
0000:b1ed  MOV DI,SI
0000:b1ef  RET
0000:b1f0  MOV SI,0x3424
0000:b1f3  CALLF 0x0000:ffff
0000:b1f8  MOV word ptr ES:[DI + 0x18],0xb226
0000:b1fe  MOV word ptr ES:[DI + 0x2e],0x0
0000:b204  MOV word ptr ES:[DI + 0x2c],0x0
0000:b20a  RET
0000:b20b  MOV SI,0x32fa
0000:b20e  CALLF 0x0000:ffff
0000:b213  MOV word ptr ES:[DI + 0x18],0xb25d
0000:b219  MOV word ptr ES:[DI + 0x2a],0x0
0000:b21f  MOV word ptr ES:[DI + 0x2c],0x0
0000:b225  RET
0000:b25d  CMP byte ptr ES:[DI + 0x2e],0x1
0000:b262  JGE 0x0000:b315
0000:b266  CMP word ptr [0x8806],0x0
0000:b26b  JZ 0x0000:b335
0000:b26f  MOV BX,word ptr ES:[DI + 0x2a]
0000:b273  CMP BX,word ptr [0x8808]
0000:b277  JLE 0x0000:b281
0000:b279  MOV word ptr ES:[DI + 0x2a],0x0
0000:b27f  XOR BX,BX
0000:b281  SHL BX,0x2
0000:b284  MOV AX,word ptr ES:[DI + 0x4]
0000:b288  SUB AX,0xf
0000:b28b  CMP word ptr [BX + 0x87de],AX
0000:b28f  JLE 0x0000:b2b8
0000:b291  ADD AX,0x1e
0000:b294  CMP word ptr [BX + 0x87de],AX
0000:b298  JGE 0x0000:b2b8
0000:b29a  MOV AX,word ptr ES:[DI + 0x8]
0000:b29e  ADD AX,0x5
0000:b2a1  CMP word ptr [BX + 0x87e0],AX
0000:b2a5  JGE 0x0000:b2b8
0000:b2a7  SUB AX,0x1e
0000:b2aa  CMP word ptr [BX + 0x87e0],AX
0000:b2ae  JLE 0x0000:b2b8
0000:b2b0  MOV word ptr [BX + 0x87de],0x0
0000:b2b6  JMP 0x0000:b2ba
0000:b2b8  JMP 0x0000:b303
0000:b2ba  INC word ptr ES:[DI + 0x2c]
0000:b2be  PUSH DI
0000:b2bf  MOV AX,0x4b70
0000:b2c2  XOR DX,DX
0000:b2c4  CALLF 0x0000:ffff
0000:b2c9  POP SI
0000:b2ca  MOV byte ptr ES:[DI + 0x17],0x2
0000:b2cf  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b2d4  MOV dword ptr ES:[DI + 0x2],EAX
0000:b2d9  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b2de  ADD EAX,0xa0000
0000:b2e4  MOV dword ptr ES:[DI + 0x6],EAX
0000:b2e9  MOV DI,SI
0000:b2eb  MOV SI,0x3308
0000:b2ee  CALLF 0x0000:ffff
0000:b2f3  MOV word ptr [0x612e],0xd
0000:b2f9  CALLF 0x0000:ffff
0000:b2fe  MOV byte ptr ES:[DI + 0x2e],0x1
0000:b303  INC word ptr ES:[DI + 0x2a]
0000:b307  CMP word ptr ES:[DI + 0x2c],0x4
0000:b30c  JLE 0x0000:b335
0000:b30e  MOV byte ptr [0x88ae],0x2
0000:b313  JMP 0x0000:b335
0000:b315  INC word ptr ES:[DI + 0x2f]
0000:b319  CMP word ptr ES:[DI + 0x2f],0x64
0000:b31e  JLE 0x0000:b335
0000:b320  MOV word ptr ES:[DI + 0x2f],0x0
0000:b326  MOV byte ptr ES:[DI + 0x2e],0x0
0000:b32b  MOV SI,0x32fa
0000:b32e  CALLF 0x0000:ffff
0000:b333  JMP 0x0000:b335
0000:b335  CALLF 0x0000:ffff
0000:b33a  RET
0000:b33b  CMP byte ptr [0x88ae],0x2
0000:b340  JGE 0x0000:b5ef
0000:b344  MOV DX,0x32
0000:b347  MOV CX,0x64
0000:b34a  MOV BX,0xffce
0000:b34d  MOV AX,0xffce
0000:b350  CALLF 0x0000:ffff
0000:b355  MOV DX,0x32
0000:b358  NEG DX
0000:b35a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b35f  JS 0x0000:b363
0000:b361  NEG DX
0000:b363  MOV AX,word ptr ES:[DI + 0x8]
0000:b367  DEC AX
0000:b368  MOV BX,word ptr ES:[DI + 0x4]
0000:b36c  ADD BX,DX
0000:b36e  CALLF 0x0000:ffff
0000:b373  JNZ 0x0000:b3b8
0000:b375  MOV DX,0x32
0000:b378  NEG DX
0000:b37a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b37f  JS 0x0000:b383
0000:b381  NEG DX
0000:b383  MOV AX,word ptr ES:[DI + 0x8]
0000:b387  SUB AX,0x11
0000:b38a  MOV BX,word ptr ES:[DI + 0x4]
0000:b38e  ADD BX,DX
0000:b390  CALLF 0x0000:ffff
0000:b395  JNZ 0x0000:b3b8
0000:b397  MOV DX,0x32
0000:b39a  NEG DX
0000:b39c  TEST byte ptr ES:[DI + 0x28],0xff
0000:b3a1  JS 0x0000:b3a6
0000:b3a3  MOV DX,0x32
0000:b3a6  MOV AX,word ptr ES:[DI + 0x8]
0000:b3aa  SUB AX,0xc
0000:b3ad  MOV BX,word ptr ES:[DI + 0x4]
0000:b3b1  ADD BX,DX
0000:b3b3  CALLF 0x0000:ffff
0000:b3b8  JMP 0x0000:b3bc
0000:b3bc  JZ 0x0000:b3c3
0000:b3be  MOV byte ptr ES:[DI + 0x3e],0x1
0000:b3c3  CMP byte ptr ES:[DI + 0x34],0x1
0000:b3c8  JGE 0x0000:b50f
0000:b3cc  CMP byte ptr ES:[DI + 0x3e],0x0
0000:b3d1  JLE 0x0000:b4c3
0000:b3d5  CMP byte ptr ES:[DI + 0x40],0x0
0000:b3da  JGE 0x0000:b469
0000:b3de  CMP word ptr ES:[DI + 0x42],0x14
0000:b3e3  JNZ 0x0000:b3e5
0000:b3e5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:b3ea  MOV AL,byte ptr ES:[DI + 0x29]
0000:b3ee  CBW
0000:b3ef  CWDE
0000:b3f1  SHL EAX,0xc
0000:b3f5  SUB EBX,EAX
0000:b3f8  CMP EBX,0xffff9000
0000:b3ff  JL 0x0000:b416
0000:b401  CMP EBX,0x7000
0000:b408  JG 0x0000:b40d
0000:b40a  CLC
0000:b40b  JMP 0x0000:b41d
0000:b40d  MOV EBX,0x7000
0000:b413  STC
0000:b414  JMP 0x0000:b41d
0000:b416  MOV EBX,0xffff9000
0000:b41c  STC
0000:b41d  MOV dword ptr ES:[DI + 0xa],EBX
0000:b422  ADD dword ptr ES:[DI + 0x2],EBX
0000:b427  DEC word ptr ES:[DI + 0x42]
0000:b42b  JGE 0x0000:b4c0
0000:b42f  NEG byte ptr ES:[DI + 0x29]
0000:b433  NEG byte ptr ES:[DI + 0x28]
0000:b437  NEG byte ptr ES:[DI + 0x40]
0000:b43b  CMP word ptr ES:[DI + 0x12],0x385
0000:b441  JZ 0x0000:b44b
0000:b443  MOV word ptr ES:[DI + 0x12],0x385
0000:b449  JMP 0x0000:b451
0000:b44b  MOV word ptr ES:[DI + 0x12],0x3b7
0000:b451  MOV AL,byte ptr ES:[DI + 0x29]

; ---- B25D count=224 ----
0000:b25d  CMP byte ptr ES:[DI + 0x2e],0x1
0000:b262  JGE 0x0000:b315
0000:b266  CMP word ptr [0x8806],0x0
0000:b26b  JZ 0x0000:b335
0000:b26f  MOV BX,word ptr ES:[DI + 0x2a]
0000:b273  CMP BX,word ptr [0x8808]
0000:b277  JLE 0x0000:b281
0000:b279  MOV word ptr ES:[DI + 0x2a],0x0
0000:b27f  XOR BX,BX
0000:b281  SHL BX,0x2
0000:b284  MOV AX,word ptr ES:[DI + 0x4]
0000:b288  SUB AX,0xf
0000:b28b  CMP word ptr [BX + 0x87de],AX
0000:b28f  JLE 0x0000:b2b8
0000:b291  ADD AX,0x1e
0000:b294  CMP word ptr [BX + 0x87de],AX
0000:b298  JGE 0x0000:b2b8
0000:b29a  MOV AX,word ptr ES:[DI + 0x8]
0000:b29e  ADD AX,0x5
0000:b2a1  CMP word ptr [BX + 0x87e0],AX
0000:b2a5  JGE 0x0000:b2b8
0000:b2a7  SUB AX,0x1e
0000:b2aa  CMP word ptr [BX + 0x87e0],AX
0000:b2ae  JLE 0x0000:b2b8
0000:b2b0  MOV word ptr [BX + 0x87de],0x0
0000:b2b6  JMP 0x0000:b2ba
0000:b2b8  JMP 0x0000:b303
0000:b2ba  INC word ptr ES:[DI + 0x2c]
0000:b2be  PUSH DI
0000:b2bf  MOV AX,0x4b70
0000:b2c2  XOR DX,DX
0000:b2c4  CALLF 0x0000:ffff
0000:b2c9  POP SI
0000:b2ca  MOV byte ptr ES:[DI + 0x17],0x2
0000:b2cf  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b2d4  MOV dword ptr ES:[DI + 0x2],EAX
0000:b2d9  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b2de  ADD EAX,0xa0000
0000:b2e4  MOV dword ptr ES:[DI + 0x6],EAX
0000:b2e9  MOV DI,SI
0000:b2eb  MOV SI,0x3308
0000:b2ee  CALLF 0x0000:ffff
0000:b2f3  MOV word ptr [0x612e],0xd
0000:b2f9  CALLF 0x0000:ffff
0000:b2fe  MOV byte ptr ES:[DI + 0x2e],0x1
0000:b303  INC word ptr ES:[DI + 0x2a]
0000:b307  CMP word ptr ES:[DI + 0x2c],0x4
0000:b30c  JLE 0x0000:b335
0000:b30e  MOV byte ptr [0x88ae],0x2
0000:b313  JMP 0x0000:b335
0000:b315  INC word ptr ES:[DI + 0x2f]
0000:b319  CMP word ptr ES:[DI + 0x2f],0x64
0000:b31e  JLE 0x0000:b335
0000:b320  MOV word ptr ES:[DI + 0x2f],0x0
0000:b326  MOV byte ptr ES:[DI + 0x2e],0x0
0000:b32b  MOV SI,0x32fa
0000:b32e  CALLF 0x0000:ffff
0000:b333  JMP 0x0000:b335
0000:b335  CALLF 0x0000:ffff
0000:b33a  RET
0000:b33b  CMP byte ptr [0x88ae],0x2
0000:b340  JGE 0x0000:b5ef
0000:b344  MOV DX,0x32
0000:b347  MOV CX,0x64
0000:b34a  MOV BX,0xffce
0000:b34d  MOV AX,0xffce
0000:b350  CALLF 0x0000:ffff
0000:b355  MOV DX,0x32
0000:b358  NEG DX
0000:b35a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b35f  JS 0x0000:b363
0000:b361  NEG DX
0000:b363  MOV AX,word ptr ES:[DI + 0x8]
0000:b367  DEC AX
0000:b368  MOV BX,word ptr ES:[DI + 0x4]
0000:b36c  ADD BX,DX
0000:b36e  CALLF 0x0000:ffff
0000:b373  JNZ 0x0000:b3b8
0000:b375  MOV DX,0x32
0000:b378  NEG DX
0000:b37a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b37f  JS 0x0000:b383
0000:b381  NEG DX
0000:b383  MOV AX,word ptr ES:[DI + 0x8]
0000:b387  SUB AX,0x11
0000:b38a  MOV BX,word ptr ES:[DI + 0x4]
0000:b38e  ADD BX,DX
0000:b390  CALLF 0x0000:ffff
0000:b395  JNZ 0x0000:b3b8
0000:b397  MOV DX,0x32
0000:b39a  NEG DX
0000:b39c  TEST byte ptr ES:[DI + 0x28],0xff
0000:b3a1  JS 0x0000:b3a6
0000:b3a3  MOV DX,0x32
0000:b3a6  MOV AX,word ptr ES:[DI + 0x8]
0000:b3aa  SUB AX,0xc
0000:b3ad  MOV BX,word ptr ES:[DI + 0x4]
0000:b3b1  ADD BX,DX
0000:b3b3  CALLF 0x0000:ffff
0000:b3b8  JMP 0x0000:b3bc
0000:b3bc  JZ 0x0000:b3c3
0000:b3be  MOV byte ptr ES:[DI + 0x3e],0x1
0000:b3c3  CMP byte ptr ES:[DI + 0x34],0x1
0000:b3c8  JGE 0x0000:b50f
0000:b3cc  CMP byte ptr ES:[DI + 0x3e],0x0
0000:b3d1  JLE 0x0000:b4c3
0000:b3d5  CMP byte ptr ES:[DI + 0x40],0x0
0000:b3da  JGE 0x0000:b469
0000:b3de  CMP word ptr ES:[DI + 0x42],0x14
0000:b3e3  JNZ 0x0000:b3e5
0000:b3e5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:b3ea  MOV AL,byte ptr ES:[DI + 0x29]
0000:b3ee  CBW
0000:b3ef  CWDE
0000:b3f1  SHL EAX,0xc
0000:b3f5  SUB EBX,EAX
0000:b3f8  CMP EBX,0xffff9000
0000:b3ff  JL 0x0000:b416
0000:b401  CMP EBX,0x7000
0000:b408  JG 0x0000:b40d
0000:b40a  CLC
0000:b40b  JMP 0x0000:b41d
0000:b40d  MOV EBX,0x7000
0000:b413  STC
0000:b414  JMP 0x0000:b41d
0000:b416  MOV EBX,0xffff9000
0000:b41c  STC
0000:b41d  MOV dword ptr ES:[DI + 0xa],EBX
0000:b422  ADD dword ptr ES:[DI + 0x2],EBX
0000:b427  DEC word ptr ES:[DI + 0x42]
0000:b42b  JGE 0x0000:b4c0
0000:b42f  NEG byte ptr ES:[DI + 0x29]
0000:b433  NEG byte ptr ES:[DI + 0x28]
0000:b437  NEG byte ptr ES:[DI + 0x40]
0000:b43b  CMP word ptr ES:[DI + 0x12],0x385
0000:b441  JZ 0x0000:b44b
0000:b443  MOV word ptr ES:[DI + 0x12],0x385
0000:b449  JMP 0x0000:b451
0000:b44b  MOV word ptr ES:[DI + 0x12],0x3b7
0000:b451  MOV AL,byte ptr ES:[DI + 0x29]
0000:b455  CBW
0000:b456  CWDE
0000:b458  SHL EAX,0x9
0000:b45c  MOV dword ptr ES:[DI + 0xa],EAX
0000:b461  MOV word ptr ES:[DI + 0x42],0x1e
0000:b467  JMP 0x0000:b4c0
0000:b469  MOV EBX,dword ptr ES:[DI + 0xa]
0000:b46e  MOV AL,byte ptr ES:[DI + 0x29]
0000:b472  CBW
0000:b473  CWDE
0000:b475  SHL EAX,0xa
0000:b479  ADD EBX,EAX
0000:b47c  CMP EBX,0xffff9000
0000:b483  JL 0x0000:b49a
0000:b485  CMP EBX,0x7000
0000:b48c  JG 0x0000:b491
0000:b48e  CLC
0000:b48f  JMP 0x0000:b4a1
0000:b491  MOV EBX,0x7000
0000:b497  STC
0000:b498  JMP 0x0000:b4a1
0000:b49a  MOV EBX,0xffff9000
0000:b4a0  STC
0000:b4a1  MOV dword ptr ES:[DI + 0xa],EBX
0000:b4a6  ADD dword ptr ES:[DI + 0x2],EBX
0000:b4ab  DEC word ptr ES:[DI + 0x42]
0000:b4af  JGE 0x0000:b4c0
0000:b4b1  NEG byte ptr ES:[DI + 0x40]
0000:b4b5  MOV byte ptr ES:[DI + 0x3e],0xff
0000:b4ba  MOV word ptr ES:[DI + 0x42],0x14
0000:b4c0  JMP 0x0000:b558
0000:b4c3  MOV EAX,dword ptr ES:[DI + 0xa]
0000:b4c8  ADD dword ptr ES:[DI + 0x2],EAX
0000:b4cd  MOV AX,word ptr ES:[DI + 0x2c]
0000:b4d1  SUB word ptr ES:[DI + 0x8],AX
0000:b4d5  MOV SI,0x7974
0000:b4d8  MOV AX,word ptr ES:[DI + 0x2e]
0000:b4dc  ADD AX,0x20
0000:b4df  AND AX,0x7ff
0000:b4e2  MOV word ptr ES:[DI + 0x2e],AX
0000:b4e6  ADD SI,AX
0000:b4e8  MOV AL,byte ptr [SI]
0000:b4ea  SAR AL,0x4
0000:b4ed  CBW
0000:b4ee  MOV word ptr ES:[DI + 0x2c],AX
0000:b4f2  ADD word ptr ES:[DI + 0x8],AX
0000:b4f6  INC word ptr ES:[DI + 0x38]
0000:b4fa  CMP word ptr ES:[DI + 0x38],0xdc
0000:b500  JLE 0x0000:b558
0000:b502  MOV word ptr ES:[DI + 0x38],0x0
0000:b508  MOV byte ptr ES:[DI + 0x34],0x1
0000:b50d  JMP 0x0000:b558
0000:b50f  PUSH DI
0000:b510  MOV AX,0xb84d
0000:b513  XOR DX,DX
0000:b515  CALLF 0x0000:ffff
0000:b51a  POP SI
0000:b51b  MOV byte ptr ES:[DI + 0x17],0x2
0000:b520  MOV AL,byte ptr ES:[SI + 0x29]
0000:b524  MOV byte ptr ES:[DI + 0x29],AL
0000:b528  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b52d  CMP byte ptr ES:[SI + 0x28],0x1
0000:b532  JNZ 0x0000:b53c
0000:b534  ADD EAX,0x1f0000
0000:b53a  JMP 0x0000:b542
0000:b53c  SUB EAX,0x1f0000
0000:b542  MOV dword ptr ES:[DI + 0x2],EAX
0000:b547  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b54c  MOV dword ptr ES:[DI + 0x6],EAX
0000:b551  MOV DI,SI
0000:b553  MOV byte ptr ES:[DI + 0x34],0x0
0000:b558  PUSH DI
0000:b559  MOV SI,DI
0000:b55b  MOV DI,word ptr ES:[DI + 0x2a]
0000:b55f  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b564  CMP byte ptr ES:[SI + 0x28],0x1
0000:b569  JNZ 0x0000:b573
0000:b56b  ADD EAX,0x60000
0000:b571  JMP 0x0000:b579
0000:b573  SUB EAX,0x60000
0000:b579  MOV dword ptr ES:[DI + 0x2],EAX
0000:b57e  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b583  SUB EAX,0x1d0000
0000:b589  MOV dword ptr ES:[DI + 0x6],EAX

; ---- B33B count=768 ----
0000:b33b  CMP byte ptr [0x88ae],0x2
0000:b340  JGE 0x0000:b5ef
0000:b344  MOV DX,0x32
0000:b347  MOV CX,0x64
0000:b34a  MOV BX,0xffce
0000:b34d  MOV AX,0xffce
0000:b350  CALLF 0x0000:ffff
0000:b355  MOV DX,0x32
0000:b358  NEG DX
0000:b35a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b35f  JS 0x0000:b363
0000:b361  NEG DX
0000:b363  MOV AX,word ptr ES:[DI + 0x8]
0000:b367  DEC AX
0000:b368  MOV BX,word ptr ES:[DI + 0x4]
0000:b36c  ADD BX,DX
0000:b36e  CALLF 0x0000:ffff
0000:b373  JNZ 0x0000:b3b8
0000:b375  MOV DX,0x32
0000:b378  NEG DX
0000:b37a  TEST byte ptr ES:[DI + 0x28],0xff
0000:b37f  JS 0x0000:b383
0000:b381  NEG DX
0000:b383  MOV AX,word ptr ES:[DI + 0x8]
0000:b387  SUB AX,0x11
0000:b38a  MOV BX,word ptr ES:[DI + 0x4]
0000:b38e  ADD BX,DX
0000:b390  CALLF 0x0000:ffff
0000:b395  JNZ 0x0000:b3b8
0000:b397  MOV DX,0x32
0000:b39a  NEG DX
0000:b39c  TEST byte ptr ES:[DI + 0x28],0xff
0000:b3a1  JS 0x0000:b3a6
0000:b3a3  MOV DX,0x32
0000:b3a6  MOV AX,word ptr ES:[DI + 0x8]
0000:b3aa  SUB AX,0xc
0000:b3ad  MOV BX,word ptr ES:[DI + 0x4]
0000:b3b1  ADD BX,DX
0000:b3b3  CALLF 0x0000:ffff
0000:b3b8  JMP 0x0000:b3bc
0000:b3bc  JZ 0x0000:b3c3
0000:b3be  MOV byte ptr ES:[DI + 0x3e],0x1
0000:b3c3  CMP byte ptr ES:[DI + 0x34],0x1
0000:b3c8  JGE 0x0000:b50f
0000:b3cc  CMP byte ptr ES:[DI + 0x3e],0x0
0000:b3d1  JLE 0x0000:b4c3
0000:b3d5  CMP byte ptr ES:[DI + 0x40],0x0
0000:b3da  JGE 0x0000:b469
0000:b3de  CMP word ptr ES:[DI + 0x42],0x14
0000:b3e3  JNZ 0x0000:b3e5
0000:b3e5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:b3ea  MOV AL,byte ptr ES:[DI + 0x29]
0000:b3ee  CBW
0000:b3ef  CWDE
0000:b3f1  SHL EAX,0xc
0000:b3f5  SUB EBX,EAX
0000:b3f8  CMP EBX,0xffff9000
0000:b3ff  JL 0x0000:b416
0000:b401  CMP EBX,0x7000
0000:b408  JG 0x0000:b40d
0000:b40a  CLC
0000:b40b  JMP 0x0000:b41d
0000:b40d  MOV EBX,0x7000
0000:b413  STC
0000:b414  JMP 0x0000:b41d
0000:b416  MOV EBX,0xffff9000
0000:b41c  STC
0000:b41d  MOV dword ptr ES:[DI + 0xa],EBX
0000:b422  ADD dword ptr ES:[DI + 0x2],EBX
0000:b427  DEC word ptr ES:[DI + 0x42]
0000:b42b  JGE 0x0000:b4c0
0000:b42f  NEG byte ptr ES:[DI + 0x29]
0000:b433  NEG byte ptr ES:[DI + 0x28]
0000:b437  NEG byte ptr ES:[DI + 0x40]
0000:b43b  CMP word ptr ES:[DI + 0x12],0x385
0000:b441  JZ 0x0000:b44b
0000:b443  MOV word ptr ES:[DI + 0x12],0x385
0000:b449  JMP 0x0000:b451
0000:b44b  MOV word ptr ES:[DI + 0x12],0x3b7
0000:b451  MOV AL,byte ptr ES:[DI + 0x29]
0000:b455  CBW
0000:b456  CWDE
0000:b458  SHL EAX,0x9
0000:b45c  MOV dword ptr ES:[DI + 0xa],EAX
0000:b461  MOV word ptr ES:[DI + 0x42],0x1e
0000:b467  JMP 0x0000:b4c0
0000:b469  MOV EBX,dword ptr ES:[DI + 0xa]
0000:b46e  MOV AL,byte ptr ES:[DI + 0x29]
0000:b472  CBW
0000:b473  CWDE
0000:b475  SHL EAX,0xa
0000:b479  ADD EBX,EAX
0000:b47c  CMP EBX,0xffff9000
0000:b483  JL 0x0000:b49a
0000:b485  CMP EBX,0x7000
0000:b48c  JG 0x0000:b491
0000:b48e  CLC
0000:b48f  JMP 0x0000:b4a1
0000:b491  MOV EBX,0x7000
0000:b497  STC
0000:b498  JMP 0x0000:b4a1
0000:b49a  MOV EBX,0xffff9000
0000:b4a0  STC
0000:b4a1  MOV dword ptr ES:[DI + 0xa],EBX
0000:b4a6  ADD dword ptr ES:[DI + 0x2],EBX
0000:b4ab  DEC word ptr ES:[DI + 0x42]
0000:b4af  JGE 0x0000:b4c0
0000:b4b1  NEG byte ptr ES:[DI + 0x40]
0000:b4b5  MOV byte ptr ES:[DI + 0x3e],0xff
0000:b4ba  MOV word ptr ES:[DI + 0x42],0x14
0000:b4c0  JMP 0x0000:b558
0000:b4c3  MOV EAX,dword ptr ES:[DI + 0xa]
0000:b4c8  ADD dword ptr ES:[DI + 0x2],EAX
0000:b4cd  MOV AX,word ptr ES:[DI + 0x2c]
0000:b4d1  SUB word ptr ES:[DI + 0x8],AX
0000:b4d5  MOV SI,0x7974
0000:b4d8  MOV AX,word ptr ES:[DI + 0x2e]
0000:b4dc  ADD AX,0x20
0000:b4df  AND AX,0x7ff
0000:b4e2  MOV word ptr ES:[DI + 0x2e],AX
0000:b4e6  ADD SI,AX
0000:b4e8  MOV AL,byte ptr [SI]
0000:b4ea  SAR AL,0x4
0000:b4ed  CBW
0000:b4ee  MOV word ptr ES:[DI + 0x2c],AX
0000:b4f2  ADD word ptr ES:[DI + 0x8],AX
0000:b4f6  INC word ptr ES:[DI + 0x38]
0000:b4fa  CMP word ptr ES:[DI + 0x38],0xdc
0000:b500  JLE 0x0000:b558
0000:b502  MOV word ptr ES:[DI + 0x38],0x0
0000:b508  MOV byte ptr ES:[DI + 0x34],0x1
0000:b50d  JMP 0x0000:b558
0000:b50f  PUSH DI
0000:b510  MOV AX,0xb84d
0000:b513  XOR DX,DX
0000:b515  CALLF 0x0000:ffff
0000:b51a  POP SI
0000:b51b  MOV byte ptr ES:[DI + 0x17],0x2
0000:b520  MOV AL,byte ptr ES:[SI + 0x29]
0000:b524  MOV byte ptr ES:[DI + 0x29],AL
0000:b528  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b52d  CMP byte ptr ES:[SI + 0x28],0x1
0000:b532  JNZ 0x0000:b53c
0000:b534  ADD EAX,0x1f0000
0000:b53a  JMP 0x0000:b542
0000:b53c  SUB EAX,0x1f0000
0000:b542  MOV dword ptr ES:[DI + 0x2],EAX
0000:b547  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b54c  MOV dword ptr ES:[DI + 0x6],EAX
0000:b551  MOV DI,SI
0000:b553  MOV byte ptr ES:[DI + 0x34],0x0
0000:b558  PUSH DI
0000:b559  MOV SI,DI
0000:b55b  MOV DI,word ptr ES:[DI + 0x2a]
0000:b55f  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b564  CMP byte ptr ES:[SI + 0x28],0x1
0000:b569  JNZ 0x0000:b573
0000:b56b  ADD EAX,0x60000
0000:b571  JMP 0x0000:b579
0000:b573  SUB EAX,0x60000
0000:b579  MOV dword ptr ES:[DI + 0x2],EAX
0000:b57e  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b583  SUB EAX,0x1d0000
0000:b589  MOV dword ptr ES:[DI + 0x6],EAX
0000:b58e  MOV SI,0x7974
0000:b591  MOV AX,word ptr ES:[DI + 0x2e]
0000:b595  ADD AX,0xa
0000:b598  AND AX,0x6ff
0000:b59b  MOV word ptr ES:[DI + 0x2e],AX
0000:b59f  ADD SI,AX
0000:b5a1  MOV AL,byte ptr [SI]
0000:b5a3  SAR AL,0x5
0000:b5a6  CBW
0000:b5a7  ADD word ptr ES:[DI + 0x8],AX
0000:b5ab  MOV AX,word ptr ES:[DI + 0x8]
0000:b5af  MOV word ptr ES:[DI + 0x8],AX
0000:b5b3  POP DI
0000:b5b4  PUSH DI
0000:b5b5  MOV SI,DI
0000:b5b7  MOV DI,word ptr ES:[DI + 0x36]
0000:b5bb  MOV EBX,dword ptr ES:[SI + 0x2]
0000:b5c0  MOV AL,byte ptr ES:[SI + 0x28]
0000:b5c4  MOV byte ptr ES:[DI + 0x28],AL
0000:b5c8  CMP AL,0x1
0000:b5ca  JNZ 0x0000:b5d5
0000:b5cc  ADD EBX,0x1f0000
0000:b5d3  JMP 0x0000:b5dc
0000:b5d5  SUB EBX,0x1f0000
0000:b5dc  MOV dword ptr ES:[DI + 0x2],EBX
0000:b5e1  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b5e6  MOV dword ptr ES:[DI + 0x6],EAX
0000:b5eb  POP DI
0000:b5ec  JMP 0x0000:b84c
0000:b5ef  CMP byte ptr [0x88ae],0x3
0000:b5f4  JGE 0x0000:b622
0000:b5f6  PUSH DI
0000:b5f7  MOV SI,DI
0000:b5f9  MOV DI,word ptr ES:[DI + 0x36]
0000:b5fd  MOV word ptr ES:[DI + 0x18],0x0
0000:b603  POP DI
0000:b604  CMP word ptr ES:[DI + 0x12],0x385
0000:b60a  JZ 0x0000:b614
0000:b60c  MOV word ptr ES:[DI + 0x12],0x3b6
0000:b612  JMP 0x0000:b61a
0000:b614  MOV word ptr ES:[DI + 0x12],0x384
0000:b61a  MOV byte ptr [0x88ae],0x3
0000:b61f  JMP 0x0000:b84c
0000:b622  CMP byte ptr [0x88ae],0x4
0000:b627  JGE 0x0000:b796
0000:b62b  MOV AX,word ptr ES:[DI + 0x2c]
0000:b62f  SUB word ptr ES:[DI + 0x8],AX
0000:b633  MOV SI,0x7974
0000:b636  MOV AX,word ptr ES:[DI + 0x2e]
0000:b63a  ADD AX,0x20
0000:b63d  AND AX,0x5ff
0000:b640  MOV word ptr ES:[DI + 0x2e],AX
0000:b644  ADD SI,AX
0000:b646  MOV AL,byte ptr [SI]
0000:b648  SAR AL,0x5
0000:b64b  CBW
0000:b64c  MOV word ptr ES:[DI + 0x2c],AX
0000:b650  ADD word ptr ES:[DI + 0x8],AX
0000:b654  PUSH DI
0000:b655  MOV SI,DI
0000:b657  MOV DI,word ptr ES:[DI + 0x2a]
0000:b65b  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b660  CMP byte ptr ES:[SI + 0x28],0x1
0000:b665  JNZ 0x0000:b66f
0000:b667  ADD EAX,0x60000
0000:b66d  JMP 0x0000:b675
0000:b66f  SUB EAX,0x60000
0000:b675  MOV dword ptr ES:[DI + 0x2],EAX
0000:b67a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b67f  SUB EAX,0x1d0000
0000:b685  MOV dword ptr ES:[DI + 0x6],EAX
0000:b68a  MOV SI,0x7974
0000:b68d  MOV AX,word ptr ES:[DI + 0x2e]
0000:b691  ADD AX,0xa
0000:b694  AND AX,0x6ff
0000:b697  MOV word ptr ES:[DI + 0x2e],AX
0000:b69b  ADD SI,AX
0000:b69d  MOV AL,byte ptr [SI]
0000:b69f  SAR AL,0x5
0000:b6a2  CBW
0000:b6a3  ADD word ptr ES:[DI + 0x8],AX
0000:b6a7  MOV AX,word ptr ES:[DI + 0x8]
0000:b6ab  MOV word ptr ES:[DI + 0x8],AX
0000:b6af  POP DI
0000:b6b0  INC word ptr ES:[DI + 0x38]
0000:b6b4  CMP word ptr ES:[DI + 0x38],0x19
0000:b6b9  JLE 0x0000:b84c
0000:b6bd  MOV word ptr ES:[DI + 0x38],0x0
0000:b6c3  PUSH DI
0000:b6c4  MOV AX,0x4b70
0000:b6c7  XOR DX,DX
0000:b6c9  CALLF 0x0000:ffff
0000:b6ce  POP SI
0000:b6cf  MOV byte ptr ES:[DI + 0x17],0x2
0000:b6d4  PUSH SI
0000:b6d5  MOV SI,0x646c
0000:b6d8  ADD SI,word ptr [0x6468]
0000:b6dc  INC word ptr [0x6468]
0000:b6e0  AND word ptr [0x6468],0xff
0000:b6e6  MOV AL,byte ptr [SI]
0000:b6e8  POP SI
0000:b6e9  SHR AL,0x2
0000:b6ec  CBW
0000:b6ed  SUB AX,0x20
0000:b6f0  MOV BX,word ptr ES:[SI + 0x4]
0000:b6f4  ADD BX,AX
0000:b6f6  MOV word ptr ES:[DI + 0x4],BX
0000:b6fa  PUSH SI
0000:b6fb  MOV SI,0x646c
0000:b6fe  ADD SI,word ptr [0x6468]
0000:b702  INC word ptr [0x6468]
0000:b706  AND word ptr [0x6468],0xff
0000:b70c  MOV AL,byte ptr [SI]
0000:b70e  POP SI
0000:b70f  SHR AL,0x3
0000:b712  CBW
0000:b713  MOV BX,word ptr ES:[SI + 0x8]
0000:b717  ADD BX,AX
0000:b719  MOV word ptr ES:[DI + 0x8],BX
0000:b71d  MOV DI,SI
0000:b71f  PUSH DI
0000:b720  MOV AX,0x4b70
0000:b723  XOR DX,DX
0000:b725  CALLF 0x0000:ffff
0000:b72a  POP SI
0000:b72b  MOV byte ptr ES:[DI + 0x17],0x2
0000:b730  PUSH SI
0000:b731  MOV SI,0x646c
0000:b734  ADD SI,word ptr [0x6468]
0000:b738  INC word ptr [0x6468]
0000:b73c  AND word ptr [0x6468],0xff
0000:b742  MOV AL,byte ptr [SI]
0000:b744  POP SI
0000:b745  SHR AL,0x2
0000:b748  CBW
0000:b749  SUB AX,0x20
0000:b74c  MOV BX,word ptr ES:[SI + 0x4]
0000:b750  ADD BX,AX
0000:b752  MOV word ptr ES:[DI + 0x4],BX
0000:b756  PUSH SI
0000:b757  MOV SI,0x646c
0000:b75a  ADD SI,word ptr [0x6468]
0000:b75e  INC word ptr [0x6468]
0000:b762  AND word ptr [0x6468],0xff
0000:b768  MOV AL,byte ptr [SI]
0000:b76a  POP SI
0000:b76b  SHR AL,0x3
0000:b76e  CBW
0000:b76f  MOV BX,word ptr ES:[SI + 0x8]
0000:b773  ADD BX,AX
0000:b775  MOV word ptr ES:[DI + 0x8],BX
0000:b779  MOV DI,SI
0000:b77b  INC word ptr ES:[DI + 0x44]
0000:b77f  CMP word ptr ES:[DI + 0x44],0xf
0000:b784  JLE 0x0000:b84c
0000:b788  MOV dword ptr ES:[DI + 0xe],0xffff0000
0000:b791  MOV byte ptr [0x88ae],0x4
0000:b796  CMP byte ptr [0x88ae],0x5
0000:b79b  JGE 0x0000:b824
0000:b79f  INC word ptr ES:[DI + 0x38]
0000:b7a3  CMP word ptr ES:[DI + 0x38],0x28
0000:b7a8  JLE 0x0000:b84c
0000:b7ac  MOV EAX,dword ptr ES:[DI + 0xe]
0000:b7b1  SUB dword ptr ES:[DI + 0xe],0x1200
0000:b7ba  ADD dword ptr ES:[DI + 0x6],EAX
0000:b7bf  PUSH DI
0000:b7c0  MOV SI,DI
0000:b7c2  MOV DI,word ptr ES:[DI + 0x2a]
0000:b7c6  MOV EAX,dword ptr ES:[SI + 0x2]
0000:b7cb  CMP byte ptr ES:[SI + 0x28],0x1
0000:b7d0  JNZ 0x0000:b7da
0000:b7d2  ADD EAX,0x60000
0000:b7d8  JMP 0x0000:b7e0
0000:b7da  SUB EAX,0x60000
0000:b7e0  MOV dword ptr ES:[DI + 0x2],EAX
0000:b7e5  MOV EAX,dword ptr ES:[SI + 0x6]
0000:b7ea  SUB EAX,0x1d0000
0000:b7f0  MOV dword ptr ES:[DI + 0x6],EAX
0000:b7f5  POP DI
0000:b7f6  MOV AX,word ptr ES:[DI + 0x4]
0000:b7fa  SUB AX,word ptr [0x81c0]
0000:b7fe  ADD AX,0x10
0000:b801  CMP AX,0x160
0000:b804  JA 0x0000:b819
0000:b806  MOV AX,word ptr ES:[DI + 0x8]
0000:b80a  SUB AX,word ptr [0x81c4]
0000:b80e  ADD AX,0x10
0000:b811  CMP AX,0xd0
0000:b814  JA 0x0000:b819
0000:b816  CLC
0000:b817  JMP 0x0000:b822
0000:b819  STC
0000:b81a  MOV word ptr ES:[DI + 0x18],0x0
0000:b820  JMP 0x0000:b824
0000:b822  JMP 0x0000:b84c
0000:b824  MOV byte ptr [0x88ae],0x5
0000:b829  PUSH DI
0000:b82a  MOV AX,0x487f
0000:b82d  XOR DX,DX
0000:b82f  CALLF 0x0000:ffff
0000:b834  POP SI
0000:b835  MOV byte ptr ES:[DI + 0x17],0x1
0000:b83a  MOV BX,word ptr ES:[SI + 0x4]
0000:b83e  MOV word ptr ES:[DI + 0x4],BX
0000:b842  MOV BX,word ptr ES:[SI + 0x8]
0000:b846  MOV word ptr ES:[DI + 0x8],BX
0000:b84a  MOV DI,SI
0000:b84c  RET
0000:b84d  MOV word ptr ES:[DI + 0x12],0x386
0000:b853  MOV word ptr ES:[DI + 0x18],0xb87b
0000:b859  MOV word ptr ES:[DI + 0x3a],0x0
0000:b85f  MOV dword ptr ES:[DI + 0x3c],0x30000
0000:b868  MOV dword ptr ES:[DI + 0xa],0x0
0000:b871  MOV dword ptr ES:[DI + 0xe],0xfffeb000
0000:b87a  RET
0000:b87b  MOV AX,word ptr ES:[DI + 0x4]
0000:b87f  SUB AX,word ptr [0x81c0]
0000:b883  ADD AX,0x10
0000:b886  CMP AX,0x160
0000:b889  JA 0x0000:b89e
0000:b88b  MOV AX,word ptr ES:[DI + 0x8]
0000:b88f  SUB AX,word ptr [0x81c4]
0000:b893  ADD AX,0x10
0000:b896  CMP AX,0xd0
0000:b899  JA 0x0000:b89e
0000:b89b  CLC
0000:b89c  JMP 0x0000:b8a5
0000:b89e  STC
0000:b89f  MOV word ptr ES:[DI + 0x18],0x0
0000:b8a5  CMP dword ptr ES:[DI + 0xe],0x0
0000:b8ab  JLE 0x0000:b932
0000:b8af  CMP word ptr ES:[DI + 0x3a],0x1
0000:b8b4  JZ 0x0000:b943
0000:b8b8  MOV CX,0x0
0000:b8bb  MOV DX,0x0
0000:b8be  CALLF 0x0000:ffff
0000:b8c3  JNC 0x0000:b8c7
0000:b8c5  JMP 0x0000:b920
0000:b8c7  CMP byte ptr ES:[DI + 0x29],0x0
0000:b8cc  JG 0x0000:b8f3
0000:b8ce  MOV AX,word ptr ES:[DI + 0x8]
0000:b8d2  MOV BX,word ptr ES:[DI + 0x4]
0000:b8d6  CALLF 0x0000:ffff
0000:b8db  TEST DL,0x70
0000:b8de  JNZ 0x0000:b918
0000:b8e0  MOV AX,word ptr ES:[DI + 0x8]
0000:b8e4  SUB AX,0x10
0000:b8e7  CALLF 0x0000:ffff
0000:b8ec  TEST DL,0x70
0000:b8ef  JNZ 0x0000:b918
0000:b8f1  JMP 0x0000:b91a
0000:b8f3  MOV AX,word ptr ES:[DI + 0x8]
0000:b8f7  MOV BX,word ptr ES:[DI + 0x4]
0000:b8fb  CALLF 0x0000:ffff
0000:b900  TEST DL,0x70
0000:b903  JNZ 0x0000:b918
0000:b905  MOV AX,word ptr ES:[DI + 0x8]
0000:b909  SUB AX,0x10
0000:b90c  CALLF 0x0000:ffff
0000:b911  TEST DL,0x70
0000:b914  JNZ 0x0000:b918
0000:b916  JMP 0x0000:b91a
0000:b918  JMP 0x0000:b91c
0000:b91a  JMP 0x0000:b932
0000:b91c  NEG byte ptr ES:[DI + 0x29]
0000:b920  NEG dword ptr ES:[DI + 0xe]
0000:b925  INC word ptr ES:[DI + 0x3a]
0000:b929  SUB dword ptr ES:[DI + 0x3c],0x5000
0000:b932  MOV BX,0xfffb
0000:b935  MOV AX,0x5
0000:b938  CALLF 0x0000:ffff
0000:b93d  JNC 0x0000:b943
0000:b93f  NEG byte ptr ES:[DI + 0x29]
0000:b943  MOV EAX,dword ptr ES:[DI + 0xe]
0000:b948  CMP EAX,0x0
0000:b94c  JG 0x0000:b977
0000:b94e  ADD EAX,0x3a98
0000:b954  CMP EAX,0xfffd0000
0000:b95a  JL 0x0000:b96e
0000:b95c  CMP EAX,dword ptr ES:[DI + 0x3c]
0000:b961  JG 0x0000:b966
0000:b963  CLC
0000:b964  JMP 0x0000:b975
0000:b966  MOV EAX,dword ptr ES:[DI + 0x3c]
0000:b96b  STC
0000:b96c  JMP 0x0000:b975
0000:b96e  MOV EAX,0xfffd0000
0000:b974  STC
0000:b975  JMP 0x0000:b99e
0000:b977  ADD EAX,0x4268
0000:b97d  CMP EAX,0xfffd0000
0000:b983  JL 0x0000:b997
0000:b985  CMP EAX,dword ptr ES:[DI + 0x3c]
0000:b98a  JG 0x0000:b98f
0000:b98c  CLC
0000:b98d  JMP 0x0000:b99e
0000:b98f  MOV EAX,dword ptr ES:[DI + 0x3c]
0000:b994  STC
0000:b995  JMP 0x0000:b99e
0000:b997  MOV EAX,0xfffd0000
0000:b99d  STC
0000:b99e  MOV dword ptr ES:[DI + 0xe],EAX
0000:b9a3  ADD dword ptr ES:[DI + 0x6],EAX
0000:b9a8  MOV DX,0x8
0000:b9ab  MOV CX,0x8
0000:b9ae  MOV BX,0xfff8
0000:b9b1  MOV AX,0xfff8
0000:b9b4  CALLF 0x0000:ffff
0000:b9b9  MOV AL,byte ptr ES:[DI + 0x29]
0000:b9bd  CBW
0000:b9be  SHL AX,0x2
0000:b9c1  ADD word ptr ES:[DI + 0x4],AX
0000:b9c5  RET
0000:b9cc  MOV AX,0xb9f3
0000:b9cf  XOR DX,DX
0000:b9d1  CALLF 0x0000:ffff
0000:b9d6  MOV byte ptr ES:[DI + 0x17],0x1
0000:b9db  MOV AX,[0x81c0]
0000:b9de  MOV BX,word ptr [0x81c4]
0000:b9e2  ADD AX,0x1c2
0000:b9e5  ADD BX,0xdc
0000:b9e9  MOV word ptr ES:[DI + 0x4],AX
0000:b9ed  MOV word ptr ES:[DI + 0x8],BX
0000:b9f1  POP DI
0000:b9f2  RETF
0000:b9f3  MOV byte ptr ES:[DI + 0x17],0x1
0000:b9f8  MOV word ptr ES:[DI + 0x18],0xbbec
0000:b9fe  MOV word ptr ES:[DI + 0x12],0x10e
0000:ba04  MOV byte ptr ES:[DI + 0x28],0xff
0000:ba09  MOV byte ptr ES:[DI + 0x29],0xff
0000:ba0e  MOV word ptr ES:[DI + 0x38],0x0
0000:ba14  MOV word ptr ES:[DI + 0x44],0x0
0000:ba1a  MOV byte ptr ES:[DI + 0x34],0x0
0000:ba1f  MOV dword ptr ES:[DI + 0xa],0xffff7000
0000:ba28  MOV byte ptr ES:[DI + 0x40],0xff
0000:ba2d  MOV word ptr ES:[DI + 0x42],0x14
0000:ba33  MOV byte ptr ES:[DI + 0x3e],0xff
0000:ba38  PUSH DI
0000:ba39  MOV AX,0xbaa1
0000:ba3c  XOR DX,DX
0000:ba3e  CALLF 0x0000:ffff
0000:ba43  POP SI
0000:ba44  MOV word ptr ES:[SI + 0x2a],DI
0000:ba48  MOV byte ptr ES:[DI + 0x17],0x2
0000:ba4d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ba52  SUB EAX,0x60000
0000:ba58  MOV dword ptr ES:[DI + 0x2],EAX
0000:ba5d  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ba62  SUB EAX,0x1d0000
0000:ba68  MOV dword ptr ES:[DI + 0x6],EAX
0000:ba6d  MOV DI,SI
0000:ba6f  PUSH DI
0000:ba70  MOV AX,0xbabc
0000:ba73  XOR DX,DX
0000:ba75  CALLF 0x0000:ffff
0000:ba7a  POP SI
0000:ba7b  MOV word ptr ES:[SI + 0x36],DI
0000:ba7f  MOV byte ptr ES:[DI + 0x17],0x2
0000:ba84  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ba89  SUB EAX,0x1f0000
0000:ba8f  MOV dword ptr ES:[DI + 0x2],EAX
0000:ba94  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ba99  MOV dword ptr ES:[DI + 0x6],EAX
0000:ba9e  MOV DI,SI
0000:baa0  RET
0000:baa1  MOV SI,0x341a
0000:baa4  CALLF 0x0000:ffff
0000:baa9  MOV word ptr ES:[DI + 0x18],0xbad7
0000:baaf  MOV word ptr ES:[DI + 0x2e],0x0
0000:bab5  MOV word ptr ES:[DI + 0x2c],0x0
0000:babb  RET
0000:babc  MOV SI,0x32fa
0000:babf  CALLF 0x0000:ffff
0000:bac4  MOV word ptr ES:[DI + 0x18],0xbb0e
0000:baca  MOV word ptr ES:[DI + 0x2a],0x0
0000:bad0  MOV word ptr ES:[DI + 0x2c],0x0
0000:bad6  RET
0000:bad7  CMP byte ptr [0x88ae],0x5
0000:badc  JNZ 0x0000:bb08
0000:bade  MOV AX,word ptr ES:[DI + 0x4]
0000:bae2  SUB AX,word ptr [0x81c0]
0000:bae6  ADD AX,0x10
0000:bae9  CMP AX,0x160
0000:baec  JA 0x0000:bb01
0000:baee  MOV AX,word ptr ES:[DI + 0x8]
0000:baf2  SUB AX,word ptr [0x81c4]
0000:baf6  ADD AX,0x10
0000:baf9  CMP AX,0xd0
0000:bafc  JA 0x0000:bb01
0000:bafe  CLC
0000:baff  JMP 0x0000:bb08
0000:bb01  STC
0000:bb02  MOV word ptr ES:[DI + 0x18],0x0
0000:bb08  CALLF 0x0000:ffff
0000:bb0d  RET
0000:bb0e  CMP byte ptr ES:[DI + 0x2e],0x1
0000:bb13  JGE 0x0000:bbc6
0000:bb17  CMP word ptr [0x8806],0x0
0000:bb1c  JZ 0x0000:bbe6
0000:bb20  MOV BX,word ptr ES:[DI + 0x2a]
0000:bb24  CMP BX,word ptr [0x8808]
0000:bb28  JLE 0x0000:bb32
0000:bb2a  MOV word ptr ES:[DI + 0x2a],0x0
0000:bb30  XOR BX,BX
0000:bb32  SHL BX,0x2
0000:bb35  MOV AX,word ptr ES:[DI + 0x4]
0000:bb39  SUB AX,0xf
0000:bb3c  CMP word ptr [BX + 0x87de],AX
0000:bb40  JLE 0x0000:bb69
0000:bb42  ADD AX,0x1e
0000:bb45  CMP word ptr [BX + 0x87de],AX
0000:bb49  JGE 0x0000:bb69
0000:bb4b  MOV AX,word ptr ES:[DI + 0x8]
0000:bb4f  ADD AX,0x5
0000:bb52  CMP word ptr [BX + 0x87e0],AX
0000:bb56  JGE 0x0000:bb69
0000:bb58  SUB AX,0x1e
0000:bb5b  CMP word ptr [BX + 0x87e0],AX
0000:bb5f  JLE 0x0000:bb69
0000:bb61  MOV word ptr [BX + 0x87de],0x0
0000:bb67  JMP 0x0000:bb6b
0000:bb69  JMP 0x0000:bbb4
0000:bb6b  INC word ptr ES:[DI + 0x2c]
0000:bb6f  PUSH DI
0000:bb70  MOV AX,0x4b70
0000:bb73  XOR DX,DX
0000:bb75  CALLF 0x0000:ffff
0000:bb7a  POP SI
0000:bb7b  MOV byte ptr ES:[DI + 0x17],0x2
0000:bb80  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bb85  MOV dword ptr ES:[DI + 0x2],EAX
0000:bb8a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bb8f  ADD EAX,0xa0000
0000:bb95  MOV dword ptr ES:[DI + 0x6],EAX
0000:bb9a  MOV DI,SI
0000:bb9c  MOV SI,0x3308
0000:bb9f  CALLF 0x0000:ffff
0000:bba4  MOV word ptr [0x612e],0xd
0000:bbaa  CALLF 0x0000:ffff
0000:bbaf  MOV byte ptr ES:[DI + 0x2e],0x1
0000:bbb4  INC word ptr ES:[DI + 0x2a]
0000:bbb8  CMP word ptr ES:[DI + 0x2c],0x5
0000:bbbd  JLE 0x0000:bbe6
0000:bbbf  MOV byte ptr [0x88ae],0x2
0000:bbc4  JMP 0x0000:bbe6
0000:bbc6  INC word ptr ES:[DI + 0x2f]
0000:bbca  CMP word ptr ES:[DI + 0x2f],0x64
0000:bbcf  JLE 0x0000:bbe6
0000:bbd1  MOV word ptr ES:[DI + 0x2f],0x0
0000:bbd7  MOV byte ptr ES:[DI + 0x2e],0x0
0000:bbdc  MOV SI,0x32fa
0000:bbdf  CALLF 0x0000:ffff
0000:bbe4  JMP 0x0000:bbe6
0000:bbe6  CALLF 0x0000:ffff
0000:bbeb  RET
0000:bbec  CMP byte ptr [0x88ae],0x2
0000:bbf1  JGE 0x0000:bea6
0000:bbf5  MOV DX,0x32
0000:bbf8  MOV CX,0x64
0000:bbfb  MOV BX,0xffce
0000:bbfe  MOV AX,0xffce
0000:bc01  CALLF 0x0000:ffff
0000:bc06  MOV DX,0x32
0000:bc09  NEG DX
0000:bc0b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc10  JS 0x0000:bc14
0000:bc12  NEG DX
0000:bc14  MOV AX,word ptr ES:[DI + 0x8]
0000:bc18  DEC AX
0000:bc19  MOV BX,word ptr ES:[DI + 0x4]
0000:bc1d  ADD BX,DX
0000:bc1f  CALLF 0x0000:ffff
0000:bc24  JNZ 0x0000:bc69
0000:bc26  MOV DX,0x32
0000:bc29  NEG DX
0000:bc2b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc30  JS 0x0000:bc34
0000:bc32  NEG DX
0000:bc34  MOV AX,word ptr ES:[DI + 0x8]
0000:bc38  SUB AX,0x11
0000:bc3b  MOV BX,word ptr ES:[DI + 0x4]
0000:bc3f  ADD BX,DX
0000:bc41  CALLF 0x0000:ffff
0000:bc46  JNZ 0x0000:bc69
0000:bc48  MOV DX,0x32
0000:bc4b  NEG DX
0000:bc4d  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc52  JS 0x0000:bc57
0000:bc54  MOV DX,0x32
0000:bc57  MOV AX,word ptr ES:[DI + 0x8]
0000:bc5b  SUB AX,0xc
0000:bc5e  MOV BX,word ptr ES:[DI + 0x4]
0000:bc62  ADD BX,DX
0000:bc64  CALLF 0x0000:ffff
0000:bc69  JMP 0x0000:bc6d
0000:bc6d  JZ 0x0000:bc74
0000:bc6f  MOV byte ptr ES:[DI + 0x3e],0x1
0000:bc74  CMP byte ptr ES:[DI + 0x34],0x1
0000:bc79  JGE 0x0000:bdc0
0000:bc7d  CMP byte ptr ES:[DI + 0x3e],0x0
0000:bc82  JLE 0x0000:bd74
0000:bc86  CMP byte ptr ES:[DI + 0x40],0x0
0000:bc8b  JGE 0x0000:bd1a
0000:bc8f  CMP word ptr ES:[DI + 0x42],0x14
0000:bc94  JNZ 0x0000:bc96
0000:bc96  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bc9b  MOV AL,byte ptr ES:[DI + 0x29]
0000:bc9f  CBW
0000:bca0  CWDE
0000:bca2  SHL EAX,0xc
0000:bca6  SUB EBX,EAX
0000:bca9  CMP EBX,0xffff7000
0000:bcb0  JL 0x0000:bcc7
0000:bcb2  CMP EBX,0x9000
0000:bcb9  JG 0x0000:bcbe
0000:bcbb  CLC
0000:bcbc  JMP 0x0000:bcce
0000:bcbe  MOV EBX,0x9000
0000:bcc4  STC
0000:bcc5  JMP 0x0000:bcce
0000:bcc7  MOV EBX,0xffff7000
0000:bccd  STC
0000:bcce  MOV dword ptr ES:[DI + 0xa],EBX
0000:bcd3  ADD dword ptr ES:[DI + 0x2],EBX
0000:bcd8  DEC word ptr ES:[DI + 0x42]
0000:bcdc  JGE 0x0000:bd71
0000:bce0  NEG byte ptr ES:[DI + 0x29]
0000:bce4  NEG byte ptr ES:[DI + 0x28]
0000:bce8  NEG byte ptr ES:[DI + 0x40]
0000:bcec  CMP word ptr ES:[DI + 0x12],0xdc
0000:bcf2  JZ 0x0000:bcfc
0000:bcf4  MOV word ptr ES:[DI + 0x12],0xdc
0000:bcfa  JMP 0x0000:bd02
0000:bcfc  MOV word ptr ES:[DI + 0x12],0x10e
0000:bd02  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd06  CBW
0000:bd07  CWDE
0000:bd09  SHL EAX,0x9
0000:bd0d  MOV dword ptr ES:[DI + 0xa],EAX
0000:bd12  MOV word ptr ES:[DI + 0x42],0x14
0000:bd18  JMP 0x0000:bd71
0000:bd1a  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bd1f  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd23  CBW
0000:bd24  CWDE
0000:bd26  SHL EAX,0xa
0000:bd2a  ADD EBX,EAX
0000:bd2d  CMP EBX,0xffff7000
0000:bd34  JL 0x0000:bd4b
0000:bd36  CMP EBX,0x9000
0000:bd3d  JG 0x0000:bd42
0000:bd3f  CLC
0000:bd40  JMP 0x0000:bd52
0000:bd42  MOV EBX,0x9000
0000:bd48  STC
0000:bd49  JMP 0x0000:bd52
0000:bd4b  MOV EBX,0xffff7000
0000:bd51  STC
0000:bd52  MOV dword ptr ES:[DI + 0xa],EBX
0000:bd57  ADD dword ptr ES:[DI + 0x2],EBX
0000:bd5c  DEC word ptr ES:[DI + 0x42]
0000:bd60  JGE 0x0000:bd71
0000:bd62  NEG byte ptr ES:[DI + 0x40]
0000:bd66  MOV byte ptr ES:[DI + 0x3e],0xff
0000:bd6b  MOV word ptr ES:[DI + 0x42],0x14
0000:bd71  JMP 0x0000:be0f
0000:bd74  MOV EAX,dword ptr ES:[DI + 0xa]
0000:bd79  ADD dword ptr ES:[DI + 0x2],EAX
0000:bd7e  MOV AX,word ptr ES:[DI + 0x2c]
0000:bd82  SUB word ptr ES:[DI + 0x8],AX
0000:bd86  MOV SI,0x7974
0000:bd89  MOV AX,word ptr ES:[DI + 0x2e]
0000:bd8d  ADD AX,0x20
0000:bd90  AND AX,0x7ff
0000:bd93  MOV word ptr ES:[DI + 0x2e],AX
0000:bd97  ADD SI,AX
0000:bd99  MOV AL,byte ptr [SI]
0000:bd9b  SAR AL,0x4
0000:bd9e  CBW
0000:bd9f  MOV word ptr ES:[DI + 0x2c],AX
0000:bda3  ADD word ptr ES:[DI + 0x8],AX
0000:bda7  INC word ptr ES:[DI + 0x38]
0000:bdab  CMP word ptr ES:[DI + 0x38],0xaa
0000:bdb1  JLE 0x0000:be0f
0000:bdb3  MOV word ptr ES:[DI + 0x38],0x0
0000:bdb9  MOV byte ptr ES:[DI + 0x34],0x1
0000:bdbe  JMP 0x0000:be0f
0000:bdc0  PUSH DI
0000:bdc1  MOV AX,0xc104
0000:bdc4  XOR DX,DX
0000:bdc6  CALLF 0x0000:ffff
0000:bdcb  POP SI
0000:bdcc  MOV byte ptr ES:[DI + 0x17],0x2
0000:bdd1  MOV AL,byte ptr ES:[SI + 0x29]
0000:bdd5  MOV byte ptr ES:[DI + 0x29],AL
0000:bdd9  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bdde  CMP byte ptr ES:[SI + 0x28],0x1
0000:bde3  JNZ 0x0000:bded
0000:bde5  ADD EAX,0x70000
0000:bdeb  JMP 0x0000:bdf3
0000:bded  SUB EAX,0x70000
0000:bdf3  MOV dword ptr ES:[DI + 0x2],EAX
0000:bdf8  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bdfd  ADD EAX,0x250000

; ---- B84D count=64 ----
0000:b84d  MOV word ptr ES:[DI + 0x12],0x386
0000:b853  MOV word ptr ES:[DI + 0x18],0xb87b
0000:b859  MOV word ptr ES:[DI + 0x3a],0x0
0000:b85f  MOV dword ptr ES:[DI + 0x3c],0x30000
0000:b868  MOV dword ptr ES:[DI + 0xa],0x0
0000:b871  MOV dword ptr ES:[DI + 0xe],0xfffeb000
0000:b87a  RET
0000:b87b  MOV AX,word ptr ES:[DI + 0x4]
0000:b87f  SUB AX,word ptr [0x81c0]
0000:b883  ADD AX,0x10
0000:b886  CMP AX,0x160
0000:b889  JA 0x0000:b89e
0000:b88b  MOV AX,word ptr ES:[DI + 0x8]
0000:b88f  SUB AX,word ptr [0x81c4]
0000:b893  ADD AX,0x10
0000:b896  CMP AX,0xd0
0000:b899  JA 0x0000:b89e
0000:b89b  CLC
0000:b89c  JMP 0x0000:b8a5
0000:b89e  STC
0000:b89f  MOV word ptr ES:[DI + 0x18],0x0
0000:b8a5  CMP dword ptr ES:[DI + 0xe],0x0
0000:b8ab  JLE 0x0000:b932
0000:b8af  CMP word ptr ES:[DI + 0x3a],0x1
0000:b8b4  JZ 0x0000:b943
0000:b8b8  MOV CX,0x0
0000:b8bb  MOV DX,0x0
0000:b8be  CALLF 0x0000:ffff
0000:b8c3  JNC 0x0000:b8c7
0000:b8c5  JMP 0x0000:b920
0000:b8c7  CMP byte ptr ES:[DI + 0x29],0x0
0000:b8cc  JG 0x0000:b8f3
0000:b8ce  MOV AX,word ptr ES:[DI + 0x8]
0000:b8d2  MOV BX,word ptr ES:[DI + 0x4]
0000:b8d6  CALLF 0x0000:ffff
0000:b8db  TEST DL,0x70
0000:b8de  JNZ 0x0000:b918
0000:b8e0  MOV AX,word ptr ES:[DI + 0x8]
0000:b8e4  SUB AX,0x10
0000:b8e7  CALLF 0x0000:ffff
0000:b8ec  TEST DL,0x70
0000:b8ef  JNZ 0x0000:b918
0000:b8f1  JMP 0x0000:b91a
0000:b8f3  MOV AX,word ptr ES:[DI + 0x8]
0000:b8f7  MOV BX,word ptr ES:[DI + 0x4]
0000:b8fb  CALLF 0x0000:ffff
0000:b900  TEST DL,0x70
0000:b903  JNZ 0x0000:b918
0000:b905  MOV AX,word ptr ES:[DI + 0x8]
0000:b909  SUB AX,0x10
0000:b90c  CALLF 0x0000:ffff
0000:b911  TEST DL,0x70
0000:b914  JNZ 0x0000:b918
0000:b916  JMP 0x0000:b91a
0000:b918  JMP 0x0000:b91c
0000:b91a  JMP 0x0000:b932
0000:b91c  NEG byte ptr ES:[DI + 0x29]
0000:b920  NEG dword ptr ES:[DI + 0xe]
0000:b925  INC word ptr ES:[DI + 0x3a]
0000:b929  SUB dword ptr ES:[DI + 0x3c],0x5000
0000:b932  MOV BX,0xfffb
0000:b935  MOV AX,0x5
0000:b938  CALLF 0x0000:ffff
0000:b93d  JNC 0x0000:b943

; ---- B87B count=64 ----
0000:b87b  MOV AX,word ptr ES:[DI + 0x4]
0000:b87f  SUB AX,word ptr [0x81c0]
0000:b883  ADD AX,0x10
0000:b886  CMP AX,0x160
0000:b889  JA 0x0000:b89e
0000:b88b  MOV AX,word ptr ES:[DI + 0x8]
0000:b88f  SUB AX,word ptr [0x81c4]
0000:b893  ADD AX,0x10
0000:b896  CMP AX,0xd0
0000:b899  JA 0x0000:b89e
0000:b89b  CLC
0000:b89c  JMP 0x0000:b8a5
0000:b89e  STC
0000:b89f  MOV word ptr ES:[DI + 0x18],0x0
0000:b8a5  CMP dword ptr ES:[DI + 0xe],0x0
0000:b8ab  JLE 0x0000:b932
0000:b8af  CMP word ptr ES:[DI + 0x3a],0x1
0000:b8b4  JZ 0x0000:b943
0000:b8b8  MOV CX,0x0
0000:b8bb  MOV DX,0x0
0000:b8be  CALLF 0x0000:ffff
0000:b8c3  JNC 0x0000:b8c7
0000:b8c5  JMP 0x0000:b920
0000:b8c7  CMP byte ptr ES:[DI + 0x29],0x0
0000:b8cc  JG 0x0000:b8f3
0000:b8ce  MOV AX,word ptr ES:[DI + 0x8]
0000:b8d2  MOV BX,word ptr ES:[DI + 0x4]
0000:b8d6  CALLF 0x0000:ffff
0000:b8db  TEST DL,0x70
0000:b8de  JNZ 0x0000:b918
0000:b8e0  MOV AX,word ptr ES:[DI + 0x8]
0000:b8e4  SUB AX,0x10
0000:b8e7  CALLF 0x0000:ffff
0000:b8ec  TEST DL,0x70
0000:b8ef  JNZ 0x0000:b918
0000:b8f1  JMP 0x0000:b91a
0000:b8f3  MOV AX,word ptr ES:[DI + 0x8]
0000:b8f7  MOV BX,word ptr ES:[DI + 0x4]
0000:b8fb  CALLF 0x0000:ffff
0000:b900  TEST DL,0x70
0000:b903  JNZ 0x0000:b918
0000:b905  MOV AX,word ptr ES:[DI + 0x8]
0000:b909  SUB AX,0x10
0000:b90c  CALLF 0x0000:ffff
0000:b911  TEST DL,0x70
0000:b914  JNZ 0x0000:b918
0000:b916  JMP 0x0000:b91a
0000:b918  JMP 0x0000:b91c
0000:b91a  JMP 0x0000:b932
0000:b91c  NEG byte ptr ES:[DI + 0x29]
0000:b920  NEG dword ptr ES:[DI + 0xe]
0000:b925  INC word ptr ES:[DI + 0x3a]
0000:b929  SUB dword ptr ES:[DI + 0x3c],0x5000
0000:b932  MOV BX,0xfffb
0000:b935  MOV AX,0x5
0000:b938  CALLF 0x0000:ffff
0000:b93d  JNC 0x0000:b943
0000:b93f  NEG byte ptr ES:[DI + 0x29]
0000:b943  MOV EAX,dword ptr ES:[DI + 0xe]
0000:b948  CMP EAX,0x0
0000:b94c  JG 0x0000:b977
0000:b94e  ADD EAX,0x3a98
0000:b954  CMP EAX,0xfffd0000
0000:b95a  JL 0x0000:b96e

; ---- B9F3 count=192 ----
0000:b9f3  MOV byte ptr ES:[DI + 0x17],0x1
0000:b9f8  MOV word ptr ES:[DI + 0x18],0xbbec
0000:b9fe  MOV word ptr ES:[DI + 0x12],0x10e
0000:ba04  MOV byte ptr ES:[DI + 0x28],0xff
0000:ba09  MOV byte ptr ES:[DI + 0x29],0xff
0000:ba0e  MOV word ptr ES:[DI + 0x38],0x0
0000:ba14  MOV word ptr ES:[DI + 0x44],0x0
0000:ba1a  MOV byte ptr ES:[DI + 0x34],0x0
0000:ba1f  MOV dword ptr ES:[DI + 0xa],0xffff7000
0000:ba28  MOV byte ptr ES:[DI + 0x40],0xff
0000:ba2d  MOV word ptr ES:[DI + 0x42],0x14
0000:ba33  MOV byte ptr ES:[DI + 0x3e],0xff
0000:ba38  PUSH DI
0000:ba39  MOV AX,0xbaa1
0000:ba3c  XOR DX,DX
0000:ba3e  CALLF 0x0000:ffff
0000:ba43  POP SI
0000:ba44  MOV word ptr ES:[SI + 0x2a],DI
0000:ba48  MOV byte ptr ES:[DI + 0x17],0x2
0000:ba4d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ba52  SUB EAX,0x60000
0000:ba58  MOV dword ptr ES:[DI + 0x2],EAX
0000:ba5d  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ba62  SUB EAX,0x1d0000
0000:ba68  MOV dword ptr ES:[DI + 0x6],EAX
0000:ba6d  MOV DI,SI
0000:ba6f  PUSH DI
0000:ba70  MOV AX,0xbabc
0000:ba73  XOR DX,DX
0000:ba75  CALLF 0x0000:ffff
0000:ba7a  POP SI
0000:ba7b  MOV word ptr ES:[SI + 0x36],DI
0000:ba7f  MOV byte ptr ES:[DI + 0x17],0x2
0000:ba84  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ba89  SUB EAX,0x1f0000
0000:ba8f  MOV dword ptr ES:[DI + 0x2],EAX
0000:ba94  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ba99  MOV dword ptr ES:[DI + 0x6],EAX
0000:ba9e  MOV DI,SI
0000:baa0  RET
0000:baa1  MOV SI,0x341a
0000:baa4  CALLF 0x0000:ffff
0000:baa9  MOV word ptr ES:[DI + 0x18],0xbad7
0000:baaf  MOV word ptr ES:[DI + 0x2e],0x0
0000:bab5  MOV word ptr ES:[DI + 0x2c],0x0
0000:babb  RET
0000:babc  MOV SI,0x32fa
0000:babf  CALLF 0x0000:ffff
0000:bac4  MOV word ptr ES:[DI + 0x18],0xbb0e
0000:baca  MOV word ptr ES:[DI + 0x2a],0x0
0000:bad0  MOV word ptr ES:[DI + 0x2c],0x0
0000:bad6  RET
0000:bad7  CMP byte ptr [0x88ae],0x5
0000:badc  JNZ 0x0000:bb08
0000:bade  MOV AX,word ptr ES:[DI + 0x4]
0000:bae2  SUB AX,word ptr [0x81c0]
0000:bae6  ADD AX,0x10
0000:bae9  CMP AX,0x160
0000:baec  JA 0x0000:bb01
0000:baee  MOV AX,word ptr ES:[DI + 0x8]
0000:baf2  SUB AX,word ptr [0x81c4]
0000:baf6  ADD AX,0x10
0000:baf9  CMP AX,0xd0
0000:bafc  JA 0x0000:bb01
0000:bafe  CLC
0000:baff  JMP 0x0000:bb08
0000:bb01  STC
0000:bb02  MOV word ptr ES:[DI + 0x18],0x0
0000:bb08  CALLF 0x0000:ffff
0000:bb0d  RET
0000:bb0e  CMP byte ptr ES:[DI + 0x2e],0x1
0000:bb13  JGE 0x0000:bbc6
0000:bb17  CMP word ptr [0x8806],0x0
0000:bb1c  JZ 0x0000:bbe6
0000:bb20  MOV BX,word ptr ES:[DI + 0x2a]
0000:bb24  CMP BX,word ptr [0x8808]
0000:bb28  JLE 0x0000:bb32
0000:bb2a  MOV word ptr ES:[DI + 0x2a],0x0
0000:bb30  XOR BX,BX
0000:bb32  SHL BX,0x2
0000:bb35  MOV AX,word ptr ES:[DI + 0x4]
0000:bb39  SUB AX,0xf
0000:bb3c  CMP word ptr [BX + 0x87de],AX
0000:bb40  JLE 0x0000:bb69
0000:bb42  ADD AX,0x1e
0000:bb45  CMP word ptr [BX + 0x87de],AX
0000:bb49  JGE 0x0000:bb69
0000:bb4b  MOV AX,word ptr ES:[DI + 0x8]
0000:bb4f  ADD AX,0x5
0000:bb52  CMP word ptr [BX + 0x87e0],AX
0000:bb56  JGE 0x0000:bb69
0000:bb58  SUB AX,0x1e
0000:bb5b  CMP word ptr [BX + 0x87e0],AX
0000:bb5f  JLE 0x0000:bb69
0000:bb61  MOV word ptr [BX + 0x87de],0x0
0000:bb67  JMP 0x0000:bb6b
0000:bb69  JMP 0x0000:bbb4
0000:bb6b  INC word ptr ES:[DI + 0x2c]
0000:bb6f  PUSH DI
0000:bb70  MOV AX,0x4b70
0000:bb73  XOR DX,DX
0000:bb75  CALLF 0x0000:ffff
0000:bb7a  POP SI
0000:bb7b  MOV byte ptr ES:[DI + 0x17],0x2
0000:bb80  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bb85  MOV dword ptr ES:[DI + 0x2],EAX
0000:bb8a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bb8f  ADD EAX,0xa0000
0000:bb95  MOV dword ptr ES:[DI + 0x6],EAX
0000:bb9a  MOV DI,SI
0000:bb9c  MOV SI,0x3308
0000:bb9f  CALLF 0x0000:ffff
0000:bba4  MOV word ptr [0x612e],0xd
0000:bbaa  CALLF 0x0000:ffff
0000:bbaf  MOV byte ptr ES:[DI + 0x2e],0x1
0000:bbb4  INC word ptr ES:[DI + 0x2a]
0000:bbb8  CMP word ptr ES:[DI + 0x2c],0x5
0000:bbbd  JLE 0x0000:bbe6
0000:bbbf  MOV byte ptr [0x88ae],0x2
0000:bbc4  JMP 0x0000:bbe6
0000:bbc6  INC word ptr ES:[DI + 0x2f]
0000:bbca  CMP word ptr ES:[DI + 0x2f],0x64
0000:bbcf  JLE 0x0000:bbe6
0000:bbd1  MOV word ptr ES:[DI + 0x2f],0x0
0000:bbd7  MOV byte ptr ES:[DI + 0x2e],0x0
0000:bbdc  MOV SI,0x32fa
0000:bbdf  CALLF 0x0000:ffff
0000:bbe4  JMP 0x0000:bbe6
0000:bbe6  CALLF 0x0000:ffff
0000:bbeb  RET
0000:bbec  CMP byte ptr [0x88ae],0x2
0000:bbf1  JGE 0x0000:bea6
0000:bbf5  MOV DX,0x32
0000:bbf8  MOV CX,0x64
0000:bbfb  MOV BX,0xffce
0000:bbfe  MOV AX,0xffce
0000:bc01  CALLF 0x0000:ffff
0000:bc06  MOV DX,0x32
0000:bc09  NEG DX
0000:bc0b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc10  JS 0x0000:bc14
0000:bc12  NEG DX
0000:bc14  MOV AX,word ptr ES:[DI + 0x8]
0000:bc18  DEC AX
0000:bc19  MOV BX,word ptr ES:[DI + 0x4]
0000:bc1d  ADD BX,DX
0000:bc1f  CALLF 0x0000:ffff
0000:bc24  JNZ 0x0000:bc69
0000:bc26  MOV DX,0x32
0000:bc29  NEG DX
0000:bc2b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc30  JS 0x0000:bc34
0000:bc32  NEG DX
0000:bc34  MOV AX,word ptr ES:[DI + 0x8]
0000:bc38  SUB AX,0x11
0000:bc3b  MOV BX,word ptr ES:[DI + 0x4]
0000:bc3f  ADD BX,DX
0000:bc41  CALLF 0x0000:ffff
0000:bc46  JNZ 0x0000:bc69
0000:bc48  MOV DX,0x32
0000:bc4b  NEG DX
0000:bc4d  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc52  JS 0x0000:bc57
0000:bc54  MOV DX,0x32
0000:bc57  MOV AX,word ptr ES:[DI + 0x8]
0000:bc5b  SUB AX,0xc
0000:bc5e  MOV BX,word ptr ES:[DI + 0x4]
0000:bc62  ADD BX,DX
0000:bc64  CALLF 0x0000:ffff
0000:bc69  JMP 0x0000:bc6d
0000:bc6d  JZ 0x0000:bc74
0000:bc6f  MOV byte ptr ES:[DI + 0x3e],0x1
0000:bc74  CMP byte ptr ES:[DI + 0x34],0x1
0000:bc79  JGE 0x0000:bdc0
0000:bc7d  CMP byte ptr ES:[DI + 0x3e],0x0
0000:bc82  JLE 0x0000:bd74
0000:bc86  CMP byte ptr ES:[DI + 0x40],0x0
0000:bc8b  JGE 0x0000:bd1a
0000:bc8f  CMP word ptr ES:[DI + 0x42],0x14
0000:bc94  JNZ 0x0000:bc96
0000:bc96  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bc9b  MOV AL,byte ptr ES:[DI + 0x29]
0000:bc9f  CBW
0000:bca0  CWDE
0000:bca2  SHL EAX,0xc
0000:bca6  SUB EBX,EAX
0000:bca9  CMP EBX,0xffff7000
0000:bcb0  JL 0x0000:bcc7
0000:bcb2  CMP EBX,0x9000
0000:bcb9  JG 0x0000:bcbe
0000:bcbb  CLC
0000:bcbc  JMP 0x0000:bcce

; ---- BB0E count=224 ----
0000:bb0e  CMP byte ptr ES:[DI + 0x2e],0x1
0000:bb13  JGE 0x0000:bbc6
0000:bb17  CMP word ptr [0x8806],0x0
0000:bb1c  JZ 0x0000:bbe6
0000:bb20  MOV BX,word ptr ES:[DI + 0x2a]
0000:bb24  CMP BX,word ptr [0x8808]
0000:bb28  JLE 0x0000:bb32
0000:bb2a  MOV word ptr ES:[DI + 0x2a],0x0
0000:bb30  XOR BX,BX
0000:bb32  SHL BX,0x2
0000:bb35  MOV AX,word ptr ES:[DI + 0x4]
0000:bb39  SUB AX,0xf
0000:bb3c  CMP word ptr [BX + 0x87de],AX
0000:bb40  JLE 0x0000:bb69
0000:bb42  ADD AX,0x1e
0000:bb45  CMP word ptr [BX + 0x87de],AX
0000:bb49  JGE 0x0000:bb69
0000:bb4b  MOV AX,word ptr ES:[DI + 0x8]
0000:bb4f  ADD AX,0x5
0000:bb52  CMP word ptr [BX + 0x87e0],AX
0000:bb56  JGE 0x0000:bb69
0000:bb58  SUB AX,0x1e
0000:bb5b  CMP word ptr [BX + 0x87e0],AX
0000:bb5f  JLE 0x0000:bb69
0000:bb61  MOV word ptr [BX + 0x87de],0x0
0000:bb67  JMP 0x0000:bb6b
0000:bb69  JMP 0x0000:bbb4
0000:bb6b  INC word ptr ES:[DI + 0x2c]
0000:bb6f  PUSH DI
0000:bb70  MOV AX,0x4b70
0000:bb73  XOR DX,DX
0000:bb75  CALLF 0x0000:ffff
0000:bb7a  POP SI
0000:bb7b  MOV byte ptr ES:[DI + 0x17],0x2
0000:bb80  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bb85  MOV dword ptr ES:[DI + 0x2],EAX
0000:bb8a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bb8f  ADD EAX,0xa0000
0000:bb95  MOV dword ptr ES:[DI + 0x6],EAX
0000:bb9a  MOV DI,SI
0000:bb9c  MOV SI,0x3308
0000:bb9f  CALLF 0x0000:ffff
0000:bba4  MOV word ptr [0x612e],0xd
0000:bbaa  CALLF 0x0000:ffff
0000:bbaf  MOV byte ptr ES:[DI + 0x2e],0x1
0000:bbb4  INC word ptr ES:[DI + 0x2a]
0000:bbb8  CMP word ptr ES:[DI + 0x2c],0x5
0000:bbbd  JLE 0x0000:bbe6
0000:bbbf  MOV byte ptr [0x88ae],0x2
0000:bbc4  JMP 0x0000:bbe6
0000:bbc6  INC word ptr ES:[DI + 0x2f]
0000:bbca  CMP word ptr ES:[DI + 0x2f],0x64
0000:bbcf  JLE 0x0000:bbe6
0000:bbd1  MOV word ptr ES:[DI + 0x2f],0x0
0000:bbd7  MOV byte ptr ES:[DI + 0x2e],0x0
0000:bbdc  MOV SI,0x32fa
0000:bbdf  CALLF 0x0000:ffff
0000:bbe4  JMP 0x0000:bbe6
0000:bbe6  CALLF 0x0000:ffff
0000:bbeb  RET
0000:bbec  CMP byte ptr [0x88ae],0x2
0000:bbf1  JGE 0x0000:bea6
0000:bbf5  MOV DX,0x32
0000:bbf8  MOV CX,0x64
0000:bbfb  MOV BX,0xffce
0000:bbfe  MOV AX,0xffce
0000:bc01  CALLF 0x0000:ffff
0000:bc06  MOV DX,0x32
0000:bc09  NEG DX
0000:bc0b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc10  JS 0x0000:bc14
0000:bc12  NEG DX
0000:bc14  MOV AX,word ptr ES:[DI + 0x8]
0000:bc18  DEC AX
0000:bc19  MOV BX,word ptr ES:[DI + 0x4]
0000:bc1d  ADD BX,DX
0000:bc1f  CALLF 0x0000:ffff
0000:bc24  JNZ 0x0000:bc69
0000:bc26  MOV DX,0x32
0000:bc29  NEG DX
0000:bc2b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc30  JS 0x0000:bc34
0000:bc32  NEG DX
0000:bc34  MOV AX,word ptr ES:[DI + 0x8]
0000:bc38  SUB AX,0x11
0000:bc3b  MOV BX,word ptr ES:[DI + 0x4]
0000:bc3f  ADD BX,DX
0000:bc41  CALLF 0x0000:ffff
0000:bc46  JNZ 0x0000:bc69
0000:bc48  MOV DX,0x32
0000:bc4b  NEG DX
0000:bc4d  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc52  JS 0x0000:bc57
0000:bc54  MOV DX,0x32
0000:bc57  MOV AX,word ptr ES:[DI + 0x8]
0000:bc5b  SUB AX,0xc
0000:bc5e  MOV BX,word ptr ES:[DI + 0x4]
0000:bc62  ADD BX,DX
0000:bc64  CALLF 0x0000:ffff
0000:bc69  JMP 0x0000:bc6d
0000:bc6d  JZ 0x0000:bc74
0000:bc6f  MOV byte ptr ES:[DI + 0x3e],0x1
0000:bc74  CMP byte ptr ES:[DI + 0x34],0x1
0000:bc79  JGE 0x0000:bdc0
0000:bc7d  CMP byte ptr ES:[DI + 0x3e],0x0
0000:bc82  JLE 0x0000:bd74
0000:bc86  CMP byte ptr ES:[DI + 0x40],0x0
0000:bc8b  JGE 0x0000:bd1a
0000:bc8f  CMP word ptr ES:[DI + 0x42],0x14
0000:bc94  JNZ 0x0000:bc96
0000:bc96  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bc9b  MOV AL,byte ptr ES:[DI + 0x29]
0000:bc9f  CBW
0000:bca0  CWDE
0000:bca2  SHL EAX,0xc
0000:bca6  SUB EBX,EAX
0000:bca9  CMP EBX,0xffff7000
0000:bcb0  JL 0x0000:bcc7
0000:bcb2  CMP EBX,0x9000
0000:bcb9  JG 0x0000:bcbe
0000:bcbb  CLC
0000:bcbc  JMP 0x0000:bcce
0000:bcbe  MOV EBX,0x9000
0000:bcc4  STC
0000:bcc5  JMP 0x0000:bcce
0000:bcc7  MOV EBX,0xffff7000
0000:bccd  STC
0000:bcce  MOV dword ptr ES:[DI + 0xa],EBX
0000:bcd3  ADD dword ptr ES:[DI + 0x2],EBX
0000:bcd8  DEC word ptr ES:[DI + 0x42]
0000:bcdc  JGE 0x0000:bd71
0000:bce0  NEG byte ptr ES:[DI + 0x29]
0000:bce4  NEG byte ptr ES:[DI + 0x28]
0000:bce8  NEG byte ptr ES:[DI + 0x40]
0000:bcec  CMP word ptr ES:[DI + 0x12],0xdc
0000:bcf2  JZ 0x0000:bcfc
0000:bcf4  MOV word ptr ES:[DI + 0x12],0xdc
0000:bcfa  JMP 0x0000:bd02
0000:bcfc  MOV word ptr ES:[DI + 0x12],0x10e
0000:bd02  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd06  CBW
0000:bd07  CWDE
0000:bd09  SHL EAX,0x9
0000:bd0d  MOV dword ptr ES:[DI + 0xa],EAX
0000:bd12  MOV word ptr ES:[DI + 0x42],0x14
0000:bd18  JMP 0x0000:bd71
0000:bd1a  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bd1f  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd23  CBW
0000:bd24  CWDE
0000:bd26  SHL EAX,0xa
0000:bd2a  ADD EBX,EAX
0000:bd2d  CMP EBX,0xffff7000
0000:bd34  JL 0x0000:bd4b
0000:bd36  CMP EBX,0x9000
0000:bd3d  JG 0x0000:bd42
0000:bd3f  CLC
0000:bd40  JMP 0x0000:bd52
0000:bd42  MOV EBX,0x9000
0000:bd48  STC
0000:bd49  JMP 0x0000:bd52
0000:bd4b  MOV EBX,0xffff7000
0000:bd51  STC
0000:bd52  MOV dword ptr ES:[DI + 0xa],EBX
0000:bd57  ADD dword ptr ES:[DI + 0x2],EBX
0000:bd5c  DEC word ptr ES:[DI + 0x42]
0000:bd60  JGE 0x0000:bd71
0000:bd62  NEG byte ptr ES:[DI + 0x40]
0000:bd66  MOV byte ptr ES:[DI + 0x3e],0xff
0000:bd6b  MOV word ptr ES:[DI + 0x42],0x14
0000:bd71  JMP 0x0000:be0f
0000:bd74  MOV EAX,dword ptr ES:[DI + 0xa]
0000:bd79  ADD dword ptr ES:[DI + 0x2],EAX
0000:bd7e  MOV AX,word ptr ES:[DI + 0x2c]
0000:bd82  SUB word ptr ES:[DI + 0x8],AX
0000:bd86  MOV SI,0x7974
0000:bd89  MOV AX,word ptr ES:[DI + 0x2e]
0000:bd8d  ADD AX,0x20
0000:bd90  AND AX,0x7ff
0000:bd93  MOV word ptr ES:[DI + 0x2e],AX
0000:bd97  ADD SI,AX
0000:bd99  MOV AL,byte ptr [SI]
0000:bd9b  SAR AL,0x4
0000:bd9e  CBW
0000:bd9f  MOV word ptr ES:[DI + 0x2c],AX
0000:bda3  ADD word ptr ES:[DI + 0x8],AX
0000:bda7  INC word ptr ES:[DI + 0x38]
0000:bdab  CMP word ptr ES:[DI + 0x38],0xaa
0000:bdb1  JLE 0x0000:be0f
0000:bdb3  MOV word ptr ES:[DI + 0x38],0x0
0000:bdb9  MOV byte ptr ES:[DI + 0x34],0x1
0000:bdbe  JMP 0x0000:be0f
0000:bdc0  PUSH DI
0000:bdc1  MOV AX,0xc104
0000:bdc4  XOR DX,DX
0000:bdc6  CALLF 0x0000:ffff
0000:bdcb  POP SI
0000:bdcc  MOV byte ptr ES:[DI + 0x17],0x2
0000:bdd1  MOV AL,byte ptr ES:[SI + 0x29]
0000:bdd5  MOV byte ptr ES:[DI + 0x29],AL
0000:bdd9  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bdde  CMP byte ptr ES:[SI + 0x28],0x1
0000:bde3  JNZ 0x0000:bded
0000:bde5  ADD EAX,0x70000
0000:bdeb  JMP 0x0000:bdf3
0000:bded  SUB EAX,0x70000
0000:bdf3  MOV dword ptr ES:[DI + 0x2],EAX
0000:bdf8  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bdfd  ADD EAX,0x250000
0000:be03  MOV dword ptr ES:[DI + 0x6],EAX
0000:be08  MOV DI,SI
0000:be0a  MOV byte ptr ES:[DI + 0x34],0x0
0000:be0f  PUSH DI
0000:be10  MOV SI,DI
0000:be12  MOV DI,word ptr ES:[DI + 0x2a]
0000:be16  MOV EAX,dword ptr ES:[SI + 0x2]
0000:be1b  CMP byte ptr ES:[SI + 0x28],0x1
0000:be20  JNZ 0x0000:be2a
0000:be22  ADD EAX,0x60000
0000:be28  JMP 0x0000:be30
0000:be2a  SUB EAX,0x60000
0000:be30  MOV dword ptr ES:[DI + 0x2],EAX
0000:be35  MOV EAX,dword ptr ES:[SI + 0x6]
0000:be3a  SUB EAX,0x1d0000

; ---- BBEC count=640 ----
0000:bbec  CMP byte ptr [0x88ae],0x2
0000:bbf1  JGE 0x0000:bea6
0000:bbf5  MOV DX,0x32
0000:bbf8  MOV CX,0x64
0000:bbfb  MOV BX,0xffce
0000:bbfe  MOV AX,0xffce
0000:bc01  CALLF 0x0000:ffff
0000:bc06  MOV DX,0x32
0000:bc09  NEG DX
0000:bc0b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc10  JS 0x0000:bc14
0000:bc12  NEG DX
0000:bc14  MOV AX,word ptr ES:[DI + 0x8]
0000:bc18  DEC AX
0000:bc19  MOV BX,word ptr ES:[DI + 0x4]
0000:bc1d  ADD BX,DX
0000:bc1f  CALLF 0x0000:ffff
0000:bc24  JNZ 0x0000:bc69
0000:bc26  MOV DX,0x32
0000:bc29  NEG DX
0000:bc2b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc30  JS 0x0000:bc34
0000:bc32  NEG DX
0000:bc34  MOV AX,word ptr ES:[DI + 0x8]
0000:bc38  SUB AX,0x11
0000:bc3b  MOV BX,word ptr ES:[DI + 0x4]
0000:bc3f  ADD BX,DX
0000:bc41  CALLF 0x0000:ffff
0000:bc46  JNZ 0x0000:bc69
0000:bc48  MOV DX,0x32
0000:bc4b  NEG DX
0000:bc4d  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc52  JS 0x0000:bc57
0000:bc54  MOV DX,0x32
0000:bc57  MOV AX,word ptr ES:[DI + 0x8]
0000:bc5b  SUB AX,0xc
0000:bc5e  MOV BX,word ptr ES:[DI + 0x4]
0000:bc62  ADD BX,DX
0000:bc64  CALLF 0x0000:ffff
0000:bc69  JMP 0x0000:bc6d
0000:bc6d  JZ 0x0000:bc74
0000:bc6f  MOV byte ptr ES:[DI + 0x3e],0x1
0000:bc74  CMP byte ptr ES:[DI + 0x34],0x1
0000:bc79  JGE 0x0000:bdc0
0000:bc7d  CMP byte ptr ES:[DI + 0x3e],0x0
0000:bc82  JLE 0x0000:bd74
0000:bc86  CMP byte ptr ES:[DI + 0x40],0x0
0000:bc8b  JGE 0x0000:bd1a
0000:bc8f  CMP word ptr ES:[DI + 0x42],0x14
0000:bc94  JNZ 0x0000:bc96
0000:bc96  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bc9b  MOV AL,byte ptr ES:[DI + 0x29]
0000:bc9f  CBW
0000:bca0  CWDE
0000:bca2  SHL EAX,0xc
0000:bca6  SUB EBX,EAX
0000:bca9  CMP EBX,0xffff7000
0000:bcb0  JL 0x0000:bcc7
0000:bcb2  CMP EBX,0x9000
0000:bcb9  JG 0x0000:bcbe
0000:bcbb  CLC
0000:bcbc  JMP 0x0000:bcce
0000:bcbe  MOV EBX,0x9000
0000:bcc4  STC
0000:bcc5  JMP 0x0000:bcce
0000:bcc7  MOV EBX,0xffff7000
0000:bccd  STC
0000:bcce  MOV dword ptr ES:[DI + 0xa],EBX
0000:bcd3  ADD dword ptr ES:[DI + 0x2],EBX
0000:bcd8  DEC word ptr ES:[DI + 0x42]
0000:bcdc  JGE 0x0000:bd71
0000:bce0  NEG byte ptr ES:[DI + 0x29]
0000:bce4  NEG byte ptr ES:[DI + 0x28]
0000:bce8  NEG byte ptr ES:[DI + 0x40]
0000:bcec  CMP word ptr ES:[DI + 0x12],0xdc
0000:bcf2  JZ 0x0000:bcfc
0000:bcf4  MOV word ptr ES:[DI + 0x12],0xdc
0000:bcfa  JMP 0x0000:bd02
0000:bcfc  MOV word ptr ES:[DI + 0x12],0x10e
0000:bd02  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd06  CBW
0000:bd07  CWDE
0000:bd09  SHL EAX,0x9
0000:bd0d  MOV dword ptr ES:[DI + 0xa],EAX
0000:bd12  MOV word ptr ES:[DI + 0x42],0x14
0000:bd18  JMP 0x0000:bd71
0000:bd1a  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bd1f  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd23  CBW
0000:bd24  CWDE
0000:bd26  SHL EAX,0xa
0000:bd2a  ADD EBX,EAX
0000:bd2d  CMP EBX,0xffff7000
0000:bd34  JL 0x0000:bd4b
0000:bd36  CMP EBX,0x9000
0000:bd3d  JG 0x0000:bd42
0000:bd3f  CLC
0000:bd40  JMP 0x0000:bd52
0000:bd42  MOV EBX,0x9000
0000:bd48  STC
0000:bd49  JMP 0x0000:bd52
0000:bd4b  MOV EBX,0xffff7000
0000:bd51  STC
0000:bd52  MOV dword ptr ES:[DI + 0xa],EBX
0000:bd57  ADD dword ptr ES:[DI + 0x2],EBX
0000:bd5c  DEC word ptr ES:[DI + 0x42]
0000:bd60  JGE 0x0000:bd71
0000:bd62  NEG byte ptr ES:[DI + 0x40]
0000:bd66  MOV byte ptr ES:[DI + 0x3e],0xff
0000:bd6b  MOV word ptr ES:[DI + 0x42],0x14
0000:bd71  JMP 0x0000:be0f
0000:bd74  MOV EAX,dword ptr ES:[DI + 0xa]
0000:bd79  ADD dword ptr ES:[DI + 0x2],EAX
0000:bd7e  MOV AX,word ptr ES:[DI + 0x2c]
0000:bd82  SUB word ptr ES:[DI + 0x8],AX
0000:bd86  MOV SI,0x7974
0000:bd89  MOV AX,word ptr ES:[DI + 0x2e]
0000:bd8d  ADD AX,0x20
0000:bd90  AND AX,0x7ff
0000:bd93  MOV word ptr ES:[DI + 0x2e],AX
0000:bd97  ADD SI,AX
0000:bd99  MOV AL,byte ptr [SI]
0000:bd9b  SAR AL,0x4
0000:bd9e  CBW
0000:bd9f  MOV word ptr ES:[DI + 0x2c],AX
0000:bda3  ADD word ptr ES:[DI + 0x8],AX
0000:bda7  INC word ptr ES:[DI + 0x38]
0000:bdab  CMP word ptr ES:[DI + 0x38],0xaa
0000:bdb1  JLE 0x0000:be0f
0000:bdb3  MOV word ptr ES:[DI + 0x38],0x0
0000:bdb9  MOV byte ptr ES:[DI + 0x34],0x1
0000:bdbe  JMP 0x0000:be0f
0000:bdc0  PUSH DI
0000:bdc1  MOV AX,0xc104
0000:bdc4  XOR DX,DX
0000:bdc6  CALLF 0x0000:ffff
0000:bdcb  POP SI
0000:bdcc  MOV byte ptr ES:[DI + 0x17],0x2
0000:bdd1  MOV AL,byte ptr ES:[SI + 0x29]
0000:bdd5  MOV byte ptr ES:[DI + 0x29],AL
0000:bdd9  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bdde  CMP byte ptr ES:[SI + 0x28],0x1
0000:bde3  JNZ 0x0000:bded
0000:bde5  ADD EAX,0x70000
0000:bdeb  JMP 0x0000:bdf3
0000:bded  SUB EAX,0x70000
0000:bdf3  MOV dword ptr ES:[DI + 0x2],EAX
0000:bdf8  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bdfd  ADD EAX,0x250000
0000:be03  MOV dword ptr ES:[DI + 0x6],EAX
0000:be08  MOV DI,SI
0000:be0a  MOV byte ptr ES:[DI + 0x34],0x0
0000:be0f  PUSH DI
0000:be10  MOV SI,DI
0000:be12  MOV DI,word ptr ES:[DI + 0x2a]
0000:be16  MOV EAX,dword ptr ES:[SI + 0x2]
0000:be1b  CMP byte ptr ES:[SI + 0x28],0x1
0000:be20  JNZ 0x0000:be2a
0000:be22  ADD EAX,0x60000
0000:be28  JMP 0x0000:be30
0000:be2a  SUB EAX,0x60000
0000:be30  MOV dword ptr ES:[DI + 0x2],EAX
0000:be35  MOV EAX,dword ptr ES:[SI + 0x6]
0000:be3a  SUB EAX,0x1d0000
0000:be40  MOV dword ptr ES:[DI + 0x6],EAX
0000:be45  MOV SI,0x7974
0000:be48  MOV AX,word ptr ES:[DI + 0x2e]
0000:be4c  ADD AX,0xa
0000:be4f  AND AX,0x6ff
0000:be52  MOV word ptr ES:[DI + 0x2e],AX
0000:be56  ADD SI,AX
0000:be58  MOV AL,byte ptr [SI]
0000:be5a  SAR AL,0x5
0000:be5d  CBW
0000:be5e  ADD word ptr ES:[DI + 0x8],AX
0000:be62  MOV AX,word ptr ES:[DI + 0x8]
0000:be66  MOV word ptr ES:[DI + 0x8],AX
0000:be6a  POP DI
0000:be6b  PUSH DI
0000:be6c  MOV SI,DI
0000:be6e  MOV DI,word ptr ES:[DI + 0x36]
0000:be72  MOV EBX,dword ptr ES:[SI + 0x2]
0000:be77  MOV AL,byte ptr ES:[SI + 0x28]
0000:be7b  MOV byte ptr ES:[DI + 0x28],AL
0000:be7f  CMP AL,0x1
0000:be81  JNZ 0x0000:be8c
0000:be83  ADD EBX,0x1f0000
0000:be8a  JMP 0x0000:be93
0000:be8c  SUB EBX,0x1f0000
0000:be93  MOV dword ptr ES:[DI + 0x2],EBX
0000:be98  MOV EAX,dword ptr ES:[SI + 0x6]
0000:be9d  MOV dword ptr ES:[DI + 0x6],EAX
0000:bea2  POP DI
0000:bea3  JMP 0x0000:c103
0000:bea6  CMP byte ptr [0x88ae],0x3
0000:beab  JGE 0x0000:bed9
0000:bead  PUSH DI
0000:beae  MOV SI,DI
0000:beb0  MOV DI,word ptr ES:[DI + 0x36]
0000:beb4  MOV word ptr ES:[DI + 0x18],0x0
0000:beba  POP DI
0000:bebb  CMP word ptr ES:[DI + 0x12],0xdc
0000:bec1  JZ 0x0000:becb
0000:bec3  MOV word ptr ES:[DI + 0x12],0x10f
0000:bec9  JMP 0x0000:bed1
0000:becb  MOV word ptr ES:[DI + 0x12],0xdd
0000:bed1  MOV byte ptr [0x88ae],0x3
0000:bed6  JMP 0x0000:c103
0000:bed9  CMP byte ptr [0x88ae],0x4
0000:bede  JGE 0x0000:c04d
0000:bee2  MOV AX,word ptr ES:[DI + 0x2c]
0000:bee6  SUB word ptr ES:[DI + 0x8],AX
0000:beea  MOV SI,0x7974
0000:beed  MOV AX,word ptr ES:[DI + 0x2e]
0000:bef1  ADD AX,0x20
0000:bef4  AND AX,0x5ff
0000:bef7  MOV word ptr ES:[DI + 0x2e],AX
0000:befb  ADD SI,AX
0000:befd  MOV AL,byte ptr [SI]
0000:beff  SAR AL,0x5
0000:bf02  CBW
0000:bf03  MOV word ptr ES:[DI + 0x2c],AX
0000:bf07  ADD word ptr ES:[DI + 0x8],AX
0000:bf0b  PUSH DI
0000:bf0c  MOV SI,DI
0000:bf0e  MOV DI,word ptr ES:[DI + 0x2a]
0000:bf12  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bf17  CMP byte ptr ES:[SI + 0x28],0x1
0000:bf1c  JNZ 0x0000:bf26
0000:bf1e  ADD EAX,0x60000
0000:bf24  JMP 0x0000:bf2c
0000:bf26  SUB EAX,0x60000
0000:bf2c  MOV dword ptr ES:[DI + 0x2],EAX
0000:bf31  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bf36  SUB EAX,0x1d0000
0000:bf3c  MOV dword ptr ES:[DI + 0x6],EAX
0000:bf41  MOV SI,0x7974
0000:bf44  MOV AX,word ptr ES:[DI + 0x2e]
0000:bf48  ADD AX,0xa
0000:bf4b  AND AX,0x6ff
0000:bf4e  MOV word ptr ES:[DI + 0x2e],AX
0000:bf52  ADD SI,AX
0000:bf54  MOV AL,byte ptr [SI]
0000:bf56  SAR AL,0x5
0000:bf59  CBW
0000:bf5a  ADD word ptr ES:[DI + 0x8],AX
0000:bf5e  MOV AX,word ptr ES:[DI + 0x8]
0000:bf62  MOV word ptr ES:[DI + 0x8],AX
0000:bf66  POP DI
0000:bf67  INC word ptr ES:[DI + 0x38]
0000:bf6b  CMP word ptr ES:[DI + 0x38],0x19
0000:bf70  JLE 0x0000:c103
0000:bf74  MOV word ptr ES:[DI + 0x38],0x0
0000:bf7a  PUSH DI
0000:bf7b  MOV AX,0x4b70
0000:bf7e  XOR DX,DX
0000:bf80  CALLF 0x0000:ffff
0000:bf85  POP SI
0000:bf86  MOV byte ptr ES:[DI + 0x17],0x2
0000:bf8b  PUSH SI
0000:bf8c  MOV SI,0x646c
0000:bf8f  ADD SI,word ptr [0x6468]
0000:bf93  INC word ptr [0x6468]
0000:bf97  AND word ptr [0x6468],0xff
0000:bf9d  MOV AL,byte ptr [SI]
0000:bf9f  POP SI
0000:bfa0  SHR AL,0x2
0000:bfa3  CBW
0000:bfa4  SUB AX,0x20
0000:bfa7  MOV BX,word ptr ES:[SI + 0x4]
0000:bfab  ADD BX,AX
0000:bfad  MOV word ptr ES:[DI + 0x4],BX
0000:bfb1  PUSH SI
0000:bfb2  MOV SI,0x646c
0000:bfb5  ADD SI,word ptr [0x6468]
0000:bfb9  INC word ptr [0x6468]
0000:bfbd  AND word ptr [0x6468],0xff
0000:bfc3  MOV AL,byte ptr [SI]
0000:bfc5  POP SI
0000:bfc6  SHR AL,0x3
0000:bfc9  CBW
0000:bfca  MOV BX,word ptr ES:[SI + 0x8]
0000:bfce  ADD BX,AX
0000:bfd0  MOV word ptr ES:[DI + 0x8],BX
0000:bfd4  MOV DI,SI
0000:bfd6  PUSH DI
0000:bfd7  MOV AX,0x4b70
0000:bfda  XOR DX,DX
0000:bfdc  CALLF 0x0000:ffff
0000:bfe1  POP SI
0000:bfe2  MOV byte ptr ES:[DI + 0x17],0x2
0000:bfe7  PUSH SI
0000:bfe8  MOV SI,0x646c
0000:bfeb  ADD SI,word ptr [0x6468]
0000:bfef  INC word ptr [0x6468]
0000:bff3  AND word ptr [0x6468],0xff
0000:bff9  MOV AL,byte ptr [SI]
0000:bffb  POP SI
0000:bffc  SHR AL,0x2
0000:bfff  CBW
0000:c000  SUB AX,0x20
0000:c003  MOV BX,word ptr ES:[SI + 0x4]
0000:c007  ADD BX,AX
0000:c009  MOV word ptr ES:[DI + 0x4],BX
0000:c00d  PUSH SI
0000:c00e  MOV SI,0x646c
0000:c011  ADD SI,word ptr [0x6468]
0000:c015  INC word ptr [0x6468]
0000:c019  AND word ptr [0x6468],0xff
0000:c01f  MOV AL,byte ptr [SI]
0000:c021  POP SI
0000:c022  SHR AL,0x3
0000:c025  CBW
0000:c026  MOV BX,word ptr ES:[SI + 0x8]
0000:c02a  ADD BX,AX
0000:c02c  MOV word ptr ES:[DI + 0x8],BX
0000:c030  MOV DI,SI
0000:c032  INC word ptr ES:[DI + 0x44]
0000:c036  CMP word ptr ES:[DI + 0x44],0xf
0000:c03b  JLE 0x0000:c103
0000:c03f  MOV dword ptr ES:[DI + 0xe],0xffff0000
0000:c048  MOV byte ptr [0x88ae],0x4
0000:c04d  CMP byte ptr [0x88ae],0x5
0000:c052  JGE 0x0000:c0db
0000:c056  INC word ptr ES:[DI + 0x38]
0000:c05a  CMP word ptr ES:[DI + 0x38],0x28
0000:c05f  JLE 0x0000:c103
0000:c063  MOV EAX,dword ptr ES:[DI + 0xe]
0000:c068  SUB dword ptr ES:[DI + 0xe],0x1200
0000:c071  ADD dword ptr ES:[DI + 0x6],EAX
0000:c076  PUSH DI
0000:c077  MOV SI,DI
0000:c079  MOV DI,word ptr ES:[DI + 0x2a]
0000:c07d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c082  CMP byte ptr ES:[SI + 0x28],0x1
0000:c087  JNZ 0x0000:c091
0000:c089  ADD EAX,0x60000
0000:c08f  JMP 0x0000:c097
0000:c091  SUB EAX,0x60000
0000:c097  MOV dword ptr ES:[DI + 0x2],EAX
0000:c09c  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c0a1  SUB EAX,0x1d0000
0000:c0a7  MOV dword ptr ES:[DI + 0x6],EAX
0000:c0ac  POP DI
0000:c0ad  MOV AX,word ptr ES:[DI + 0x4]
0000:c0b1  SUB AX,word ptr [0x81c0]
0000:c0b5  ADD AX,0x10
0000:c0b8  CMP AX,0x160
0000:c0bb  JA 0x0000:c0d0
0000:c0bd  MOV AX,word ptr ES:[DI + 0x8]
0000:c0c1  SUB AX,word ptr [0x81c4]
0000:c0c5  ADD AX,0x10
0000:c0c8  CMP AX,0xd0
0000:c0cb  JA 0x0000:c0d0
0000:c0cd  CLC
0000:c0ce  JMP 0x0000:c0d9
0000:c0d0  STC
0000:c0d1  MOV word ptr ES:[DI + 0x18],0x0
0000:c0d7  JMP 0x0000:c0db
0000:c0d9  JMP 0x0000:c103
0000:c0db  MOV byte ptr [0x88ae],0x5
0000:c0e0  PUSH DI
0000:c0e1  MOV AX,0x487f
0000:c0e4  XOR DX,DX
0000:c0e6  CALLF 0x0000:ffff
0000:c0eb  POP SI
0000:c0ec  MOV byte ptr ES:[DI + 0x17],0x1
0000:c0f1  MOV BX,word ptr ES:[SI + 0x4]
0000:c0f5  MOV word ptr ES:[DI + 0x4],BX
0000:c0f9  MOV BX,word ptr ES:[SI + 0x8]
0000:c0fd  MOV word ptr ES:[DI + 0x8],BX
0000:c101  MOV DI,SI
0000:c103  RET
0000:c104  MOV word ptr ES:[DI + 0x12],0xed
0000:c10a  MOV word ptr ES:[DI + 0x18],0xc1a0
0000:c110  MOV dword ptr ES:[DI + 0xe],0xfffee000
0000:c119  PUSH DI
0000:c11a  MOV AX,0xc147
0000:c11d  XOR DX,DX
0000:c11f  CALLF 0x0000:ffff
0000:c124  POP SI
0000:c125  MOV byte ptr ES:[DI + 0x17],0x2
0000:c12a  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c12f  MOV dword ptr ES:[DI + 0x2],EAX
0000:c134  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c139  ADD EAX,0xf0000
0000:c13f  MOV dword ptr ES:[DI + 0x6],EAX
0000:c144  MOV DI,SI
0000:c146  RET
0000:c147  MOV word ptr ES:[DI + 0x12],0xee
0000:c14d  MOV word ptr ES:[DI + 0x18],0xc1a0
0000:c153  MOV dword ptr ES:[DI + 0xe],0xfffed000
0000:c15c  PUSH DI
0000:c15d  MOV AX,0xc18a
0000:c160  XOR DX,DX
0000:c162  CALLF 0x0000:ffff
0000:c167  POP SI
0000:c168  MOV byte ptr ES:[DI + 0x17],0x2
0000:c16d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c172  MOV dword ptr ES:[DI + 0x2],EAX
0000:c177  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c17c  ADD EAX,0xa0000
0000:c182  MOV dword ptr ES:[DI + 0x6],EAX
0000:c187  MOV DI,SI
0000:c189  RET
0000:c18a  MOV word ptr ES:[DI + 0x12],0xef
0000:c190  MOV word ptr ES:[DI + 0x18],0xc1a0
0000:c196  MOV dword ptr ES:[DI + 0xe],0xfffec000
0000:c19f  RET
0000:c1a0  CALLF 0x0000:ffff
0000:c1a5  JNC 0x0000:c1ae
0000:c1a7  MOV word ptr ES:[DI + 0x18],0x0
0000:c1ad  RET
0000:c1ae  MOV CX,0x0
0000:c1b1  MOV DX,0x0
0000:c1b4  CALLF 0x0000:ffff
0000:c1b9  JNC 0x0000:c1bd
0000:c1bb  JMP 0x0000:c212
0000:c1bd  CMP byte ptr ES:[DI + 0x29],0x0
0000:c1c2  JG 0x0000:c1e9
0000:c1c4  MOV AX,word ptr ES:[DI + 0x8]
0000:c1c8  MOV BX,word ptr ES:[DI + 0x4]
0000:c1cc  CALLF 0x0000:ffff
0000:c1d1  TEST DL,0x70
0000:c1d4  JNZ 0x0000:c20e
0000:c1d6  MOV AX,word ptr ES:[DI + 0x8]
0000:c1da  SUB AX,0x10
0000:c1dd  CALLF 0x0000:ffff
0000:c1e2  TEST DL,0x70
0000:c1e5  JNZ 0x0000:c20e
0000:c1e7  JMP 0x0000:c210
0000:c1e9  MOV AX,word ptr ES:[DI + 0x8]
0000:c1ed  MOV BX,word ptr ES:[DI + 0x4]
0000:c1f1  CALLF 0x0000:ffff
0000:c1f6  TEST DL,0x70
0000:c1f9  JNZ 0x0000:c20e
0000:c1fb  MOV AX,word ptr ES:[DI + 0x8]
0000:c1ff  SUB AX,0x10
0000:c202  CALLF 0x0000:ffff
0000:c207  TEST DL,0x70
0000:c20a  JNZ 0x0000:c20e
0000:c20c  JMP 0x0000:c210
0000:c20e  JMP 0x0000:c212
0000:c210  JMP 0x0000:c219
0000:c212  MOV word ptr ES:[DI + 0x18],0x0
0000:c218  RET
0000:c219  ADD EAX,0x3a98
0000:c21f  CMP EAX,0xfffd0000
0000:c225  JL 0x0000:c23b
0000:c227  CMP EAX,0x30000
0000:c22d  JG 0x0000:c232
0000:c22f  CLC
0000:c230  JMP 0x0000:c242
0000:c232  MOV EAX,0x30000
0000:c238  STC
0000:c239  JMP 0x0000:c242
0000:c23b  MOV EAX,0xfffd0000
0000:c241  STC
0000:c242  MOV dword ptr ES:[DI + 0xe],EAX
0000:c247  ADD dword ptr ES:[DI + 0x6],EAX
0000:c24c  MOV DX,0x10
0000:c24f  MOV CX,0x10
0000:c252  MOV BX,0xfff8
0000:c255  MOV AX,0xfff8
0000:c258  CALLF 0x0000:ffff
0000:c25d  RET
0000:c264  MOV AX,0xc28a
0000:c267  XOR DX,DX
0000:c269  CALLF 0x0000:ffff
0000:c26e  MOV byte ptr ES:[DI + 0x17],0x1
0000:c273  MOV AX,[0x81c0]
0000:c276  MOV BX,word ptr [0x81c4]
0000:c27a  ADD AX,0x244
0000:c27d  SUB BX,0x26
0000:c280  MOV word ptr ES:[DI + 0x4],AX
0000:c284  MOV word ptr ES:[DI + 0x8],BX
0000:c288  POP DI
0000:c289  RETF
0000:c28a  MOV byte ptr ES:[DI + 0x17],0x1
0000:c28f  MOV word ptr ES:[DI + 0x18],0xc40b
0000:c295  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c29b  MOV byte ptr ES:[DI + 0x28],0xff
0000:c2a0  MOV byte ptr ES:[DI + 0x29],0xff
0000:c2a5  MOV word ptr ES:[DI + 0x38],0x0
0000:c2ab  MOV word ptr ES:[DI + 0x46],0x0
0000:c2b1  MOV word ptr ES:[DI + 0x44],0x0
0000:c2b7  MOV byte ptr ES:[DI + 0x34],0x0
0000:c2bc  MOV dword ptr ES:[DI + 0xa],0xfffed000
0000:c2c5  MOV byte ptr ES:[DI + 0x40],0xff
0000:c2ca  MOV word ptr ES:[DI + 0x42],0x14
0000:c2d0  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c2d5  PUSH DI
0000:c2d6  MOV AX,0xc30d
0000:c2d9  XOR DX,DX
0000:c2db  CALLF 0x0000:ffff
0000:c2e0  POP SI
0000:c2e1  MOV word ptr ES:[SI + 0x36],DI
0000:c2e5  MOV byte ptr ES:[DI + 0x17],0x2
0000:c2ea  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c2ef  SUB EAX,0x1f0000
0000:c2f5  MOV dword ptr ES:[DI + 0x2],EAX
0000:c2fa  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c2ff  SUB EAX,0x190000
0000:c305  MOV dword ptr ES:[DI + 0x6],EAX
0000:c30a  MOV DI,SI
0000:c30c  RET
0000:c30d  MOV SI,0x32fa
0000:c310  CALLF 0x0000:ffff
0000:c315  MOV word ptr ES:[DI + 0x18],0xc328
0000:c31b  MOV word ptr ES:[DI + 0x2a],0x0
0000:c321  MOV word ptr ES:[DI + 0x2c],0x0
0000:c327  RET
0000:c328  CMP byte ptr ES:[DI + 0x2e],0x1
0000:c32d  JGE 0x0000:c3e5
0000:c331  CMP word ptr [0x8806],0x0
0000:c336  JZ 0x0000:c405
0000:c33a  MOV BX,word ptr ES:[DI + 0x2a]
0000:c33e  CMP BX,word ptr [0x8808]
0000:c342  JLE 0x0000:c34c
0000:c344  MOV word ptr ES:[DI + 0x2a],0x0
0000:c34a  XOR BX,BX
0000:c34c  SHL BX,0x2
0000:c34f  MOV AX,word ptr ES:[DI + 0x4]
0000:c353  SUB AX,0xf
0000:c356  CMP word ptr [BX + 0x87de],AX
0000:c35a  JLE 0x0000:c383
0000:c35c  ADD AX,0x1e
0000:c35f  CMP word ptr [BX + 0x87de],AX
0000:c363  JGE 0x0000:c383
0000:c365  MOV AX,word ptr ES:[DI + 0x8]
0000:c369  ADD AX,0x5
0000:c36c  CMP word ptr [BX + 0x87e0],AX
0000:c370  JGE 0x0000:c383
0000:c372  SUB AX,0x1e
0000:c375  CMP word ptr [BX + 0x87e0],AX
0000:c379  JLE 0x0000:c383
0000:c37b  MOV word ptr [BX + 0x87de],0x0
0000:c381  JMP 0x0000:c385
0000:c383  JMP 0x0000:c3ce
0000:c385  INC word ptr ES:[DI + 0x2c]
0000:c389  PUSH DI
0000:c38a  MOV AX,0x4b70
0000:c38d  XOR DX,DX
0000:c38f  CALLF 0x0000:ffff
0000:c394  POP SI
0000:c395  MOV byte ptr ES:[DI + 0x17],0x2
0000:c39a  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c39f  MOV dword ptr ES:[DI + 0x2],EAX
0000:c3a4  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c3a9  ADD EAX,0xa0000
0000:c3af  MOV dword ptr ES:[DI + 0x6],EAX
0000:c3b4  MOV DI,SI
0000:c3b6  MOV SI,0x3308
0000:c3b9  CALLF 0x0000:ffff
0000:c3be  MOV word ptr [0x612e],0xd
0000:c3c4  CALLF 0x0000:ffff
0000:c3c9  MOV byte ptr ES:[DI + 0x2e],0x1
0000:c3ce  INC word ptr ES:[DI + 0x2a]
0000:c3d2  CMP word ptr ES:[DI + 0x2c],0x4
0000:c3d7  JLE 0x0000:c405
0000:c3d9  INC byte ptr [0x88ae]
0000:c3dd  MOV word ptr ES:[DI + 0x2c],0x0
0000:c3e3  JMP 0x0000:c405
0000:c3e5  INC word ptr ES:[DI + 0x2f]
0000:c3e9  CMP word ptr ES:[DI + 0x2f],0x64
0000:c3ee  JLE 0x0000:c405
0000:c3f0  MOV word ptr ES:[DI + 0x2f],0x0
0000:c3f6  MOV byte ptr ES:[DI + 0x2e],0x0
0000:c3fb  MOV SI,0x32fa
0000:c3fe  CALLF 0x0000:ffff
0000:c403  JMP 0x0000:c405
0000:c405  CALLF 0x0000:ffff
0000:c40a  RET
0000:c40b  CMP byte ptr [0x88ae],0x3
0000:c410  JGE 0x0000:c788
0000:c414  MOV DX,0x32
0000:c417  MOV CX,0x64
0000:c41a  MOV BX,0xffce
0000:c41d  MOV AX,0xffce
0000:c420  CALLF 0x0000:ffff
0000:c425  CMP byte ptr ES:[DI + 0x29],0x0
0000:c42a  JG 0x0000:c440
0000:c42c  MOV AX,word ptr ES:[DI + 0x8]
0000:c430  MOV BX,word ptr ES:[DI + 0x4]
0000:c434  SUB BX,0x3c
0000:c437  CALLF 0x0000:ffff
0000:c43c  JZ 0x0000:c454
0000:c43e  JMP 0x0000:c459
0000:c440  MOV AX,word ptr ES:[DI + 0x8]
0000:c444  MOV BX,word ptr ES:[DI + 0x4]
0000:c448  ADD BX,0x3c
0000:c44b  CALLF 0x0000:ffff
0000:c450  JZ 0x0000:c454
0000:c452  JMP 0x0000:c459
0000:c454  MOV byte ptr ES:[DI + 0x3e],0x1
0000:c459  CMP byte ptr ES:[DI + 0x34],0x1
0000:c45e  JGE 0x0000:c6d2
0000:c462  CMP byte ptr ES:[DI + 0x3e],0x0
0000:c467  JLE 0x0000:c6bf
0000:c46b  CMP byte ptr [0x88ae],0x2
0000:c470  JGE 0x0000:c5d1
0000:c474  CMP word ptr ES:[DI + 0x46],0x1
0000:c479  JZ 0x0000:c524
0000:c47d  CMP byte ptr ES:[DI + 0x40],0x0
0000:c482  JGE 0x0000:c577
0000:c486  CMP word ptr ES:[DI + 0x42],0x14
0000:c48b  JNZ 0x0000:c48d
0000:c48d  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c492  MOV AL,byte ptr ES:[DI + 0x29]
0000:c496  CBW
0000:c497  CWDE
0000:c499  SHL EAX,0xa
0000:c49d  SUB EBX,EAX
0000:c4a0  CMP EBX,0xfffed000
0000:c4a7  JL 0x0000:c4be
0000:c4a9  CMP EBX,0x13000
0000:c4b0  JG 0x0000:c4b5
0000:c4b2  CLC
0000:c4b3  JMP 0x0000:c4c5
0000:c4b5  MOV EBX,0x13000
0000:c4bb  STC
0000:c4bc  JMP 0x0000:c4c5
0000:c4be  MOV EBX,0xfffed000
0000:c4c4  STC
0000:c4c5  MOV dword ptr ES:[DI + 0xa],EBX
0000:c4ca  ADD dword ptr ES:[DI + 0x2],EBX
0000:c4cf  DEC word ptr ES:[DI + 0x42]
0000:c4d3  JGE 0x0000:c5ce
0000:c4d7  MOV word ptr ES:[DI + 0x46],0x1
0000:c4dd  PUSH DI
0000:c4de  MOV AX,0x6616
0000:c4e1  XOR DX,DX
0000:c4e3  CALLF 0x0000:ffff
0000:c4e8  POP SI
0000:c4e9  MOV byte ptr ES:[DI + 0x17],0x2
0000:c4ee  MOV AL,byte ptr ES:[SI + 0x29]
0000:c4f2  MOV byte ptr ES:[DI + 0x29],AL
0000:c4f6  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c4fb  CMP byte ptr ES:[SI + 0x28],0x1
0000:c500  JNZ 0x0000:c50a

; ---- C1A0 count=224 ----
0000:c1a0  CALLF 0x0000:ffff
0000:c1a5  JNC 0x0000:c1ae
0000:c1a7  MOV word ptr ES:[DI + 0x18],0x0
0000:c1ad  RET
0000:c1ae  MOV CX,0x0
0000:c1b1  MOV DX,0x0
0000:c1b4  CALLF 0x0000:ffff
0000:c1b9  JNC 0x0000:c1bd
0000:c1bb  JMP 0x0000:c212
0000:c1bd  CMP byte ptr ES:[DI + 0x29],0x0
0000:c1c2  JG 0x0000:c1e9
0000:c1c4  MOV AX,word ptr ES:[DI + 0x8]
0000:c1c8  MOV BX,word ptr ES:[DI + 0x4]
0000:c1cc  CALLF 0x0000:ffff
0000:c1d1  TEST DL,0x70
0000:c1d4  JNZ 0x0000:c20e
0000:c1d6  MOV AX,word ptr ES:[DI + 0x8]
0000:c1da  SUB AX,0x10
0000:c1dd  CALLF 0x0000:ffff
0000:c1e2  TEST DL,0x70
0000:c1e5  JNZ 0x0000:c20e
0000:c1e7  JMP 0x0000:c210
0000:c1e9  MOV AX,word ptr ES:[DI + 0x8]
0000:c1ed  MOV BX,word ptr ES:[DI + 0x4]
0000:c1f1  CALLF 0x0000:ffff
0000:c1f6  TEST DL,0x70
0000:c1f9  JNZ 0x0000:c20e
0000:c1fb  MOV AX,word ptr ES:[DI + 0x8]
0000:c1ff  SUB AX,0x10
0000:c202  CALLF 0x0000:ffff
0000:c207  TEST DL,0x70
0000:c20a  JNZ 0x0000:c20e
0000:c20c  JMP 0x0000:c210
0000:c20e  JMP 0x0000:c212
0000:c210  JMP 0x0000:c219
0000:c212  MOV word ptr ES:[DI + 0x18],0x0
0000:c218  RET
0000:c219  ADD EAX,0x3a98
0000:c21f  CMP EAX,0xfffd0000
0000:c225  JL 0x0000:c23b
0000:c227  CMP EAX,0x30000
0000:c22d  JG 0x0000:c232
0000:c22f  CLC
0000:c230  JMP 0x0000:c242
0000:c232  MOV EAX,0x30000
0000:c238  STC
0000:c239  JMP 0x0000:c242
0000:c23b  MOV EAX,0xfffd0000
0000:c241  STC
0000:c242  MOV dword ptr ES:[DI + 0xe],EAX
0000:c247  ADD dword ptr ES:[DI + 0x6],EAX
0000:c24c  MOV DX,0x10
0000:c24f  MOV CX,0x10
0000:c252  MOV BX,0xfff8
0000:c255  MOV AX,0xfff8
0000:c258  CALLF 0x0000:ffff
0000:c25d  RET
0000:c264  MOV AX,0xc28a
0000:c267  XOR DX,DX
0000:c269  CALLF 0x0000:ffff
0000:c26e  MOV byte ptr ES:[DI + 0x17],0x1
0000:c273  MOV AX,[0x81c0]
0000:c276  MOV BX,word ptr [0x81c4]
0000:c27a  ADD AX,0x244
0000:c27d  SUB BX,0x26
0000:c280  MOV word ptr ES:[DI + 0x4],AX
0000:c284  MOV word ptr ES:[DI + 0x8],BX
0000:c288  POP DI
0000:c289  RETF
0000:c28a  MOV byte ptr ES:[DI + 0x17],0x1
0000:c28f  MOV word ptr ES:[DI + 0x18],0xc40b
0000:c295  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c29b  MOV byte ptr ES:[DI + 0x28],0xff
0000:c2a0  MOV byte ptr ES:[DI + 0x29],0xff
0000:c2a5  MOV word ptr ES:[DI + 0x38],0x0
0000:c2ab  MOV word ptr ES:[DI + 0x46],0x0
0000:c2b1  MOV word ptr ES:[DI + 0x44],0x0
0000:c2b7  MOV byte ptr ES:[DI + 0x34],0x0
0000:c2bc  MOV dword ptr ES:[DI + 0xa],0xfffed000
0000:c2c5  MOV byte ptr ES:[DI + 0x40],0xff
0000:c2ca  MOV word ptr ES:[DI + 0x42],0x14
0000:c2d0  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c2d5  PUSH DI
0000:c2d6  MOV AX,0xc30d
0000:c2d9  XOR DX,DX
0000:c2db  CALLF 0x0000:ffff
0000:c2e0  POP SI
0000:c2e1  MOV word ptr ES:[SI + 0x36],DI
0000:c2e5  MOV byte ptr ES:[DI + 0x17],0x2
0000:c2ea  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c2ef  SUB EAX,0x1f0000
0000:c2f5  MOV dword ptr ES:[DI + 0x2],EAX
0000:c2fa  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c2ff  SUB EAX,0x190000
0000:c305  MOV dword ptr ES:[DI + 0x6],EAX
0000:c30a  MOV DI,SI
0000:c30c  RET
0000:c30d  MOV SI,0x32fa
0000:c310  CALLF 0x0000:ffff
0000:c315  MOV word ptr ES:[DI + 0x18],0xc328
0000:c31b  MOV word ptr ES:[DI + 0x2a],0x0
0000:c321  MOV word ptr ES:[DI + 0x2c],0x0
0000:c327  RET
0000:c328  CMP byte ptr ES:[DI + 0x2e],0x1
0000:c32d  JGE 0x0000:c3e5
0000:c331  CMP word ptr [0x8806],0x0
0000:c336  JZ 0x0000:c405
0000:c33a  MOV BX,word ptr ES:[DI + 0x2a]
0000:c33e  CMP BX,word ptr [0x8808]
0000:c342  JLE 0x0000:c34c
0000:c344  MOV word ptr ES:[DI + 0x2a],0x0
0000:c34a  XOR BX,BX
0000:c34c  SHL BX,0x2
0000:c34f  MOV AX,word ptr ES:[DI + 0x4]
0000:c353  SUB AX,0xf
0000:c356  CMP word ptr [BX + 0x87de],AX
0000:c35a  JLE 0x0000:c383
0000:c35c  ADD AX,0x1e
0000:c35f  CMP word ptr [BX + 0x87de],AX
0000:c363  JGE 0x0000:c383
0000:c365  MOV AX,word ptr ES:[DI + 0x8]
0000:c369  ADD AX,0x5
0000:c36c  CMP word ptr [BX + 0x87e0],AX
0000:c370  JGE 0x0000:c383
0000:c372  SUB AX,0x1e
0000:c375  CMP word ptr [BX + 0x87e0],AX
0000:c379  JLE 0x0000:c383
0000:c37b  MOV word ptr [BX + 0x87de],0x0
0000:c381  JMP 0x0000:c385
0000:c383  JMP 0x0000:c3ce
0000:c385  INC word ptr ES:[DI + 0x2c]
0000:c389  PUSH DI
0000:c38a  MOV AX,0x4b70
0000:c38d  XOR DX,DX
0000:c38f  CALLF 0x0000:ffff
0000:c394  POP SI
0000:c395  MOV byte ptr ES:[DI + 0x17],0x2
0000:c39a  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c39f  MOV dword ptr ES:[DI + 0x2],EAX
0000:c3a4  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c3a9  ADD EAX,0xa0000
0000:c3af  MOV dword ptr ES:[DI + 0x6],EAX
0000:c3b4  MOV DI,SI
0000:c3b6  MOV SI,0x3308
0000:c3b9  CALLF 0x0000:ffff
0000:c3be  MOV word ptr [0x612e],0xd
0000:c3c4  CALLF 0x0000:ffff
0000:c3c9  MOV byte ptr ES:[DI + 0x2e],0x1
0000:c3ce  INC word ptr ES:[DI + 0x2a]
0000:c3d2  CMP word ptr ES:[DI + 0x2c],0x4
0000:c3d7  JLE 0x0000:c405
0000:c3d9  INC byte ptr [0x88ae]
0000:c3dd  MOV word ptr ES:[DI + 0x2c],0x0
0000:c3e3  JMP 0x0000:c405
0000:c3e5  INC word ptr ES:[DI + 0x2f]
0000:c3e9  CMP word ptr ES:[DI + 0x2f],0x64
0000:c3ee  JLE 0x0000:c405
0000:c3f0  MOV word ptr ES:[DI + 0x2f],0x0
0000:c3f6  MOV byte ptr ES:[DI + 0x2e],0x0
0000:c3fb  MOV SI,0x32fa
0000:c3fe  CALLF 0x0000:ffff
0000:c403  JMP 0x0000:c405
0000:c405  CALLF 0x0000:ffff
0000:c40a  RET
0000:c40b  CMP byte ptr [0x88ae],0x3
0000:c410  JGE 0x0000:c788
0000:c414  MOV DX,0x32
0000:c417  MOV CX,0x64
0000:c41a  MOV BX,0xffce
0000:c41d  MOV AX,0xffce
0000:c420  CALLF 0x0000:ffff
0000:c425  CMP byte ptr ES:[DI + 0x29],0x0
0000:c42a  JG 0x0000:c440
0000:c42c  MOV AX,word ptr ES:[DI + 0x8]
0000:c430  MOV BX,word ptr ES:[DI + 0x4]
0000:c434  SUB BX,0x3c
0000:c437  CALLF 0x0000:ffff
0000:c43c  JZ 0x0000:c454
0000:c43e  JMP 0x0000:c459
0000:c440  MOV AX,word ptr ES:[DI + 0x8]
0000:c444  MOV BX,word ptr ES:[DI + 0x4]
0000:c448  ADD BX,0x3c
0000:c44b  CALLF 0x0000:ffff
0000:c450  JZ 0x0000:c454
0000:c452  JMP 0x0000:c459
0000:c454  MOV byte ptr ES:[DI + 0x3e],0x1
0000:c459  CMP byte ptr ES:[DI + 0x34],0x1
0000:c45e  JGE 0x0000:c6d2
0000:c462  CMP byte ptr ES:[DI + 0x3e],0x0
0000:c467  JLE 0x0000:c6bf
0000:c46b  CMP byte ptr [0x88ae],0x2
0000:c470  JGE 0x0000:c5d1
0000:c474  CMP word ptr ES:[DI + 0x46],0x1
0000:c479  JZ 0x0000:c524
0000:c47d  CMP byte ptr ES:[DI + 0x40],0x0
0000:c482  JGE 0x0000:c577
0000:c486  CMP word ptr ES:[DI + 0x42],0x14
0000:c48b  JNZ 0x0000:c48d
0000:c48d  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c492  MOV AL,byte ptr ES:[DI + 0x29]
0000:c496  CBW
0000:c497  CWDE
0000:c499  SHL EAX,0xa
0000:c49d  SUB EBX,EAX
0000:c4a0  CMP EBX,0xfffed000
0000:c4a7  JL 0x0000:c4be
0000:c4a9  CMP EBX,0x13000
0000:c4b0  JG 0x0000:c4b5
0000:c4b2  CLC
0000:c4b3  JMP 0x0000:c4c5
0000:c4b5  MOV EBX,0x13000
0000:c4bb  STC
0000:c4bc  JMP 0x0000:c4c5
0000:c4be  MOV EBX,0xfffed000
0000:c4c4  STC
0000:c4c5  MOV dword ptr ES:[DI + 0xa],EBX
0000:c4ca  ADD dword ptr ES:[DI + 0x2],EBX
0000:c4cf  DEC word ptr ES:[DI + 0x42]
0000:c4d3  JGE 0x0000:c5ce
0000:c4d7  MOV word ptr ES:[DI + 0x46],0x1
0000:c4dd  PUSH DI
0000:c4de  MOV AX,0x6616
0000:c4e1  XOR DX,DX
0000:c4e3  CALLF 0x0000:ffff

; ---- BAD7 count=224 ----
0000:bad7  CMP byte ptr [0x88ae],0x5
0000:badc  JNZ 0x0000:bb08
0000:bade  MOV AX,word ptr ES:[DI + 0x4]
0000:bae2  SUB AX,word ptr [0x81c0]
0000:bae6  ADD AX,0x10
0000:bae9  CMP AX,0x160
0000:baec  JA 0x0000:bb01
0000:baee  MOV AX,word ptr ES:[DI + 0x8]
0000:baf2  SUB AX,word ptr [0x81c4]
0000:baf6  ADD AX,0x10
0000:baf9  CMP AX,0xd0
0000:bafc  JA 0x0000:bb01
0000:bafe  CLC
0000:baff  JMP 0x0000:bb08
0000:bb01  STC
0000:bb02  MOV word ptr ES:[DI + 0x18],0x0
0000:bb08  CALLF 0x0000:ffff
0000:bb0d  RET
0000:bb0e  CMP byte ptr ES:[DI + 0x2e],0x1
0000:bb13  JGE 0x0000:bbc6
0000:bb17  CMP word ptr [0x8806],0x0
0000:bb1c  JZ 0x0000:bbe6
0000:bb20  MOV BX,word ptr ES:[DI + 0x2a]
0000:bb24  CMP BX,word ptr [0x8808]
0000:bb28  JLE 0x0000:bb32
0000:bb2a  MOV word ptr ES:[DI + 0x2a],0x0
0000:bb30  XOR BX,BX
0000:bb32  SHL BX,0x2
0000:bb35  MOV AX,word ptr ES:[DI + 0x4]
0000:bb39  SUB AX,0xf
0000:bb3c  CMP word ptr [BX + 0x87de],AX
0000:bb40  JLE 0x0000:bb69
0000:bb42  ADD AX,0x1e
0000:bb45  CMP word ptr [BX + 0x87de],AX
0000:bb49  JGE 0x0000:bb69
0000:bb4b  MOV AX,word ptr ES:[DI + 0x8]
0000:bb4f  ADD AX,0x5
0000:bb52  CMP word ptr [BX + 0x87e0],AX
0000:bb56  JGE 0x0000:bb69
0000:bb58  SUB AX,0x1e
0000:bb5b  CMP word ptr [BX + 0x87e0],AX
0000:bb5f  JLE 0x0000:bb69
0000:bb61  MOV word ptr [BX + 0x87de],0x0
0000:bb67  JMP 0x0000:bb6b
0000:bb69  JMP 0x0000:bbb4
0000:bb6b  INC word ptr ES:[DI + 0x2c]
0000:bb6f  PUSH DI
0000:bb70  MOV AX,0x4b70
0000:bb73  XOR DX,DX
0000:bb75  CALLF 0x0000:ffff
0000:bb7a  POP SI
0000:bb7b  MOV byte ptr ES:[DI + 0x17],0x2
0000:bb80  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bb85  MOV dword ptr ES:[DI + 0x2],EAX
0000:bb8a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:bb8f  ADD EAX,0xa0000
0000:bb95  MOV dword ptr ES:[DI + 0x6],EAX
0000:bb9a  MOV DI,SI
0000:bb9c  MOV SI,0x3308
0000:bb9f  CALLF 0x0000:ffff
0000:bba4  MOV word ptr [0x612e],0xd
0000:bbaa  CALLF 0x0000:ffff
0000:bbaf  MOV byte ptr ES:[DI + 0x2e],0x1
0000:bbb4  INC word ptr ES:[DI + 0x2a]
0000:bbb8  CMP word ptr ES:[DI + 0x2c],0x5
0000:bbbd  JLE 0x0000:bbe6
0000:bbbf  MOV byte ptr [0x88ae],0x2
0000:bbc4  JMP 0x0000:bbe6
0000:bbc6  INC word ptr ES:[DI + 0x2f]
0000:bbca  CMP word ptr ES:[DI + 0x2f],0x64
0000:bbcf  JLE 0x0000:bbe6
0000:bbd1  MOV word ptr ES:[DI + 0x2f],0x0
0000:bbd7  MOV byte ptr ES:[DI + 0x2e],0x0
0000:bbdc  MOV SI,0x32fa
0000:bbdf  CALLF 0x0000:ffff
0000:bbe4  JMP 0x0000:bbe6
0000:bbe6  CALLF 0x0000:ffff
0000:bbeb  RET
0000:bbec  CMP byte ptr [0x88ae],0x2
0000:bbf1  JGE 0x0000:bea6
0000:bbf5  MOV DX,0x32
0000:bbf8  MOV CX,0x64
0000:bbfb  MOV BX,0xffce
0000:bbfe  MOV AX,0xffce
0000:bc01  CALLF 0x0000:ffff
0000:bc06  MOV DX,0x32
0000:bc09  NEG DX
0000:bc0b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc10  JS 0x0000:bc14
0000:bc12  NEG DX
0000:bc14  MOV AX,word ptr ES:[DI + 0x8]
0000:bc18  DEC AX
0000:bc19  MOV BX,word ptr ES:[DI + 0x4]
0000:bc1d  ADD BX,DX
0000:bc1f  CALLF 0x0000:ffff
0000:bc24  JNZ 0x0000:bc69
0000:bc26  MOV DX,0x32
0000:bc29  NEG DX
0000:bc2b  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc30  JS 0x0000:bc34
0000:bc32  NEG DX
0000:bc34  MOV AX,word ptr ES:[DI + 0x8]
0000:bc38  SUB AX,0x11
0000:bc3b  MOV BX,word ptr ES:[DI + 0x4]
0000:bc3f  ADD BX,DX
0000:bc41  CALLF 0x0000:ffff
0000:bc46  JNZ 0x0000:bc69
0000:bc48  MOV DX,0x32
0000:bc4b  NEG DX
0000:bc4d  TEST byte ptr ES:[DI + 0x28],0xff
0000:bc52  JS 0x0000:bc57
0000:bc54  MOV DX,0x32
0000:bc57  MOV AX,word ptr ES:[DI + 0x8]
0000:bc5b  SUB AX,0xc
0000:bc5e  MOV BX,word ptr ES:[DI + 0x4]
0000:bc62  ADD BX,DX
0000:bc64  CALLF 0x0000:ffff
0000:bc69  JMP 0x0000:bc6d
0000:bc6d  JZ 0x0000:bc74
0000:bc6f  MOV byte ptr ES:[DI + 0x3e],0x1
0000:bc74  CMP byte ptr ES:[DI + 0x34],0x1
0000:bc79  JGE 0x0000:bdc0
0000:bc7d  CMP byte ptr ES:[DI + 0x3e],0x0
0000:bc82  JLE 0x0000:bd74
0000:bc86  CMP byte ptr ES:[DI + 0x40],0x0
0000:bc8b  JGE 0x0000:bd1a
0000:bc8f  CMP word ptr ES:[DI + 0x42],0x14
0000:bc94  JNZ 0x0000:bc96
0000:bc96  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bc9b  MOV AL,byte ptr ES:[DI + 0x29]
0000:bc9f  CBW
0000:bca0  CWDE
0000:bca2  SHL EAX,0xc
0000:bca6  SUB EBX,EAX
0000:bca9  CMP EBX,0xffff7000
0000:bcb0  JL 0x0000:bcc7
0000:bcb2  CMP EBX,0x9000
0000:bcb9  JG 0x0000:bcbe
0000:bcbb  CLC
0000:bcbc  JMP 0x0000:bcce
0000:bcbe  MOV EBX,0x9000
0000:bcc4  STC
0000:bcc5  JMP 0x0000:bcce
0000:bcc7  MOV EBX,0xffff7000
0000:bccd  STC
0000:bcce  MOV dword ptr ES:[DI + 0xa],EBX
0000:bcd3  ADD dword ptr ES:[DI + 0x2],EBX
0000:bcd8  DEC word ptr ES:[DI + 0x42]
0000:bcdc  JGE 0x0000:bd71
0000:bce0  NEG byte ptr ES:[DI + 0x29]
0000:bce4  NEG byte ptr ES:[DI + 0x28]
0000:bce8  NEG byte ptr ES:[DI + 0x40]
0000:bcec  CMP word ptr ES:[DI + 0x12],0xdc
0000:bcf2  JZ 0x0000:bcfc
0000:bcf4  MOV word ptr ES:[DI + 0x12],0xdc
0000:bcfa  JMP 0x0000:bd02
0000:bcfc  MOV word ptr ES:[DI + 0x12],0x10e
0000:bd02  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd06  CBW
0000:bd07  CWDE
0000:bd09  SHL EAX,0x9
0000:bd0d  MOV dword ptr ES:[DI + 0xa],EAX
0000:bd12  MOV word ptr ES:[DI + 0x42],0x14
0000:bd18  JMP 0x0000:bd71
0000:bd1a  MOV EBX,dword ptr ES:[DI + 0xa]
0000:bd1f  MOV AL,byte ptr ES:[DI + 0x29]
0000:bd23  CBW
0000:bd24  CWDE
0000:bd26  SHL EAX,0xa
0000:bd2a  ADD EBX,EAX
0000:bd2d  CMP EBX,0xffff7000
0000:bd34  JL 0x0000:bd4b
0000:bd36  CMP EBX,0x9000
0000:bd3d  JG 0x0000:bd42
0000:bd3f  CLC
0000:bd40  JMP 0x0000:bd52
0000:bd42  MOV EBX,0x9000
0000:bd48  STC
0000:bd49  JMP 0x0000:bd52
0000:bd4b  MOV EBX,0xffff7000
0000:bd51  STC
0000:bd52  MOV dword ptr ES:[DI + 0xa],EBX
0000:bd57  ADD dword ptr ES:[DI + 0x2],EBX
0000:bd5c  DEC word ptr ES:[DI + 0x42]
0000:bd60  JGE 0x0000:bd71
0000:bd62  NEG byte ptr ES:[DI + 0x40]
0000:bd66  MOV byte ptr ES:[DI + 0x3e],0xff
0000:bd6b  MOV word ptr ES:[DI + 0x42],0x14
0000:bd71  JMP 0x0000:be0f
0000:bd74  MOV EAX,dword ptr ES:[DI + 0xa]
0000:bd79  ADD dword ptr ES:[DI + 0x2],EAX
0000:bd7e  MOV AX,word ptr ES:[DI + 0x2c]
0000:bd82  SUB word ptr ES:[DI + 0x8],AX
0000:bd86  MOV SI,0x7974
0000:bd89  MOV AX,word ptr ES:[DI + 0x2e]
0000:bd8d  ADD AX,0x20
0000:bd90  AND AX,0x7ff
0000:bd93  MOV word ptr ES:[DI + 0x2e],AX
0000:bd97  ADD SI,AX
0000:bd99  MOV AL,byte ptr [SI]
0000:bd9b  SAR AL,0x4
0000:bd9e  CBW
0000:bd9f  MOV word ptr ES:[DI + 0x2c],AX
0000:bda3  ADD word ptr ES:[DI + 0x8],AX
0000:bda7  INC word ptr ES:[DI + 0x38]
0000:bdab  CMP word ptr ES:[DI + 0x38],0xaa
0000:bdb1  JLE 0x0000:be0f
0000:bdb3  MOV word ptr ES:[DI + 0x38],0x0
0000:bdb9  MOV byte ptr ES:[DI + 0x34],0x1
0000:bdbe  JMP 0x0000:be0f
0000:bdc0  PUSH DI
0000:bdc1  MOV AX,0xc104
0000:bdc4  XOR DX,DX
0000:bdc6  CALLF 0x0000:ffff
0000:bdcb  POP SI
0000:bdcc  MOV byte ptr ES:[DI + 0x17],0x2
0000:bdd1  MOV AL,byte ptr ES:[SI + 0x29]
0000:bdd5  MOV byte ptr ES:[DI + 0x29],AL
0000:bdd9  MOV EAX,dword ptr ES:[SI + 0x2]
0000:bdde  CMP byte ptr ES:[SI + 0x28],0x1
0000:bde3  JNZ 0x0000:bded
0000:bde5  ADD EAX,0x70000
0000:bdeb  JMP 0x0000:bdf3
0000:bded  SUB EAX,0x70000

; ---- C28A count=192 ----
0000:c28a  MOV byte ptr ES:[DI + 0x17],0x1
0000:c28f  MOV word ptr ES:[DI + 0x18],0xc40b
0000:c295  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c29b  MOV byte ptr ES:[DI + 0x28],0xff
0000:c2a0  MOV byte ptr ES:[DI + 0x29],0xff
0000:c2a5  MOV word ptr ES:[DI + 0x38],0x0
0000:c2ab  MOV word ptr ES:[DI + 0x46],0x0
0000:c2b1  MOV word ptr ES:[DI + 0x44],0x0
0000:c2b7  MOV byte ptr ES:[DI + 0x34],0x0
0000:c2bc  MOV dword ptr ES:[DI + 0xa],0xfffed000
0000:c2c5  MOV byte ptr ES:[DI + 0x40],0xff
0000:c2ca  MOV word ptr ES:[DI + 0x42],0x14
0000:c2d0  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c2d5  PUSH DI
0000:c2d6  MOV AX,0xc30d
0000:c2d9  XOR DX,DX
0000:c2db  CALLF 0x0000:ffff
0000:c2e0  POP SI
0000:c2e1  MOV word ptr ES:[SI + 0x36],DI
0000:c2e5  MOV byte ptr ES:[DI + 0x17],0x2
0000:c2ea  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c2ef  SUB EAX,0x1f0000
0000:c2f5  MOV dword ptr ES:[DI + 0x2],EAX
0000:c2fa  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c2ff  SUB EAX,0x190000
0000:c305  MOV dword ptr ES:[DI + 0x6],EAX
0000:c30a  MOV DI,SI
0000:c30c  RET
0000:c30d  MOV SI,0x32fa
0000:c310  CALLF 0x0000:ffff
0000:c315  MOV word ptr ES:[DI + 0x18],0xc328
0000:c31b  MOV word ptr ES:[DI + 0x2a],0x0
0000:c321  MOV word ptr ES:[DI + 0x2c],0x0
0000:c327  RET
0000:c328  CMP byte ptr ES:[DI + 0x2e],0x1
0000:c32d  JGE 0x0000:c3e5
0000:c331  CMP word ptr [0x8806],0x0
0000:c336  JZ 0x0000:c405
0000:c33a  MOV BX,word ptr ES:[DI + 0x2a]
0000:c33e  CMP BX,word ptr [0x8808]
0000:c342  JLE 0x0000:c34c
0000:c344  MOV word ptr ES:[DI + 0x2a],0x0
0000:c34a  XOR BX,BX
0000:c34c  SHL BX,0x2
0000:c34f  MOV AX,word ptr ES:[DI + 0x4]
0000:c353  SUB AX,0xf
0000:c356  CMP word ptr [BX + 0x87de],AX
0000:c35a  JLE 0x0000:c383
0000:c35c  ADD AX,0x1e
0000:c35f  CMP word ptr [BX + 0x87de],AX
0000:c363  JGE 0x0000:c383
0000:c365  MOV AX,word ptr ES:[DI + 0x8]
0000:c369  ADD AX,0x5
0000:c36c  CMP word ptr [BX + 0x87e0],AX
0000:c370  JGE 0x0000:c383
0000:c372  SUB AX,0x1e
0000:c375  CMP word ptr [BX + 0x87e0],AX
0000:c379  JLE 0x0000:c383
0000:c37b  MOV word ptr [BX + 0x87de],0x0
0000:c381  JMP 0x0000:c385
0000:c383  JMP 0x0000:c3ce
0000:c385  INC word ptr ES:[DI + 0x2c]
0000:c389  PUSH DI
0000:c38a  MOV AX,0x4b70
0000:c38d  XOR DX,DX
0000:c38f  CALLF 0x0000:ffff
0000:c394  POP SI
0000:c395  MOV byte ptr ES:[DI + 0x17],0x2
0000:c39a  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c39f  MOV dword ptr ES:[DI + 0x2],EAX
0000:c3a4  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c3a9  ADD EAX,0xa0000
0000:c3af  MOV dword ptr ES:[DI + 0x6],EAX
0000:c3b4  MOV DI,SI
0000:c3b6  MOV SI,0x3308
0000:c3b9  CALLF 0x0000:ffff
0000:c3be  MOV word ptr [0x612e],0xd
0000:c3c4  CALLF 0x0000:ffff
0000:c3c9  MOV byte ptr ES:[DI + 0x2e],0x1
0000:c3ce  INC word ptr ES:[DI + 0x2a]
0000:c3d2  CMP word ptr ES:[DI + 0x2c],0x4
0000:c3d7  JLE 0x0000:c405
0000:c3d9  INC byte ptr [0x88ae]
0000:c3dd  MOV word ptr ES:[DI + 0x2c],0x0
0000:c3e3  JMP 0x0000:c405
0000:c3e5  INC word ptr ES:[DI + 0x2f]
0000:c3e9  CMP word ptr ES:[DI + 0x2f],0x64
0000:c3ee  JLE 0x0000:c405
0000:c3f0  MOV word ptr ES:[DI + 0x2f],0x0
0000:c3f6  MOV byte ptr ES:[DI + 0x2e],0x0
0000:c3fb  MOV SI,0x32fa
0000:c3fe  CALLF 0x0000:ffff
0000:c403  JMP 0x0000:c405
0000:c405  CALLF 0x0000:ffff
0000:c40a  RET
0000:c40b  CMP byte ptr [0x88ae],0x3
0000:c410  JGE 0x0000:c788
0000:c414  MOV DX,0x32
0000:c417  MOV CX,0x64
0000:c41a  MOV BX,0xffce
0000:c41d  MOV AX,0xffce
0000:c420  CALLF 0x0000:ffff
0000:c425  CMP byte ptr ES:[DI + 0x29],0x0
0000:c42a  JG 0x0000:c440
0000:c42c  MOV AX,word ptr ES:[DI + 0x8]
0000:c430  MOV BX,word ptr ES:[DI + 0x4]
0000:c434  SUB BX,0x3c
0000:c437  CALLF 0x0000:ffff
0000:c43c  JZ 0x0000:c454
0000:c43e  JMP 0x0000:c459
0000:c440  MOV AX,word ptr ES:[DI + 0x8]
0000:c444  MOV BX,word ptr ES:[DI + 0x4]
0000:c448  ADD BX,0x3c
0000:c44b  CALLF 0x0000:ffff
0000:c450  JZ 0x0000:c454
0000:c452  JMP 0x0000:c459
0000:c454  MOV byte ptr ES:[DI + 0x3e],0x1
0000:c459  CMP byte ptr ES:[DI + 0x34],0x1
0000:c45e  JGE 0x0000:c6d2
0000:c462  CMP byte ptr ES:[DI + 0x3e],0x0
0000:c467  JLE 0x0000:c6bf
0000:c46b  CMP byte ptr [0x88ae],0x2
0000:c470  JGE 0x0000:c5d1
0000:c474  CMP word ptr ES:[DI + 0x46],0x1
0000:c479  JZ 0x0000:c524
0000:c47d  CMP byte ptr ES:[DI + 0x40],0x0
0000:c482  JGE 0x0000:c577
0000:c486  CMP word ptr ES:[DI + 0x42],0x14
0000:c48b  JNZ 0x0000:c48d
0000:c48d  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c492  MOV AL,byte ptr ES:[DI + 0x29]
0000:c496  CBW
0000:c497  CWDE
0000:c499  SHL EAX,0xa
0000:c49d  SUB EBX,EAX
0000:c4a0  CMP EBX,0xfffed000
0000:c4a7  JL 0x0000:c4be
0000:c4a9  CMP EBX,0x13000
0000:c4b0  JG 0x0000:c4b5
0000:c4b2  CLC
0000:c4b3  JMP 0x0000:c4c5
0000:c4b5  MOV EBX,0x13000
0000:c4bb  STC
0000:c4bc  JMP 0x0000:c4c5
0000:c4be  MOV EBX,0xfffed000
0000:c4c4  STC
0000:c4c5  MOV dword ptr ES:[DI + 0xa],EBX
0000:c4ca  ADD dword ptr ES:[DI + 0x2],EBX
0000:c4cf  DEC word ptr ES:[DI + 0x42]
0000:c4d3  JGE 0x0000:c5ce
0000:c4d7  MOV word ptr ES:[DI + 0x46],0x1
0000:c4dd  PUSH DI
0000:c4de  MOV AX,0x6616
0000:c4e1  XOR DX,DX
0000:c4e3  CALLF 0x0000:ffff
0000:c4e8  POP SI
0000:c4e9  MOV byte ptr ES:[DI + 0x17],0x2
0000:c4ee  MOV AL,byte ptr ES:[SI + 0x29]
0000:c4f2  MOV byte ptr ES:[DI + 0x29],AL
0000:c4f6  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c4fb  CMP byte ptr ES:[SI + 0x28],0x1
0000:c500  JNZ 0x0000:c50a
0000:c502  ADD EAX,0x70000
0000:c508  JMP 0x0000:c510
0000:c50a  SUB EAX,0x70000
0000:c510  MOV dword ptr ES:[DI + 0x2],EAX
0000:c515  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c51a  MOV dword ptr ES:[DI + 0x6],EAX
0000:c51f  MOV DI,SI
0000:c521  JMP 0x0000:c5ce
0000:c524  INC word ptr ES:[DI + 0x38]
0000:c528  CMP word ptr ES:[DI + 0x38],0x64
0000:c52d  JLE 0x0000:c5ce
0000:c531  MOV word ptr ES:[DI + 0x38],0x0
0000:c537  MOV word ptr ES:[DI + 0x46],0x0
0000:c53d  NEG byte ptr ES:[DI + 0x29]
0000:c541  NEG byte ptr ES:[DI + 0x28]
0000:c545  NEG byte ptr ES:[DI + 0x40]
0000:c549  CMP word ptr ES:[DI + 0x12],0x385
0000:c54f  JZ 0x0000:c559
0000:c551  MOV word ptr ES:[DI + 0x12],0x385
0000:c557  JMP 0x0000:c55f
0000:c559  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c55f  MOV AL,byte ptr ES:[DI + 0x29]
0000:c563  CBW
0000:c564  CWDE
0000:c566  SHL EAX,0x9
0000:c56a  MOV dword ptr ES:[DI + 0xa],EAX
0000:c56f  MOV word ptr ES:[DI + 0x42],0x28
0000:c575  JMP 0x0000:c5ce
0000:c577  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c57c  MOV AL,byte ptr ES:[DI + 0x29]

; ---- C328 count=224 ----
0000:c328  CMP byte ptr ES:[DI + 0x2e],0x1
0000:c32d  JGE 0x0000:c3e5
0000:c331  CMP word ptr [0x8806],0x0
0000:c336  JZ 0x0000:c405
0000:c33a  MOV BX,word ptr ES:[DI + 0x2a]
0000:c33e  CMP BX,word ptr [0x8808]
0000:c342  JLE 0x0000:c34c
0000:c344  MOV word ptr ES:[DI + 0x2a],0x0
0000:c34a  XOR BX,BX
0000:c34c  SHL BX,0x2
0000:c34f  MOV AX,word ptr ES:[DI + 0x4]
0000:c353  SUB AX,0xf
0000:c356  CMP word ptr [BX + 0x87de],AX
0000:c35a  JLE 0x0000:c383
0000:c35c  ADD AX,0x1e
0000:c35f  CMP word ptr [BX + 0x87de],AX
0000:c363  JGE 0x0000:c383
0000:c365  MOV AX,word ptr ES:[DI + 0x8]
0000:c369  ADD AX,0x5
0000:c36c  CMP word ptr [BX + 0x87e0],AX
0000:c370  JGE 0x0000:c383
0000:c372  SUB AX,0x1e
0000:c375  CMP word ptr [BX + 0x87e0],AX
0000:c379  JLE 0x0000:c383
0000:c37b  MOV word ptr [BX + 0x87de],0x0
0000:c381  JMP 0x0000:c385
0000:c383  JMP 0x0000:c3ce
0000:c385  INC word ptr ES:[DI + 0x2c]
0000:c389  PUSH DI
0000:c38a  MOV AX,0x4b70
0000:c38d  XOR DX,DX
0000:c38f  CALLF 0x0000:ffff
0000:c394  POP SI
0000:c395  MOV byte ptr ES:[DI + 0x17],0x2
0000:c39a  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c39f  MOV dword ptr ES:[DI + 0x2],EAX
0000:c3a4  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c3a9  ADD EAX,0xa0000
0000:c3af  MOV dword ptr ES:[DI + 0x6],EAX
0000:c3b4  MOV DI,SI
0000:c3b6  MOV SI,0x3308
0000:c3b9  CALLF 0x0000:ffff
0000:c3be  MOV word ptr [0x612e],0xd
0000:c3c4  CALLF 0x0000:ffff
0000:c3c9  MOV byte ptr ES:[DI + 0x2e],0x1
0000:c3ce  INC word ptr ES:[DI + 0x2a]
0000:c3d2  CMP word ptr ES:[DI + 0x2c],0x4
0000:c3d7  JLE 0x0000:c405
0000:c3d9  INC byte ptr [0x88ae]
0000:c3dd  MOV word ptr ES:[DI + 0x2c],0x0
0000:c3e3  JMP 0x0000:c405
0000:c3e5  INC word ptr ES:[DI + 0x2f]
0000:c3e9  CMP word ptr ES:[DI + 0x2f],0x64
0000:c3ee  JLE 0x0000:c405
0000:c3f0  MOV word ptr ES:[DI + 0x2f],0x0
0000:c3f6  MOV byte ptr ES:[DI + 0x2e],0x0
0000:c3fb  MOV SI,0x32fa
0000:c3fe  CALLF 0x0000:ffff
0000:c403  JMP 0x0000:c405
0000:c405  CALLF 0x0000:ffff
0000:c40a  RET
0000:c40b  CMP byte ptr [0x88ae],0x3
0000:c410  JGE 0x0000:c788
0000:c414  MOV DX,0x32
0000:c417  MOV CX,0x64
0000:c41a  MOV BX,0xffce
0000:c41d  MOV AX,0xffce
0000:c420  CALLF 0x0000:ffff
0000:c425  CMP byte ptr ES:[DI + 0x29],0x0
0000:c42a  JG 0x0000:c440
0000:c42c  MOV AX,word ptr ES:[DI + 0x8]
0000:c430  MOV BX,word ptr ES:[DI + 0x4]
0000:c434  SUB BX,0x3c
0000:c437  CALLF 0x0000:ffff
0000:c43c  JZ 0x0000:c454
0000:c43e  JMP 0x0000:c459
0000:c440  MOV AX,word ptr ES:[DI + 0x8]
0000:c444  MOV BX,word ptr ES:[DI + 0x4]
0000:c448  ADD BX,0x3c
0000:c44b  CALLF 0x0000:ffff
0000:c450  JZ 0x0000:c454
0000:c452  JMP 0x0000:c459
0000:c454  MOV byte ptr ES:[DI + 0x3e],0x1
0000:c459  CMP byte ptr ES:[DI + 0x34],0x1
0000:c45e  JGE 0x0000:c6d2
0000:c462  CMP byte ptr ES:[DI + 0x3e],0x0
0000:c467  JLE 0x0000:c6bf
0000:c46b  CMP byte ptr [0x88ae],0x2
0000:c470  JGE 0x0000:c5d1
0000:c474  CMP word ptr ES:[DI + 0x46],0x1
0000:c479  JZ 0x0000:c524
0000:c47d  CMP byte ptr ES:[DI + 0x40],0x0
0000:c482  JGE 0x0000:c577
0000:c486  CMP word ptr ES:[DI + 0x42],0x14
0000:c48b  JNZ 0x0000:c48d
0000:c48d  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c492  MOV AL,byte ptr ES:[DI + 0x29]
0000:c496  CBW
0000:c497  CWDE
0000:c499  SHL EAX,0xa
0000:c49d  SUB EBX,EAX
0000:c4a0  CMP EBX,0xfffed000
0000:c4a7  JL 0x0000:c4be
0000:c4a9  CMP EBX,0x13000
0000:c4b0  JG 0x0000:c4b5
0000:c4b2  CLC
0000:c4b3  JMP 0x0000:c4c5
0000:c4b5  MOV EBX,0x13000
0000:c4bb  STC
0000:c4bc  JMP 0x0000:c4c5
0000:c4be  MOV EBX,0xfffed000
0000:c4c4  STC
0000:c4c5  MOV dword ptr ES:[DI + 0xa],EBX
0000:c4ca  ADD dword ptr ES:[DI + 0x2],EBX
0000:c4cf  DEC word ptr ES:[DI + 0x42]
0000:c4d3  JGE 0x0000:c5ce
0000:c4d7  MOV word ptr ES:[DI + 0x46],0x1
0000:c4dd  PUSH DI
0000:c4de  MOV AX,0x6616
0000:c4e1  XOR DX,DX
0000:c4e3  CALLF 0x0000:ffff
0000:c4e8  POP SI
0000:c4e9  MOV byte ptr ES:[DI + 0x17],0x2
0000:c4ee  MOV AL,byte ptr ES:[SI + 0x29]
0000:c4f2  MOV byte ptr ES:[DI + 0x29],AL
0000:c4f6  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c4fb  CMP byte ptr ES:[SI + 0x28],0x1
0000:c500  JNZ 0x0000:c50a
0000:c502  ADD EAX,0x70000
0000:c508  JMP 0x0000:c510
0000:c50a  SUB EAX,0x70000
0000:c510  MOV dword ptr ES:[DI + 0x2],EAX
0000:c515  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c51a  MOV dword ptr ES:[DI + 0x6],EAX
0000:c51f  MOV DI,SI
0000:c521  JMP 0x0000:c5ce
0000:c524  INC word ptr ES:[DI + 0x38]
0000:c528  CMP word ptr ES:[DI + 0x38],0x64
0000:c52d  JLE 0x0000:c5ce
0000:c531  MOV word ptr ES:[DI + 0x38],0x0
0000:c537  MOV word ptr ES:[DI + 0x46],0x0
0000:c53d  NEG byte ptr ES:[DI + 0x29]
0000:c541  NEG byte ptr ES:[DI + 0x28]
0000:c545  NEG byte ptr ES:[DI + 0x40]
0000:c549  CMP word ptr ES:[DI + 0x12],0x385
0000:c54f  JZ 0x0000:c559
0000:c551  MOV word ptr ES:[DI + 0x12],0x385
0000:c557  JMP 0x0000:c55f
0000:c559  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c55f  MOV AL,byte ptr ES:[DI + 0x29]
0000:c563  CBW
0000:c564  CWDE
0000:c566  SHL EAX,0x9
0000:c56a  MOV dword ptr ES:[DI + 0xa],EAX
0000:c56f  MOV word ptr ES:[DI + 0x42],0x28
0000:c575  JMP 0x0000:c5ce
0000:c577  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c57c  MOV AL,byte ptr ES:[DI + 0x29]
0000:c580  CBW
0000:c581  CWDE
0000:c583  SHL EAX,0xc
0000:c587  ADD EBX,EAX
0000:c58a  CMP EBX,0xfffed000
0000:c591  JL 0x0000:c5a8
0000:c593  CMP EBX,0x13000
0000:c59a  JG 0x0000:c59f
0000:c59c  CLC
0000:c59d  JMP 0x0000:c5af
0000:c59f  MOV EBX,0x13000
0000:c5a5  STC
0000:c5a6  JMP 0x0000:c5af
0000:c5a8  MOV EBX,0xfffed000
0000:c5ae  STC
0000:c5af  MOV dword ptr ES:[DI + 0xa],EBX
0000:c5b4  ADD dword ptr ES:[DI + 0x2],EBX
0000:c5b9  DEC word ptr ES:[DI + 0x42]
0000:c5bd  JGE 0x0000:c5ce
0000:c5bf  NEG byte ptr ES:[DI + 0x40]
0000:c5c3  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c5c8  MOV word ptr ES:[DI + 0x42],0x14
0000:c5ce  JMP 0x0000:c747
0000:c5d1  CMP byte ptr ES:[DI + 0x40],0x0
0000:c5d6  JGE 0x0000:c665
0000:c5da  CMP word ptr ES:[DI + 0x42],0x14
0000:c5df  JNZ 0x0000:c5e1
0000:c5e1  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c5e6  MOV AL,byte ptr ES:[DI + 0x29]
0000:c5ea  CBW
0000:c5eb  CWDE
0000:c5ed  SHL EAX,0xd
0000:c5f1  SUB EBX,EAX
0000:c5f4  CMP EBX,0xfffeb000
0000:c5fb  JL 0x0000:c612
0000:c5fd  CMP EBX,0x15000
0000:c604  JG 0x0000:c609
0000:c606  CLC
0000:c607  JMP 0x0000:c619
0000:c609  MOV EBX,0x15000
0000:c60f  STC
0000:c610  JMP 0x0000:c619
0000:c612  MOV EBX,0xfffeb000
0000:c618  STC
0000:c619  MOV dword ptr ES:[DI + 0xa],EBX
0000:c61e  ADD dword ptr ES:[DI + 0x2],EBX
0000:c623  DEC word ptr ES:[DI + 0x42]
0000:c627  JGE 0x0000:c6bc
0000:c62b  NEG byte ptr ES:[DI + 0x29]
0000:c62f  NEG byte ptr ES:[DI + 0x28]
0000:c633  NEG byte ptr ES:[DI + 0x40]
0000:c637  CMP word ptr ES:[DI + 0x12],0x385
0000:c63d  JZ 0x0000:c647
0000:c63f  MOV word ptr ES:[DI + 0x12],0x385
0000:c645  JMP 0x0000:c64d
0000:c647  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c64d  MOV AL,byte ptr ES:[DI + 0x29]
0000:c651  CBW
0000:c652  CWDE
0000:c654  SHL EAX,0x9
0000:c658  MOV dword ptr ES:[DI + 0xa],EAX
0000:c65d  MOV word ptr ES:[DI + 0x42],0x14
0000:c663  JMP 0x0000:c6bc
0000:c665  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c66a  MOV AL,byte ptr ES:[DI + 0x29]
0000:c66e  CBW

; ---- C40B count=768 ----
0000:c40b  CMP byte ptr [0x88ae],0x3
0000:c410  JGE 0x0000:c788
0000:c414  MOV DX,0x32
0000:c417  MOV CX,0x64
0000:c41a  MOV BX,0xffce
0000:c41d  MOV AX,0xffce
0000:c420  CALLF 0x0000:ffff
0000:c425  CMP byte ptr ES:[DI + 0x29],0x0
0000:c42a  JG 0x0000:c440
0000:c42c  MOV AX,word ptr ES:[DI + 0x8]
0000:c430  MOV BX,word ptr ES:[DI + 0x4]
0000:c434  SUB BX,0x3c
0000:c437  CALLF 0x0000:ffff
0000:c43c  JZ 0x0000:c454
0000:c43e  JMP 0x0000:c459
0000:c440  MOV AX,word ptr ES:[DI + 0x8]
0000:c444  MOV BX,word ptr ES:[DI + 0x4]
0000:c448  ADD BX,0x3c
0000:c44b  CALLF 0x0000:ffff
0000:c450  JZ 0x0000:c454
0000:c452  JMP 0x0000:c459
0000:c454  MOV byte ptr ES:[DI + 0x3e],0x1
0000:c459  CMP byte ptr ES:[DI + 0x34],0x1
0000:c45e  JGE 0x0000:c6d2
0000:c462  CMP byte ptr ES:[DI + 0x3e],0x0
0000:c467  JLE 0x0000:c6bf
0000:c46b  CMP byte ptr [0x88ae],0x2
0000:c470  JGE 0x0000:c5d1
0000:c474  CMP word ptr ES:[DI + 0x46],0x1
0000:c479  JZ 0x0000:c524
0000:c47d  CMP byte ptr ES:[DI + 0x40],0x0
0000:c482  JGE 0x0000:c577
0000:c486  CMP word ptr ES:[DI + 0x42],0x14
0000:c48b  JNZ 0x0000:c48d
0000:c48d  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c492  MOV AL,byte ptr ES:[DI + 0x29]
0000:c496  CBW
0000:c497  CWDE
0000:c499  SHL EAX,0xa
0000:c49d  SUB EBX,EAX
0000:c4a0  CMP EBX,0xfffed000
0000:c4a7  JL 0x0000:c4be
0000:c4a9  CMP EBX,0x13000
0000:c4b0  JG 0x0000:c4b5
0000:c4b2  CLC
0000:c4b3  JMP 0x0000:c4c5
0000:c4b5  MOV EBX,0x13000
0000:c4bb  STC
0000:c4bc  JMP 0x0000:c4c5
0000:c4be  MOV EBX,0xfffed000
0000:c4c4  STC
0000:c4c5  MOV dword ptr ES:[DI + 0xa],EBX
0000:c4ca  ADD dword ptr ES:[DI + 0x2],EBX
0000:c4cf  DEC word ptr ES:[DI + 0x42]
0000:c4d3  JGE 0x0000:c5ce
0000:c4d7  MOV word ptr ES:[DI + 0x46],0x1
0000:c4dd  PUSH DI
0000:c4de  MOV AX,0x6616
0000:c4e1  XOR DX,DX
0000:c4e3  CALLF 0x0000:ffff
0000:c4e8  POP SI
0000:c4e9  MOV byte ptr ES:[DI + 0x17],0x2
0000:c4ee  MOV AL,byte ptr ES:[SI + 0x29]
0000:c4f2  MOV byte ptr ES:[DI + 0x29],AL
0000:c4f6  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c4fb  CMP byte ptr ES:[SI + 0x28],0x1
0000:c500  JNZ 0x0000:c50a
0000:c502  ADD EAX,0x70000
0000:c508  JMP 0x0000:c510
0000:c50a  SUB EAX,0x70000
0000:c510  MOV dword ptr ES:[DI + 0x2],EAX
0000:c515  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c51a  MOV dword ptr ES:[DI + 0x6],EAX
0000:c51f  MOV DI,SI
0000:c521  JMP 0x0000:c5ce
0000:c524  INC word ptr ES:[DI + 0x38]
0000:c528  CMP word ptr ES:[DI + 0x38],0x64
0000:c52d  JLE 0x0000:c5ce
0000:c531  MOV word ptr ES:[DI + 0x38],0x0
0000:c537  MOV word ptr ES:[DI + 0x46],0x0
0000:c53d  NEG byte ptr ES:[DI + 0x29]
0000:c541  NEG byte ptr ES:[DI + 0x28]
0000:c545  NEG byte ptr ES:[DI + 0x40]
0000:c549  CMP word ptr ES:[DI + 0x12],0x385
0000:c54f  JZ 0x0000:c559
0000:c551  MOV word ptr ES:[DI + 0x12],0x385
0000:c557  JMP 0x0000:c55f
0000:c559  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c55f  MOV AL,byte ptr ES:[DI + 0x29]
0000:c563  CBW
0000:c564  CWDE
0000:c566  SHL EAX,0x9
0000:c56a  MOV dword ptr ES:[DI + 0xa],EAX
0000:c56f  MOV word ptr ES:[DI + 0x42],0x28
0000:c575  JMP 0x0000:c5ce
0000:c577  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c57c  MOV AL,byte ptr ES:[DI + 0x29]
0000:c580  CBW
0000:c581  CWDE
0000:c583  SHL EAX,0xc
0000:c587  ADD EBX,EAX
0000:c58a  CMP EBX,0xfffed000
0000:c591  JL 0x0000:c5a8
0000:c593  CMP EBX,0x13000
0000:c59a  JG 0x0000:c59f
0000:c59c  CLC
0000:c59d  JMP 0x0000:c5af
0000:c59f  MOV EBX,0x13000
0000:c5a5  STC
0000:c5a6  JMP 0x0000:c5af
0000:c5a8  MOV EBX,0xfffed000
0000:c5ae  STC
0000:c5af  MOV dword ptr ES:[DI + 0xa],EBX
0000:c5b4  ADD dword ptr ES:[DI + 0x2],EBX
0000:c5b9  DEC word ptr ES:[DI + 0x42]
0000:c5bd  JGE 0x0000:c5ce
0000:c5bf  NEG byte ptr ES:[DI + 0x40]
0000:c5c3  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c5c8  MOV word ptr ES:[DI + 0x42],0x14
0000:c5ce  JMP 0x0000:c747
0000:c5d1  CMP byte ptr ES:[DI + 0x40],0x0
0000:c5d6  JGE 0x0000:c665
0000:c5da  CMP word ptr ES:[DI + 0x42],0x14
0000:c5df  JNZ 0x0000:c5e1
0000:c5e1  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c5e6  MOV AL,byte ptr ES:[DI + 0x29]
0000:c5ea  CBW
0000:c5eb  CWDE
0000:c5ed  SHL EAX,0xd
0000:c5f1  SUB EBX,EAX
0000:c5f4  CMP EBX,0xfffeb000
0000:c5fb  JL 0x0000:c612
0000:c5fd  CMP EBX,0x15000
0000:c604  JG 0x0000:c609
0000:c606  CLC
0000:c607  JMP 0x0000:c619
0000:c609  MOV EBX,0x15000
0000:c60f  STC
0000:c610  JMP 0x0000:c619
0000:c612  MOV EBX,0xfffeb000
0000:c618  STC
0000:c619  MOV dword ptr ES:[DI + 0xa],EBX
0000:c61e  ADD dword ptr ES:[DI + 0x2],EBX
0000:c623  DEC word ptr ES:[DI + 0x42]
0000:c627  JGE 0x0000:c6bc
0000:c62b  NEG byte ptr ES:[DI + 0x29]
0000:c62f  NEG byte ptr ES:[DI + 0x28]
0000:c633  NEG byte ptr ES:[DI + 0x40]
0000:c637  CMP word ptr ES:[DI + 0x12],0x385
0000:c63d  JZ 0x0000:c647
0000:c63f  MOV word ptr ES:[DI + 0x12],0x385
0000:c645  JMP 0x0000:c64d
0000:c647  MOV word ptr ES:[DI + 0x12],0x3b7
0000:c64d  MOV AL,byte ptr ES:[DI + 0x29]
0000:c651  CBW
0000:c652  CWDE
0000:c654  SHL EAX,0x9
0000:c658  MOV dword ptr ES:[DI + 0xa],EAX
0000:c65d  MOV word ptr ES:[DI + 0x42],0x14
0000:c663  JMP 0x0000:c6bc
0000:c665  MOV EBX,dword ptr ES:[DI + 0xa]
0000:c66a  MOV AL,byte ptr ES:[DI + 0x29]
0000:c66e  CBW
0000:c66f  CWDE
0000:c671  SHL EAX,0xd
0000:c675  ADD EBX,EAX
0000:c678  CMP EBX,0xfffeb000
0000:c67f  JL 0x0000:c696
0000:c681  CMP EBX,0x15000
0000:c688  JG 0x0000:c68d
0000:c68a  CLC
0000:c68b  JMP 0x0000:c69d
0000:c68d  MOV EBX,0x15000
0000:c693  STC
0000:c694  JMP 0x0000:c69d
0000:c696  MOV EBX,0xfffeb000
0000:c69c  STC
0000:c69d  MOV dword ptr ES:[DI + 0xa],EBX
0000:c6a2  ADD dword ptr ES:[DI + 0x2],EBX
0000:c6a7  DEC word ptr ES:[DI + 0x42]
0000:c6ab  JGE 0x0000:c6bc
0000:c6ad  NEG byte ptr ES:[DI + 0x40]
0000:c6b1  MOV byte ptr ES:[DI + 0x3e],0xff
0000:c6b6  MOV word ptr ES:[DI + 0x42],0x14
0000:c6bc  JMP 0x0000:c747
0000:c6bf  MOV EAX,dword ptr ES:[DI + 0xa]
0000:c6c4  ADD dword ptr ES:[DI + 0x2],EAX
0000:c6c9  CMP byte ptr [0x88ae],0x2
0000:c6ce  JGE 0x0000:c6d2
0000:c6d0  JMP 0x0000:c747
0000:c6d2  INC word ptr ES:[DI + 0x38]
0000:c6d6  CMP word ptr ES:[DI + 0x38],0x28
0000:c6db  JLE 0x0000:c747
0000:c6dd  MOV SI,0x646c
0000:c6e0  ADD SI,word ptr [0x6468]
0000:c6e4  INC word ptr [0x6468]
0000:c6e8  AND word ptr [0x6468],0xff
0000:c6ee  MOV AL,byte ptr [SI]
0000:c6f0  SHR AL,0x4
0000:c6f3  CBW
0000:c6f4  MOV word ptr ES:[DI + 0x38],AX
0000:c6f8  PUSH DI
0000:c6f9  MOV AX,0xc955
0000:c6fc  XOR DX,DX
0000:c6fe  CALLF 0x0000:ffff
0000:c703  POP SI
0000:c704  MOV byte ptr ES:[DI + 0x17],0x2
0000:c709  MOV AL,byte ptr ES:[SI + 0x29]
0000:c70d  MOV byte ptr ES:[DI + 0x29],AL
0000:c711  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c716  CMP byte ptr ES:[SI + 0x28],0x1
0000:c71b  JNZ 0x0000:c725
0000:c71d  SUB EAX,0x1e0000
0000:c723  JMP 0x0000:c72b
0000:c725  ADD EAX,0x1e0000
0000:c72b  MOV dword ptr ES:[DI + 0x2],EAX
0000:c730  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c735  SUB EAX,0x500000
0000:c73b  MOV dword ptr ES:[DI + 0x6],EAX
0000:c740  MOV DI,SI
0000:c742  MOV byte ptr ES:[DI + 0x34],0x0
0000:c747  PUSH DI
0000:c748  MOV SI,DI
0000:c74a  MOV DI,word ptr ES:[DI + 0x36]
0000:c74e  MOV EBX,dword ptr ES:[SI + 0x2]
0000:c753  MOV AL,byte ptr ES:[SI + 0x28]
0000:c757  MOV byte ptr ES:[DI + 0x28],AL
0000:c75b  CMP AL,0x1
0000:c75d  JNZ 0x0000:c768
0000:c75f  ADD EBX,0x1f0000
0000:c766  JMP 0x0000:c76f
0000:c768  SUB EBX,0x1f0000
0000:c76f  MOV dword ptr ES:[DI + 0x2],EBX
0000:c774  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c779  SUB EAX,0x190000
0000:c77f  MOV dword ptr ES:[DI + 0x6],EAX
0000:c784  POP DI
0000:c785  JMP 0x0000:c954
0000:c788  CMP byte ptr [0x88ae],0x4
0000:c78d  JGE 0x0000:c7bb
0000:c78f  PUSH DI
0000:c790  MOV SI,DI
0000:c792  MOV DI,word ptr ES:[DI + 0x36]
0000:c796  MOV word ptr ES:[DI + 0x18],0x0
0000:c79c  POP DI
0000:c79d  CMP word ptr ES:[DI + 0x12],0x385
0000:c7a3  JZ 0x0000:c7ad
0000:c7a5  MOV word ptr ES:[DI + 0x12],0x3b6
0000:c7ab  JMP 0x0000:c7b3
0000:c7ad  MOV word ptr ES:[DI + 0x12],0x384
0000:c7b3  MOV byte ptr [0x88ae],0x4
0000:c7b8  JMP 0x0000:c954
0000:c7bb  CMP byte ptr [0x88ae],0x5
0000:c7c0  JGE 0x0000:c8d9
0000:c7c4  MOV AX,word ptr ES:[DI + 0x2c]
0000:c7c8  SUB word ptr ES:[DI + 0x8],AX
0000:c7cc  MOV SI,0x7974
0000:c7cf  MOV AX,word ptr ES:[DI + 0x2e]
0000:c7d3  ADD AX,0x20
0000:c7d6  AND AX,0x5ff
0000:c7d9  MOV word ptr ES:[DI + 0x2e],AX
0000:c7dd  ADD SI,AX
0000:c7df  MOV AL,byte ptr [SI]
0000:c7e1  SAR AL,0x5
0000:c7e4  CBW
0000:c7e5  MOV word ptr ES:[DI + 0x2c],AX
0000:c7e9  ADD word ptr ES:[DI + 0x8],AX
0000:c7ed  INC word ptr ES:[DI + 0x38]
0000:c7f1  CMP word ptr ES:[DI + 0x38],0x19
0000:c7f6  JLE 0x0000:c954
0000:c7fa  MOV word ptr ES:[DI + 0x38],0x0
0000:c800  PUSH DI
0000:c801  MOV AX,0x4b70
0000:c804  XOR DX,DX
0000:c806  CALLF 0x0000:ffff
0000:c80b  POP SI
0000:c80c  MOV byte ptr ES:[DI + 0x17],0x2
0000:c811  PUSH SI
0000:c812  MOV SI,0x646c
0000:c815  ADD SI,word ptr [0x6468]
0000:c819  INC word ptr [0x6468]
0000:c81d  AND word ptr [0x6468],0xff
0000:c823  MOV AL,byte ptr [SI]
0000:c825  POP SI
0000:c826  SHR AL,0x2
0000:c829  CBW
0000:c82a  SUB AX,0x20
0000:c82d  MOV BX,word ptr ES:[SI + 0x4]
0000:c831  ADD BX,AX
0000:c833  MOV word ptr ES:[DI + 0x4],BX
0000:c837  PUSH SI
0000:c838  MOV SI,0x646c
0000:c83b  ADD SI,word ptr [0x6468]
0000:c83f  INC word ptr [0x6468]
0000:c843  AND word ptr [0x6468],0xff
0000:c849  MOV AL,byte ptr [SI]
0000:c84b  POP SI
0000:c84c  SHR AL,0x3
0000:c84f  CBW
0000:c850  MOV BX,word ptr ES:[SI + 0x8]
0000:c854  ADD BX,AX
0000:c856  SUB BX,0x14
0000:c859  MOV word ptr ES:[DI + 0x8],BX
0000:c85d  MOV DI,SI
0000:c85f  PUSH DI
0000:c860  MOV AX,0x4b70
0000:c863  XOR DX,DX
0000:c865  CALLF 0x0000:ffff
0000:c86a  POP SI
0000:c86b  MOV byte ptr ES:[DI + 0x17],0x2
0000:c870  PUSH SI
0000:c871  MOV SI,0x646c
0000:c874  ADD SI,word ptr [0x6468]
0000:c878  INC word ptr [0x6468]
0000:c87c  AND word ptr [0x6468],0xff
0000:c882  MOV AL,byte ptr [SI]
0000:c884  POP SI
0000:c885  SHR AL,0x2
0000:c888  CBW
0000:c889  SUB AX,0x20
0000:c88c  MOV BX,word ptr ES:[SI + 0x4]
0000:c890  ADD BX,AX
0000:c892  MOV word ptr ES:[DI + 0x4],BX
0000:c896  PUSH SI
0000:c897  MOV SI,0x646c
0000:c89a  ADD SI,word ptr [0x6468]
0000:c89e  INC word ptr [0x6468]
0000:c8a2  AND word ptr [0x6468],0xff
0000:c8a8  MOV AL,byte ptr [SI]
0000:c8aa  POP SI
0000:c8ab  SHR AL,0x3
0000:c8ae  CBW
0000:c8af  MOV BX,word ptr ES:[SI + 0x8]
0000:c8b3  ADD BX,AX
0000:c8b5  SUB BX,0x14
0000:c8b8  MOV word ptr ES:[DI + 0x8],BX
0000:c8bc  MOV DI,SI
0000:c8be  INC word ptr ES:[DI + 0x44]
0000:c8c2  CMP word ptr ES:[DI + 0x44],0xf
0000:c8c7  JLE 0x0000:c954
0000:c8cb  MOV dword ptr ES:[DI + 0xe],0xffff0000
0000:c8d4  MOV byte ptr [0x88ae],0x5
0000:c8d9  CMP byte ptr [0x88ae],0x6
0000:c8de  JGE 0x0000:c92c
0000:c8e0  INC word ptr ES:[DI + 0x38]
0000:c8e4  CMP word ptr ES:[DI + 0x38],0x28
0000:c8e9  JLE 0x0000:c954
0000:c8eb  MOV EAX,dword ptr ES:[DI + 0xe]
0000:c8f0  SUB dword ptr ES:[DI + 0xe],0x1200
0000:c8f9  ADD dword ptr ES:[DI + 0x6],EAX
0000:c8fe  MOV AX,word ptr ES:[DI + 0x4]
0000:c902  SUB AX,word ptr [0x81c0]
0000:c906  ADD AX,0x10
0000:c909  CMP AX,0x160
0000:c90c  JA 0x0000:c921
0000:c90e  MOV AX,word ptr ES:[DI + 0x8]
0000:c912  SUB AX,word ptr [0x81c4]
0000:c916  ADD AX,0x10
0000:c919  CMP AX,0xd0
0000:c91c  JA 0x0000:c921
0000:c91e  CLC
0000:c91f  JMP 0x0000:c92a
0000:c921  STC
0000:c922  MOV word ptr ES:[DI + 0x18],0x0
0000:c928  JMP 0x0000:c92c
0000:c92a  JMP 0x0000:c954
0000:c92c  MOV byte ptr [0x88ae],0x6
0000:c931  PUSH DI
0000:c932  MOV AX,0x487f
0000:c935  XOR DX,DX
0000:c937  CALLF 0x0000:ffff
0000:c93c  POP SI
0000:c93d  MOV byte ptr ES:[DI + 0x17],0x1
0000:c942  MOV BX,word ptr ES:[SI + 0x4]
0000:c946  MOV word ptr ES:[DI + 0x4],BX
0000:c94a  MOV BX,word ptr ES:[SI + 0x8]
0000:c94e  MOV word ptr ES:[DI + 0x8],BX
0000:c952  MOV DI,SI
0000:c954  RET
0000:c955  MOV word ptr ES:[DI + 0x12],0x389
0000:c95b  MOV word ptr ES:[DI + 0x18],0xcb11
0000:c961  MOV SI,0x646c
0000:c964  ADD SI,word ptr [0x6468]
0000:c968  INC word ptr [0x6468]
0000:c96c  AND word ptr [0x6468],0xff
0000:c972  MOV AL,byte ptr [SI]
0000:c974  CBW
0000:c975  CWDE
0000:c977  SHL EAX,0x8
0000:c97b  NEG EAX
0000:c97e  ADD EAX,0xfffec000
0000:c984  MOV dword ptr ES:[DI + 0xe],EAX
0000:c989  MOV SI,0x646c
0000:c98c  ADD SI,word ptr [0x6468]
0000:c990  INC word ptr [0x6468]
0000:c994  AND word ptr [0x6468],0xff
0000:c99a  MOV AL,byte ptr [SI]
0000:c99c  CBW
0000:c99d  CWDE
0000:c99f  SHL EAX,0xa
0000:c9a3  ADD EAX,0xffffa000
0000:c9a9  MOV dword ptr ES:[DI + 0xa],EAX
0000:c9ae  CMP EAX,0x0
0000:c9b2  JL 0x0000:c9c0
0000:c9b4  MOV byte ptr ES:[DI + 0x28],0x1
0000:c9b9  MOV byte ptr ES:[DI + 0x29],0x1
0000:c9be  JMP 0x0000:c9ca
0000:c9c0  MOV byte ptr ES:[DI + 0x28],0xff
0000:c9c5  MOV byte ptr ES:[DI + 0x29],0xff
0000:c9ca  PUSH DI
0000:c9cb  MOV AX,0xc9f8
0000:c9ce  XOR DX,DX
0000:c9d0  CALLF 0x0000:ffff
0000:c9d5  POP SI
0000:c9d6  MOV byte ptr ES:[DI + 0x17],0x2
0000:c9db  MOV EAX,dword ptr ES:[SI + 0x2]
0000:c9e0  MOV dword ptr ES:[DI + 0x2],EAX
0000:c9e5  MOV EAX,dword ptr ES:[SI + 0x6]
0000:c9ea  ADD EAX,0xf0000
0000:c9f0  MOV dword ptr ES:[DI + 0x6],EAX
0000:c9f5  MOV DI,SI
0000:c9f7  RET
0000:c9f8  MOV word ptr ES:[DI + 0x12],0x387
0000:c9fe  MOV word ptr ES:[DI + 0x18],0xcb11
0000:ca04  MOV SI,0x646c
0000:ca07  ADD SI,word ptr [0x6468]
0000:ca0b  INC word ptr [0x6468]
0000:ca0f  AND word ptr [0x6468],0xff
0000:ca15  MOV AL,byte ptr [SI]
0000:ca17  CBW
0000:ca18  CWDE
0000:ca1a  SHL EAX,0x8
0000:ca1e  NEG EAX
0000:ca21  ADD EAX,0xfffec000
0000:ca27  MOV dword ptr ES:[DI + 0xe],EAX
0000:ca2c  MOV SI,0x646c
0000:ca2f  ADD SI,word ptr [0x6468]
0000:ca33  INC word ptr [0x6468]
0000:ca37  AND word ptr [0x6468],0xff
0000:ca3d  MOV AL,byte ptr [SI]
0000:ca3f  CBW
0000:ca40  CWDE
0000:ca42  SHL EAX,0xa
0000:ca46  ADD EAX,0xffffa000
0000:ca4c  MOV dword ptr ES:[DI + 0xa],EAX
0000:ca51  CMP EAX,0x0
0000:ca55  JL 0x0000:ca63
0000:ca57  MOV byte ptr ES:[DI + 0x28],0x1
0000:ca5c  MOV byte ptr ES:[DI + 0x29],0x1
0000:ca61  JMP 0x0000:ca6d
0000:ca63  MOV byte ptr ES:[DI + 0x28],0xff
0000:ca68  MOV byte ptr ES:[DI + 0x29],0xff
0000:ca6d  PUSH DI
0000:ca6e  MOV AX,0xca9b
0000:ca71  XOR DX,DX
0000:ca73  CALLF 0x0000:ffff
0000:ca78  POP SI
0000:ca79  MOV byte ptr ES:[DI + 0x17],0x2
0000:ca7e  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ca83  MOV dword ptr ES:[DI + 0x2],EAX
0000:ca88  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ca8d  ADD EAX,0xa0000
0000:ca93  MOV dword ptr ES:[DI + 0x6],EAX
0000:ca98  MOV DI,SI
0000:ca9a  RET
0000:ca9b  MOV word ptr ES:[DI + 0x12],0x388
0000:caa1  MOV word ptr ES:[DI + 0x18],0xcb11
0000:caa7  MOV SI,0x646c
0000:caaa  ADD SI,word ptr [0x6468]
0000:caae  INC word ptr [0x6468]
0000:cab2  AND word ptr [0x6468],0xff
0000:cab8  MOV AL,byte ptr [SI]
0000:caba  CBW
0000:cabb  CWDE
0000:cabd  SHL EAX,0x8
0000:cac1  NEG EAX
0000:cac4  ADD EAX,0xfffec000
0000:caca  MOV dword ptr ES:[DI + 0xe],EAX
0000:cacf  MOV SI,0x646c
0000:cad2  ADD SI,word ptr [0x6468]
0000:cad6  INC word ptr [0x6468]
0000:cada  AND word ptr [0x6468],0xff
0000:cae0  MOV AL,byte ptr [SI]
0000:cae2  CBW
0000:cae3  CWDE
0000:cae5  SHL EAX,0xa
0000:cae9  ADD EAX,0xffffa000
0000:caef  MOV dword ptr ES:[DI + 0xa],EAX
0000:caf4  CMP EAX,0x0
0000:caf8  JL 0x0000:cb06
0000:cafa  MOV byte ptr ES:[DI + 0x28],0x1
0000:caff  MOV byte ptr ES:[DI + 0x29],0x1
0000:cb04  JMP 0x0000:cb10
0000:cb06  MOV byte ptr ES:[DI + 0x28],0xff
0000:cb0b  MOV byte ptr ES:[DI + 0x29],0xff
0000:cb10  RET
0000:cb11  MOV AX,word ptr ES:[DI + 0x4]
0000:cb15  SUB AX,word ptr [0x81c0]
0000:cb19  ADD AX,0x40
0000:cb1c  CMP AX,0x1c0
0000:cb1f  JA 0x0000:cb34
0000:cb21  MOV AX,word ptr ES:[DI + 0x8]
0000:cb25  SUB AX,word ptr [0x81c4]
0000:cb29  ADD AX,0x40
0000:cb2c  CMP AX,0x130
0000:cb2f  JA 0x0000:cb34
0000:cb31  CLC
0000:cb32  JMP 0x0000:cb3b
0000:cb34  STC
0000:cb35  MOV word ptr ES:[DI + 0x18],0x0
0000:cb3b  MOV CX,0x0
0000:cb3e  MOV DX,0x0
0000:cb41  CALLF 0x0000:ffff
0000:cb46  JNC 0x0000:cb4a
0000:cb48  JMP 0x0000:cba5
0000:cb4a  CMP byte ptr ES:[DI + 0x29],0x0
0000:cb4f  JG 0x0000:cb79
0000:cb51  MOV AX,word ptr ES:[DI + 0x8]
0000:cb55  MOV BX,word ptr ES:[DI + 0x4]
0000:cb59  SUB BX,0x0
0000:cb5c  CALLF 0x0000:ffff
0000:cb61  TEST DL,0x70
0000:cb64  JNZ 0x0000:cba1
0000:cb66  MOV AX,word ptr ES:[DI + 0x8]
0000:cb6a  SUB AX,0x7
0000:cb6d  CALLF 0x0000:ffff
0000:cb72  TEST DL,0x70
0000:cb75  JNZ 0x0000:cba1
0000:cb77  JMP 0x0000:cba3
0000:cb79  MOV AX,word ptr ES:[DI + 0x8]
0000:cb7d  MOV BX,word ptr ES:[DI + 0x4]
0000:cb81  ADD BX,0x0
0000:cb84  CALLF 0x0000:ffff
0000:cb89  TEST DL,0x70
0000:cb8c  JNZ 0x0000:cba1
0000:cb8e  MOV AX,word ptr ES:[DI + 0x8]
0000:cb92  SUB AX,0x7
0000:cb95  CALLF 0x0000:ffff
0000:cb9a  TEST DL,0x70
0000:cb9d  JNZ 0x0000:cba1
0000:cb9f  JMP 0x0000:cba3
0000:cba1  JMP 0x0000:cba5
0000:cba3  JMP 0x0000:cbac
0000:cba5  MOV word ptr ES:[DI + 0x18],0x0
0000:cbab  RET
0000:cbac  MOV EBX,dword ptr ES:[DI + 0xe]
0000:cbb1  ADD dword ptr ES:[DI + 0x6],EBX
0000:cbb6  ADD EBX,0x1770
0000:cbbd  CMP EBX,0xfffd0000
0000:cbc4  JL 0x0000:cbdb
0000:cbc6  CMP EBX,0x35000
0000:cbcd  JG 0x0000:cbd2
0000:cbcf  CLC
0000:cbd0  JMP 0x0000:cbe2
0000:cbd2  MOV EBX,0x35000
0000:cbd8  STC
0000:cbd9  JMP 0x0000:cbe2
0000:cbdb  MOV EBX,0xfffd0000
0000:cbe1  STC
0000:cbe2  MOV dword ptr ES:[DI + 0xe],EBX
0000:cbe7  MOV EBX,dword ptr ES:[DI + 0xa]
0000:cbec  ADD dword ptr ES:[DI + 0x2],EBX
0000:cbf1  MOV AL,byte ptr ES:[DI + 0x29]
0000:cbf5  CBW
0000:cbf6  CWDE
0000:cbf8  SHL EAX,0xb
0000:cbfc  ADD EBX,EAX
0000:cbff  CMP EBX,0xfffea000
0000:cc06  JL 0x0000:cc1d
0000:cc08  CMP EBX,0x16000
0000:cc0f  JG 0x0000:cc14
0000:cc11  CLC
0000:cc12  JMP 0x0000:cc24
0000:cc14  MOV EBX,0x16000
0000:cc1a  STC
0000:cc1b  JMP 0x0000:cc24
0000:cc1d  MOV EBX,0xfffea000
0000:cc23  STC
0000:cc24  MOV dword ptr ES:[DI + 0xa],EBX
0000:cc29  MOV DX,0x8
0000:cc2c  MOV CX,0x10
0000:cc2f  MOV BX,0xfff8
0000:cc32  MOV AX,0xfff8
0000:cc35  CALLF 0x0000:ffff
0000:cc3a  RET
0000:cc41  MOV AX,0xcc68
0000:cc44  XOR DX,DX
0000:cc46  CALLF 0x0000:ffff
0000:cc4b  MOV byte ptr ES:[DI + 0x17],0x1
0000:cc50  MOV AX,[0x81c0]
0000:cc53  MOV BX,word ptr [0x81c4]
0000:cc57  ADD AX,0x258
0000:cc5a  ADD BX,0x96
0000:cc5e  MOV word ptr ES:[DI + 0x4],AX
0000:cc62  MOV word ptr ES:[DI + 0x8],BX
0000:cc66  POP DI
0000:cc67  RETF
0000:cc68  MOV byte ptr ES:[DI + 0x17],0x1
0000:cc6d  MOV word ptr ES:[DI + 0x18],0xce81
0000:cc73  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cc79  MOV byte ptr ES:[DI + 0x28],0xff
0000:cc7e  MOV byte ptr ES:[DI + 0x29],0xff
0000:cc83  MOV word ptr ES:[DI + 0x38],0x0
0000:cc89  MOV word ptr ES:[DI + 0x44],0x0
0000:cc8f  MOV byte ptr ES:[DI + 0x34],0x0
0000:cc94  MOV dword ptr ES:[DI + 0xa],0xffff9000
0000:cc9d  MOV dword ptr ES:[DI + 0xe],0xffff9000
0000:cca6  MOV byte ptr ES:[DI + 0x40],0xff
0000:ccab  MOV word ptr ES:[DI + 0x42],0x14
0000:ccb1  MOV byte ptr ES:[DI + 0x3e],0xff
0000:ccb6  PUSH DI
0000:ccb7  MOV AX,0xcd25
0000:ccba  XOR DX,DX
0000:ccbc  CALLF 0x0000:ffff
0000:ccc1  POP SI
0000:ccc2  MOV word ptr ES:[SI + 0x2a],DI
0000:ccc6  MOV byte ptr ES:[DI + 0x17],0x1
0000:cccb  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ccd0  SUB EAX,0x50000
0000:ccd6  MOV dword ptr ES:[DI + 0x2],EAX
0000:ccdb  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cce0  SUB EAX,0xc0000
0000:cce6  MOV dword ptr ES:[DI + 0x6],EAX
0000:cceb  MOV DI,SI
0000:cced  PUSH DI
0000:ccee  MOV AX,0xcd88
0000:ccf1  XOR DX,DX
0000:ccf3  CALLF 0x0000:ffff
0000:ccf8  POP SI
0000:ccf9  MOV word ptr ES:[SI + 0x36],DI
0000:ccfd  MOV byte ptr ES:[DI + 0x17],0x2
0000:cd02  MOV EAX,dword ptr ES:[SI + 0x2]
0000:cd07  SUB EAX,0x1f0000
0000:cd0d  MOV dword ptr ES:[DI + 0x2],EAX
0000:cd12  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cd17  SUB EAX,0x1c0000
0000:cd1d  MOV dword ptr ES:[DI + 0x6],EAX
0000:cd22  MOV DI,SI
0000:cd24  RET
0000:cd25  MOV SI,0x3384
0000:cd28  CALLF 0x0000:ffff
0000:cd2d  MOV word ptr ES:[DI + 0x18],0xcd40
0000:cd33  MOV word ptr ES:[DI + 0x2e],0x0
0000:cd39  MOV word ptr ES:[DI + 0x2c],0x0
0000:cd3f  RET
0000:cd40  CMP byte ptr [0x88ae],0x5
0000:cd45  JNZ 0x0000:cd71
0000:cd47  MOV AX,word ptr ES:[DI + 0x4]
0000:cd4b  SUB AX,word ptr [0x81c0]
0000:cd4f  ADD AX,0x10
0000:cd52  CMP AX,0x160
0000:cd55  JA 0x0000:cd6a
0000:cd57  MOV AX,word ptr ES:[DI + 0x8]
0000:cd5b  SUB AX,word ptr [0x81c4]
0000:cd5f  ADD AX,0x10
0000:cd62  CMP AX,0xd0
0000:cd65  JA 0x0000:cd6a
0000:cd67  CLC
0000:cd68  JMP 0x0000:cd71
0000:cd6a  STC
0000:cd6b  MOV word ptr ES:[DI + 0x18],0x0
0000:cd71  MOV DX,0xffbf
0000:cd74  MOV CX,0x5a
0000:cd77  MOV BX,0x41
0000:cd7a  MOV AX,0xffd3
0000:cd7d  CALLF 0x0000:ffff
0000:cd82  CALLF 0x0000:ffff
0000:cd87  RET
0000:cd88  MOV SI,0x32fa
0000:cd8b  CALLF 0x0000:ffff
0000:cd90  MOV word ptr ES:[DI + 0x18],0xcda3
0000:cd96  MOV word ptr ES:[DI + 0x2a],0x0
0000:cd9c  MOV word ptr ES:[DI + 0x2c],0x0
0000:cda2  RET
0000:cda3  CMP byte ptr ES:[DI + 0x2e],0x1
0000:cda8  JGE 0x0000:ce5b
0000:cdac  CMP word ptr [0x8806],0x0
0000:cdb1  JZ 0x0000:ce7b
0000:cdb5  MOV BX,word ptr ES:[DI + 0x2a]
0000:cdb9  CMP BX,word ptr [0x8808]
0000:cdbd  JLE 0x0000:cdc7
0000:cdbf  MOV word ptr ES:[DI + 0x2a],0x0
0000:cdc5  XOR BX,BX
0000:cdc7  SHL BX,0x2
0000:cdca  MOV AX,word ptr ES:[DI + 0x4]
0000:cdce  SUB AX,0xf
0000:cdd1  CMP word ptr [BX + 0x87de],AX
0000:cdd5  JLE 0x0000:cdfe
0000:cdd7  ADD AX,0x1e
0000:cdda  CMP word ptr [BX + 0x87de],AX
0000:cdde  JGE 0x0000:cdfe
0000:cde0  MOV AX,word ptr ES:[DI + 0x8]
0000:cde4  ADD AX,0x5
0000:cde7  CMP word ptr [BX + 0x87e0],AX
0000:cdeb  JGE 0x0000:cdfe
0000:cded  SUB AX,0x1e
0000:cdf0  CMP word ptr [BX + 0x87e0],AX
0000:cdf4  JLE 0x0000:cdfe
0000:cdf6  MOV word ptr [BX + 0x87de],0x0
0000:cdfc  JMP 0x0000:ce00
0000:cdfe  JMP 0x0000:ce49
0000:ce00  INC word ptr ES:[DI + 0x2c]
0000:ce04  PUSH DI
0000:ce05  MOV AX,0x4b70
0000:ce08  XOR DX,DX
0000:ce0a  CALLF 0x0000:ffff
0000:ce0f  POP SI
0000:ce10  MOV byte ptr ES:[DI + 0x17],0x2
0000:ce15  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ce1a  MOV dword ptr ES:[DI + 0x2],EAX
0000:ce1f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ce24  ADD EAX,0xa0000
0000:ce2a  MOV dword ptr ES:[DI + 0x6],EAX
0000:ce2f  MOV DI,SI
0000:ce31  MOV SI,0x3308
0000:ce34  CALLF 0x0000:ffff
0000:ce39  MOV word ptr [0x612e],0xd
0000:ce3f  CALLF 0x0000:ffff
0000:ce44  MOV byte ptr ES:[DI + 0x2e],0x1
0000:ce49  INC word ptr ES:[DI + 0x2a]
0000:ce4d  CMP word ptr ES:[DI + 0x2c],0x5
0000:ce52  JLE 0x0000:ce7b
0000:ce54  MOV byte ptr [0x88ae],0x2
0000:ce59  JMP 0x0000:ce7b
0000:ce5b  INC word ptr ES:[DI + 0x2f]
0000:ce5f  CMP word ptr ES:[DI + 0x2f],0x64
0000:ce64  JLE 0x0000:ce7b
0000:ce66  MOV word ptr ES:[DI + 0x2f],0x0
0000:ce6c  MOV byte ptr ES:[DI + 0x2e],0x0
0000:ce71  MOV SI,0x32fa
0000:ce74  CALLF 0x0000:ffff
0000:ce79  JMP 0x0000:ce7b
0000:ce7b  CALLF 0x0000:ffff
0000:ce80  RET
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX
0000:cebd  CALLF 0x0000:ffff
0000:cec2  JNZ 0x0000:cf07
0000:cec4  MOV DX,0x32
0000:cec7  NEG DX
0000:cec9  TEST byte ptr ES:[DI + 0x28],0xff
0000:cece  JS 0x0000:ced2
0000:ced0  NEG DX
0000:ced2  MOV AX,word ptr ES:[DI + 0x8]
0000:ced6  SUB AX,0x11
0000:ced9  MOV BX,word ptr ES:[DI + 0x4]
0000:cedd  ADD BX,DX
0000:cedf  CALLF 0x0000:ffff
0000:cee4  JNZ 0x0000:cf07
0000:cee6  MOV DX,0x32
0000:cee9  NEG DX
0000:ceeb  TEST byte ptr ES:[DI + 0x28],0xff

; ---- CB11 count=256 ----
0000:cb11  MOV AX,word ptr ES:[DI + 0x4]
0000:cb15  SUB AX,word ptr [0x81c0]
0000:cb19  ADD AX,0x40
0000:cb1c  CMP AX,0x1c0
0000:cb1f  JA 0x0000:cb34
0000:cb21  MOV AX,word ptr ES:[DI + 0x8]
0000:cb25  SUB AX,word ptr [0x81c4]
0000:cb29  ADD AX,0x40
0000:cb2c  CMP AX,0x130
0000:cb2f  JA 0x0000:cb34
0000:cb31  CLC
0000:cb32  JMP 0x0000:cb3b
0000:cb34  STC
0000:cb35  MOV word ptr ES:[DI + 0x18],0x0
0000:cb3b  MOV CX,0x0
0000:cb3e  MOV DX,0x0
0000:cb41  CALLF 0x0000:ffff
0000:cb46  JNC 0x0000:cb4a
0000:cb48  JMP 0x0000:cba5
0000:cb4a  CMP byte ptr ES:[DI + 0x29],0x0
0000:cb4f  JG 0x0000:cb79
0000:cb51  MOV AX,word ptr ES:[DI + 0x8]
0000:cb55  MOV BX,word ptr ES:[DI + 0x4]
0000:cb59  SUB BX,0x0
0000:cb5c  CALLF 0x0000:ffff
0000:cb61  TEST DL,0x70
0000:cb64  JNZ 0x0000:cba1
0000:cb66  MOV AX,word ptr ES:[DI + 0x8]
0000:cb6a  SUB AX,0x7
0000:cb6d  CALLF 0x0000:ffff
0000:cb72  TEST DL,0x70
0000:cb75  JNZ 0x0000:cba1
0000:cb77  JMP 0x0000:cba3
0000:cb79  MOV AX,word ptr ES:[DI + 0x8]
0000:cb7d  MOV BX,word ptr ES:[DI + 0x4]
0000:cb81  ADD BX,0x0
0000:cb84  CALLF 0x0000:ffff
0000:cb89  TEST DL,0x70
0000:cb8c  JNZ 0x0000:cba1
0000:cb8e  MOV AX,word ptr ES:[DI + 0x8]
0000:cb92  SUB AX,0x7
0000:cb95  CALLF 0x0000:ffff
0000:cb9a  TEST DL,0x70
0000:cb9d  JNZ 0x0000:cba1
0000:cb9f  JMP 0x0000:cba3
0000:cba1  JMP 0x0000:cba5
0000:cba3  JMP 0x0000:cbac
0000:cba5  MOV word ptr ES:[DI + 0x18],0x0
0000:cbab  RET
0000:cbac  MOV EBX,dword ptr ES:[DI + 0xe]
0000:cbb1  ADD dword ptr ES:[DI + 0x6],EBX
0000:cbb6  ADD EBX,0x1770
0000:cbbd  CMP EBX,0xfffd0000
0000:cbc4  JL 0x0000:cbdb
0000:cbc6  CMP EBX,0x35000
0000:cbcd  JG 0x0000:cbd2
0000:cbcf  CLC
0000:cbd0  JMP 0x0000:cbe2
0000:cbd2  MOV EBX,0x35000
0000:cbd8  STC
0000:cbd9  JMP 0x0000:cbe2
0000:cbdb  MOV EBX,0xfffd0000
0000:cbe1  STC
0000:cbe2  MOV dword ptr ES:[DI + 0xe],EBX
0000:cbe7  MOV EBX,dword ptr ES:[DI + 0xa]
0000:cbec  ADD dword ptr ES:[DI + 0x2],EBX
0000:cbf1  MOV AL,byte ptr ES:[DI + 0x29]
0000:cbf5  CBW
0000:cbf6  CWDE
0000:cbf8  SHL EAX,0xb
0000:cbfc  ADD EBX,EAX
0000:cbff  CMP EBX,0xfffea000
0000:cc06  JL 0x0000:cc1d
0000:cc08  CMP EBX,0x16000
0000:cc0f  JG 0x0000:cc14
0000:cc11  CLC
0000:cc12  JMP 0x0000:cc24
0000:cc14  MOV EBX,0x16000
0000:cc1a  STC
0000:cc1b  JMP 0x0000:cc24
0000:cc1d  MOV EBX,0xfffea000
0000:cc23  STC
0000:cc24  MOV dword ptr ES:[DI + 0xa],EBX
0000:cc29  MOV DX,0x8
0000:cc2c  MOV CX,0x10
0000:cc2f  MOV BX,0xfff8
0000:cc32  MOV AX,0xfff8
0000:cc35  CALLF 0x0000:ffff
0000:cc3a  RET
0000:cc41  MOV AX,0xcc68
0000:cc44  XOR DX,DX
0000:cc46  CALLF 0x0000:ffff
0000:cc4b  MOV byte ptr ES:[DI + 0x17],0x1
0000:cc50  MOV AX,[0x81c0]
0000:cc53  MOV BX,word ptr [0x81c4]
0000:cc57  ADD AX,0x258
0000:cc5a  ADD BX,0x96
0000:cc5e  MOV word ptr ES:[DI + 0x4],AX
0000:cc62  MOV word ptr ES:[DI + 0x8],BX
0000:cc66  POP DI
0000:cc67  RETF
0000:cc68  MOV byte ptr ES:[DI + 0x17],0x1
0000:cc6d  MOV word ptr ES:[DI + 0x18],0xce81
0000:cc73  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cc79  MOV byte ptr ES:[DI + 0x28],0xff
0000:cc7e  MOV byte ptr ES:[DI + 0x29],0xff
0000:cc83  MOV word ptr ES:[DI + 0x38],0x0
0000:cc89  MOV word ptr ES:[DI + 0x44],0x0
0000:cc8f  MOV byte ptr ES:[DI + 0x34],0x0
0000:cc94  MOV dword ptr ES:[DI + 0xa],0xffff9000
0000:cc9d  MOV dword ptr ES:[DI + 0xe],0xffff9000
0000:cca6  MOV byte ptr ES:[DI + 0x40],0xff
0000:ccab  MOV word ptr ES:[DI + 0x42],0x14
0000:ccb1  MOV byte ptr ES:[DI + 0x3e],0xff
0000:ccb6  PUSH DI
0000:ccb7  MOV AX,0xcd25
0000:ccba  XOR DX,DX
0000:ccbc  CALLF 0x0000:ffff
0000:ccc1  POP SI
0000:ccc2  MOV word ptr ES:[SI + 0x2a],DI
0000:ccc6  MOV byte ptr ES:[DI + 0x17],0x1
0000:cccb  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ccd0  SUB EAX,0x50000
0000:ccd6  MOV dword ptr ES:[DI + 0x2],EAX
0000:ccdb  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cce0  SUB EAX,0xc0000
0000:cce6  MOV dword ptr ES:[DI + 0x6],EAX
0000:cceb  MOV DI,SI
0000:cced  PUSH DI
0000:ccee  MOV AX,0xcd88
0000:ccf1  XOR DX,DX
0000:ccf3  CALLF 0x0000:ffff
0000:ccf8  POP SI
0000:ccf9  MOV word ptr ES:[SI + 0x36],DI
0000:ccfd  MOV byte ptr ES:[DI + 0x17],0x2
0000:cd02  MOV EAX,dword ptr ES:[SI + 0x2]
0000:cd07  SUB EAX,0x1f0000
0000:cd0d  MOV dword ptr ES:[DI + 0x2],EAX
0000:cd12  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cd17  SUB EAX,0x1c0000
0000:cd1d  MOV dword ptr ES:[DI + 0x6],EAX
0000:cd22  MOV DI,SI
0000:cd24  RET
0000:cd25  MOV SI,0x3384
0000:cd28  CALLF 0x0000:ffff
0000:cd2d  MOV word ptr ES:[DI + 0x18],0xcd40
0000:cd33  MOV word ptr ES:[DI + 0x2e],0x0
0000:cd39  MOV word ptr ES:[DI + 0x2c],0x0
0000:cd3f  RET
0000:cd40  CMP byte ptr [0x88ae],0x5
0000:cd45  JNZ 0x0000:cd71
0000:cd47  MOV AX,word ptr ES:[DI + 0x4]
0000:cd4b  SUB AX,word ptr [0x81c0]
0000:cd4f  ADD AX,0x10
0000:cd52  CMP AX,0x160
0000:cd55  JA 0x0000:cd6a
0000:cd57  MOV AX,word ptr ES:[DI + 0x8]
0000:cd5b  SUB AX,word ptr [0x81c4]
0000:cd5f  ADD AX,0x10
0000:cd62  CMP AX,0xd0
0000:cd65  JA 0x0000:cd6a
0000:cd67  CLC
0000:cd68  JMP 0x0000:cd71
0000:cd6a  STC
0000:cd6b  MOV word ptr ES:[DI + 0x18],0x0
0000:cd71  MOV DX,0xffbf
0000:cd74  MOV CX,0x5a
0000:cd77  MOV BX,0x41
0000:cd7a  MOV AX,0xffd3
0000:cd7d  CALLF 0x0000:ffff
0000:cd82  CALLF 0x0000:ffff
0000:cd87  RET
0000:cd88  MOV SI,0x32fa
0000:cd8b  CALLF 0x0000:ffff
0000:cd90  MOV word ptr ES:[DI + 0x18],0xcda3
0000:cd96  MOV word ptr ES:[DI + 0x2a],0x0
0000:cd9c  MOV word ptr ES:[DI + 0x2c],0x0
0000:cda2  RET
0000:cda3  CMP byte ptr ES:[DI + 0x2e],0x1
0000:cda8  JGE 0x0000:ce5b
0000:cdac  CMP word ptr [0x8806],0x0
0000:cdb1  JZ 0x0000:ce7b
0000:cdb5  MOV BX,word ptr ES:[DI + 0x2a]
0000:cdb9  CMP BX,word ptr [0x8808]
0000:cdbd  JLE 0x0000:cdc7
0000:cdbf  MOV word ptr ES:[DI + 0x2a],0x0
0000:cdc5  XOR BX,BX
0000:cdc7  SHL BX,0x2
0000:cdca  MOV AX,word ptr ES:[DI + 0x4]
0000:cdce  SUB AX,0xf
0000:cdd1  CMP word ptr [BX + 0x87de],AX
0000:cdd5  JLE 0x0000:cdfe
0000:cdd7  ADD AX,0x1e
0000:cdda  CMP word ptr [BX + 0x87de],AX
0000:cdde  JGE 0x0000:cdfe
0000:cde0  MOV AX,word ptr ES:[DI + 0x8]
0000:cde4  ADD AX,0x5
0000:cde7  CMP word ptr [BX + 0x87e0],AX
0000:cdeb  JGE 0x0000:cdfe
0000:cded  SUB AX,0x1e
0000:cdf0  CMP word ptr [BX + 0x87e0],AX
0000:cdf4  JLE 0x0000:cdfe
0000:cdf6  MOV word ptr [BX + 0x87de],0x0
0000:cdfc  JMP 0x0000:ce00
0000:cdfe  JMP 0x0000:ce49
0000:ce00  INC word ptr ES:[DI + 0x2c]
0000:ce04  PUSH DI
0000:ce05  MOV AX,0x4b70
0000:ce08  XOR DX,DX
0000:ce0a  CALLF 0x0000:ffff
0000:ce0f  POP SI
0000:ce10  MOV byte ptr ES:[DI + 0x17],0x2
0000:ce15  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ce1a  MOV dword ptr ES:[DI + 0x2],EAX
0000:ce1f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ce24  ADD EAX,0xa0000
0000:ce2a  MOV dword ptr ES:[DI + 0x6],EAX
0000:ce2f  MOV DI,SI
0000:ce31  MOV SI,0x3308
0000:ce34  CALLF 0x0000:ffff
0000:ce39  MOV word ptr [0x612e],0xd
0000:ce3f  CALLF 0x0000:ffff
0000:ce44  MOV byte ptr ES:[DI + 0x2e],0x1
0000:ce49  INC word ptr ES:[DI + 0x2a]
0000:ce4d  CMP word ptr ES:[DI + 0x2c],0x5
0000:ce52  JLE 0x0000:ce7b
0000:ce54  MOV byte ptr [0x88ae],0x2
0000:ce59  JMP 0x0000:ce7b
0000:ce5b  INC word ptr ES:[DI + 0x2f]
0000:ce5f  CMP word ptr ES:[DI + 0x2f],0x64
0000:ce64  JLE 0x0000:ce7b
0000:ce66  MOV word ptr ES:[DI + 0x2f],0x0
0000:ce6c  MOV byte ptr ES:[DI + 0x2e],0x0
0000:ce71  MOV SI,0x32fa
0000:ce74  CALLF 0x0000:ffff
0000:ce79  JMP 0x0000:ce7b
0000:ce7b  CALLF 0x0000:ffff
0000:ce80  RET
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX

; ---- CC68 count=192 ----
0000:cc68  MOV byte ptr ES:[DI + 0x17],0x1
0000:cc6d  MOV word ptr ES:[DI + 0x18],0xce81
0000:cc73  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cc79  MOV byte ptr ES:[DI + 0x28],0xff
0000:cc7e  MOV byte ptr ES:[DI + 0x29],0xff
0000:cc83  MOV word ptr ES:[DI + 0x38],0x0
0000:cc89  MOV word ptr ES:[DI + 0x44],0x0
0000:cc8f  MOV byte ptr ES:[DI + 0x34],0x0
0000:cc94  MOV dword ptr ES:[DI + 0xa],0xffff9000
0000:cc9d  MOV dword ptr ES:[DI + 0xe],0xffff9000
0000:cca6  MOV byte ptr ES:[DI + 0x40],0xff
0000:ccab  MOV word ptr ES:[DI + 0x42],0x14
0000:ccb1  MOV byte ptr ES:[DI + 0x3e],0xff
0000:ccb6  PUSH DI
0000:ccb7  MOV AX,0xcd25
0000:ccba  XOR DX,DX
0000:ccbc  CALLF 0x0000:ffff
0000:ccc1  POP SI
0000:ccc2  MOV word ptr ES:[SI + 0x2a],DI
0000:ccc6  MOV byte ptr ES:[DI + 0x17],0x1
0000:cccb  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ccd0  SUB EAX,0x50000
0000:ccd6  MOV dword ptr ES:[DI + 0x2],EAX
0000:ccdb  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cce0  SUB EAX,0xc0000
0000:cce6  MOV dword ptr ES:[DI + 0x6],EAX
0000:cceb  MOV DI,SI
0000:cced  PUSH DI
0000:ccee  MOV AX,0xcd88
0000:ccf1  XOR DX,DX
0000:ccf3  CALLF 0x0000:ffff
0000:ccf8  POP SI
0000:ccf9  MOV word ptr ES:[SI + 0x36],DI
0000:ccfd  MOV byte ptr ES:[DI + 0x17],0x2
0000:cd02  MOV EAX,dword ptr ES:[SI + 0x2]
0000:cd07  SUB EAX,0x1f0000
0000:cd0d  MOV dword ptr ES:[DI + 0x2],EAX
0000:cd12  MOV EAX,dword ptr ES:[SI + 0x6]
0000:cd17  SUB EAX,0x1c0000
0000:cd1d  MOV dword ptr ES:[DI + 0x6],EAX
0000:cd22  MOV DI,SI
0000:cd24  RET
0000:cd25  MOV SI,0x3384
0000:cd28  CALLF 0x0000:ffff
0000:cd2d  MOV word ptr ES:[DI + 0x18],0xcd40
0000:cd33  MOV word ptr ES:[DI + 0x2e],0x0
0000:cd39  MOV word ptr ES:[DI + 0x2c],0x0
0000:cd3f  RET
0000:cd40  CMP byte ptr [0x88ae],0x5
0000:cd45  JNZ 0x0000:cd71
0000:cd47  MOV AX,word ptr ES:[DI + 0x4]
0000:cd4b  SUB AX,word ptr [0x81c0]
0000:cd4f  ADD AX,0x10
0000:cd52  CMP AX,0x160
0000:cd55  JA 0x0000:cd6a
0000:cd57  MOV AX,word ptr ES:[DI + 0x8]
0000:cd5b  SUB AX,word ptr [0x81c4]
0000:cd5f  ADD AX,0x10
0000:cd62  CMP AX,0xd0
0000:cd65  JA 0x0000:cd6a
0000:cd67  CLC
0000:cd68  JMP 0x0000:cd71
0000:cd6a  STC
0000:cd6b  MOV word ptr ES:[DI + 0x18],0x0
0000:cd71  MOV DX,0xffbf
0000:cd74  MOV CX,0x5a
0000:cd77  MOV BX,0x41
0000:cd7a  MOV AX,0xffd3
0000:cd7d  CALLF 0x0000:ffff
0000:cd82  CALLF 0x0000:ffff
0000:cd87  RET
0000:cd88  MOV SI,0x32fa
0000:cd8b  CALLF 0x0000:ffff
0000:cd90  MOV word ptr ES:[DI + 0x18],0xcda3
0000:cd96  MOV word ptr ES:[DI + 0x2a],0x0
0000:cd9c  MOV word ptr ES:[DI + 0x2c],0x0
0000:cda2  RET
0000:cda3  CMP byte ptr ES:[DI + 0x2e],0x1
0000:cda8  JGE 0x0000:ce5b
0000:cdac  CMP word ptr [0x8806],0x0
0000:cdb1  JZ 0x0000:ce7b
0000:cdb5  MOV BX,word ptr ES:[DI + 0x2a]
0000:cdb9  CMP BX,word ptr [0x8808]
0000:cdbd  JLE 0x0000:cdc7
0000:cdbf  MOV word ptr ES:[DI + 0x2a],0x0
0000:cdc5  XOR BX,BX
0000:cdc7  SHL BX,0x2
0000:cdca  MOV AX,word ptr ES:[DI + 0x4]
0000:cdce  SUB AX,0xf
0000:cdd1  CMP word ptr [BX + 0x87de],AX
0000:cdd5  JLE 0x0000:cdfe
0000:cdd7  ADD AX,0x1e
0000:cdda  CMP word ptr [BX + 0x87de],AX
0000:cdde  JGE 0x0000:cdfe
0000:cde0  MOV AX,word ptr ES:[DI + 0x8]
0000:cde4  ADD AX,0x5
0000:cde7  CMP word ptr [BX + 0x87e0],AX
0000:cdeb  JGE 0x0000:cdfe
0000:cded  SUB AX,0x1e
0000:cdf0  CMP word ptr [BX + 0x87e0],AX
0000:cdf4  JLE 0x0000:cdfe
0000:cdf6  MOV word ptr [BX + 0x87de],0x0
0000:cdfc  JMP 0x0000:ce00
0000:cdfe  JMP 0x0000:ce49
0000:ce00  INC word ptr ES:[DI + 0x2c]
0000:ce04  PUSH DI
0000:ce05  MOV AX,0x4b70
0000:ce08  XOR DX,DX
0000:ce0a  CALLF 0x0000:ffff
0000:ce0f  POP SI
0000:ce10  MOV byte ptr ES:[DI + 0x17],0x2
0000:ce15  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ce1a  MOV dword ptr ES:[DI + 0x2],EAX
0000:ce1f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ce24  ADD EAX,0xa0000
0000:ce2a  MOV dword ptr ES:[DI + 0x6],EAX
0000:ce2f  MOV DI,SI
0000:ce31  MOV SI,0x3308
0000:ce34  CALLF 0x0000:ffff
0000:ce39  MOV word ptr [0x612e],0xd
0000:ce3f  CALLF 0x0000:ffff
0000:ce44  MOV byte ptr ES:[DI + 0x2e],0x1
0000:ce49  INC word ptr ES:[DI + 0x2a]
0000:ce4d  CMP word ptr ES:[DI + 0x2c],0x5
0000:ce52  JLE 0x0000:ce7b
0000:ce54  MOV byte ptr [0x88ae],0x2
0000:ce59  JMP 0x0000:ce7b
0000:ce5b  INC word ptr ES:[DI + 0x2f]
0000:ce5f  CMP word ptr ES:[DI + 0x2f],0x64
0000:ce64  JLE 0x0000:ce7b
0000:ce66  MOV word ptr ES:[DI + 0x2f],0x0
0000:ce6c  MOV byte ptr ES:[DI + 0x2e],0x0
0000:ce71  MOV SI,0x32fa
0000:ce74  CALLF 0x0000:ffff
0000:ce79  JMP 0x0000:ce7b
0000:ce7b  CALLF 0x0000:ffff
0000:ce80  RET
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX
0000:cebd  CALLF 0x0000:ffff
0000:cec2  JNZ 0x0000:cf07
0000:cec4  MOV DX,0x32
0000:cec7  NEG DX
0000:cec9  TEST byte ptr ES:[DI + 0x28],0xff
0000:cece  JS 0x0000:ced2
0000:ced0  NEG DX
0000:ced2  MOV AX,word ptr ES:[DI + 0x8]
0000:ced6  SUB AX,0x11
0000:ced9  MOV BX,word ptr ES:[DI + 0x4]
0000:cedd  ADD BX,DX
0000:cedf  CALLF 0x0000:ffff
0000:cee4  JNZ 0x0000:cf07
0000:cee6  MOV DX,0x32
0000:cee9  NEG DX
0000:ceeb  TEST byte ptr ES:[DI + 0x28],0xff
0000:cef0  JS 0x0000:cef5
0000:cef2  MOV DX,0x32
0000:cef5  MOV AX,word ptr ES:[DI + 0x8]
0000:cef9  SUB AX,0xc
0000:cefc  MOV BX,word ptr ES:[DI + 0x4]
0000:cf00  ADD BX,DX
0000:cf02  CALLF 0x0000:ffff
0000:cf07  JMP 0x0000:cf0b
0000:cf0b  JZ 0x0000:cf12
0000:cf0d  MOV byte ptr ES:[DI + 0x3e],0x1
0000:cf12  CMP word ptr ES:[DI + 0x8],0x1a4
0000:cf18  JGE 0x0000:cf1c
0000:cf1a  JMP 0x0000:cf29
0000:cf1c  NEG dword ptr ES:[DI + 0xe]
0000:cf21  MOV byte ptr ES:[DI + 0x34],0x1
0000:cf26  JMP 0x0000:cffe
0000:cf29  CMP byte ptr ES:[DI + 0x3e],0x0
0000:cf2e  JLE 0x0000:cf58
0000:cf30  NEG dword ptr ES:[DI + 0xa]
0000:cf35  NEG byte ptr ES:[DI + 0x28]
0000:cf39  NEG byte ptr ES:[DI + 0x29]

; ---- CDA3 count=224 ----
0000:cda3  CMP byte ptr ES:[DI + 0x2e],0x1
0000:cda8  JGE 0x0000:ce5b
0000:cdac  CMP word ptr [0x8806],0x0
0000:cdb1  JZ 0x0000:ce7b
0000:cdb5  MOV BX,word ptr ES:[DI + 0x2a]
0000:cdb9  CMP BX,word ptr [0x8808]
0000:cdbd  JLE 0x0000:cdc7
0000:cdbf  MOV word ptr ES:[DI + 0x2a],0x0
0000:cdc5  XOR BX,BX
0000:cdc7  SHL BX,0x2
0000:cdca  MOV AX,word ptr ES:[DI + 0x4]
0000:cdce  SUB AX,0xf
0000:cdd1  CMP word ptr [BX + 0x87de],AX
0000:cdd5  JLE 0x0000:cdfe
0000:cdd7  ADD AX,0x1e
0000:cdda  CMP word ptr [BX + 0x87de],AX
0000:cdde  JGE 0x0000:cdfe
0000:cde0  MOV AX,word ptr ES:[DI + 0x8]
0000:cde4  ADD AX,0x5
0000:cde7  CMP word ptr [BX + 0x87e0],AX
0000:cdeb  JGE 0x0000:cdfe
0000:cded  SUB AX,0x1e
0000:cdf0  CMP word ptr [BX + 0x87e0],AX
0000:cdf4  JLE 0x0000:cdfe
0000:cdf6  MOV word ptr [BX + 0x87de],0x0
0000:cdfc  JMP 0x0000:ce00
0000:cdfe  JMP 0x0000:ce49
0000:ce00  INC word ptr ES:[DI + 0x2c]
0000:ce04  PUSH DI
0000:ce05  MOV AX,0x4b70
0000:ce08  XOR DX,DX
0000:ce0a  CALLF 0x0000:ffff
0000:ce0f  POP SI
0000:ce10  MOV byte ptr ES:[DI + 0x17],0x2
0000:ce15  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ce1a  MOV dword ptr ES:[DI + 0x2],EAX
0000:ce1f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ce24  ADD EAX,0xa0000
0000:ce2a  MOV dword ptr ES:[DI + 0x6],EAX
0000:ce2f  MOV DI,SI
0000:ce31  MOV SI,0x3308
0000:ce34  CALLF 0x0000:ffff
0000:ce39  MOV word ptr [0x612e],0xd
0000:ce3f  CALLF 0x0000:ffff
0000:ce44  MOV byte ptr ES:[DI + 0x2e],0x1
0000:ce49  INC word ptr ES:[DI + 0x2a]
0000:ce4d  CMP word ptr ES:[DI + 0x2c],0x5
0000:ce52  JLE 0x0000:ce7b
0000:ce54  MOV byte ptr [0x88ae],0x2
0000:ce59  JMP 0x0000:ce7b
0000:ce5b  INC word ptr ES:[DI + 0x2f]
0000:ce5f  CMP word ptr ES:[DI + 0x2f],0x64
0000:ce64  JLE 0x0000:ce7b
0000:ce66  MOV word ptr ES:[DI + 0x2f],0x0
0000:ce6c  MOV byte ptr ES:[DI + 0x2e],0x0
0000:ce71  MOV SI,0x32fa
0000:ce74  CALLF 0x0000:ffff
0000:ce79  JMP 0x0000:ce7b
0000:ce7b  CALLF 0x0000:ffff
0000:ce80  RET
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX
0000:cebd  CALLF 0x0000:ffff
0000:cec2  JNZ 0x0000:cf07
0000:cec4  MOV DX,0x32
0000:cec7  NEG DX
0000:cec9  TEST byte ptr ES:[DI + 0x28],0xff
0000:cece  JS 0x0000:ced2
0000:ced0  NEG DX
0000:ced2  MOV AX,word ptr ES:[DI + 0x8]
0000:ced6  SUB AX,0x11
0000:ced9  MOV BX,word ptr ES:[DI + 0x4]
0000:cedd  ADD BX,DX
0000:cedf  CALLF 0x0000:ffff
0000:cee4  JNZ 0x0000:cf07
0000:cee6  MOV DX,0x32
0000:cee9  NEG DX
0000:ceeb  TEST byte ptr ES:[DI + 0x28],0xff
0000:cef0  JS 0x0000:cef5
0000:cef2  MOV DX,0x32
0000:cef5  MOV AX,word ptr ES:[DI + 0x8]
0000:cef9  SUB AX,0xc
0000:cefc  MOV BX,word ptr ES:[DI + 0x4]
0000:cf00  ADD BX,DX
0000:cf02  CALLF 0x0000:ffff
0000:cf07  JMP 0x0000:cf0b
0000:cf0b  JZ 0x0000:cf12
0000:cf0d  MOV byte ptr ES:[DI + 0x3e],0x1
0000:cf12  CMP word ptr ES:[DI + 0x8],0x1a4
0000:cf18  JGE 0x0000:cf1c
0000:cf1a  JMP 0x0000:cf29
0000:cf1c  NEG dword ptr ES:[DI + 0xe]
0000:cf21  MOV byte ptr ES:[DI + 0x34],0x1
0000:cf26  JMP 0x0000:cffe
0000:cf29  CMP byte ptr ES:[DI + 0x3e],0x0
0000:cf2e  JLE 0x0000:cf58
0000:cf30  NEG dword ptr ES:[DI + 0xa]
0000:cf35  NEG byte ptr ES:[DI + 0x28]
0000:cf39  NEG byte ptr ES:[DI + 0x29]
0000:cf3d  MOV byte ptr ES:[DI + 0x3e],0x0
0000:cf42  CMP word ptr ES:[DI + 0x12],0x385
0000:cf48  JZ 0x0000:cf52
0000:cf4a  MOV word ptr ES:[DI + 0x12],0x385
0000:cf50  JMP 0x0000:cf58
0000:cf52  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cf58  MOV EAX,dword ptr ES:[DI + 0xa]
0000:cf5d  ADD dword ptr ES:[DI + 0x2],EAX
0000:cf62  MOV EAX,dword ptr ES:[DI + 0xe]
0000:cf67  CMP EAX,0x0
0000:cf6b  JG 0x0000:cf98
0000:cf6d  ADD EAX,0xfa0
0000:cf73  CMP EAX,0xfffcb000
0000:cf79  JL 0x0000:cf8f
0000:cf7b  CMP EAX,0x35000
0000:cf81  JG 0x0000:cf86
0000:cf83  CLC
0000:cf84  JMP 0x0000:cf96
0000:cf86  MOV EAX,0x35000
0000:cf8c  STC
0000:cf8d  JMP 0x0000:cf96
0000:cf8f  MOV EAX,0xfffcb000
0000:cf95  STC
0000:cf96  JMP 0x0000:cfc1
0000:cf98  ADD EAX,0x1388
0000:cf9e  CMP EAX,0xfffcb000
0000:cfa4  JL 0x0000:cfba
0000:cfa6  CMP EAX,0x35000
0000:cfac  JG 0x0000:cfb1
0000:cfae  CLC
0000:cfaf  JMP 0x0000:cfc1
0000:cfb1  MOV EAX,0x35000
0000:cfb7  STC
0000:cfb8  JMP 0x0000:cfc1
0000:cfba  MOV EAX,0xfffcb000
0000:cfc0  STC
0000:cfc1  MOV dword ptr ES:[DI + 0xe],EAX
0000:cfc6  ADD dword ptr ES:[DI + 0x6],EAX
0000:cfcb  CMP word ptr ES:[DI + 0x38],0xdc
0000:cfd1  JLE 0x0000:cffe
0000:cfd3  MOV word ptr ES:[DI + 0x38],0x0
0000:cfd9  MOV byte ptr ES:[DI + 0x34],0x1
0000:cfde  JMP 0x0000:cffe
0000:cfe0  SUB dword ptr ES:[DI + 0x6],0xa0000
0000:cfe9  PUSH DI
0000:cfea  MOV DI,word ptr ES:[DI + 0x2a]
0000:cfee  MOV SI,0x3384
0000:cff1  CALLF 0x0000:ffff
0000:cff6  POP DI
0000:cff7  MOV byte ptr ES:[DI + 0x34],0x0
0000:cffc  JMP 0x0000:cffe
0000:cffe  PUSH DI
0000:cfff  MOV SI,DI
0000:d001  MOV DI,word ptr ES:[DI + 0x36]
0000:d005  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d00a  MOV AL,byte ptr ES:[SI + 0x28]
0000:d00e  MOV byte ptr ES:[DI + 0x28],AL
0000:d012  CMP AL,0x1
0000:d014  JNZ 0x0000:d01f
0000:d016  ADD EBX,0x1f0000
0000:d01d  JMP 0x0000:d026
0000:d01f  SUB EBX,0x1f0000
0000:d026  MOV dword ptr ES:[DI + 0x2],EBX
0000:d02b  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d030  SUB EAX,0x1c0000
0000:d036  MOV dword ptr ES:[DI + 0x6],EAX
0000:d03b  POP DI
0000:d03c  PUSH DI
0000:d03d  MOV SI,DI
0000:d03f  MOV DI,word ptr ES:[DI + 0x2a]
0000:d043  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d048  MOV AL,byte ptr ES:[SI + 0x28]
0000:d04c  MOV byte ptr ES:[DI + 0x28],AL
0000:d050  CMP AL,0x1
0000:d052  JNZ 0x0000:d05d
0000:d054  ADD EBX,0x50000
0000:d05b  JMP 0x0000:d064
0000:d05d  SUB EBX,0x50000
0000:d064  MOV dword ptr ES:[DI + 0x2],EBX
0000:d069  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d06e  SUB EAX,0xc0000
0000:d074  MOV dword ptr ES:[DI + 0x6],EAX
0000:d079  POP DI
0000:d07a  JMP 0x0000:d2c9
0000:d07d  CMP byte ptr [0x88ae],0x3
0000:d082  JGE 0x0000:d0b0
0000:d084  PUSH DI
0000:d085  MOV SI,DI
0000:d087  MOV DI,word ptr ES:[DI + 0x36]
0000:d08b  MOV word ptr ES:[DI + 0x18],0x0
0000:d091  POP DI
0000:d092  CMP word ptr ES:[DI + 0x12],0x385
0000:d098  JZ 0x0000:d0a2
0000:d09a  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d0a0  JMP 0x0000:d0a8
0000:d0a2  MOV word ptr ES:[DI + 0x12],0x384
0000:d0a8  MOV byte ptr [0x88ae],0x3
0000:d0ad  JMP 0x0000:d2c9
0000:d0b0  CMP byte ptr [0x88ae],0x4
0000:d0b5  JGE 0x0000:d20c
0000:d0b9  MOV AX,word ptr ES:[DI + 0x2c]
0000:d0bd  SUB word ptr ES:[DI + 0x8],AX
0000:d0c1  MOV SI,0x7974
0000:d0c4  MOV AX,word ptr ES:[DI + 0x2e]
0000:d0c8  ADD AX,0x20
0000:d0cb  AND AX,0x5ff
0000:d0ce  MOV word ptr ES:[DI + 0x2e],AX
0000:d0d2  ADD SI,AX
0000:d0d4  MOV AL,byte ptr [SI]

; ---- CE81 count=640 ----
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX
0000:cebd  CALLF 0x0000:ffff
0000:cec2  JNZ 0x0000:cf07
0000:cec4  MOV DX,0x32
0000:cec7  NEG DX
0000:cec9  TEST byte ptr ES:[DI + 0x28],0xff
0000:cece  JS 0x0000:ced2
0000:ced0  NEG DX
0000:ced2  MOV AX,word ptr ES:[DI + 0x8]
0000:ced6  SUB AX,0x11
0000:ced9  MOV BX,word ptr ES:[DI + 0x4]
0000:cedd  ADD BX,DX
0000:cedf  CALLF 0x0000:ffff
0000:cee4  JNZ 0x0000:cf07
0000:cee6  MOV DX,0x32
0000:cee9  NEG DX
0000:ceeb  TEST byte ptr ES:[DI + 0x28],0xff
0000:cef0  JS 0x0000:cef5
0000:cef2  MOV DX,0x32
0000:cef5  MOV AX,word ptr ES:[DI + 0x8]
0000:cef9  SUB AX,0xc
0000:cefc  MOV BX,word ptr ES:[DI + 0x4]
0000:cf00  ADD BX,DX
0000:cf02  CALLF 0x0000:ffff
0000:cf07  JMP 0x0000:cf0b
0000:cf0b  JZ 0x0000:cf12
0000:cf0d  MOV byte ptr ES:[DI + 0x3e],0x1
0000:cf12  CMP word ptr ES:[DI + 0x8],0x1a4
0000:cf18  JGE 0x0000:cf1c
0000:cf1a  JMP 0x0000:cf29
0000:cf1c  NEG dword ptr ES:[DI + 0xe]
0000:cf21  MOV byte ptr ES:[DI + 0x34],0x1
0000:cf26  JMP 0x0000:cffe
0000:cf29  CMP byte ptr ES:[DI + 0x3e],0x0
0000:cf2e  JLE 0x0000:cf58
0000:cf30  NEG dword ptr ES:[DI + 0xa]
0000:cf35  NEG byte ptr ES:[DI + 0x28]
0000:cf39  NEG byte ptr ES:[DI + 0x29]
0000:cf3d  MOV byte ptr ES:[DI + 0x3e],0x0
0000:cf42  CMP word ptr ES:[DI + 0x12],0x385
0000:cf48  JZ 0x0000:cf52
0000:cf4a  MOV word ptr ES:[DI + 0x12],0x385
0000:cf50  JMP 0x0000:cf58
0000:cf52  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cf58  MOV EAX,dword ptr ES:[DI + 0xa]
0000:cf5d  ADD dword ptr ES:[DI + 0x2],EAX
0000:cf62  MOV EAX,dword ptr ES:[DI + 0xe]
0000:cf67  CMP EAX,0x0
0000:cf6b  JG 0x0000:cf98
0000:cf6d  ADD EAX,0xfa0
0000:cf73  CMP EAX,0xfffcb000
0000:cf79  JL 0x0000:cf8f
0000:cf7b  CMP EAX,0x35000
0000:cf81  JG 0x0000:cf86
0000:cf83  CLC
0000:cf84  JMP 0x0000:cf96
0000:cf86  MOV EAX,0x35000
0000:cf8c  STC
0000:cf8d  JMP 0x0000:cf96
0000:cf8f  MOV EAX,0xfffcb000
0000:cf95  STC
0000:cf96  JMP 0x0000:cfc1
0000:cf98  ADD EAX,0x1388
0000:cf9e  CMP EAX,0xfffcb000
0000:cfa4  JL 0x0000:cfba
0000:cfa6  CMP EAX,0x35000
0000:cfac  JG 0x0000:cfb1
0000:cfae  CLC
0000:cfaf  JMP 0x0000:cfc1
0000:cfb1  MOV EAX,0x35000
0000:cfb7  STC
0000:cfb8  JMP 0x0000:cfc1
0000:cfba  MOV EAX,0xfffcb000
0000:cfc0  STC
0000:cfc1  MOV dword ptr ES:[DI + 0xe],EAX
0000:cfc6  ADD dword ptr ES:[DI + 0x6],EAX
0000:cfcb  CMP word ptr ES:[DI + 0x38],0xdc
0000:cfd1  JLE 0x0000:cffe
0000:cfd3  MOV word ptr ES:[DI + 0x38],0x0
0000:cfd9  MOV byte ptr ES:[DI + 0x34],0x1
0000:cfde  JMP 0x0000:cffe
0000:cfe0  SUB dword ptr ES:[DI + 0x6],0xa0000
0000:cfe9  PUSH DI
0000:cfea  MOV DI,word ptr ES:[DI + 0x2a]
0000:cfee  MOV SI,0x3384
0000:cff1  CALLF 0x0000:ffff
0000:cff6  POP DI
0000:cff7  MOV byte ptr ES:[DI + 0x34],0x0
0000:cffc  JMP 0x0000:cffe
0000:cffe  PUSH DI
0000:cfff  MOV SI,DI
0000:d001  MOV DI,word ptr ES:[DI + 0x36]
0000:d005  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d00a  MOV AL,byte ptr ES:[SI + 0x28]
0000:d00e  MOV byte ptr ES:[DI + 0x28],AL
0000:d012  CMP AL,0x1
0000:d014  JNZ 0x0000:d01f
0000:d016  ADD EBX,0x1f0000
0000:d01d  JMP 0x0000:d026
0000:d01f  SUB EBX,0x1f0000
0000:d026  MOV dword ptr ES:[DI + 0x2],EBX
0000:d02b  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d030  SUB EAX,0x1c0000
0000:d036  MOV dword ptr ES:[DI + 0x6],EAX
0000:d03b  POP DI
0000:d03c  PUSH DI
0000:d03d  MOV SI,DI
0000:d03f  MOV DI,word ptr ES:[DI + 0x2a]
0000:d043  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d048  MOV AL,byte ptr ES:[SI + 0x28]
0000:d04c  MOV byte ptr ES:[DI + 0x28],AL
0000:d050  CMP AL,0x1
0000:d052  JNZ 0x0000:d05d
0000:d054  ADD EBX,0x50000
0000:d05b  JMP 0x0000:d064
0000:d05d  SUB EBX,0x50000
0000:d064  MOV dword ptr ES:[DI + 0x2],EBX
0000:d069  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d06e  SUB EAX,0xc0000
0000:d074  MOV dword ptr ES:[DI + 0x6],EAX
0000:d079  POP DI
0000:d07a  JMP 0x0000:d2c9
0000:d07d  CMP byte ptr [0x88ae],0x3
0000:d082  JGE 0x0000:d0b0
0000:d084  PUSH DI
0000:d085  MOV SI,DI
0000:d087  MOV DI,word ptr ES:[DI + 0x36]
0000:d08b  MOV word ptr ES:[DI + 0x18],0x0
0000:d091  POP DI
0000:d092  CMP word ptr ES:[DI + 0x12],0x385
0000:d098  JZ 0x0000:d0a2
0000:d09a  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d0a0  JMP 0x0000:d0a8
0000:d0a2  MOV word ptr ES:[DI + 0x12],0x384
0000:d0a8  MOV byte ptr [0x88ae],0x3
0000:d0ad  JMP 0x0000:d2c9
0000:d0b0  CMP byte ptr [0x88ae],0x4
0000:d0b5  JGE 0x0000:d20c
0000:d0b9  MOV AX,word ptr ES:[DI + 0x2c]
0000:d0bd  SUB word ptr ES:[DI + 0x8],AX
0000:d0c1  MOV SI,0x7974
0000:d0c4  MOV AX,word ptr ES:[DI + 0x2e]
0000:d0c8  ADD AX,0x20
0000:d0cb  AND AX,0x5ff
0000:d0ce  MOV word ptr ES:[DI + 0x2e],AX
0000:d0d2  ADD SI,AX
0000:d0d4  MOV AL,byte ptr [SI]
0000:d0d6  SAR AL,0x5
0000:d0d9  CBW
0000:d0da  MOV word ptr ES:[DI + 0x2c],AX
0000:d0de  ADD word ptr ES:[DI + 0x8],AX
0000:d0e2  PUSH DI
0000:d0e3  MOV SI,DI
0000:d0e5  MOV DI,word ptr ES:[DI + 0x2a]
0000:d0e9  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d0ee  MOV AL,byte ptr ES:[SI + 0x28]
0000:d0f2  MOV byte ptr ES:[DI + 0x28],AL
0000:d0f6  CMP AL,0x1
0000:d0f8  JNZ 0x0000:d103
0000:d0fa  ADD EBX,0x50000
0000:d101  JMP 0x0000:d10a
0000:d103  SUB EBX,0x50000
0000:d10a  MOV dword ptr ES:[DI + 0x2],EBX
0000:d10f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d114  SUB EAX,0xc0000
0000:d11a  MOV dword ptr ES:[DI + 0x6],EAX
0000:d11f  POP DI
0000:d120  INC word ptr ES:[DI + 0x38]
0000:d124  CMP word ptr ES:[DI + 0x38],0x19
0000:d129  JLE 0x0000:d2c9
0000:d12d  MOV word ptr ES:[DI + 0x38],0x0
0000:d133  PUSH DI
0000:d134  MOV AX,0x4b70
0000:d137  XOR DX,DX
0000:d139  CALLF 0x0000:ffff
0000:d13e  POP SI
0000:d13f  MOV byte ptr ES:[DI + 0x17],0x2
0000:d144  PUSH SI
0000:d145  MOV SI,0x646c
0000:d148  ADD SI,word ptr [0x6468]
0000:d14c  INC word ptr [0x6468]
0000:d150  AND word ptr [0x6468],0xff
0000:d156  MOV AL,byte ptr [SI]
0000:d158  POP SI
0000:d159  SHR AL,0x2
0000:d15c  CBW
0000:d15d  SUB AX,0x20
0000:d160  MOV BX,word ptr ES:[SI + 0x4]
0000:d164  ADD BX,AX
0000:d166  MOV word ptr ES:[DI + 0x4],BX
0000:d16a  PUSH SI
0000:d16b  MOV SI,0x646c
0000:d16e  ADD SI,word ptr [0x6468]
0000:d172  INC word ptr [0x6468]
0000:d176  AND word ptr [0x6468],0xff
0000:d17c  MOV AL,byte ptr [SI]
0000:d17e  POP SI
0000:d17f  SHR AL,0x3
0000:d182  CBW
0000:d183  MOV BX,word ptr ES:[SI + 0x8]
0000:d187  ADD BX,AX
0000:d189  SUB BX,0x1b
0000:d18c  MOV word ptr ES:[DI + 0x8],BX
0000:d190  MOV DI,SI
0000:d192  PUSH DI
0000:d193  MOV AX,0x4b70
0000:d196  XOR DX,DX
0000:d198  CALLF 0x0000:ffff
0000:d19d  POP SI
0000:d19e  MOV byte ptr ES:[DI + 0x17],0x2
0000:d1a3  PUSH SI
0000:d1a4  MOV SI,0x646c
0000:d1a7  ADD SI,word ptr [0x6468]
0000:d1ab  INC word ptr [0x6468]
0000:d1af  AND word ptr [0x6468],0xff
0000:d1b5  MOV AL,byte ptr [SI]
0000:d1b7  POP SI
0000:d1b8  SHR AL,0x2
0000:d1bb  CBW
0000:d1bc  SUB AX,0x20
0000:d1bf  MOV BX,word ptr ES:[SI + 0x4]
0000:d1c3  ADD BX,AX
0000:d1c5  MOV word ptr ES:[DI + 0x4],BX
0000:d1c9  PUSH SI
0000:d1ca  MOV SI,0x646c
0000:d1cd  ADD SI,word ptr [0x6468]
0000:d1d1  INC word ptr [0x6468]
0000:d1d5  AND word ptr [0x6468],0xff
0000:d1db  MOV AL,byte ptr [SI]
0000:d1dd  POP SI
0000:d1de  SHR AL,0x3
0000:d1e1  CBW
0000:d1e2  MOV BX,word ptr ES:[SI + 0x8]
0000:d1e6  ADD BX,AX
0000:d1e8  SUB BX,0x1b
0000:d1eb  MOV word ptr ES:[DI + 0x8],BX
0000:d1ef  MOV DI,SI
0000:d1f1  INC word ptr ES:[DI + 0x44]
0000:d1f5  CMP word ptr ES:[DI + 0x44],0xf
0000:d1fa  JLE 0x0000:d2c9
0000:d1fe  MOV dword ptr ES:[DI + 0xe],0xffff0000
0000:d207  MOV byte ptr [0x88ae],0x4
0000:d20c  CMP byte ptr [0x88ae],0x5
0000:d211  JGE 0x0000:d2a1
0000:d215  INC word ptr ES:[DI + 0x38]
0000:d219  CMP word ptr ES:[DI + 0x38],0x28
0000:d21e  JLE 0x0000:d2c9
0000:d222  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d227  SUB dword ptr ES:[DI + 0xe],0x1200
0000:d230  ADD dword ptr ES:[DI + 0x6],EAX
0000:d235  PUSH DI
0000:d236  MOV SI,DI
0000:d238  MOV DI,word ptr ES:[DI + 0x2a]
0000:d23c  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d241  MOV AL,byte ptr ES:[SI + 0x28]
0000:d245  MOV byte ptr ES:[DI + 0x28],AL
0000:d249  CMP AL,0x1
0000:d24b  JNZ 0x0000:d256
0000:d24d  ADD EBX,0x50000
0000:d254  JMP 0x0000:d25d
0000:d256  SUB EBX,0x50000
0000:d25d  MOV dword ptr ES:[DI + 0x2],EBX
0000:d262  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d267  SUB EAX,0xc0000
0000:d26d  MOV dword ptr ES:[DI + 0x6],EAX
0000:d272  POP DI
0000:d273  MOV AX,word ptr ES:[DI + 0x4]
0000:d277  SUB AX,word ptr [0x81c0]
0000:d27b  ADD AX,0x10
0000:d27e  CMP AX,0x160
0000:d281  JA 0x0000:d296
0000:d283  MOV AX,word ptr ES:[DI + 0x8]
0000:d287  SUB AX,word ptr [0x81c4]
0000:d28b  ADD AX,0x10
0000:d28e  CMP AX,0xd0
0000:d291  JA 0x0000:d296
0000:d293  CLC
0000:d294  JMP 0x0000:d29f
0000:d296  STC
0000:d297  MOV word ptr ES:[DI + 0x18],0x0
0000:d29d  JMP 0x0000:d2a1
0000:d29f  JMP 0x0000:d2c9
0000:d2a1  MOV byte ptr [0x88ae],0x5
0000:d2a6  PUSH DI
0000:d2a7  MOV AX,0x487f
0000:d2aa  XOR DX,DX
0000:d2ac  CALLF 0x0000:ffff
0000:d2b1  POP SI
0000:d2b2  MOV byte ptr ES:[DI + 0x17],0x1
0000:d2b7  MOV BX,word ptr ES:[SI + 0x4]
0000:d2bb  MOV word ptr ES:[DI + 0x4],BX
0000:d2bf  MOV BX,word ptr ES:[SI + 0x8]
0000:d2c3  MOV word ptr ES:[DI + 0x8],BX
0000:d2c7  MOV DI,SI
0000:d2c9  RET
0000:d2d0  MOV AX,0xd2f6
0000:d2d3  XOR DX,DX
0000:d2d5  CALLF 0x0000:ffff
0000:d2da  MOV byte ptr ES:[DI + 0x17],0x1
0000:d2df  MOV AX,[0x81c0]
0000:d2e2  MOV BX,word ptr [0x81c4]
0000:d2e6  ADD AX,0x190
0000:d2e9  ADD BX,0x14
0000:d2ec  MOV word ptr ES:[DI + 0x4],AX
0000:d2f0  MOV word ptr ES:[DI + 0x8],BX
0000:d2f4  POP DI
0000:d2f5  RETF
0000:d2f6  MOV byte ptr ES:[DI + 0x17],0x1
0000:d2fb  MOV word ptr ES:[DI + 0x18],0xd63d
0000:d301  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d307  MOV byte ptr ES:[DI + 0x28],0xff
0000:d30c  MOV byte ptr ES:[DI + 0x29],0xff
0000:d311  MOV word ptr ES:[DI + 0x38],0x0
0000:d317  MOV word ptr ES:[DI + 0x46],0x0
0000:d31d  MOV word ptr ES:[DI + 0x44],0x0
0000:d323  MOV byte ptr ES:[DI + 0x34],0x0
0000:d328  MOV dword ptr ES:[DI + 0xa],0xfffed000
0000:d331  MOV byte ptr ES:[DI + 0x40],0xff
0000:d336  MOV word ptr ES:[DI + 0x42],0x14
0000:d33c  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d341  PUSH DI
0000:d342  MOV AX,0xd3e1
0000:d345  XOR DX,DX
0000:d347  CALLF 0x0000:ffff
0000:d34c  POP SI
0000:d34d  MOV word ptr ES:[SI + 0x2a],DI
0000:d351  MOV byte ptr ES:[DI + 0x17],0x1
0000:d356  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d35b  SUB EAX,0x40000
0000:d361  MOV dword ptr ES:[DI + 0x2],EAX
0000:d366  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d36b  SUB EAX,0x370000
0000:d371  MOV dword ptr ES:[DI + 0x6],EAX
0000:d376  MOV DI,SI
0000:d378  PUSH DI
0000:d379  MOV AX,0xd53f
0000:d37c  XOR DX,DX
0000:d37e  CALLF 0x0000:ffff
0000:d383  POP SI
0000:d384  MOV word ptr ES:[SI + 0x36],DI
0000:d388  MOV byte ptr ES:[DI + 0x17],0x2
0000:d38d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d392  SUB EAX,0x1f0000
0000:d398  MOV dword ptr ES:[DI + 0x2],EAX
0000:d39d  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d3a2  SUB EAX,0x1d0000
0000:d3a8  MOV dword ptr ES:[DI + 0x6],EAX
0000:d3ad  MOV DI,SI
0000:d3af  PUSH DI
0000:d3b0  MOV AX,0xd498
0000:d3b3  XOR DX,DX
0000:d3b5  CALLF 0x0000:ffff
0000:d3ba  POP SI
0000:d3bb  MOV word ptr ES:[SI + 0x48],DI
0000:d3bf  MOV byte ptr ES:[DI + 0x17],0x1
0000:d3c4  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d3c9  MOV dword ptr ES:[DI + 0x2],EAX
0000:d3ce  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d3d3  ADD EAX,0x280000
0000:d3d9  MOV dword ptr ES:[DI + 0x6],EAX
0000:d3de  MOV DI,SI
0000:d3e0  RET
0000:d3e1  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d3e7  MOV word ptr ES:[DI + 0x18],0xd3ee
0000:d3ed  RET
0000:d3ee  CMP byte ptr [0x88ae],0x5
0000:d3f3  JNZ 0x0000:d41f
0000:d3f5  MOV AX,word ptr ES:[DI + 0x4]
0000:d3f9  SUB AX,word ptr [0x81c0]
0000:d3fd  ADD AX,0x60
0000:d400  CMP AX,0x200
0000:d403  JA 0x0000:d418
0000:d405  MOV AX,word ptr ES:[DI + 0x8]
0000:d409  SUB AX,word ptr [0x81c4]
0000:d40d  ADD AX,0x60
0000:d410  CMP AX,0x170
0000:d413  JA 0x0000:d418
0000:d415  CLC
0000:d416  JMP 0x0000:d41f
0000:d418  STC
0000:d419  MOV word ptr ES:[DI + 0x18],0x0
0000:d41f  RET
0000:d420  MOV SI,0x342e
0000:d423  CALLF 0x0000:ffff
0000:d428  MOV word ptr ES:[DI + 0x18],0xd438
0000:d42e  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d437  RET
0000:d438  CMP byte ptr [0x88ae],0x4
0000:d43d  JNZ 0x0000:d481
0000:d43f  MOV AX,word ptr ES:[DI + 0x4]
0000:d443  SUB AX,word ptr [0x81c0]
0000:d447  ADD AX,0x10
0000:d44a  CMP AX,0x160
0000:d44d  JA 0x0000:d462
0000:d44f  MOV AX,word ptr ES:[DI + 0x8]
0000:d453  SUB AX,word ptr [0x81c4]
0000:d457  ADD AX,0x10
0000:d45a  CMP AX,0xd0
0000:d45d  JA 0x0000:d462
0000:d45f  CLC
0000:d460  JMP 0x0000:d469
0000:d462  STC
0000:d463  MOV word ptr ES:[DI + 0x18],0x0
0000:d469  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d46e  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d477  MOV dword ptr ES:[DI + 0xe],EAX
0000:d47c  ADD dword ptr ES:[DI + 0x6],EAX
0000:d481  MOV DX,0xffec
0000:d484  MOV CX,0x1e
0000:d487  MOV BX,0x14
0000:d48a  MOV AX,0xfff1
0000:d48d  CALLF 0x0000:ffff
0000:d492  CALLF 0x0000:ffff
0000:d497  RET
0000:d498  MOV word ptr ES:[DI + 0x12],0x38a
0000:d49e  MOV word ptr ES:[DI + 0x18],0xd4d9
0000:d4a4  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d4ad  PUSH DI
0000:d4ae  MOV AX,0xd420
0000:d4b1  XOR DX,DX
0000:d4b3  CALLF 0x0000:ffff
0000:d4b8  POP SI
0000:d4b9  MOV word ptr ES:[SI + 0x2a],DI
0000:d4bd  MOV byte ptr ES:[DI + 0x17],0x1
0000:d4c2  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d4c7  MOV dword ptr ES:[DI + 0x2],EAX
0000:d4cc  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4d1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4d6  MOV DI,SI
0000:d4d8  RET
0000:d4d9  PUSH DI
0000:d4da  MOV SI,DI
0000:d4dc  MOV DI,word ptr ES:[DI + 0x2a]
0000:d4e0  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d4e5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d4ea  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4ef  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4f4  POP DI
0000:d4f5  CMP byte ptr [0x88ae],0x4
0000:d4fa  JNZ 0x0000:d53e
0000:d4fc  MOV AX,word ptr ES:[DI + 0x4]
0000:d500  SUB AX,word ptr [0x81c0]
0000:d504  ADD AX,0x10
0000:d507  CMP AX,0x160
0000:d50a  JA 0x0000:d51f
0000:d50c  MOV AX,word ptr ES:[DI + 0x8]
0000:d510  SUB AX,word ptr [0x81c4]
0000:d514  ADD AX,0x10
0000:d517  CMP AX,0xd0
0000:d51a  JA 0x0000:d51f
0000:d51c  CLC
0000:d51d  JMP 0x0000:d526
0000:d51f  STC
0000:d520  MOV word ptr ES:[DI + 0x18],0x0
0000:d526  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d52b  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d534  MOV dword ptr ES:[DI + 0xe],EAX
0000:d539  ADD dword ptr ES:[DI + 0x6],EAX
0000:d53e  RET
0000:d53f  MOV SI,0x32fa
0000:d542  CALLF 0x0000:ffff
0000:d547  MOV word ptr ES:[DI + 0x18],0xd55a
0000:d54d  MOV word ptr ES:[DI + 0x2a],0x0
0000:d553  MOV word ptr ES:[DI + 0x2c],0x0
0000:d559  RET
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2
0000:d5cc  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d5d1  MOV dword ptr ES:[DI + 0x2],EAX
0000:d5d6  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d5db  ADD EAX,0xa0000
0000:d5e1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d5e6  MOV DI,SI
0000:d5e8  MOV SI,0x3308
0000:d5eb  CALLF 0x0000:ffff
0000:d5f0  MOV word ptr [0x612e],0xd
0000:d5f6  CALLF 0x0000:ffff
0000:d5fb  MOV byte ptr ES:[DI + 0x2e],0x1
0000:d600  INC word ptr ES:[DI + 0x2a]
0000:d604  CMP word ptr ES:[DI + 0x2c],0x3
0000:d609  JLE 0x0000:d637
0000:d60b  INC byte ptr [0x88ae]
0000:d60f  MOV word ptr ES:[DI + 0x2c],0x0
0000:d615  JMP 0x0000:d637
0000:d617  INC word ptr ES:[DI + 0x2f]
0000:d61b  CMP word ptr ES:[DI + 0x2f],0x64
0000:d620  JLE 0x0000:d637
0000:d622  MOV word ptr ES:[DI + 0x2f],0x0
0000:d628  MOV byte ptr ES:[DI + 0x2e],0x0
0000:d62d  MOV SI,0x32fa
0000:d630  CALLF 0x0000:ffff
0000:d635  JMP 0x0000:d637
0000:d637  CALLF 0x0000:ffff
0000:d63c  RET
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28
0000:d78e  JMP 0x0000:d7e7
0000:d790  MOV EBX,dword ptr ES:[DI + 0xa]

; ---- CD40 count=224 ----
0000:cd40  CMP byte ptr [0x88ae],0x5
0000:cd45  JNZ 0x0000:cd71
0000:cd47  MOV AX,word ptr ES:[DI + 0x4]
0000:cd4b  SUB AX,word ptr [0x81c0]
0000:cd4f  ADD AX,0x10
0000:cd52  CMP AX,0x160
0000:cd55  JA 0x0000:cd6a
0000:cd57  MOV AX,word ptr ES:[DI + 0x8]
0000:cd5b  SUB AX,word ptr [0x81c4]
0000:cd5f  ADD AX,0x10
0000:cd62  CMP AX,0xd0
0000:cd65  JA 0x0000:cd6a
0000:cd67  CLC
0000:cd68  JMP 0x0000:cd71
0000:cd6a  STC
0000:cd6b  MOV word ptr ES:[DI + 0x18],0x0
0000:cd71  MOV DX,0xffbf
0000:cd74  MOV CX,0x5a
0000:cd77  MOV BX,0x41
0000:cd7a  MOV AX,0xffd3
0000:cd7d  CALLF 0x0000:ffff
0000:cd82  CALLF 0x0000:ffff
0000:cd87  RET
0000:cd88  MOV SI,0x32fa
0000:cd8b  CALLF 0x0000:ffff
0000:cd90  MOV word ptr ES:[DI + 0x18],0xcda3
0000:cd96  MOV word ptr ES:[DI + 0x2a],0x0
0000:cd9c  MOV word ptr ES:[DI + 0x2c],0x0
0000:cda2  RET
0000:cda3  CMP byte ptr ES:[DI + 0x2e],0x1
0000:cda8  JGE 0x0000:ce5b
0000:cdac  CMP word ptr [0x8806],0x0
0000:cdb1  JZ 0x0000:ce7b
0000:cdb5  MOV BX,word ptr ES:[DI + 0x2a]
0000:cdb9  CMP BX,word ptr [0x8808]
0000:cdbd  JLE 0x0000:cdc7
0000:cdbf  MOV word ptr ES:[DI + 0x2a],0x0
0000:cdc5  XOR BX,BX
0000:cdc7  SHL BX,0x2
0000:cdca  MOV AX,word ptr ES:[DI + 0x4]
0000:cdce  SUB AX,0xf
0000:cdd1  CMP word ptr [BX + 0x87de],AX
0000:cdd5  JLE 0x0000:cdfe
0000:cdd7  ADD AX,0x1e
0000:cdda  CMP word ptr [BX + 0x87de],AX
0000:cdde  JGE 0x0000:cdfe
0000:cde0  MOV AX,word ptr ES:[DI + 0x8]
0000:cde4  ADD AX,0x5
0000:cde7  CMP word ptr [BX + 0x87e0],AX
0000:cdeb  JGE 0x0000:cdfe
0000:cded  SUB AX,0x1e
0000:cdf0  CMP word ptr [BX + 0x87e0],AX
0000:cdf4  JLE 0x0000:cdfe
0000:cdf6  MOV word ptr [BX + 0x87de],0x0
0000:cdfc  JMP 0x0000:ce00
0000:cdfe  JMP 0x0000:ce49
0000:ce00  INC word ptr ES:[DI + 0x2c]
0000:ce04  PUSH DI
0000:ce05  MOV AX,0x4b70
0000:ce08  XOR DX,DX
0000:ce0a  CALLF 0x0000:ffff
0000:ce0f  POP SI
0000:ce10  MOV byte ptr ES:[DI + 0x17],0x2
0000:ce15  MOV EAX,dword ptr ES:[SI + 0x2]
0000:ce1a  MOV dword ptr ES:[DI + 0x2],EAX
0000:ce1f  MOV EAX,dword ptr ES:[SI + 0x6]
0000:ce24  ADD EAX,0xa0000
0000:ce2a  MOV dword ptr ES:[DI + 0x6],EAX
0000:ce2f  MOV DI,SI
0000:ce31  MOV SI,0x3308
0000:ce34  CALLF 0x0000:ffff
0000:ce39  MOV word ptr [0x612e],0xd
0000:ce3f  CALLF 0x0000:ffff
0000:ce44  MOV byte ptr ES:[DI + 0x2e],0x1
0000:ce49  INC word ptr ES:[DI + 0x2a]
0000:ce4d  CMP word ptr ES:[DI + 0x2c],0x5
0000:ce52  JLE 0x0000:ce7b
0000:ce54  MOV byte ptr [0x88ae],0x2
0000:ce59  JMP 0x0000:ce7b
0000:ce5b  INC word ptr ES:[DI + 0x2f]
0000:ce5f  CMP word ptr ES:[DI + 0x2f],0x64
0000:ce64  JLE 0x0000:ce7b
0000:ce66  MOV word ptr ES:[DI + 0x2f],0x0
0000:ce6c  MOV byte ptr ES:[DI + 0x2e],0x0
0000:ce71  MOV SI,0x32fa
0000:ce74  CALLF 0x0000:ffff
0000:ce79  JMP 0x0000:ce7b
0000:ce7b  CALLF 0x0000:ffff
0000:ce80  RET
0000:ce81  CMP byte ptr [0x88ae],0x2
0000:ce86  JGE 0x0000:d07d
0000:ce8a  CMP byte ptr ES:[DI + 0x34],0x1
0000:ce8f  JGE 0x0000:cfe0
0000:ce93  MOV DX,0x32
0000:ce96  MOV CX,0x64
0000:ce99  MOV BX,0xffce
0000:ce9c  MOV AX,0xffce
0000:ce9f  CALLF 0x0000:ffff
0000:cea4  MOV DX,0x32
0000:cea7  NEG DX
0000:cea9  TEST byte ptr ES:[DI + 0x28],0xff
0000:ceae  JS 0x0000:ceb2
0000:ceb0  NEG DX
0000:ceb2  MOV AX,word ptr ES:[DI + 0x8]
0000:ceb6  DEC AX
0000:ceb7  MOV BX,word ptr ES:[DI + 0x4]
0000:cebb  ADD BX,DX
0000:cebd  CALLF 0x0000:ffff
0000:cec2  JNZ 0x0000:cf07
0000:cec4  MOV DX,0x32
0000:cec7  NEG DX
0000:cec9  TEST byte ptr ES:[DI + 0x28],0xff
0000:cece  JS 0x0000:ced2
0000:ced0  NEG DX
0000:ced2  MOV AX,word ptr ES:[DI + 0x8]
0000:ced6  SUB AX,0x11
0000:ced9  MOV BX,word ptr ES:[DI + 0x4]
0000:cedd  ADD BX,DX
0000:cedf  CALLF 0x0000:ffff
0000:cee4  JNZ 0x0000:cf07
0000:cee6  MOV DX,0x32
0000:cee9  NEG DX
0000:ceeb  TEST byte ptr ES:[DI + 0x28],0xff
0000:cef0  JS 0x0000:cef5
0000:cef2  MOV DX,0x32
0000:cef5  MOV AX,word ptr ES:[DI + 0x8]
0000:cef9  SUB AX,0xc
0000:cefc  MOV BX,word ptr ES:[DI + 0x4]
0000:cf00  ADD BX,DX
0000:cf02  CALLF 0x0000:ffff
0000:cf07  JMP 0x0000:cf0b
0000:cf0b  JZ 0x0000:cf12
0000:cf0d  MOV byte ptr ES:[DI + 0x3e],0x1
0000:cf12  CMP word ptr ES:[DI + 0x8],0x1a4
0000:cf18  JGE 0x0000:cf1c
0000:cf1a  JMP 0x0000:cf29
0000:cf1c  NEG dword ptr ES:[DI + 0xe]
0000:cf21  MOV byte ptr ES:[DI + 0x34],0x1
0000:cf26  JMP 0x0000:cffe
0000:cf29  CMP byte ptr ES:[DI + 0x3e],0x0
0000:cf2e  JLE 0x0000:cf58
0000:cf30  NEG dword ptr ES:[DI + 0xa]
0000:cf35  NEG byte ptr ES:[DI + 0x28]
0000:cf39  NEG byte ptr ES:[DI + 0x29]
0000:cf3d  MOV byte ptr ES:[DI + 0x3e],0x0
0000:cf42  CMP word ptr ES:[DI + 0x12],0x385
0000:cf48  JZ 0x0000:cf52
0000:cf4a  MOV word ptr ES:[DI + 0x12],0x385
0000:cf50  JMP 0x0000:cf58
0000:cf52  MOV word ptr ES:[DI + 0x12],0x3b7
0000:cf58  MOV EAX,dword ptr ES:[DI + 0xa]
0000:cf5d  ADD dword ptr ES:[DI + 0x2],EAX
0000:cf62  MOV EAX,dword ptr ES:[DI + 0xe]
0000:cf67  CMP EAX,0x0
0000:cf6b  JG 0x0000:cf98
0000:cf6d  ADD EAX,0xfa0
0000:cf73  CMP EAX,0xfffcb000
0000:cf79  JL 0x0000:cf8f
0000:cf7b  CMP EAX,0x35000
0000:cf81  JG 0x0000:cf86
0000:cf83  CLC
0000:cf84  JMP 0x0000:cf96
0000:cf86  MOV EAX,0x35000
0000:cf8c  STC
0000:cf8d  JMP 0x0000:cf96
0000:cf8f  MOV EAX,0xfffcb000
0000:cf95  STC
0000:cf96  JMP 0x0000:cfc1
0000:cf98  ADD EAX,0x1388
0000:cf9e  CMP EAX,0xfffcb000
0000:cfa4  JL 0x0000:cfba
0000:cfa6  CMP EAX,0x35000
0000:cfac  JG 0x0000:cfb1
0000:cfae  CLC
0000:cfaf  JMP 0x0000:cfc1
0000:cfb1  MOV EAX,0x35000
0000:cfb7  STC
0000:cfb8  JMP 0x0000:cfc1
0000:cfba  MOV EAX,0xfffcb000
0000:cfc0  STC
0000:cfc1  MOV dword ptr ES:[DI + 0xe],EAX
0000:cfc6  ADD dword ptr ES:[DI + 0x6],EAX
0000:cfcb  CMP word ptr ES:[DI + 0x38],0xdc
0000:cfd1  JLE 0x0000:cffe
0000:cfd3  MOV word ptr ES:[DI + 0x38],0x0
0000:cfd9  MOV byte ptr ES:[DI + 0x34],0x1
0000:cfde  JMP 0x0000:cffe
0000:cfe0  SUB dword ptr ES:[DI + 0x6],0xa0000
0000:cfe9  PUSH DI
0000:cfea  MOV DI,word ptr ES:[DI + 0x2a]
0000:cfee  MOV SI,0x3384
0000:cff1  CALLF 0x0000:ffff
0000:cff6  POP DI
0000:cff7  MOV byte ptr ES:[DI + 0x34],0x0
0000:cffc  JMP 0x0000:cffe
0000:cffe  PUSH DI
0000:cfff  MOV SI,DI
0000:d001  MOV DI,word ptr ES:[DI + 0x36]
0000:d005  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d00a  MOV AL,byte ptr ES:[SI + 0x28]
0000:d00e  MOV byte ptr ES:[DI + 0x28],AL
0000:d012  CMP AL,0x1
0000:d014  JNZ 0x0000:d01f
0000:d016  ADD EBX,0x1f0000
0000:d01d  JMP 0x0000:d026
0000:d01f  SUB EBX,0x1f0000
0000:d026  MOV dword ptr ES:[DI + 0x2],EBX
0000:d02b  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d030  SUB EAX,0x1c0000
0000:d036  MOV dword ptr ES:[DI + 0x6],EAX
0000:d03b  POP DI
0000:d03c  PUSH DI
0000:d03d  MOV SI,DI
0000:d03f  MOV DI,word ptr ES:[DI + 0x2a]
0000:d043  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d048  MOV AL,byte ptr ES:[SI + 0x28]
0000:d04c  MOV byte ptr ES:[DI + 0x28],AL
0000:d050  CMP AL,0x1
0000:d052  JNZ 0x0000:d05d
0000:d054  ADD EBX,0x50000
0000:d05b  JMP 0x0000:d064
0000:d05d  SUB EBX,0x50000
0000:d064  MOV dword ptr ES:[DI + 0x2],EBX
0000:d069  MOV EAX,dword ptr ES:[SI + 0x6]

; ---- D2F6 count=192 ----
0000:d2f6  MOV byte ptr ES:[DI + 0x17],0x1
0000:d2fb  MOV word ptr ES:[DI + 0x18],0xd63d
0000:d301  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d307  MOV byte ptr ES:[DI + 0x28],0xff
0000:d30c  MOV byte ptr ES:[DI + 0x29],0xff
0000:d311  MOV word ptr ES:[DI + 0x38],0x0
0000:d317  MOV word ptr ES:[DI + 0x46],0x0
0000:d31d  MOV word ptr ES:[DI + 0x44],0x0
0000:d323  MOV byte ptr ES:[DI + 0x34],0x0
0000:d328  MOV dword ptr ES:[DI + 0xa],0xfffed000
0000:d331  MOV byte ptr ES:[DI + 0x40],0xff
0000:d336  MOV word ptr ES:[DI + 0x42],0x14
0000:d33c  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d341  PUSH DI
0000:d342  MOV AX,0xd3e1
0000:d345  XOR DX,DX
0000:d347  CALLF 0x0000:ffff
0000:d34c  POP SI
0000:d34d  MOV word ptr ES:[SI + 0x2a],DI
0000:d351  MOV byte ptr ES:[DI + 0x17],0x1
0000:d356  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d35b  SUB EAX,0x40000
0000:d361  MOV dword ptr ES:[DI + 0x2],EAX
0000:d366  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d36b  SUB EAX,0x370000
0000:d371  MOV dword ptr ES:[DI + 0x6],EAX
0000:d376  MOV DI,SI
0000:d378  PUSH DI
0000:d379  MOV AX,0xd53f
0000:d37c  XOR DX,DX
0000:d37e  CALLF 0x0000:ffff
0000:d383  POP SI
0000:d384  MOV word ptr ES:[SI + 0x36],DI
0000:d388  MOV byte ptr ES:[DI + 0x17],0x2
0000:d38d  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d392  SUB EAX,0x1f0000
0000:d398  MOV dword ptr ES:[DI + 0x2],EAX
0000:d39d  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d3a2  SUB EAX,0x1d0000
0000:d3a8  MOV dword ptr ES:[DI + 0x6],EAX
0000:d3ad  MOV DI,SI
0000:d3af  PUSH DI
0000:d3b0  MOV AX,0xd498
0000:d3b3  XOR DX,DX
0000:d3b5  CALLF 0x0000:ffff
0000:d3ba  POP SI
0000:d3bb  MOV word ptr ES:[SI + 0x48],DI
0000:d3bf  MOV byte ptr ES:[DI + 0x17],0x1
0000:d3c4  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d3c9  MOV dword ptr ES:[DI + 0x2],EAX
0000:d3ce  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d3d3  ADD EAX,0x280000
0000:d3d9  MOV dword ptr ES:[DI + 0x6],EAX
0000:d3de  MOV DI,SI
0000:d3e0  RET
0000:d3e1  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d3e7  MOV word ptr ES:[DI + 0x18],0xd3ee
0000:d3ed  RET
0000:d3ee  CMP byte ptr [0x88ae],0x5
0000:d3f3  JNZ 0x0000:d41f
0000:d3f5  MOV AX,word ptr ES:[DI + 0x4]
0000:d3f9  SUB AX,word ptr [0x81c0]
0000:d3fd  ADD AX,0x60
0000:d400  CMP AX,0x200
0000:d403  JA 0x0000:d418
0000:d405  MOV AX,word ptr ES:[DI + 0x8]
0000:d409  SUB AX,word ptr [0x81c4]
0000:d40d  ADD AX,0x60
0000:d410  CMP AX,0x170
0000:d413  JA 0x0000:d418
0000:d415  CLC
0000:d416  JMP 0x0000:d41f
0000:d418  STC
0000:d419  MOV word ptr ES:[DI + 0x18],0x0
0000:d41f  RET
0000:d420  MOV SI,0x342e
0000:d423  CALLF 0x0000:ffff
0000:d428  MOV word ptr ES:[DI + 0x18],0xd438
0000:d42e  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d437  RET
0000:d438  CMP byte ptr [0x88ae],0x4
0000:d43d  JNZ 0x0000:d481
0000:d43f  MOV AX,word ptr ES:[DI + 0x4]
0000:d443  SUB AX,word ptr [0x81c0]
0000:d447  ADD AX,0x10
0000:d44a  CMP AX,0x160
0000:d44d  JA 0x0000:d462
0000:d44f  MOV AX,word ptr ES:[DI + 0x8]
0000:d453  SUB AX,word ptr [0x81c4]
0000:d457  ADD AX,0x10
0000:d45a  CMP AX,0xd0
0000:d45d  JA 0x0000:d462
0000:d45f  CLC
0000:d460  JMP 0x0000:d469
0000:d462  STC
0000:d463  MOV word ptr ES:[DI + 0x18],0x0
0000:d469  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d46e  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d477  MOV dword ptr ES:[DI + 0xe],EAX
0000:d47c  ADD dword ptr ES:[DI + 0x6],EAX
0000:d481  MOV DX,0xffec
0000:d484  MOV CX,0x1e
0000:d487  MOV BX,0x14
0000:d48a  MOV AX,0xfff1
0000:d48d  CALLF 0x0000:ffff
0000:d492  CALLF 0x0000:ffff
0000:d497  RET
0000:d498  MOV word ptr ES:[DI + 0x12],0x38a
0000:d49e  MOV word ptr ES:[DI + 0x18],0xd4d9
0000:d4a4  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d4ad  PUSH DI
0000:d4ae  MOV AX,0xd420
0000:d4b1  XOR DX,DX
0000:d4b3  CALLF 0x0000:ffff
0000:d4b8  POP SI
0000:d4b9  MOV word ptr ES:[SI + 0x2a],DI
0000:d4bd  MOV byte ptr ES:[DI + 0x17],0x1
0000:d4c2  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d4c7  MOV dword ptr ES:[DI + 0x2],EAX
0000:d4cc  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4d1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4d6  MOV DI,SI
0000:d4d8  RET
0000:d4d9  PUSH DI
0000:d4da  MOV SI,DI
0000:d4dc  MOV DI,word ptr ES:[DI + 0x2a]
0000:d4e0  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d4e5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d4ea  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4ef  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4f4  POP DI
0000:d4f5  CMP byte ptr [0x88ae],0x4
0000:d4fa  JNZ 0x0000:d53e
0000:d4fc  MOV AX,word ptr ES:[DI + 0x4]
0000:d500  SUB AX,word ptr [0x81c0]
0000:d504  ADD AX,0x10
0000:d507  CMP AX,0x160
0000:d50a  JA 0x0000:d51f
0000:d50c  MOV AX,word ptr ES:[DI + 0x8]
0000:d510  SUB AX,word ptr [0x81c4]
0000:d514  ADD AX,0x10
0000:d517  CMP AX,0xd0
0000:d51a  JA 0x0000:d51f
0000:d51c  CLC
0000:d51d  JMP 0x0000:d526
0000:d51f  STC
0000:d520  MOV word ptr ES:[DI + 0x18],0x0
0000:d526  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d52b  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d534  MOV dword ptr ES:[DI + 0xe],EAX
0000:d539  ADD dword ptr ES:[DI + 0x6],EAX
0000:d53e  RET
0000:d53f  MOV SI,0x32fa
0000:d542  CALLF 0x0000:ffff
0000:d547  MOV word ptr ES:[DI + 0x18],0xd55a
0000:d54d  MOV word ptr ES:[DI + 0x2a],0x0
0000:d553  MOV word ptr ES:[DI + 0x2c],0x0
0000:d559  RET
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2

; ---- D55A count=224 ----
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2
0000:d5cc  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d5d1  MOV dword ptr ES:[DI + 0x2],EAX
0000:d5d6  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d5db  ADD EAX,0xa0000
0000:d5e1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d5e6  MOV DI,SI
0000:d5e8  MOV SI,0x3308
0000:d5eb  CALLF 0x0000:ffff
0000:d5f0  MOV word ptr [0x612e],0xd
0000:d5f6  CALLF 0x0000:ffff
0000:d5fb  MOV byte ptr ES:[DI + 0x2e],0x1
0000:d600  INC word ptr ES:[DI + 0x2a]
0000:d604  CMP word ptr ES:[DI + 0x2c],0x3
0000:d609  JLE 0x0000:d637
0000:d60b  INC byte ptr [0x88ae]
0000:d60f  MOV word ptr ES:[DI + 0x2c],0x0
0000:d615  JMP 0x0000:d637
0000:d617  INC word ptr ES:[DI + 0x2f]
0000:d61b  CMP word ptr ES:[DI + 0x2f],0x64
0000:d620  JLE 0x0000:d637
0000:d622  MOV word ptr ES:[DI + 0x2f],0x0
0000:d628  MOV byte ptr ES:[DI + 0x2e],0x0
0000:d62d  MOV SI,0x32fa
0000:d630  CALLF 0x0000:ffff
0000:d635  JMP 0x0000:d637
0000:d637  CALLF 0x0000:ffff
0000:d63c  RET
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28
0000:d78e  JMP 0x0000:d7e7
0000:d790  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d795  MOV AL,byte ptr ES:[DI + 0x29]
0000:d799  CBW
0000:d79a  CWDE
0000:d79c  SHL EAX,0xc
0000:d7a0  ADD EBX,EAX
0000:d7a3  CMP EBX,0xfffed000
0000:d7aa  JL 0x0000:d7c1
0000:d7ac  CMP EBX,0x13000
0000:d7b3  JG 0x0000:d7b8
0000:d7b5  CLC
0000:d7b6  JMP 0x0000:d7c8
0000:d7b8  MOV EBX,0x13000
0000:d7be  STC
0000:d7bf  JMP 0x0000:d7c8
0000:d7c1  MOV EBX,0xfffed000
0000:d7c7  STC
0000:d7c8  MOV dword ptr ES:[DI + 0xa],EBX
0000:d7cd  ADD dword ptr ES:[DI + 0x2],EBX
0000:d7d2  DEC word ptr ES:[DI + 0x42]
0000:d7d6  JGE 0x0000:d7e7
0000:d7d8  NEG byte ptr ES:[DI + 0x40]
0000:d7dc  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d7e1  MOV word ptr ES:[DI + 0x42],0x14
0000:d7e7  JMP 0x0000:d96f
0000:d7ea  CMP byte ptr ES:[DI + 0x40],0x0
0000:d7ef  JGE 0x0000:d89a
0000:d7f3  CMP word ptr ES:[DI + 0x42],0x14
0000:d7f8  JNZ 0x0000:d7fa
0000:d7fa  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d7ff  MOV AL,byte ptr ES:[DI + 0x29]
0000:d803  CBW
0000:d804  CWDE
0000:d806  SHL EAX,0xd
0000:d80a  SUB EBX,EAX
0000:d80d  CMP EBX,0xfffea000
0000:d814  JL 0x0000:d82b
0000:d816  CMP EBX,0x16000
0000:d81d  JG 0x0000:d822
0000:d81f  CLC
0000:d820  JMP 0x0000:d832
0000:d822  MOV EBX,0x16000
0000:d828  STC
0000:d829  JMP 0x0000:d832
0000:d82b  MOV EBX,0xfffea000
0000:d831  STC
0000:d832  MOV dword ptr ES:[DI + 0xa],EBX
0000:d837  ADD dword ptr ES:[DI + 0x2],EBX
0000:d83c  DEC word ptr ES:[DI + 0x42]
0000:d840  JGE 0x0000:d8f1
0000:d844  NEG byte ptr ES:[DI + 0x29]
0000:d848  NEG byte ptr ES:[DI + 0x28]
0000:d84c  NEG byte ptr ES:[DI + 0x40]
0000:d850  CMP word ptr ES:[DI + 0x12],0x384
0000:d856  JZ 0x0000:d860
0000:d858  MOV word ptr ES:[DI + 0x12],0x384
0000:d85e  JMP 0x0000:d866
0000:d860  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d866  PUSH DI
0000:d867  MOV DI,word ptr ES:[DI + 0x2a]
0000:d86b  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d871  JZ 0x0000:d87b
0000:d873  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d879  JMP 0x0000:d881
0000:d87b  MOV word ptr ES:[DI + 0x12],0x389
0000:d881  POP DI
0000:d882  MOV AL,byte ptr ES:[DI + 0x29]

; ---- D63D count=768 ----
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28
0000:d78e  JMP 0x0000:d7e7
0000:d790  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d795  MOV AL,byte ptr ES:[DI + 0x29]
0000:d799  CBW
0000:d79a  CWDE
0000:d79c  SHL EAX,0xc
0000:d7a0  ADD EBX,EAX
0000:d7a3  CMP EBX,0xfffed000
0000:d7aa  JL 0x0000:d7c1
0000:d7ac  CMP EBX,0x13000
0000:d7b3  JG 0x0000:d7b8
0000:d7b5  CLC
0000:d7b6  JMP 0x0000:d7c8
0000:d7b8  MOV EBX,0x13000
0000:d7be  STC
0000:d7bf  JMP 0x0000:d7c8
0000:d7c1  MOV EBX,0xfffed000
0000:d7c7  STC
0000:d7c8  MOV dword ptr ES:[DI + 0xa],EBX
0000:d7cd  ADD dword ptr ES:[DI + 0x2],EBX
0000:d7d2  DEC word ptr ES:[DI + 0x42]
0000:d7d6  JGE 0x0000:d7e7
0000:d7d8  NEG byte ptr ES:[DI + 0x40]
0000:d7dc  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d7e1  MOV word ptr ES:[DI + 0x42],0x14
0000:d7e7  JMP 0x0000:d96f
0000:d7ea  CMP byte ptr ES:[DI + 0x40],0x0
0000:d7ef  JGE 0x0000:d89a
0000:d7f3  CMP word ptr ES:[DI + 0x42],0x14
0000:d7f8  JNZ 0x0000:d7fa
0000:d7fa  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d7ff  MOV AL,byte ptr ES:[DI + 0x29]
0000:d803  CBW
0000:d804  CWDE
0000:d806  SHL EAX,0xd
0000:d80a  SUB EBX,EAX
0000:d80d  CMP EBX,0xfffea000
0000:d814  JL 0x0000:d82b
0000:d816  CMP EBX,0x16000
0000:d81d  JG 0x0000:d822
0000:d81f  CLC
0000:d820  JMP 0x0000:d832
0000:d822  MOV EBX,0x16000
0000:d828  STC
0000:d829  JMP 0x0000:d832
0000:d82b  MOV EBX,0xfffea000
0000:d831  STC
0000:d832  MOV dword ptr ES:[DI + 0xa],EBX
0000:d837  ADD dword ptr ES:[DI + 0x2],EBX
0000:d83c  DEC word ptr ES:[DI + 0x42]
0000:d840  JGE 0x0000:d8f1
0000:d844  NEG byte ptr ES:[DI + 0x29]
0000:d848  NEG byte ptr ES:[DI + 0x28]
0000:d84c  NEG byte ptr ES:[DI + 0x40]
0000:d850  CMP word ptr ES:[DI + 0x12],0x384
0000:d856  JZ 0x0000:d860
0000:d858  MOV word ptr ES:[DI + 0x12],0x384
0000:d85e  JMP 0x0000:d866
0000:d860  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d866  PUSH DI
0000:d867  MOV DI,word ptr ES:[DI + 0x2a]
0000:d86b  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d871  JZ 0x0000:d87b
0000:d873  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d879  JMP 0x0000:d881
0000:d87b  MOV word ptr ES:[DI + 0x12],0x389
0000:d881  POP DI
0000:d882  MOV AL,byte ptr ES:[DI + 0x29]
0000:d886  CBW
0000:d887  CWDE
0000:d889  SHL EAX,0x9
0000:d88d  MOV dword ptr ES:[DI + 0xa],EAX
0000:d892  MOV word ptr ES:[DI + 0x42],0x14
0000:d898  JMP 0x0000:d8f1
0000:d89a  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d89f  MOV AL,byte ptr ES:[DI + 0x29]
0000:d8a3  CBW
0000:d8a4  CWDE
0000:d8a6  SHL EAX,0xd
0000:d8aa  ADD EBX,EAX
0000:d8ad  CMP EBX,0xfffea000
0000:d8b4  JL 0x0000:d8cb
0000:d8b6  CMP EBX,0x16000
0000:d8bd  JG 0x0000:d8c2
0000:d8bf  CLC
0000:d8c0  JMP 0x0000:d8d2
0000:d8c2  MOV EBX,0x16000
0000:d8c8  STC
0000:d8c9  JMP 0x0000:d8d2
0000:d8cb  MOV EBX,0xfffea000
0000:d8d1  STC
0000:d8d2  MOV dword ptr ES:[DI + 0xa],EBX
0000:d8d7  ADD dword ptr ES:[DI + 0x2],EBX
0000:d8dc  DEC word ptr ES:[DI + 0x42]
0000:d8e0  JGE 0x0000:d8f1
0000:d8e2  NEG byte ptr ES:[DI + 0x40]
0000:d8e6  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d8eb  MOV word ptr ES:[DI + 0x42],0x14
0000:d8f1  JMP 0x0000:d96f
0000:d8f3  MOV EAX,dword ptr ES:[DI + 0xa]
0000:d8f8  ADD dword ptr ES:[DI + 0x2],EAX
0000:d8fd  CMP byte ptr [0x88ae],0x2
0000:d902  JGE 0x0000:d906
0000:d904  JMP 0x0000:d96f
0000:d906  INC word ptr ES:[DI + 0x38]
0000:d90a  CMP word ptr ES:[DI + 0x38],0x3c
0000:d90f  JLE 0x0000:d96f
0000:d911  MOV SI,0x646c
0000:d914  ADD SI,word ptr [0x6468]
0000:d918  INC word ptr [0x6468]
0000:d91c  AND word ptr [0x6468],0xff
0000:d922  MOV AL,byte ptr [SI]
0000:d924  SHR AL,0x4
0000:d927  CBW
0000:d928  MOV word ptr ES:[DI + 0x38],AX
0000:d92c  PUSH DI
0000:d92d  MOV AX,0xdc09
0000:d930  XOR DX,DX
0000:d932  CALLF 0x0000:ffff
0000:d937  POP SI
0000:d938  MOV byte ptr ES:[DI + 0x17],0x2
0000:d93d  MOV AL,byte ptr ES:[SI + 0x29]
0000:d941  MOV byte ptr ES:[DI + 0x29],AL
0000:d945  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d94a  CMP byte ptr ES:[SI + 0x28],0x1
0000:d94f  JNZ 0x0000:d953
0000:d951  JMP 0x0000:d953
0000:d953  MOV dword ptr ES:[DI + 0x2],EAX
0000:d958  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d95d  SUB EAX,0x500000
0000:d963  MOV dword ptr ES:[DI + 0x6],EAX
0000:d968  MOV DI,SI
0000:d96a  MOV byte ptr ES:[DI + 0x34],0x0
0000:d96f  PUSH DI
0000:d970  MOV SI,DI
0000:d972  MOV DI,word ptr ES:[DI + 0x36]
0000:d976  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d97b  MOV AL,byte ptr ES:[SI + 0x28]
0000:d97f  MOV byte ptr ES:[DI + 0x28],AL
0000:d983  CMP AL,0x1
0000:d985  JNZ 0x0000:d990
0000:d987  ADD EBX,0x1f0000
0000:d98e  JMP 0x0000:d997
0000:d990  SUB EBX,0x1f0000
0000:d997  MOV dword ptr ES:[DI + 0x2],EBX
0000:d99c  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d9a1  SUB EAX,0x1d0000
0000:d9a7  MOV dword ptr ES:[DI + 0x6],EAX
0000:d9ac  POP DI
0000:d9ad  PUSH DI
0000:d9ae  MOV SI,DI
0000:d9b0  MOV DI,word ptr ES:[DI + 0x2a]
0000:d9b4  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d9b9  MOV AL,byte ptr ES:[SI + 0x28]
0000:d9bd  MOV byte ptr ES:[DI + 0x28],AL
0000:d9c1  CMP AL,0x1
0000:d9c3  JNZ 0x0000:d9ce
0000:d9c5  ADD EBX,0x40000
0000:d9cc  JMP 0x0000:d9d5
0000:d9ce  SUB EBX,0x40000
0000:d9d5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d9da  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d9df  SUB EAX,0x370000
0000:d9e5  MOV dword ptr ES:[DI + 0x6],EAX
0000:d9ea  POP DI
0000:d9eb  PUSH DI
0000:d9ec  MOV SI,DI
0000:d9ee  MOV DI,word ptr ES:[DI + 0x48]
0000:d9f2  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d9f7  CMP byte ptr ES:[SI + 0x28],0x1
0000:d9fc  JNZ 0x0000:da00
0000:d9fe  JMP 0x0000:da00
0000:da00  MOV dword ptr ES:[DI + 0x2],EAX
0000:da05  MOV EAX,dword ptr ES:[SI + 0x6]
0000:da0a  ADD EAX,0x280000
0000:da10  MOV dword ptr ES:[DI + 0x6],EAX
0000:da15  MOV SI,0x7974
0000:da18  MOV AX,word ptr ES:[DI + 0x2e]
0000:da1c  ADD AX,0x14
0000:da1f  AND AX,0x7ff
0000:da22  MOV word ptr ES:[DI + 0x2e],AX
0000:da26  ADD SI,AX
0000:da28  MOV AL,byte ptr [SI]
0000:da2a  SAR AL,0x3
0000:da2d  CBW
0000:da2e  ADD word ptr ES:[DI + 0x8],AX
0000:da32  MOV AX,word ptr ES:[DI + 0x8]
0000:da36  MOV word ptr ES:[DI + 0x8],AX
0000:da3a  POP DI
0000:da3b  JMP 0x0000:dc08
0000:da3e  CMP byte ptr [0x88ae],0x4
0000:da43  JGE 0x0000:da71
0000:da45  PUSH DI
0000:da46  MOV SI,DI
0000:da48  MOV DI,word ptr ES:[DI + 0x36]
0000:da4c  MOV word ptr ES:[DI + 0x18],0x0
0000:da52  POP DI
0000:da53  CMP word ptr ES:[DI + 0x12],0x384
0000:da59  JZ 0x0000:da63
0000:da5b  MOV word ptr ES:[DI + 0x12],0x3b6
0000:da61  JMP 0x0000:da69
0000:da63  MOV word ptr ES:[DI + 0x12],0x384
0000:da69  MOV byte ptr [0x88ae],0x4
0000:da6e  JMP 0x0000:dc08
0000:da71  CMP byte ptr [0x88ae],0x5
0000:da76  JGE 0x0000:db8f
0000:da7a  MOV AX,word ptr ES:[DI + 0x2c]
0000:da7e  SUB word ptr ES:[DI + 0x8],AX
0000:da82  MOV SI,0x7974
0000:da85  MOV AX,word ptr ES:[DI + 0x2e]
0000:da89  ADD AX,0x20
0000:da8c  AND AX,0x5ff
0000:da8f  MOV word ptr ES:[DI + 0x2e],AX
0000:da93  ADD SI,AX
0000:da95  MOV AL,byte ptr [SI]
0000:da97  SAR AL,0x5
0000:da9a  CBW
0000:da9b  MOV word ptr ES:[DI + 0x2c],AX
0000:da9f  ADD word ptr ES:[DI + 0x8],AX
0000:daa3  INC word ptr ES:[DI + 0x38]
0000:daa7  CMP word ptr ES:[DI + 0x38],0x19
0000:daac  JLE 0x0000:dc08
0000:dab0  MOV word ptr ES:[DI + 0x38],0x0
0000:dab6  PUSH DI
0000:dab7  MOV AX,0x4b70
0000:daba  XOR DX,DX
0000:dabc  CALLF 0x0000:ffff
0000:dac1  POP SI
0000:dac2  MOV byte ptr ES:[DI + 0x17],0x2
0000:dac7  PUSH SI
0000:dac8  MOV SI,0x646c
0000:dacb  ADD SI,word ptr [0x6468]
0000:dacf  INC word ptr [0x6468]
0000:dad3  AND word ptr [0x6468],0xff
0000:dad9  MOV AL,byte ptr [SI]
0000:dadb  POP SI
0000:dadc  SHR AL,0x2
0000:dadf  CBW
0000:dae0  SUB AX,0x20
0000:dae3  MOV BX,word ptr ES:[SI + 0x4]
0000:dae7  ADD BX,AX
0000:dae9  MOV word ptr ES:[DI + 0x4],BX
0000:daed  PUSH SI
0000:daee  MOV SI,0x646c
0000:daf1  ADD SI,word ptr [0x6468]
0000:daf5  INC word ptr [0x6468]
0000:daf9  AND word ptr [0x6468],0xff
0000:daff  MOV AL,byte ptr [SI]
0000:db01  POP SI
0000:db02  SHR AL,0x3
0000:db05  CBW
0000:db06  MOV BX,word ptr ES:[SI + 0x8]
0000:db0a  ADD BX,AX
0000:db0c  SUB BX,0x1e
0000:db0f  MOV word ptr ES:[DI + 0x8],BX
0000:db13  MOV DI,SI
0000:db15  PUSH DI
0000:db16  MOV AX,0x4b70
0000:db19  XOR DX,DX
0000:db1b  CALLF 0x0000:ffff
0000:db20  POP SI
0000:db21  MOV byte ptr ES:[DI + 0x17],0x2
0000:db26  PUSH SI
0000:db27  MOV SI,0x646c
0000:db2a  ADD SI,word ptr [0x6468]
0000:db2e  INC word ptr [0x6468]
0000:db32  AND word ptr [0x6468],0xff
0000:db38  MOV AL,byte ptr [SI]
0000:db3a  POP SI
0000:db3b  SHR AL,0x2
0000:db3e  CBW
0000:db3f  SUB AX,0x20
0000:db42  MOV BX,word ptr ES:[SI + 0x4]
0000:db46  ADD BX,AX
0000:db48  MOV word ptr ES:[DI + 0x4],BX
0000:db4c  PUSH SI
0000:db4d  MOV SI,0x646c
0000:db50  ADD SI,word ptr [0x6468]
0000:db54  INC word ptr [0x6468]
0000:db58  AND word ptr [0x6468],0xff
0000:db5e  MOV AL,byte ptr [SI]
0000:db60  POP SI
0000:db61  SHR AL,0x3
0000:db64  CBW
0000:db65  MOV BX,word ptr ES:[SI + 0x8]
0000:db69  ADD BX,AX
0000:db6b  SUB BX,0x1e
0000:db6e  MOV word ptr ES:[DI + 0x8],BX
0000:db72  MOV DI,SI
0000:db74  INC word ptr ES:[DI + 0x44]
0000:db78  CMP word ptr ES:[DI + 0x44],0xf
0000:db7d  JLE 0x0000:dc08
0000:db81  MOV dword ptr ES:[DI + 0xe],0xffff0000
0000:db8a  MOV byte ptr [0x88ae],0x5
0000:db8f  CMP byte ptr [0x88ae],0x6
0000:db94  JGE 0x0000:dbe2
0000:db96  INC word ptr ES:[DI + 0x38]
0000:db9a  CMP word ptr ES:[DI + 0x38],0x28
0000:db9f  JLE 0x0000:dc08
0000:dba1  MOV EAX,dword ptr ES:[DI + 0xe]
0000:dba6  SUB dword ptr ES:[DI + 0xe],0x1200
0000:dbaf  ADD dword ptr ES:[DI + 0x6],EAX
0000:dbb4  MOV AX,word ptr ES:[DI + 0x4]
0000:dbb8  SUB AX,word ptr [0x81c0]
0000:dbbc  ADD AX,0x10
0000:dbbf  CMP AX,0x160
0000:dbc2  JA 0x0000:dbd7
0000:dbc4  MOV AX,word ptr ES:[DI + 0x8]
0000:dbc8  SUB AX,word ptr [0x81c4]
0000:dbcc  ADD AX,0x10
0000:dbcf  CMP AX,0xd0
0000:dbd2  JA 0x0000:dbd7
0000:dbd4  CLC
0000:dbd5  JMP 0x0000:dbe0
0000:dbd7  STC
0000:dbd8  MOV word ptr ES:[DI + 0x18],0x0
0000:dbde  JMP 0x0000:dbe2
0000:dbe0  JMP 0x0000:dc08
0000:dbe2  MOV byte ptr [0x88ae],0x6
0000:dbe7  PUSH DI
0000:dbe8  MOV AX,0x487f
0000:dbeb  XOR DX,DX
0000:dbed  CALLF 0x0000:ffff
0000:dbf2  POP SI
0000:dbf3  MOV byte ptr ES:[DI + 0x17],0x1
0000:dbf8  MOV word ptr ES:[DI + 0x4],0x203
0000:dbfe  MOV BX,word ptr ES:[SI + 0x8]
0000:dc02  MOV word ptr ES:[DI + 0x8],BX
0000:dc06  MOV DI,SI
0000:dc08  RET
0000:dc09  MOV word ptr ES:[DI + 0x12],0x387
0000:dc0f  MOV word ptr ES:[DI + 0x18],0xdd22
0000:dc15  MOV SI,0x646c
0000:dc18  ADD SI,word ptr [0x6468]
0000:dc1c  INC word ptr [0x6468]
0000:dc20  AND word ptr [0x6468],0xff
0000:dc26  MOV AL,byte ptr [SI]
0000:dc28  CBW
0000:dc29  CWDE
0000:dc2b  SHL EAX,0x8
0000:dc2f  NEG EAX
0000:dc32  ADD EAX,0xfffec000
0000:dc38  MOV dword ptr ES:[DI + 0xe],EAX
0000:dc3d  MOV SI,0x646c
0000:dc40  ADD SI,word ptr [0x6468]
0000:dc44  INC word ptr [0x6468]
0000:dc48  AND word ptr [0x6468],0xff
0000:dc4e  MOV AL,byte ptr [SI]
0000:dc50  CBW
0000:dc51  CWDE
0000:dc53  SHL EAX,0xa
0000:dc57  ADD EAX,0xffffa000
0000:dc5d  MOV dword ptr ES:[DI + 0xa],EAX
0000:dc62  CMP EAX,0x0
0000:dc66  JL 0x0000:dc74
0000:dc68  MOV byte ptr ES:[DI + 0x28],0x1
0000:dc6d  MOV byte ptr ES:[DI + 0x29],0x1
0000:dc72  JMP 0x0000:dc7e
0000:dc74  MOV byte ptr ES:[DI + 0x28],0xff
0000:dc79  MOV byte ptr ES:[DI + 0x29],0xff
0000:dc7e  PUSH DI
0000:dc7f  MOV AX,0xdcac
0000:dc82  XOR DX,DX
0000:dc84  CALLF 0x0000:ffff
0000:dc89  POP SI
0000:dc8a  MOV byte ptr ES:[DI + 0x17],0x2
0000:dc8f  MOV EAX,dword ptr ES:[SI + 0x2]
0000:dc94  MOV dword ptr ES:[DI + 0x2],EAX
0000:dc99  MOV EAX,dword ptr ES:[SI + 0x6]
0000:dc9e  ADD EAX,0xf0000
0000:dca4  MOV dword ptr ES:[DI + 0x6],EAX
0000:dca9  MOV DI,SI
0000:dcab  RET
0000:dcac  MOV word ptr ES:[DI + 0x12],0x388
0000:dcb2  MOV word ptr ES:[DI + 0x18],0xdd22
0000:dcb8  MOV SI,0x646c
0000:dcbb  ADD SI,word ptr [0x6468]
0000:dcbf  INC word ptr [0x6468]
0000:dcc3  AND word ptr [0x6468],0xff
0000:dcc9  MOV AL,byte ptr [SI]
0000:dccb  CBW
0000:dccc  CWDE
0000:dcce  SHL EAX,0x8
0000:dcd2  NEG EAX
0000:dcd5  ADD EAX,0xfffec000
0000:dcdb  MOV dword ptr ES:[DI + 0xe],EAX
0000:dce0  MOV SI,0x646c
0000:dce3  ADD SI,word ptr [0x6468]
0000:dce7  INC word ptr [0x6468]
0000:dceb  AND word ptr [0x6468],0xff
0000:dcf1  MOV AL,byte ptr [SI]
0000:dcf3  CBW
0000:dcf4  CWDE
0000:dcf6  SHL EAX,0xa
0000:dcfa  ADD EAX,0xffffa000
0000:dd00  MOV dword ptr ES:[DI + 0xa],EAX
0000:dd05  CMP EAX,0x0
0000:dd09  JL 0x0000:dd17
0000:dd0b  MOV byte ptr ES:[DI + 0x28],0x1
0000:dd10  MOV byte ptr ES:[DI + 0x29],0x1
0000:dd15  JMP 0x0000:dd21
0000:dd17  MOV byte ptr ES:[DI + 0x28],0xff
0000:dd1c  MOV byte ptr ES:[DI + 0x29],0xff
0000:dd21  RET
0000:dd22  MOV AX,word ptr ES:[DI + 0x4]
0000:dd26  SUB AX,word ptr [0x81c0]
0000:dd2a  ADD AX,0x40
0000:dd2d  CMP AX,0x1c0
0000:dd30  JA 0x0000:dd45
0000:dd32  MOV AX,word ptr ES:[DI + 0x8]
0000:dd36  SUB AX,word ptr [0x81c4]
0000:dd3a  ADD AX,0x40
0000:dd3d  CMP AX,0x130
0000:dd40  JA 0x0000:dd45
0000:dd42  CLC
0000:dd43  JMP 0x0000:dd4c
0000:dd45  STC
0000:dd46  MOV word ptr ES:[DI + 0x18],0x0
0000:dd4c  MOV CX,0x0
0000:dd4f  MOV DX,0x0
0000:dd52  CALLF 0x0000:ffff
0000:dd57  JNC 0x0000:dd5b
0000:dd59  JMP 0x0000:ddb6
0000:dd5b  CMP byte ptr ES:[DI + 0x29],0x0
0000:dd60  JG 0x0000:dd8a
0000:dd62  MOV AX,word ptr ES:[DI + 0x8]
0000:dd66  MOV BX,word ptr ES:[DI + 0x4]
0000:dd6a  SUB BX,0x0
0000:dd6d  CALLF 0x0000:ffff
0000:dd72  TEST DL,0x70
0000:dd75  JNZ 0x0000:ddb2
0000:dd77  MOV AX,word ptr ES:[DI + 0x8]
0000:dd7b  SUB AX,0x7
0000:dd7e  CALLF 0x0000:ffff
0000:dd83  TEST DL,0x70
0000:dd86  JNZ 0x0000:ddb2
0000:dd88  JMP 0x0000:ddb4
0000:dd8a  MOV AX,word ptr ES:[DI + 0x8]
0000:dd8e  MOV BX,word ptr ES:[DI + 0x4]
0000:dd92  ADD BX,0x0
0000:dd95  CALLF 0x0000:ffff
0000:dd9a  TEST DL,0x70
0000:dd9d  JNZ 0x0000:ddb2
0000:dd9f  MOV AX,word ptr ES:[DI + 0x8]
0000:dda3  SUB AX,0x7
0000:dda6  CALLF 0x0000:ffff
0000:ddab  TEST DL,0x70
0000:ddae  JNZ 0x0000:ddb2
0000:ddb0  JMP 0x0000:ddb4
0000:ddb2  JMP 0x0000:ddb6
0000:ddb4  JMP 0x0000:ddbd
0000:ddb6  MOV word ptr ES:[DI + 0x18],0x0
0000:ddbc  RET
0000:ddbd  MOV EBX,dword ptr ES:[DI + 0xe]
0000:ddc2  ADD dword ptr ES:[DI + 0x6],EBX
0000:ddc7  ADD EBX,0x1770
0000:ddce  CMP EBX,0xfffd0000
0000:ddd5  JL 0x0000:ddec
0000:ddd7  CMP EBX,0x35000
0000:ddde  JG 0x0000:dde3
0000:dde0  CLC
0000:dde1  JMP 0x0000:ddf3
0000:dde3  MOV EBX,0x35000
0000:dde9  STC
0000:ddea  JMP 0x0000:ddf3
0000:ddec  MOV EBX,0xfffd0000
0000:ddf2  STC
0000:ddf3  MOV dword ptr ES:[DI + 0xe],EBX
0000:ddf8  MOV EBX,dword ptr ES:[DI + 0xa]
0000:ddfd  ADD dword ptr ES:[DI + 0x2],EBX
0000:de02  MOV AL,byte ptr ES:[DI + 0x29]
0000:de06  CBW
0000:de07  CWDE
0000:de09  SHL EAX,0xb
0000:de0d  ADD EBX,EAX
0000:de10  CMP EBX,0xfffea000
0000:de17  JL 0x0000:de2e
0000:de19  CMP EBX,0x16000
0000:de20  JG 0x0000:de25
0000:de22  CLC
0000:de23  JMP 0x0000:de35
0000:de25  MOV EBX,0x16000
0000:de2b  STC
0000:de2c  JMP 0x0000:de35
0000:de2e  MOV EBX,0xfffea000
0000:de34  STC
0000:de35  MOV dword ptr ES:[DI + 0xa],EBX
0000:de3a  MOV DX,0x8
0000:de3d  MOV CX,0x10
0000:de40  MOV BX,0xfff8
0000:de43  MOV AX,0xfff8
0000:de46  CALLF 0x0000:ffff
0000:de4b  RET
0000:def2  MOV SI,0x646c
0000:def5  ADD SI,word ptr [0x6468]
0000:def9  INC word ptr [0x6468]
0000:defd  AND word ptr [0x6468],0xff
0000:df03  MOV AL,byte ptr [SI]
0000:df05  CBW
0000:df06  ADD AX,0x82
0000:df09  CMP AX,0x2a
0000:df0c  JG 0x0000:df18
0000:df0e  MOV SI,0x328c
0000:df11  CALLF 0x0000:ffff
0000:df16  JMP 0x0000:df5c
0000:df18  CMP AX,0x54
0000:df1b  JG 0x0000:df27
0000:df1d  MOV SI,0x3298
0000:df20  CALLF 0x0000:ffff
0000:df25  JMP 0x0000:df5c
0000:df27  CMP AX,0x7e
0000:df2a  JG 0x0000:df36
0000:df2c  MOV SI,0x32a4
0000:df2f  CALLF 0x0000:ffff
0000:df34  JMP 0x0000:df5c
0000:df36  CMP AX,0xa8
0000:df39  JG 0x0000:df45
0000:df3b  MOV SI,0x32b0
0000:df3e  CALLF 0x0000:ffff
0000:df43  JMP 0x0000:df5c
0000:df45  CMP AX,0xd2
0000:df48  JG 0x0000:df54
0000:df4a  MOV SI,0x32bc
0000:df4d  CALLF 0x0000:ffff
0000:df52  JMP 0x0000:df5c
0000:df54  MOV SI,0x32c8
0000:df57  CALLF 0x0000:ffff
0000:df5c  RET
0000:df5d  MOV SI,0x646c
0000:df60  ADD SI,word ptr [0x6468]
0000:df64  INC word ptr [0x6468]
0000:df68  AND word ptr [0x6468],0xff
0000:df6e  MOV AL,byte ptr [SI]
0000:df70  CBW
0000:df71  CMP AX,0x46
0000:df74  JLE 0x0000:dfb2
0000:df76  PUSH DI
0000:df77  MOV AX,0xe39e
0000:df7a  XOR DX,DX
0000:df7c  CALLF 0x0000:ffff
0000:df81  MOV byte ptr ES:[DI + 0x17],0x2
0000:df86  MOV AX,0xa0
0000:df89  MOV CX,word ptr [0x81c0]
0000:df8d  MOV BX,word ptr [0x81c4]
0000:df91  ADD CX,AX
0000:df93  MOV SI,0x646c
0000:df96  ADD SI,word ptr [0x6468]
0000:df9a  INC word ptr [0x6468]
0000:df9e  AND word ptr [0x6468],0xff
0000:dfa4  MOV AL,byte ptr [SI]
0000:dfa6  CBW
0000:dfa7  ADD CX,AX
0000:dfa9  MOV word ptr ES:[DI + 0x4],CX
0000:dfad  MOV word ptr ES:[DI + 0x8],BX
0000:dfb1  POP DI
0000:dfb2  RET
0000:dfb3  CALL 0x0000:def2
0000:dfb6  MOV word ptr ES:[DI + 0x18],0xe0f5
0000:dfbc  MOV byte ptr ES:[DI + 0x29],0x1
0000:dfc1  MOV byte ptr ES:[DI + 0x28],0x1
0000:dfc6  CALLF 0x0000:ffff
0000:dfcb  MOV word ptr ES:[DI + 0x2e],AX
0000:dfcf  CWDE
0000:dfd1  SHL EAX,0x7
0000:dfd5  ADD EAX,0xffffc000
0000:dfdb  MOV dword ptr ES:[DI + 0xe],EAX
0000:dfe0  MOV dword ptr ES:[DI + 0xa],0xffffd000
0000:dfe9  PUSH DI
0000:dfea  MOV AX,0xe087
0000:dfed  XOR DX,DX
0000:dfef  CALLF 0x0000:ffff
0000:dff4  POP SI
0000:dff5  MOV byte ptr ES:[DI + 0x17],0x2
0000:dffa  MOV EAX,dword ptr ES:[SI + 0x2]
0000:dfff  ADD EAX,0x50000
0000:e005  MOV dword ptr ES:[DI + 0x2],EAX
0000:e00a  MOV EAX,dword ptr ES:[SI + 0x6]
0000:e00f  ADD EAX,0x60000
0000:e015  MOV dword ptr ES:[DI + 0x6],EAX
0000:e01a  MOV DI,SI
0000:e01c  RET
0000:e01d  CALL 0x0000:def2
0000:e020  MOV word ptr ES:[DI + 0x18],0xe2bf
0000:e026  MOV byte ptr ES:[DI + 0x29],0x1
0000:e02b  MOV byte ptr ES:[DI + 0x28],0x1
0000:e030  CALLF 0x0000:ffff
0000:e035  MOV word ptr ES:[DI + 0x2e],AX
0000:e039  CWDE
0000:e03b  SHL EAX,0x7
0000:e03f  ADD EAX,0xffff8000
0000:e045  MOV dword ptr ES:[DI + 0xe],EAX
0000:e04a  MOV dword ptr ES:[DI + 0xa],0x4000
0000:e053  PUSH DI
0000:e054  MOV AX,0xe0be
0000:e057  XOR DX,DX
0000:e059  CALLF 0x0000:ffff
0000:e05e  POP SI
0000:e05f  MOV byte ptr ES:[DI + 0x17],0x2
0000:e064  MOV EAX,dword ptr ES:[SI + 0x2]
0000:e069  ADD EAX,0x50000
0000:e06f  MOV dword ptr ES:[DI + 0x2],EAX
0000:e074  MOV EAX,dword ptr ES:[SI + 0x6]
0000:e079  ADD EAX,0x60000
0000:e07f  MOV dword ptr ES:[DI + 0x6],EAX
0000:e084  MOV DI,SI
0000:e086  RET
0000:e087  CALL 0x0000:def2
0000:e08a  MOV word ptr ES:[DI + 0x18],0xe1e0
0000:e090  MOV byte ptr ES:[DI + 0x29],0x1
0000:e095  MOV byte ptr ES:[DI + 0x28],0x1
0000:e09a  CALLF 0x0000:ffff
0000:e09f  MOV word ptr ES:[DI + 0x2e],AX
0000:e0a3  CWDE
0000:e0a5  SHL EAX,0x7
0000:e0a9  ADD EAX,0x4000
0000:e0af  MOV dword ptr ES:[DI + 0xe],EAX
0000:e0b4  MOV dword ptr ES:[DI + 0xa],0xffff9000
0000:e0bd  RET
0000:e0be  CALL 0x0000:def2
0000:e0c1  MOV word ptr ES:[DI + 0x18],0xe0f5
0000:e0c7  MOV byte ptr ES:[DI + 0x29],0x1
0000:e0cc  MOV byte ptr ES:[DI + 0x28],0x1
0000:e0d1  CALLF 0x0000:ffff
0000:e0d6  MOV word ptr ES:[DI + 0x2e],AX
0000:e0da  CWDE
0000:e0dc  SHL EAX,0x7
0000:e0e0  ADD EAX,0xffffd000
0000:e0e6  MOV dword ptr ES:[DI + 0xe],EAX
0000:e0eb  MOV dword ptr ES:[DI + 0xa],0xffffd000
0000:e0f4  RET
0000:e0f5  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e0fa  SUB dword ptr ES:[DI + 0x6],EAX
0000:e0ff  MOV EAX,dword ptr ES:[DI + 0xa]
0000:e104  SUB dword ptr ES:[DI + 0x2],EAX
0000:e109  MOV AX,[0x81c0]
0000:e10c  CMP word ptr ES:[DI + 0x4],AX
0000:e110  JGE 0x0000:e144
0000:e112  ADD AX,0x140
0000:e115  MOV word ptr ES:[DI + 0x4],AX
0000:e119  CALL 0x0000:def2
0000:e11c  CALL 0x0000:df5d
0000:e11f  MOV SI,0x646c
0000:e122  ADD SI,word ptr [0x6468]
0000:e126  INC word ptr [0x6468]
0000:e12a  AND word ptr [0x6468],0xff
0000:e130  MOV AL,byte ptr [SI]
0000:e132  CBW
0000:e133  SHR AX,0x1
0000:e135  MOV BX,word ptr [0x81c4]
0000:e139  ADD AX,0x3c
0000:e13c  ADD BX,AX
0000:e13e  MOV word ptr ES:[DI + 0x8],BX
0000:e142  JMP 0x0000:e198
0000:e144  MOV AX,[0x81c0]
0000:e147  ADD AX,0x140
0000:e14a  CMP word ptr ES:[DI + 0x4],AX
0000:e14e  JLE 0x0000:e180
0000:e150  SUB AX,0x140
0000:e153  MOV word ptr ES:[DI + 0x4],AX
0000:e157  CALL 0x0000:df5d
0000:e15a  CALL 0x0000:def2
0000:e15d  MOV SI,0x646c
0000:e160  ADD SI,word ptr [0x6468]
0000:e164  INC word ptr [0x6468]
0000:e168  AND word ptr [0x6468],0xff
0000:e16e  MOV AL,byte ptr [SI]
0000:e170  CBW
0000:e171  SHR AX,0x1
0000:e173  MOV BX,word ptr [0x81c4]
0000:e177  ADD AX,0x3c
0000:e17a  ADD BX,AX
0000:e17c  MOV word ptr ES:[DI + 0x8],BX
0000:e180  MOV AX,[0x81c4]

; ---- D3EE count=256 ----
0000:d3ee  CMP byte ptr [0x88ae],0x5
0000:d3f3  JNZ 0x0000:d41f
0000:d3f5  MOV AX,word ptr ES:[DI + 0x4]
0000:d3f9  SUB AX,word ptr [0x81c0]
0000:d3fd  ADD AX,0x60
0000:d400  CMP AX,0x200
0000:d403  JA 0x0000:d418
0000:d405  MOV AX,word ptr ES:[DI + 0x8]
0000:d409  SUB AX,word ptr [0x81c4]
0000:d40d  ADD AX,0x60
0000:d410  CMP AX,0x170
0000:d413  JA 0x0000:d418
0000:d415  CLC
0000:d416  JMP 0x0000:d41f
0000:d418  STC
0000:d419  MOV word ptr ES:[DI + 0x18],0x0
0000:d41f  RET
0000:d420  MOV SI,0x342e
0000:d423  CALLF 0x0000:ffff
0000:d428  MOV word ptr ES:[DI + 0x18],0xd438
0000:d42e  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d437  RET
0000:d438  CMP byte ptr [0x88ae],0x4
0000:d43d  JNZ 0x0000:d481
0000:d43f  MOV AX,word ptr ES:[DI + 0x4]
0000:d443  SUB AX,word ptr [0x81c0]
0000:d447  ADD AX,0x10
0000:d44a  CMP AX,0x160
0000:d44d  JA 0x0000:d462
0000:d44f  MOV AX,word ptr ES:[DI + 0x8]
0000:d453  SUB AX,word ptr [0x81c4]
0000:d457  ADD AX,0x10
0000:d45a  CMP AX,0xd0
0000:d45d  JA 0x0000:d462
0000:d45f  CLC
0000:d460  JMP 0x0000:d469
0000:d462  STC
0000:d463  MOV word ptr ES:[DI + 0x18],0x0
0000:d469  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d46e  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d477  MOV dword ptr ES:[DI + 0xe],EAX
0000:d47c  ADD dword ptr ES:[DI + 0x6],EAX
0000:d481  MOV DX,0xffec
0000:d484  MOV CX,0x1e
0000:d487  MOV BX,0x14
0000:d48a  MOV AX,0xfff1
0000:d48d  CALLF 0x0000:ffff
0000:d492  CALLF 0x0000:ffff
0000:d497  RET
0000:d498  MOV word ptr ES:[DI + 0x12],0x38a
0000:d49e  MOV word ptr ES:[DI + 0x18],0xd4d9
0000:d4a4  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d4ad  PUSH DI
0000:d4ae  MOV AX,0xd420
0000:d4b1  XOR DX,DX
0000:d4b3  CALLF 0x0000:ffff
0000:d4b8  POP SI
0000:d4b9  MOV word ptr ES:[SI + 0x2a],DI
0000:d4bd  MOV byte ptr ES:[DI + 0x17],0x1
0000:d4c2  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d4c7  MOV dword ptr ES:[DI + 0x2],EAX
0000:d4cc  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4d1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4d6  MOV DI,SI
0000:d4d8  RET
0000:d4d9  PUSH DI
0000:d4da  MOV SI,DI
0000:d4dc  MOV DI,word ptr ES:[DI + 0x2a]
0000:d4e0  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d4e5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d4ea  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4ef  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4f4  POP DI
0000:d4f5  CMP byte ptr [0x88ae],0x4
0000:d4fa  JNZ 0x0000:d53e
0000:d4fc  MOV AX,word ptr ES:[DI + 0x4]
0000:d500  SUB AX,word ptr [0x81c0]
0000:d504  ADD AX,0x10
0000:d507  CMP AX,0x160
0000:d50a  JA 0x0000:d51f
0000:d50c  MOV AX,word ptr ES:[DI + 0x8]
0000:d510  SUB AX,word ptr [0x81c4]
0000:d514  ADD AX,0x10
0000:d517  CMP AX,0xd0
0000:d51a  JA 0x0000:d51f
0000:d51c  CLC
0000:d51d  JMP 0x0000:d526
0000:d51f  STC
0000:d520  MOV word ptr ES:[DI + 0x18],0x0
0000:d526  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d52b  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d534  MOV dword ptr ES:[DI + 0xe],EAX
0000:d539  ADD dword ptr ES:[DI + 0x6],EAX
0000:d53e  RET
0000:d53f  MOV SI,0x32fa
0000:d542  CALLF 0x0000:ffff
0000:d547  MOV word ptr ES:[DI + 0x18],0xd55a
0000:d54d  MOV word ptr ES:[DI + 0x2a],0x0
0000:d553  MOV word ptr ES:[DI + 0x2c],0x0
0000:d559  RET
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2
0000:d5cc  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d5d1  MOV dword ptr ES:[DI + 0x2],EAX
0000:d5d6  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d5db  ADD EAX,0xa0000
0000:d5e1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d5e6  MOV DI,SI
0000:d5e8  MOV SI,0x3308
0000:d5eb  CALLF 0x0000:ffff
0000:d5f0  MOV word ptr [0x612e],0xd
0000:d5f6  CALLF 0x0000:ffff
0000:d5fb  MOV byte ptr ES:[DI + 0x2e],0x1
0000:d600  INC word ptr ES:[DI + 0x2a]
0000:d604  CMP word ptr ES:[DI + 0x2c],0x3
0000:d609  JLE 0x0000:d637
0000:d60b  INC byte ptr [0x88ae]
0000:d60f  MOV word ptr ES:[DI + 0x2c],0x0
0000:d615  JMP 0x0000:d637
0000:d617  INC word ptr ES:[DI + 0x2f]
0000:d61b  CMP word ptr ES:[DI + 0x2f],0x64
0000:d620  JLE 0x0000:d637
0000:d622  MOV word ptr ES:[DI + 0x2f],0x0
0000:d628  MOV byte ptr ES:[DI + 0x2e],0x0
0000:d62d  MOV SI,0x32fa
0000:d630  CALLF 0x0000:ffff
0000:d635  JMP 0x0000:d637
0000:d637  CALLF 0x0000:ffff
0000:d63c  RET
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28

; ---- D4D9 count=256 ----
0000:d4d9  PUSH DI
0000:d4da  MOV SI,DI
0000:d4dc  MOV DI,word ptr ES:[DI + 0x2a]
0000:d4e0  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d4e5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d4ea  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4ef  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4f4  POP DI
0000:d4f5  CMP byte ptr [0x88ae],0x4
0000:d4fa  JNZ 0x0000:d53e
0000:d4fc  MOV AX,word ptr ES:[DI + 0x4]
0000:d500  SUB AX,word ptr [0x81c0]
0000:d504  ADD AX,0x10
0000:d507  CMP AX,0x160
0000:d50a  JA 0x0000:d51f
0000:d50c  MOV AX,word ptr ES:[DI + 0x8]
0000:d510  SUB AX,word ptr [0x81c4]
0000:d514  ADD AX,0x10
0000:d517  CMP AX,0xd0
0000:d51a  JA 0x0000:d51f
0000:d51c  CLC
0000:d51d  JMP 0x0000:d526
0000:d51f  STC
0000:d520  MOV word ptr ES:[DI + 0x18],0x0
0000:d526  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d52b  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d534  MOV dword ptr ES:[DI + 0xe],EAX
0000:d539  ADD dword ptr ES:[DI + 0x6],EAX
0000:d53e  RET
0000:d53f  MOV SI,0x32fa
0000:d542  CALLF 0x0000:ffff
0000:d547  MOV word ptr ES:[DI + 0x18],0xd55a
0000:d54d  MOV word ptr ES:[DI + 0x2a],0x0
0000:d553  MOV word ptr ES:[DI + 0x2c],0x0
0000:d559  RET
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2
0000:d5cc  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d5d1  MOV dword ptr ES:[DI + 0x2],EAX
0000:d5d6  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d5db  ADD EAX,0xa0000
0000:d5e1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d5e6  MOV DI,SI
0000:d5e8  MOV SI,0x3308
0000:d5eb  CALLF 0x0000:ffff
0000:d5f0  MOV word ptr [0x612e],0xd
0000:d5f6  CALLF 0x0000:ffff
0000:d5fb  MOV byte ptr ES:[DI + 0x2e],0x1
0000:d600  INC word ptr ES:[DI + 0x2a]
0000:d604  CMP word ptr ES:[DI + 0x2c],0x3
0000:d609  JLE 0x0000:d637
0000:d60b  INC byte ptr [0x88ae]
0000:d60f  MOV word ptr ES:[DI + 0x2c],0x0
0000:d615  JMP 0x0000:d637
0000:d617  INC word ptr ES:[DI + 0x2f]
0000:d61b  CMP word ptr ES:[DI + 0x2f],0x64
0000:d620  JLE 0x0000:d637
0000:d622  MOV word ptr ES:[DI + 0x2f],0x0
0000:d628  MOV byte ptr ES:[DI + 0x2e],0x0
0000:d62d  MOV SI,0x32fa
0000:d630  CALLF 0x0000:ffff
0000:d635  JMP 0x0000:d637
0000:d637  CALLF 0x0000:ffff
0000:d63c  RET
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28
0000:d78e  JMP 0x0000:d7e7
0000:d790  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d795  MOV AL,byte ptr ES:[DI + 0x29]
0000:d799  CBW
0000:d79a  CWDE
0000:d79c  SHL EAX,0xc
0000:d7a0  ADD EBX,EAX
0000:d7a3  CMP EBX,0xfffed000
0000:d7aa  JL 0x0000:d7c1
0000:d7ac  CMP EBX,0x13000
0000:d7b3  JG 0x0000:d7b8
0000:d7b5  CLC
0000:d7b6  JMP 0x0000:d7c8
0000:d7b8  MOV EBX,0x13000
0000:d7be  STC
0000:d7bf  JMP 0x0000:d7c8
0000:d7c1  MOV EBX,0xfffed000
0000:d7c7  STC
0000:d7c8  MOV dword ptr ES:[DI + 0xa],EBX
0000:d7cd  ADD dword ptr ES:[DI + 0x2],EBX
0000:d7d2  DEC word ptr ES:[DI + 0x42]
0000:d7d6  JGE 0x0000:d7e7
0000:d7d8  NEG byte ptr ES:[DI + 0x40]
0000:d7dc  MOV byte ptr ES:[DI + 0x3e],0xff
0000:d7e1  MOV word ptr ES:[DI + 0x42],0x14
0000:d7e7  JMP 0x0000:d96f
0000:d7ea  CMP byte ptr ES:[DI + 0x40],0x0
0000:d7ef  JGE 0x0000:d89a
0000:d7f3  CMP word ptr ES:[DI + 0x42],0x14
0000:d7f8  JNZ 0x0000:d7fa
0000:d7fa  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d7ff  MOV AL,byte ptr ES:[DI + 0x29]
0000:d803  CBW
0000:d804  CWDE
0000:d806  SHL EAX,0xd
0000:d80a  SUB EBX,EAX
0000:d80d  CMP EBX,0xfffea000
0000:d814  JL 0x0000:d82b
0000:d816  CMP EBX,0x16000
0000:d81d  JG 0x0000:d822
0000:d81f  CLC
0000:d820  JMP 0x0000:d832
0000:d822  MOV EBX,0x16000
0000:d828  STC
0000:d829  JMP 0x0000:d832
0000:d82b  MOV EBX,0xfffea000
0000:d831  STC
0000:d832  MOV dword ptr ES:[DI + 0xa],EBX
0000:d837  ADD dword ptr ES:[DI + 0x2],EBX
0000:d83c  DEC word ptr ES:[DI + 0x42]
0000:d840  JGE 0x0000:d8f1
0000:d844  NEG byte ptr ES:[DI + 0x29]
0000:d848  NEG byte ptr ES:[DI + 0x28]
0000:d84c  NEG byte ptr ES:[DI + 0x40]
0000:d850  CMP word ptr ES:[DI + 0x12],0x384
0000:d856  JZ 0x0000:d860
0000:d858  MOV word ptr ES:[DI + 0x12],0x384
0000:d85e  JMP 0x0000:d866
0000:d860  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d866  PUSH DI
0000:d867  MOV DI,word ptr ES:[DI + 0x2a]
0000:d86b  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d871  JZ 0x0000:d87b
0000:d873  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d879  JMP 0x0000:d881

; ---- D438 count=256 ----
0000:d438  CMP byte ptr [0x88ae],0x4
0000:d43d  JNZ 0x0000:d481
0000:d43f  MOV AX,word ptr ES:[DI + 0x4]
0000:d443  SUB AX,word ptr [0x81c0]
0000:d447  ADD AX,0x10
0000:d44a  CMP AX,0x160
0000:d44d  JA 0x0000:d462
0000:d44f  MOV AX,word ptr ES:[DI + 0x8]
0000:d453  SUB AX,word ptr [0x81c4]
0000:d457  ADD AX,0x10
0000:d45a  CMP AX,0xd0
0000:d45d  JA 0x0000:d462
0000:d45f  CLC
0000:d460  JMP 0x0000:d469
0000:d462  STC
0000:d463  MOV word ptr ES:[DI + 0x18],0x0
0000:d469  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d46e  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d477  MOV dword ptr ES:[DI + 0xe],EAX
0000:d47c  ADD dword ptr ES:[DI + 0x6],EAX
0000:d481  MOV DX,0xffec
0000:d484  MOV CX,0x1e
0000:d487  MOV BX,0x14
0000:d48a  MOV AX,0xfff1
0000:d48d  CALLF 0x0000:ffff
0000:d492  CALLF 0x0000:ffff
0000:d497  RET
0000:d498  MOV word ptr ES:[DI + 0x12],0x38a
0000:d49e  MOV word ptr ES:[DI + 0x18],0xd4d9
0000:d4a4  MOV dword ptr ES:[DI + 0xe],0x12000
0000:d4ad  PUSH DI
0000:d4ae  MOV AX,0xd420
0000:d4b1  XOR DX,DX
0000:d4b3  CALLF 0x0000:ffff
0000:d4b8  POP SI
0000:d4b9  MOV word ptr ES:[SI + 0x2a],DI
0000:d4bd  MOV byte ptr ES:[DI + 0x17],0x1
0000:d4c2  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d4c7  MOV dword ptr ES:[DI + 0x2],EAX
0000:d4cc  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4d1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4d6  MOV DI,SI
0000:d4d8  RET
0000:d4d9  PUSH DI
0000:d4da  MOV SI,DI
0000:d4dc  MOV DI,word ptr ES:[DI + 0x2a]
0000:d4e0  MOV EBX,dword ptr ES:[SI + 0x2]
0000:d4e5  MOV dword ptr ES:[DI + 0x2],EBX
0000:d4ea  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d4ef  MOV dword ptr ES:[DI + 0x6],EAX
0000:d4f4  POP DI
0000:d4f5  CMP byte ptr [0x88ae],0x4
0000:d4fa  JNZ 0x0000:d53e
0000:d4fc  MOV AX,word ptr ES:[DI + 0x4]
0000:d500  SUB AX,word ptr [0x81c0]
0000:d504  ADD AX,0x10
0000:d507  CMP AX,0x160
0000:d50a  JA 0x0000:d51f
0000:d50c  MOV AX,word ptr ES:[DI + 0x8]
0000:d510  SUB AX,word ptr [0x81c4]
0000:d514  ADD AX,0x10
0000:d517  CMP AX,0xd0
0000:d51a  JA 0x0000:d51f
0000:d51c  CLC
0000:d51d  JMP 0x0000:d526
0000:d51f  STC
0000:d520  MOV word ptr ES:[DI + 0x18],0x0
0000:d526  MOV EAX,dword ptr ES:[DI + 0xe]
0000:d52b  ADD dword ptr ES:[DI + 0xe],0xbb8
0000:d534  MOV dword ptr ES:[DI + 0xe],EAX
0000:d539  ADD dword ptr ES:[DI + 0x6],EAX
0000:d53e  RET
0000:d53f  MOV SI,0x32fa
0000:d542  CALLF 0x0000:ffff
0000:d547  MOV word ptr ES:[DI + 0x18],0xd55a
0000:d54d  MOV word ptr ES:[DI + 0x2a],0x0
0000:d553  MOV word ptr ES:[DI + 0x2c],0x0
0000:d559  RET
0000:d55a  CMP byte ptr ES:[DI + 0x2e],0x1
0000:d55f  JGE 0x0000:d617
0000:d563  CMP word ptr [0x8806],0x0
0000:d568  JZ 0x0000:d637
0000:d56c  MOV BX,word ptr ES:[DI + 0x2a]
0000:d570  CMP BX,word ptr [0x8808]
0000:d574  JLE 0x0000:d57e
0000:d576  MOV word ptr ES:[DI + 0x2a],0x0
0000:d57c  XOR BX,BX
0000:d57e  SHL BX,0x2
0000:d581  MOV AX,word ptr ES:[DI + 0x4]
0000:d585  SUB AX,0xf
0000:d588  CMP word ptr [BX + 0x87de],AX
0000:d58c  JLE 0x0000:d5b5
0000:d58e  ADD AX,0x1e
0000:d591  CMP word ptr [BX + 0x87de],AX
0000:d595  JGE 0x0000:d5b5
0000:d597  MOV AX,word ptr ES:[DI + 0x8]
0000:d59b  ADD AX,0x5
0000:d59e  CMP word ptr [BX + 0x87e0],AX
0000:d5a2  JGE 0x0000:d5b5
0000:d5a4  SUB AX,0x1e
0000:d5a7  CMP word ptr [BX + 0x87e0],AX
0000:d5ab  JLE 0x0000:d5b5
0000:d5ad  MOV word ptr [BX + 0x87de],0x0
0000:d5b3  JMP 0x0000:d5b7
0000:d5b5  JMP 0x0000:d600
0000:d5b7  INC word ptr ES:[DI + 0x2c]
0000:d5bb  PUSH DI
0000:d5bc  MOV AX,0x4b70
0000:d5bf  XOR DX,DX
0000:d5c1  CALLF 0x0000:ffff
0000:d5c6  POP SI
0000:d5c7  MOV byte ptr ES:[DI + 0x17],0x2
0000:d5cc  MOV EAX,dword ptr ES:[SI + 0x2]
0000:d5d1  MOV dword ptr ES:[DI + 0x2],EAX
0000:d5d6  MOV EAX,dword ptr ES:[SI + 0x6]
0000:d5db  ADD EAX,0xa0000
0000:d5e1  MOV dword ptr ES:[DI + 0x6],EAX
0000:d5e6  MOV DI,SI
0000:d5e8  MOV SI,0x3308
0000:d5eb  CALLF 0x0000:ffff
0000:d5f0  MOV word ptr [0x612e],0xd
0000:d5f6  CALLF 0x0000:ffff
0000:d5fb  MOV byte ptr ES:[DI + 0x2e],0x1
0000:d600  INC word ptr ES:[DI + 0x2a]
0000:d604  CMP word ptr ES:[DI + 0x2c],0x3
0000:d609  JLE 0x0000:d637
0000:d60b  INC byte ptr [0x88ae]
0000:d60f  MOV word ptr ES:[DI + 0x2c],0x0
0000:d615  JMP 0x0000:d637
0000:d617  INC word ptr ES:[DI + 0x2f]
0000:d61b  CMP word ptr ES:[DI + 0x2f],0x64
0000:d620  JLE 0x0000:d637
0000:d622  MOV word ptr ES:[DI + 0x2f],0x0
0000:d628  MOV byte ptr ES:[DI + 0x2e],0x0
0000:d62d  MOV SI,0x32fa
0000:d630  CALLF 0x0000:ffff
0000:d635  JMP 0x0000:d637
0000:d637  CALLF 0x0000:ffff
0000:d63c  RET
0000:d63d  CMP byte ptr [0x88ae],0x3
0000:d642  JGE 0x0000:da3e
0000:d646  MOV DX,0x32
0000:d649  MOV CX,0x64
0000:d64c  MOV BX,0xffce
0000:d64f  MOV AX,0xffce
0000:d652  CALLF 0x0000:ffff
0000:d657  MOV DX,0x41
0000:d65a  NEG DX
0000:d65c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d661  JS 0x0000:d665
0000:d663  NEG DX
0000:d665  MOV AX,word ptr ES:[DI + 0x8]
0000:d669  DEC AX
0000:d66a  MOV BX,word ptr ES:[DI + 0x4]
0000:d66e  ADD BX,DX
0000:d670  CALLF 0x0000:ffff
0000:d675  JNZ 0x0000:d6ba
0000:d677  MOV DX,0x41
0000:d67a  NEG DX
0000:d67c  TEST byte ptr ES:[DI + 0x28],0xff
0000:d681  JS 0x0000:d685
0000:d683  NEG DX
0000:d685  MOV AX,word ptr ES:[DI + 0x8]
0000:d689  SUB AX,0x11
0000:d68c  MOV BX,word ptr ES:[DI + 0x4]
0000:d690  ADD BX,DX
0000:d692  CALLF 0x0000:ffff
0000:d697  JNZ 0x0000:d6ba
0000:d699  MOV DX,0x41
0000:d69c  NEG DX
0000:d69e  TEST byte ptr ES:[DI + 0x28],0xff
0000:d6a3  JS 0x0000:d6a8
0000:d6a5  MOV DX,0x41
0000:d6a8  MOV AX,word ptr ES:[DI + 0x8]
0000:d6ac  SUB AX,0xc
0000:d6af  MOV BX,word ptr ES:[DI + 0x4]
0000:d6b3  ADD BX,DX
0000:d6b5  CALLF 0x0000:ffff
0000:d6ba  JMP 0x0000:d6be
0000:d6be  JZ 0x0000:d6c5
0000:d6c0  MOV byte ptr ES:[DI + 0x3e],0x1
0000:d6c5  CMP byte ptr ES:[DI + 0x34],0x1
0000:d6ca  JGE 0x0000:d906
0000:d6ce  CMP byte ptr ES:[DI + 0x3e],0x0
0000:d6d3  JLE 0x0000:d8f3
0000:d6d7  CMP byte ptr [0x88ae],0x2
0000:d6dc  JGE 0x0000:d7ea
0000:d6e0  CMP byte ptr ES:[DI + 0x40],0x0
0000:d6e5  JGE 0x0000:d790
0000:d6e9  CMP word ptr ES:[DI + 0x42],0x14
0000:d6ee  JNZ 0x0000:d6f0
0000:d6f0  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d6f5  MOV AL,byte ptr ES:[DI + 0x29]
0000:d6f9  CBW
0000:d6fa  CWDE
0000:d6fc  SHL EAX,0xa
0000:d700  SUB EBX,EAX
0000:d703  CMP EBX,0xfffed000
0000:d70a  JL 0x0000:d721
0000:d70c  CMP EBX,0x13000
0000:d713  JG 0x0000:d718
0000:d715  CLC
0000:d716  JMP 0x0000:d728
0000:d718  MOV EBX,0x13000
0000:d71e  STC
0000:d71f  JMP 0x0000:d728
0000:d721  MOV EBX,0xfffed000
0000:d727  STC
0000:d728  MOV dword ptr ES:[DI + 0xa],EBX
0000:d72d  ADD dword ptr ES:[DI + 0x2],EBX
0000:d732  DEC word ptr ES:[DI + 0x42]
0000:d736  JGE 0x0000:d7e7
0000:d73a  NEG byte ptr ES:[DI + 0x29]
0000:d73e  NEG byte ptr ES:[DI + 0x28]
0000:d742  NEG byte ptr ES:[DI + 0x40]
0000:d746  CMP word ptr ES:[DI + 0x12],0x384
0000:d74c  JZ 0x0000:d756
0000:d74e  MOV word ptr ES:[DI + 0x12],0x384
0000:d754  JMP 0x0000:d75c
0000:d756  MOV word ptr ES:[DI + 0x12],0x3b6
0000:d75c  PUSH DI
0000:d75d  MOV DI,word ptr ES:[DI + 0x2a]
0000:d761  CMP word ptr ES:[DI + 0x12],0x3bb
0000:d767  JZ 0x0000:d771
0000:d769  MOV word ptr ES:[DI + 0x12],0x3bb
0000:d76f  JMP 0x0000:d777
0000:d771  MOV word ptr ES:[DI + 0x12],0x389
0000:d777  POP DI
0000:d778  MOV AL,byte ptr ES:[DI + 0x29]
0000:d77c  CBW
0000:d77d  CWDE
0000:d77f  SHL EAX,0x9
0000:d783  MOV dword ptr ES:[DI + 0xa],EAX
0000:d788  MOV word ptr ES:[DI + 0x42],0x28
0000:d78e  JMP 0x0000:d7e7
0000:d790  MOV EBX,dword ptr ES:[DI + 0xa]
0000:d795  MOV AL,byte ptr ES:[DI + 0x29]
0000:d799  CBW
0000:d79a  CWDE
0000:d79c  SHL EAX,0xc
0000:d7a0  ADD EBX,EAX
0000:d7a3  CMP EBX,0xfffed000
0000:d7aa  JL 0x0000:d7c1
0000:d7ac  CMP EBX,0x13000
0000:d7b3  JG 0x0000:d7b8
0000:d7b5  CLC
0000:d7b6  JMP 0x0000:d7c8
0000:d7b8  MOV EBX,0x13000
0000:d7be  STC
0000:d7bf  JMP 0x0000:d7c8
0000:d7c1  MOV EBX,0xfffed000
0000:d7c7  STC
0000:d7c8  MOV dword ptr ES:[DI + 0xa],EBX
0000:d7cd  ADD dword ptr ES:[DI + 0x2],EBX
0000:d7d2  DEC word ptr ES:[DI + 0x42]
0000:d7d6  JGE 0x0000:d7e7

; ---- E0F5 count=256 ----
0000:e0f5  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e0fa  SUB dword ptr ES:[DI + 0x6],EAX
0000:e0ff  MOV EAX,dword ptr ES:[DI + 0xa]
0000:e104  SUB dword ptr ES:[DI + 0x2],EAX
0000:e109  MOV AX,[0x81c0]
0000:e10c  CMP word ptr ES:[DI + 0x4],AX
0000:e110  JGE 0x0000:e144
0000:e112  ADD AX,0x140
0000:e115  MOV word ptr ES:[DI + 0x4],AX
0000:e119  CALL 0x0000:def2
0000:e11c  CALL 0x0000:df5d
0000:e11f  MOV SI,0x646c
0000:e122  ADD SI,word ptr [0x6468]
0000:e126  INC word ptr [0x6468]
0000:e12a  AND word ptr [0x6468],0xff
0000:e130  MOV AL,byte ptr [SI]
0000:e132  CBW
0000:e133  SHR AX,0x1
0000:e135  MOV BX,word ptr [0x81c4]
0000:e139  ADD AX,0x3c
0000:e13c  ADD BX,AX
0000:e13e  MOV word ptr ES:[DI + 0x8],BX
0000:e142  JMP 0x0000:e198
0000:e144  MOV AX,[0x81c0]
0000:e147  ADD AX,0x140
0000:e14a  CMP word ptr ES:[DI + 0x4],AX
0000:e14e  JLE 0x0000:e180
0000:e150  SUB AX,0x140
0000:e153  MOV word ptr ES:[DI + 0x4],AX
0000:e157  CALL 0x0000:df5d
0000:e15a  CALL 0x0000:def2
0000:e15d  MOV SI,0x646c
0000:e160  ADD SI,word ptr [0x6468]
0000:e164  INC word ptr [0x6468]
0000:e168  AND word ptr [0x6468],0xff
0000:e16e  MOV AL,byte ptr [SI]
0000:e170  CBW
0000:e171  SHR AX,0x1
0000:e173  MOV BX,word ptr [0x81c4]
0000:e177  ADD AX,0x3c
0000:e17a  ADD BX,AX
0000:e17c  MOV word ptr ES:[DI + 0x8],BX
0000:e180  MOV AX,[0x81c4]
0000:e183  CMP word ptr ES:[DI + 0x8],AX
0000:e187  JGE 0x0000:e198
0000:e189  ADD AX,0xc8
0000:e18c  MOV word ptr ES:[DI + 0x8],AX
0000:e190  CALL 0x0000:def2
0000:e193  CALL 0x0000:df5d
0000:e196  JMP 0x0000:e1b1
0000:e198  MOV AX,[0x81c4]
0000:e19b  ADD AX,0xc8
0000:e19e  CMP word ptr ES:[DI + 0x8],AX
0000:e1a2  JLE 0x0000:e1b1
0000:e1a4  SUB AX,0xc8
0000:e1a7  MOV word ptr ES:[DI + 0x8],AX
0000:e1ab  CALL 0x0000:def2
0000:e1ae  CALL 0x0000:df5d
0000:e1b1  MOV AX,word ptr ES:[DI + 0x2c]
0000:e1b5  SUB word ptr ES:[DI + 0x4],AX
0000:e1b9  MOV SI,0x7974
0000:e1bc  MOV AX,word ptr ES:[DI + 0x2e]
0000:e1c0  ADD AX,0xa
0000:e1c3  AND AX,0x7ff
0000:e1c6  MOV word ptr ES:[DI + 0x2e],AX
0000:e1ca  ADD SI,AX
0000:e1cc  MOV AL,byte ptr [SI]
0000:e1ce  SAR AL,0x3
0000:e1d1  CBW
0000:e1d2  MOV word ptr ES:[DI + 0x2c],AX
0000:e1d6  ADD word ptr ES:[DI + 0x4],AX
0000:e1da  CALLF 0x0000:ffff
0000:e1df  RET
0000:e1e0  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e1e5  SUB dword ptr ES:[DI + 0x6],EAX
0000:e1ea  MOV EAX,dword ptr ES:[DI + 0xa]
0000:e1ef  ADD dword ptr ES:[DI + 0x2],EAX
0000:e1f4  MOV AX,[0x81c0]
0000:e1f7  CMP word ptr ES:[DI + 0x4],AX
0000:e1fb  JGE 0x0000:e22c
0000:e1fd  ADD AX,0x140
0000:e200  MOV word ptr ES:[DI + 0x4],AX
0000:e204  CALL 0x0000:def2
0000:e207  MOV SI,0x646c
0000:e20a  ADD SI,word ptr [0x6468]
0000:e20e  INC word ptr [0x6468]
0000:e212  AND word ptr [0x6468],0xff
0000:e218  MOV AL,byte ptr [SI]
0000:e21a  CBW
0000:e21b  SHR AX,0x1
0000:e21d  MOV BX,word ptr [0x81c4]
0000:e221  ADD AX,0x3c
0000:e224  ADD BX,AX
0000:e226  MOV word ptr ES:[DI + 0x8],BX
0000:e22a  JMP 0x0000:e27a
0000:e22c  MOV AX,[0x81c0]
0000:e22f  ADD AX,0x140
0000:e232  CMP word ptr ES:[DI + 0x4],AX
0000:e236  JLE 0x0000:e265
0000:e238  SUB AX,0x140
0000:e23b  MOV word ptr ES:[DI + 0x4],AX
0000:e23f  CALL 0x0000:def2
0000:e242  MOV SI,0x646c
0000:e245  ADD SI,word ptr [0x6468]
0000:e249  INC word ptr [0x6468]
0000:e24d  AND word ptr [0x6468],0xff
0000:e253  MOV AL,byte ptr [SI]
0000:e255  CBW
0000:e256  SHR AX,0x1
0000:e258  MOV BX,word ptr [0x81c4]
0000:e25c  ADD AX,0x3c
0000:e25f  ADD BX,AX
0000:e261  MOV word ptr ES:[DI + 0x8],BX
0000:e265  MOV AX,[0x81c4]
0000:e268  CMP word ptr ES:[DI + 0x8],AX
0000:e26c  JGE 0x0000:e27a
0000:e26e  ADD AX,0xc8
0000:e271  MOV word ptr ES:[DI + 0x8],AX
0000:e275  CALL 0x0000:def2
0000:e278  JMP 0x0000:e290
0000:e27a  MOV AX,[0x81c4]
0000:e27d  ADD AX,0xc8
0000:e280  CMP word ptr ES:[DI + 0x8],AX
0000:e284  JLE 0x0000:e290
0000:e286  SUB AX,0xc8
0000:e289  CALL 0x0000:def2
0000:e28c  MOV word ptr ES:[DI + 0x8],AX
0000:e290  MOV AX,word ptr ES:[DI + 0x2c]
0000:e294  SUB word ptr ES:[DI + 0x4],AX
0000:e298  MOV SI,0x7974
0000:e29b  MOV AX,word ptr ES:[DI + 0x2e]
0000:e29f  ADD AX,0xf
0000:e2a2  AND AX,0x7ff
0000:e2a5  MOV word ptr ES:[DI + 0x2e],AX
0000:e2a9  ADD SI,AX
0000:e2ab  MOV AL,byte ptr [SI]
0000:e2ad  SAR AL,0x4
0000:e2b0  CBW
0000:e2b1  MOV word ptr ES:[DI + 0x2c],AX
0000:e2b5  ADD word ptr ES:[DI + 0x4],AX
0000:e2b9  CALLF 0x0000:ffff
0000:e2be  RET
0000:e2bf  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e2c4  SUB dword ptr ES:[DI + 0x6],EAX
0000:e2c9  MOV EAX,dword ptr ES:[DI + 0xa]
0000:e2ce  SUB dword ptr ES:[DI + 0x2],EAX
0000:e2d3  MOV AX,[0x81c0]
0000:e2d6  CMP word ptr ES:[DI + 0x4],AX
0000:e2da  JGE 0x0000:e30b
0000:e2dc  ADD AX,0x140
0000:e2df  MOV word ptr ES:[DI + 0x4],AX
0000:e2e3  CALL 0x0000:def2
0000:e2e6  MOV SI,0x646c
0000:e2e9  ADD SI,word ptr [0x6468]
0000:e2ed  INC word ptr [0x6468]
0000:e2f1  AND word ptr [0x6468],0xff
0000:e2f7  MOV AL,byte ptr [SI]
0000:e2f9  CBW
0000:e2fa  SHR AX,0x1
0000:e2fc  MOV BX,word ptr [0x81c4]
0000:e300  ADD AX,0x3c
0000:e303  ADD BX,AX
0000:e305  MOV word ptr ES:[DI + 0x8],BX
0000:e309  JMP 0x0000:e359
0000:e30b  MOV AX,[0x81c0]
0000:e30e  ADD AX,0x140
0000:e311  CMP word ptr ES:[DI + 0x4],AX
0000:e315  JLE 0x0000:e344
0000:e317  SUB AX,0x140
0000:e31a  MOV word ptr ES:[DI + 0x4],AX
0000:e31e  CALL 0x0000:def2
0000:e321  MOV SI,0x646c
0000:e324  ADD SI,word ptr [0x6468]
0000:e328  INC word ptr [0x6468]
0000:e32c  AND word ptr [0x6468],0xff
0000:e332  MOV AL,byte ptr [SI]
0000:e334  CBW
0000:e335  SHR AX,0x1
0000:e337  MOV BX,word ptr [0x81c4]
0000:e33b  ADD AX,0x3c
0000:e33e  ADD BX,AX
0000:e340  MOV word ptr ES:[DI + 0x8],BX
0000:e344  MOV AX,[0x81c4]
0000:e347  CMP word ptr ES:[DI + 0x8],AX
0000:e34b  JGE 0x0000:e359
0000:e34d  ADD AX,0xc8
0000:e350  CALL 0x0000:def2
0000:e353  MOV word ptr ES:[DI + 0x8],AX
0000:e357  JMP 0x0000:e36f
0000:e359  MOV AX,[0x81c4]
0000:e35c  ADD AX,0xc8
0000:e35f  CMP word ptr ES:[DI + 0x8],AX
0000:e363  JLE 0x0000:e36f
0000:e365  SUB AX,0xc8
0000:e368  CALL 0x0000:def2
0000:e36b  MOV word ptr ES:[DI + 0x8],AX
0000:e36f  MOV AX,word ptr ES:[DI + 0x2c]
0000:e373  SUB word ptr ES:[DI + 0x4],AX
0000:e377  MOV SI,0x7974
0000:e37a  MOV AX,word ptr ES:[DI + 0x2e]
0000:e37e  ADD AX,0xa
0000:e381  AND AX,0x7ff
0000:e384  MOV word ptr ES:[DI + 0x2e],AX
0000:e388  ADD SI,AX
0000:e38a  MOV AL,byte ptr [SI]
0000:e38c  SAR AL,0x4
0000:e38f  CBW
0000:e390  MOV word ptr ES:[DI + 0x2c],AX
0000:e394  ADD word ptr ES:[DI + 0x4],AX
0000:e398  CALLF 0x0000:ffff
0000:e39d  RET
0000:e39e  CMP byte ptr [0x88ae],0x6
0000:e3a3  JNZ 0x0000:e3e8
0000:e3a5  MOV SI,0x646c
0000:e3a8  ADD SI,word ptr [0x6468]
0000:e3ac  INC word ptr [0x6468]
0000:e3b0  AND word ptr [0x6468],0xff
0000:e3b6  MOV AL,byte ptr [SI]
0000:e3b8  CBW
0000:e3b9  CMP AX,0xffc0
0000:e3bc  JG 0x0000:e3c6
0000:e3be  MOV word ptr ES:[DI + 0x12],0x25f
0000:e3c4  JMP 0x0000:e415
0000:e3c6  CMP AX,0x0
0000:e3c9  JG 0x0000:e3d3
0000:e3cb  MOV word ptr ES:[DI + 0x12],0x260
0000:e3d1  JMP 0x0000:e415
0000:e3d3  CMP AX,0x40
0000:e3d6  JG 0x0000:e3e0
0000:e3d8  MOV word ptr ES:[DI + 0x12],0x261
0000:e3de  JMP 0x0000:e415
0000:e3e0  MOV word ptr ES:[DI + 0x12],0x262
0000:e3e6  JMP 0x0000:e415
0000:e3e8  MOV SI,0x646c
0000:e3eb  ADD SI,word ptr [0x6468]
0000:e3ef  INC word ptr [0x6468]
0000:e3f3  AND word ptr [0x6468],0xff
0000:e3f9  MOV AL,byte ptr [SI]
0000:e3fb  CBW
0000:e3fc  CMP AX,0x0
0000:e3ff  JG 0x0000:e40b
0000:e401  MOV SI,0x32d4
0000:e404  CALLF 0x0000:ffff
0000:e409  JMP 0x0000:e415
0000:e40b  MOV SI,0x32e0
0000:e40e  CALLF 0x0000:ffff
0000:e413  JMP 0x0000:e415
0000:e415  MOV word ptr ES:[DI + 0x18],0xe44b
0000:e41b  MOV byte ptr ES:[DI + 0x29],0x1
0000:e420  MOV byte ptr ES:[DI + 0x28],0x1
0000:e425  MOV SI,0x646c
0000:e428  ADD SI,word ptr [0x6468]
0000:e42c  INC word ptr [0x6468]
0000:e430  AND word ptr [0x6468],0xff
0000:e436  MOV AL,byte ptr [SI]
0000:e438  CBW

; ---- E2BF count=256 ----
0000:e2bf  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e2c4  SUB dword ptr ES:[DI + 0x6],EAX
0000:e2c9  MOV EAX,dword ptr ES:[DI + 0xa]
0000:e2ce  SUB dword ptr ES:[DI + 0x2],EAX
0000:e2d3  MOV AX,[0x81c0]
0000:e2d6  CMP word ptr ES:[DI + 0x4],AX
0000:e2da  JGE 0x0000:e30b
0000:e2dc  ADD AX,0x140
0000:e2df  MOV word ptr ES:[DI + 0x4],AX
0000:e2e3  CALL 0x0000:def2
0000:e2e6  MOV SI,0x646c
0000:e2e9  ADD SI,word ptr [0x6468]
0000:e2ed  INC word ptr [0x6468]
0000:e2f1  AND word ptr [0x6468],0xff
0000:e2f7  MOV AL,byte ptr [SI]
0000:e2f9  CBW
0000:e2fa  SHR AX,0x1
0000:e2fc  MOV BX,word ptr [0x81c4]
0000:e300  ADD AX,0x3c
0000:e303  ADD BX,AX
0000:e305  MOV word ptr ES:[DI + 0x8],BX
0000:e309  JMP 0x0000:e359
0000:e30b  MOV AX,[0x81c0]
0000:e30e  ADD AX,0x140
0000:e311  CMP word ptr ES:[DI + 0x4],AX
0000:e315  JLE 0x0000:e344
0000:e317  SUB AX,0x140
0000:e31a  MOV word ptr ES:[DI + 0x4],AX
0000:e31e  CALL 0x0000:def2
0000:e321  MOV SI,0x646c
0000:e324  ADD SI,word ptr [0x6468]
0000:e328  INC word ptr [0x6468]
0000:e32c  AND word ptr [0x6468],0xff
0000:e332  MOV AL,byte ptr [SI]
0000:e334  CBW
0000:e335  SHR AX,0x1
0000:e337  MOV BX,word ptr [0x81c4]
0000:e33b  ADD AX,0x3c
0000:e33e  ADD BX,AX
0000:e340  MOV word ptr ES:[DI + 0x8],BX
0000:e344  MOV AX,[0x81c4]
0000:e347  CMP word ptr ES:[DI + 0x8],AX
0000:e34b  JGE 0x0000:e359
0000:e34d  ADD AX,0xc8
0000:e350  CALL 0x0000:def2
0000:e353  MOV word ptr ES:[DI + 0x8],AX
0000:e357  JMP 0x0000:e36f
0000:e359  MOV AX,[0x81c4]
0000:e35c  ADD AX,0xc8
0000:e35f  CMP word ptr ES:[DI + 0x8],AX
0000:e363  JLE 0x0000:e36f
0000:e365  SUB AX,0xc8
0000:e368  CALL 0x0000:def2
0000:e36b  MOV word ptr ES:[DI + 0x8],AX
0000:e36f  MOV AX,word ptr ES:[DI + 0x2c]
0000:e373  SUB word ptr ES:[DI + 0x4],AX
0000:e377  MOV SI,0x7974
0000:e37a  MOV AX,word ptr ES:[DI + 0x2e]
0000:e37e  ADD AX,0xa
0000:e381  AND AX,0x7ff
0000:e384  MOV word ptr ES:[DI + 0x2e],AX
0000:e388  ADD SI,AX
0000:e38a  MOV AL,byte ptr [SI]
0000:e38c  SAR AL,0x4
0000:e38f  CBW
0000:e390  MOV word ptr ES:[DI + 0x2c],AX
0000:e394  ADD word ptr ES:[DI + 0x4],AX
0000:e398  CALLF 0x0000:ffff
0000:e39d  RET
0000:e39e  CMP byte ptr [0x88ae],0x6
0000:e3a3  JNZ 0x0000:e3e8
0000:e3a5  MOV SI,0x646c
0000:e3a8  ADD SI,word ptr [0x6468]
0000:e3ac  INC word ptr [0x6468]
0000:e3b0  AND word ptr [0x6468],0xff
0000:e3b6  MOV AL,byte ptr [SI]
0000:e3b8  CBW
0000:e3b9  CMP AX,0xffc0
0000:e3bc  JG 0x0000:e3c6
0000:e3be  MOV word ptr ES:[DI + 0x12],0x25f
0000:e3c4  JMP 0x0000:e415
0000:e3c6  CMP AX,0x0
0000:e3c9  JG 0x0000:e3d3
0000:e3cb  MOV word ptr ES:[DI + 0x12],0x260
0000:e3d1  JMP 0x0000:e415
0000:e3d3  CMP AX,0x40
0000:e3d6  JG 0x0000:e3e0
0000:e3d8  MOV word ptr ES:[DI + 0x12],0x261
0000:e3de  JMP 0x0000:e415
0000:e3e0  MOV word ptr ES:[DI + 0x12],0x262
0000:e3e6  JMP 0x0000:e415
0000:e3e8  MOV SI,0x646c
0000:e3eb  ADD SI,word ptr [0x6468]
0000:e3ef  INC word ptr [0x6468]
0000:e3f3  AND word ptr [0x6468],0xff
0000:e3f9  MOV AL,byte ptr [SI]
0000:e3fb  CBW
0000:e3fc  CMP AX,0x0
0000:e3ff  JG 0x0000:e40b
0000:e401  MOV SI,0x32d4
0000:e404  CALLF 0x0000:ffff
0000:e409  JMP 0x0000:e415
0000:e40b  MOV SI,0x32e0
0000:e40e  CALLF 0x0000:ffff
0000:e413  JMP 0x0000:e415
0000:e415  MOV word ptr ES:[DI + 0x18],0xe44b
0000:e41b  MOV byte ptr ES:[DI + 0x29],0x1
0000:e420  MOV byte ptr ES:[DI + 0x28],0x1
0000:e425  MOV SI,0x646c
0000:e428  ADD SI,word ptr [0x6468]
0000:e42c  INC word ptr [0x6468]
0000:e430  AND word ptr [0x6468],0xff
0000:e436  MOV AL,byte ptr [SI]
0000:e438  CBW
0000:e439  CWDE
0000:e43b  SHL EAX,0x7
0000:e43f  ADD EAX,0x18000
0000:e445  MOV dword ptr ES:[DI + 0xe],EAX
0000:e44a  RET
0000:e44b  MOV AX,word ptr ES:[DI + 0x4]
0000:e44f  SUB AX,word ptr [0x81c0]
0000:e453  ADD AX,0x20
0000:e456  CMP AX,0x180
0000:e459  JA 0x0000:e46e
0000:e45b  MOV AX,word ptr ES:[DI + 0x8]
0000:e45f  SUB AX,word ptr [0x81c4]
0000:e463  ADD AX,0x20
0000:e466  CMP AX,0xf0
0000:e469  JA 0x0000:e46e
0000:e46b  CLC
0000:e46c  JMP 0x0000:e475
0000:e46e  STC
0000:e46f  MOV word ptr ES:[DI + 0x18],0x0
0000:e475  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e47a  ADD dword ptr ES:[DI + 0x6],EAX
0000:e47f  CMP byte ptr [0x88ae],0x6
0000:e484  JNZ 0x0000:e487
0000:e486  RET
0000:e487  CALLF 0x0000:ffff
0000:e48c  RET
0000:e572  MOV SI,0x3436
0000:e575  CALLF 0x0000:ffff
0000:e57a  MOV word ptr ES:[DI + 0x18],0xe6a4
0000:e580  MOV byte ptr ES:[DI + 0x29],0x1
0000:e585  MOV byte ptr ES:[DI + 0x28],0x1
0000:e58a  CALLF 0x0000:ffff
0000:e58f  MOV word ptr ES:[DI + 0x2e],AX
0000:e593  CWDE
0000:e595  SHL EAX,0x7
0000:e599  ADD EAX,0xffffa000
0000:e59f  MOV dword ptr ES:[DI + 0xe],EAX
0000:e5a4  PUSH DI
0000:e5a5  MOV AX,0xe63e
0000:e5a8  XOR DX,DX
0000:e5aa  CALLF 0x0000:ffff
0000:e5af  POP SI
0000:e5b0  MOV byte ptr ES:[DI + 0x17],0x2
0000:e5b5  MOV EAX,dword ptr ES:[SI + 0x2]
0000:e5ba  ADD EAX,0x50000
0000:e5c0  MOV dword ptr ES:[DI + 0x2],EAX
0000:e5c5  MOV EAX,dword ptr ES:[SI + 0x6]
0000:e5ca  ADD EAX,0x60000
0000:e5d0  MOV dword ptr ES:[DI + 0x6],EAX
0000:e5d5  MOV DI,SI
0000:e5d7  RET
0000:e5d8  MOV SI,0x3440
0000:e5db  CALLF 0x0000:ffff
0000:e5e0  MOV word ptr ES:[DI + 0x18],0xe836
0000:e5e6  MOV byte ptr ES:[DI + 0x29],0x1
0000:e5eb  MOV byte ptr ES:[DI + 0x28],0x1
0000:e5f0  CALLF 0x0000:ffff
0000:e5f5  MOV word ptr ES:[DI + 0x2e],AX
0000:e5f9  CWDE
0000:e5fb  SHL EAX,0x7
0000:e5ff  ADD EAX,0xffffa000
0000:e605  MOV dword ptr ES:[DI + 0xe],EAX
0000:e60a  PUSH DI
0000:e60b  MOV AX,0xe671
0000:e60e  XOR DX,DX
0000:e610  CALLF 0x0000:ffff
0000:e615  POP SI
0000:e616  MOV byte ptr ES:[DI + 0x17],0x2
0000:e61b  MOV EAX,dword ptr ES:[SI + 0x2]
0000:e620  ADD EAX,0x50000
0000:e626  MOV dword ptr ES:[DI + 0x2],EAX
0000:e62b  MOV EAX,dword ptr ES:[SI + 0x6]
0000:e630  ADD EAX,0x60000
0000:e636  MOV dword ptr ES:[DI + 0x6],EAX
0000:e63b  MOV DI,SI
0000:e63d  RET
0000:e63e  MOV SI,0x344c
0000:e641  CALLF 0x0000:ffff
0000:e646  MOV word ptr ES:[DI + 0x18],0xe76d
0000:e64c  MOV byte ptr ES:[DI + 0x29],0x1
0000:e651  MOV byte ptr ES:[DI + 0x28],0x1
0000:e656  CALLF 0x0000:ffff
0000:e65b  MOV word ptr ES:[DI + 0x2e],AX
0000:e65f  CWDE
0000:e661  SHL EAX,0x7
0000:e665  ADD EAX,0xffff9000
0000:e66b  MOV dword ptr ES:[DI + 0xe],EAX
0000:e670  RET
0000:e671  MOV SI,0x3456
0000:e674  CALLF 0x0000:ffff
0000:e679  MOV word ptr ES:[DI + 0x18],0xe6a4
0000:e67f  MOV byte ptr ES:[DI + 0x29],0x1
0000:e684  MOV byte ptr ES:[DI + 0x28],0x1
0000:e689  CALLF 0x0000:ffff
0000:e68e  MOV word ptr ES:[DI + 0x2e],AX
0000:e692  CWDE
0000:e694  SHL EAX,0x7
0000:e698  ADD EAX,0xffff9000
0000:e69e  MOV dword ptr ES:[DI + 0xe],EAX
0000:e6a3  RET
0000:e6a4  MOV EAX,dword ptr ES:[DI + 0xe]
0000:e6a9  ADD dword ptr ES:[DI + 0x6],EAX
0000:e6ae  MOV AX,[0x81c0]
0000:e6b1  CMP word ptr ES:[DI + 0x4],AX
0000:e6b5  JGE 0x0000:e6e3
0000:e6b7  ADD AX,0x140
0000:e6ba  MOV word ptr ES:[DI + 0x4],AX
0000:e6be  MOV SI,0x646c
0000:e6c1  ADD SI,word ptr [0x6468]
0000:e6c5  INC word ptr [0x6468]
0000:e6c9  AND word ptr [0x6468],0xff
0000:e6cf  MOV AL,byte ptr [SI]
0000:e6d1  CBW
0000:e6d2  SHR AX,0x1
0000:e6d4  MOV BX,word ptr [0x81c4]
0000:e6d8  ADD AX,0x3c
0000:e6db  ADD BX,AX
0000:e6dd  MOV word ptr ES:[DI + 0x8],BX
0000:e6e1  JMP 0x0000:e72b
0000:e6e3  MOV AX,[0x81c0]
0000:e6e6  ADD AX,0x140
0000:e6e9  CMP word ptr ES:[DI + 0x4],AX
0000:e6ed  JLE 0x0000:e719
0000:e6ef  SUB AX,0x140
0000:e6f2  MOV word ptr ES:[DI + 0x4],AX
0000:e6f6  MOV SI,0x646c
0000:e6f9  ADD SI,word ptr [0x6468]
0000:e6fd  INC word ptr [0x6468]
0000:e701  AND word ptr [0x6468],0xff
0000:e707  MOV AL,byte ptr [SI]
0000:e709  CBW
0000:e70a  SHR AX,0x1
0000:e70c  MOV BX,word ptr [0x81c4]
0000:e710  ADD AX,0x3c
0000:e713  ADD BX,AX
0000:e715  MOV word ptr ES:[DI + 0x8],BX
0000:e719  MOV AX,[0x81c4]
0000:e71c  CMP word ptr ES:[DI + 0x8],AX
0000:e720  JGE 0x0000:e72b
0000:e722  ADD AX,0xc8
0000:e725  MOV word ptr ES:[DI + 0x8],AX
0000:e729  JMP 0x0000:e73e

; ---- 0E06 count=64 ----
0000:0e06  PUSH CX
0000:0e07  MOV CX,0x40
0000:0e0a  LES DI,[0x755e]
0000:0e0e  MOV BX,word ptr ES:[DI + 0x18]
0000:0e12  AND BX,0xffff
0000:0e16  JZ 0x0000:0e39
0000:0e18  ADD DI,word ptr [0x30ce]
0000:0e1c  LOOP 0x0000:0e0e
0000:0e1e  POP CX
0000:0e1f  PUSH DX
0000:0e20  PUSH AX
0000:0e21  MOV DX,0x3c8
0000:0e24  XOR AL,AL
0000:0e26  OUT DX,AL
0000:0e27  INC DX
0000:0e28  MOV AL,0x3f
0000:0e2a  OUT DX,AL
0000:0e2b  MOV AL,0x0
0000:0e2d  OUT DX,AL
0000:0e2e  MOV AL,0x0
0000:0e30  OUT DX,AL
0000:0e31  POP AX
0000:0e32  POP DX
0000:0e33  JMP 0x0000:0e33
0000:0e39  MOV word ptr ES:[DI + 0x18],AX
0000:0e3d  MOV word ptr ES:[DI + 0x1c],0x1997
0000:0e43  MOV byte ptr ES:[DI + 0x28],0x1
0000:0e48  MOV byte ptr ES:[DI + 0x17],0x1
0000:0e4d  MOV word ptr ES:[DI + 0x12],0xffff
0000:0e53  MOV word ptr ES:[DI + 0x1a],0xffff
0000:0e59  MOV word ptr ES:[DI + 0x14],0x0
0000:0e5f  POP CX
0000:0e60  CALLF 0x0000:ffff
0000:0e65  RETF
0000:0e66  ADD CX,0xa
0000:0e69  PUSH CX
0000:0e6a  PUSH ES
0000:0e6b  PUSH DI
0000:0e6c  PUSH BX
0000:0e6d  MOV word ptr [0x88c8],0x0
0000:0e73  MOV CX,0x40
0000:0e76  LES DI,[0x755e]
0000:0e7a  MOV BX,word ptr ES:[DI + 0x18]
0000:0e7e  AND BX,0xffff
0000:0e82  JZ 0x0000:0e88
0000:0e84  INC word ptr [0x88c8]
0000:0e88  ADD DI,word ptr [0x30ce]
0000:0e8c  LOOP 0x0000:0e7a
0000:0e8e  POP BX
0000:0e8f  POP DI
0000:0e90  POP ES
0000:0e91  POP CX
0000:0e92  RETF
0000:0e96  MOV AX,[0x7560]
0000:0e99  MOV AX,ES
0000:0e9b  MOV word ptr [0x88c8],0x0
0000:0ea1  MOV AX,[0x7966]
0000:0ea4  MOV BX,AX
0000:0ea6  ADD AX,0x200
0000:0ea9  AND AX,0x200
0000:0eac  MOV [0x7966],AX
0000:0eaf  AND BX,0x200
0000:0eb3  MOV SI,BX
0000:0eb5  ADD SI,0x7566

; ---- 0E96 count=192 ----
0000:0e96  MOV AX,[0x7560]
0000:0e99  MOV AX,ES
0000:0e9b  MOV word ptr [0x88c8],0x0
0000:0ea1  MOV AX,[0x7966]
0000:0ea4  MOV BX,AX
0000:0ea6  ADD AX,0x200
0000:0ea9  AND AX,0x200
0000:0eac  MOV [0x7966],AX
0000:0eaf  AND BX,0x200
0000:0eb3  MOV SI,BX
0000:0eb5  ADD SI,0x7566
0000:0eb9  PUSH SI
0000:0eba  MOV AX,word ptr [SI]
0000:0ebc  CMP AX,0xffff
0000:0ebf  JZ 0x0000:0ee2
0000:0ec1  MOV DI,word ptr [SI + 0x4]
0000:0ec4  ADD SI,0x8
0000:0ec7  CMP byte ptr ES:[DI + 0x17],0x0
0000:0ecc  JNZ 0x0000:0eba
0000:0ece  PUSHA
0000:0ecf  PUSH ES
0000:0ed0  PUSH DS
0000:0ed1  CALL AX
0000:0ed3  CALLF 0x0000:ffff
0000:0ed8  POP DS
0000:0ed9  POP ES
0000:0eda  POPA
0000:0edb  NOP
0000:0edc  INC word ptr [0x88c8]
0000:0ee0  JMP 0x0000:0eba
0000:0ee2  POP SI
0000:0ee3  PUSH SI
0000:0ee4  MOV AX,word ptr [SI]
0000:0ee6  CMP AX,0xffff
0000:0ee9  JZ 0x0000:0f0c
0000:0eeb  MOV DI,word ptr [SI + 0x4]
0000:0eee  ADD SI,0x8
0000:0ef1  CMP byte ptr ES:[DI + 0x17],0x1
0000:0ef6  JNZ 0x0000:0ee4
0000:0ef8  PUSHA
0000:0ef9  PUSH ES
0000:0efa  PUSH DS
0000:0efb  CALL AX
0000:0efd  CALLF 0x0000:ffff
0000:0f02  POP DS
0000:0f03  POP ES
0000:0f04  POPA
0000:0f05  NOP
0000:0f06  INC word ptr [0x88c8]
0000:0f0a  JMP 0x0000:0ee4
0000:0f0c  POP SI
0000:0f0d  MOV AX,word ptr [SI]
0000:0f0f  CMP AX,0xffff
0000:0f12  JZ 0x0000:0f35
0000:0f14  MOV DI,word ptr [SI + 0x4]
0000:0f17  ADD SI,0x8
0000:0f1a  CMP byte ptr ES:[DI + 0x17],0x2
0000:0f1f  JNZ 0x0000:0f0d
0000:0f21  PUSHA
0000:0f22  PUSH ES
0000:0f23  PUSH DS
0000:0f24  CALL AX
0000:0f26  CALLF 0x0000:ffff
0000:0f2b  POP DS
0000:0f2c  POP ES
0000:0f2d  POPA
0000:0f2e  NOP
0000:0f2f  INC word ptr [0x88c8]
0000:0f33  JMP 0x0000:0f0d
0000:0f35  CALL 0x0000:0fdc
0000:0f38  RETF
0000:0fa2  MOV AX,[0x7560]
0000:0fa5  MOV AX,ES
0000:0fa7  MOV AX,[0x7966]
0000:0faa  AND AX,0x200
0000:0fad  MOV SI,AX
0000:0faf  ADD SI,0x7566
0000:0fb3  MOV AX,word ptr [SI]
0000:0fb5  CMP AX,0xffff
0000:0fb8  JZ 0x0000:0fd8
0000:0fba  MOV AX,word ptr [SI + 0x2]
0000:0fbd  AND AX,0xffff
0000:0fc0  JNZ 0x0000:0fc7
0000:0fc2  ADD SI,0x8
0000:0fc5  JMP 0x0000:0fb3
0000:0fc7  MOV DI,word ptr [SI + 0x4]
0000:0fca  PUSHA
0000:0fcb  PUSH ES
0000:0fcc  PUSH DS
0000:0fcd  CALL AX
0000:0fcf  POP DS
0000:0fd0  POP ES
0000:0fd1  POPA
0000:0fd2  NOP
0000:0fd3  ADD SI,0x8
0000:0fd6  JMP 0x0000:0fb3
0000:0fd8  RETF
0000:0fdc  MOV word ptr [0x8174],0x0
0000:0fe2  MOV AX,[0x7560]
0000:0fe5  MOV AX,ES
0000:0fe7  MOV AX,[0x7966]
0000:0fea  AND AX,0x200
0000:0fed  MOV SI,0x7566
0000:0ff0  ADD SI,AX
0000:0ff2  MOV AX,word ptr [SI]
0000:0ff4  CMP AX,0xffff
0000:0ff7  JZ 0x0000:1032
0000:0ff9  MOV AX,word ptr [SI + 0x2]
0000:0ffc  AND AX,0xffff
0000:0fff  JNZ 0x0000:1006
0000:1001  ADD SI,0x8
0000:1004  JMP 0x0000:0ff2
0000:1006  MOV DI,word ptr [SI + 0x4]
0000:1009  PUSHA
0000:100a  PUSH ES
0000:100b  PUSH DS
0000:100c  MOV DX,word ptr ES:[DI + 0x12]
0000:1010  MOV CX,word ptr ES:[DI + 0x8]
0000:1014  MOV AX,word ptr ES:[DI + 0x4]
0000:1018  MOV BX,CX
0000:101a  MOV CL,byte ptr ES:[DI + 0x16]
0000:101e  TEST DX,0x8000
0000:1022  JNZ 0x0000:1029
0000:1024  CALLF 0x0000:ffff
0000:1029  POP DS
0000:102a  POP ES
0000:102b  POPA
0000:102c  NOP
0000:102d  ADD SI,0x8
0000:1030  JMP 0x0000:0ff2
0000:1032  RET
0000:1036  AND word ptr ES:[DI + 0x18],0xffff
0000:103c  JNZ 0x0000:1042
0000:103e  RETF
0000:1042  PUSHA
0000:1043  MOV SI,0x7566
0000:1046  ADD SI,word ptr [0x7966]
0000:104a  MOV BX,word ptr ES:[DI + 0x1c]
0000:104e  MOV word ptr [SI + 0x2],BX
0000:1051  MOV AX,word ptr ES:[DI + 0x18]
0000:1055  MOV word ptr [SI],AX
0000:1057  MOV word ptr [SI + 0x4],DI
0000:105a  ADD word ptr [0x7966],0x8
0000:105f  MOV word ptr [SI + 0x8],0xffff
0000:1064  POPA
0000:1065  NOP
0000:1066  RETF
0000:106a  CALLF 0x0000:ffff
0000:106f  MOV AX,[0x7560]
0000:1072  MOV AX,ES
0000:1074  MOV AX,[0x7966]
0000:1077  MOV BX,AX
0000:1079  ADD AX,0x200
0000:107c  AND AX,0x200
0000:107f  MOV [0x7966],AX
0000:1082  AND BX,0x200
0000:1086  MOV SI,BX
0000:1088  ADD SI,0x7566
0000:108c  MOV AX,word ptr [SI]
0000:108e  CMP AX,0xffff
0000:1091  JZ 0x0000:10b1
0000:1093  MOV DI,word ptr [SI + 0x4]
0000:1096  ADD SI,0x8
0000:1099  CMP word ptr ES:[DI + 0x1a],-0x1
0000:109e  JZ 0x0000:10a9
0000:10a0  PUSH BX
0000:10a1  CALLF 0x0000:ffff
0000:10a6  POP BX
0000:10a7  JMP 0x0000:108c
0000:10a9  MOV word ptr ES:[DI + 0x18],0x0
0000:10af  JMP 0x0000:108c
0000:10b1  RETF
0000:1749  MOV AX,0x80
0000:174c  CMP word ptr [0x895e],AX
0000:1750  JZ 0x0000:178c
0000:1752  PUSH DI
0000:1753  PUSH AX
0000:1754  MOV DI,0x8960
0000:1757  ADD DI,word ptr [0x895e]
0000:175b  INC word ptr [0x895e]
0000:175f  MOV AL,byte ptr [DI]
0000:1761  CBW
0000:1762  SHL AX,0x3
0000:1765  MOV SI,0x6586
0000:1768  ADD SI,AX
0000:176a  POP AX
0000:176b  POP DI
0000:176c  ROR EDX,0x10
0000:1770  AND DX,0xfff0
0000:1774  MOV dword ptr [SI],EDX
0000:1777  MOV word ptr [SI + 0x4],BX
0000:177a  PUSH SI

; ---- 0FA2 count=96 ----
0000:0fa2  MOV AX,[0x7560]
0000:0fa5  MOV AX,ES
0000:0fa7  MOV AX,[0x7966]
0000:0faa  AND AX,0x200
0000:0fad  MOV SI,AX
0000:0faf  ADD SI,0x7566
0000:0fb3  MOV AX,word ptr [SI]
0000:0fb5  CMP AX,0xffff
0000:0fb8  JZ 0x0000:0fd8
0000:0fba  MOV AX,word ptr [SI + 0x2]
0000:0fbd  AND AX,0xffff
0000:0fc0  JNZ 0x0000:0fc7
0000:0fc2  ADD SI,0x8
0000:0fc5  JMP 0x0000:0fb3
0000:0fc7  MOV DI,word ptr [SI + 0x4]
0000:0fca  PUSHA
0000:0fcb  PUSH ES
0000:0fcc  PUSH DS
0000:0fcd  CALL AX
0000:0fcf  POP DS
0000:0fd0  POP ES
0000:0fd1  POPA
0000:0fd2  NOP
0000:0fd3  ADD SI,0x8
0000:0fd6  JMP 0x0000:0fb3
0000:0fd8  RETF
0000:0fdc  MOV word ptr [0x8174],0x0
0000:0fe2  MOV AX,[0x7560]
0000:0fe5  MOV AX,ES
0000:0fe7  MOV AX,[0x7966]
0000:0fea  AND AX,0x200
0000:0fed  MOV SI,0x7566
0000:0ff0  ADD SI,AX
0000:0ff2  MOV AX,word ptr [SI]
0000:0ff4  CMP AX,0xffff
0000:0ff7  JZ 0x0000:1032
0000:0ff9  MOV AX,word ptr [SI + 0x2]
0000:0ffc  AND AX,0xffff
0000:0fff  JNZ 0x0000:1006
0000:1001  ADD SI,0x8
0000:1004  JMP 0x0000:0ff2
0000:1006  MOV DI,word ptr [SI + 0x4]
0000:1009  PUSHA
0000:100a  PUSH ES
0000:100b  PUSH DS
0000:100c  MOV DX,word ptr ES:[DI + 0x12]
0000:1010  MOV CX,word ptr ES:[DI + 0x8]
0000:1014  MOV AX,word ptr ES:[DI + 0x4]
0000:1018  MOV BX,CX
0000:101a  MOV CL,byte ptr ES:[DI + 0x16]
0000:101e  TEST DX,0x8000
0000:1022  JNZ 0x0000:1029
0000:1024  CALLF 0x0000:ffff
0000:1029  POP DS
0000:102a  POP ES
0000:102b  POPA
0000:102c  NOP
0000:102d  ADD SI,0x8
0000:1030  JMP 0x0000:0ff2
0000:1032  RET
0000:1036  AND word ptr ES:[DI + 0x18],0xffff
0000:103c  JNZ 0x0000:1042
0000:103e  RETF
0000:1042  PUSHA
0000:1043  MOV SI,0x7566
0000:1046  ADD SI,word ptr [0x7966]
0000:104a  MOV BX,word ptr ES:[DI + 0x1c]
0000:104e  MOV word ptr [SI + 0x2],BX
0000:1051  MOV AX,word ptr ES:[DI + 0x18]
0000:1055  MOV word ptr [SI],AX
0000:1057  MOV word ptr [SI + 0x4],DI
0000:105a  ADD word ptr [0x7966],0x8
0000:105f  MOV word ptr [SI + 0x8],0xffff
0000:1064  POPA
0000:1065  NOP
0000:1066  RETF
0000:106a  CALLF 0x0000:ffff
0000:106f  MOV AX,[0x7560]
0000:1072  MOV AX,ES
0000:1074  MOV AX,[0x7966]
0000:1077  MOV BX,AX
0000:1079  ADD AX,0x200
0000:107c  AND AX,0x200
0000:107f  MOV [0x7966],AX
0000:1082  AND BX,0x200
0000:1086  MOV SI,BX
0000:1088  ADD SI,0x7566
0000:108c  MOV AX,word ptr [SI]
0000:108e  CMP AX,0xffff
0000:1091  JZ 0x0000:10b1
0000:1093  MOV DI,word ptr [SI + 0x4]
0000:1096  ADD SI,0x8
0000:1099  CMP word ptr ES:[DI + 0x1a],-0x1
0000:109e  JZ 0x0000:10a9
0000:10a0  PUSH BX
0000:10a1  CALLF 0x0000:ffff

; ---- 1036 count=128 ----
0000:1036  AND word ptr ES:[DI + 0x18],0xffff
0000:103c  JNZ 0x0000:1042
0000:103e  RETF
0000:1042  PUSHA
0000:1043  MOV SI,0x7566
0000:1046  ADD SI,word ptr [0x7966]
0000:104a  MOV BX,word ptr ES:[DI + 0x1c]
0000:104e  MOV word ptr [SI + 0x2],BX
0000:1051  MOV AX,word ptr ES:[DI + 0x18]
0000:1055  MOV word ptr [SI],AX
0000:1057  MOV word ptr [SI + 0x4],DI
0000:105a  ADD word ptr [0x7966],0x8
0000:105f  MOV word ptr [SI + 0x8],0xffff
0000:1064  POPA
0000:1065  NOP
0000:1066  RETF
0000:106a  CALLF 0x0000:ffff
0000:106f  MOV AX,[0x7560]
0000:1072  MOV AX,ES
0000:1074  MOV AX,[0x7966]
0000:1077  MOV BX,AX
0000:1079  ADD AX,0x200
0000:107c  AND AX,0x200
0000:107f  MOV [0x7966],AX
0000:1082  AND BX,0x200
0000:1086  MOV SI,BX
0000:1088  ADD SI,0x7566
0000:108c  MOV AX,word ptr [SI]
0000:108e  CMP AX,0xffff
0000:1091  JZ 0x0000:10b1
0000:1093  MOV DI,word ptr [SI + 0x4]
0000:1096  ADD SI,0x8
0000:1099  CMP word ptr ES:[DI + 0x1a],-0x1
0000:109e  JZ 0x0000:10a9
0000:10a0  PUSH BX
0000:10a1  CALLF 0x0000:ffff
0000:10a6  POP BX
0000:10a7  JMP 0x0000:108c
0000:10a9  MOV word ptr ES:[DI + 0x18],0x0
0000:10af  JMP 0x0000:108c
0000:10b1  RETF
0000:1749  MOV AX,0x80
0000:174c  CMP word ptr [0x895e],AX
0000:1750  JZ 0x0000:178c
0000:1752  PUSH DI
0000:1753  PUSH AX
0000:1754  MOV DI,0x8960
0000:1757  ADD DI,word ptr [0x895e]
0000:175b  INC word ptr [0x895e]
0000:175f  MOV AL,byte ptr [DI]
0000:1761  CBW
0000:1762  SHL AX,0x3
0000:1765  MOV SI,0x6586
0000:1768  ADD SI,AX
0000:176a  POP AX
0000:176b  POP DI
0000:176c  ROR EDX,0x10
0000:1770  AND DX,0xfff0
0000:1774  MOV dword ptr [SI],EDX
0000:1777  MOV word ptr [SI + 0x4],BX
0000:177a  PUSH SI
0000:177b  CALLF 0x0000:ffff
0000:1780  POP SI
0000:1781  AND AL,0x7
0000:1783  MOV byte ptr [SI + 0x6],AL
0000:1786  MOV AL,[0x36ee]
0000:1789  MOV byte ptr [SI + 0x7],AL
0000:178c  RETF
0000:178d  MOV byte ptr [0x36ee],0x0
0000:1792  CALLF 0x0000:ffff
0000:1797  RETF
0000:1798  MOV byte ptr [0x36ee],0x8
0000:179d  CALLF 0x0000:ffff
0000:17a2  RETF
0000:17a3  MOV byte ptr [0x36ee],0x10
0000:17a8  CALLF 0x0000:ffff
0000:17ad  RETF
0000:199d  MOV word ptr [0x8950],0x0
0000:19a3  MOV word ptr [0x89ea],0xffff
0000:19a9  CALLF 0x0000:ffff
0000:19ae  MOV dword ptr ES:[DI + 0xe],0xfffe0000
0000:19b7  MOV dword ptr ES:[DI + 0x4c],0x2000
0000:19c0  MOV dword ptr ES:[DI + 0x50],0x2000
0000:19c9  MOV dword ptr ES:[DI + 0x5c],0x18000
0000:19d2  MOV dword ptr ES:[DI + 0x60],0x40000
0000:19db  DEC word ptr [0x880a]
0000:19df  MOV word ptr [0x8822],0x0
0000:19e5  RETF
0000:19e6  PUSH DI
0000:19e7  MOV DI,word ptr [0x881a]
0000:19eb  TEST word ptr ES:[DI + 0x34],0xffff
0000:19f1  JNZ 0x0000:1a95
0000:19f5  MOV word ptr [0x612e],0x1
0000:19fb  CALLF 0x0000:ffff
0000:1a00  DEC word ptr [0x8822]
0000:1a04  JNZ 0x0000:1a73
0000:1a06  DEC word ptr [0x880a]
0000:1a0a  MOV word ptr [0x8950],0x0
0000:1a10  MOV dword ptr ES:[DI + 0xe],0xfffe0000
0000:1a19  MOV dword ptr ES:[DI + 0x4c],0x2000
0000:1a22  MOV dword ptr ES:[DI + 0x50],0x2000
0000:1a2b  MOV dword ptr ES:[DI + 0x5c],0x18000
0000:1a34  MOV dword ptr ES:[DI + 0x60],0x40000
0000:1a3d  MOV word ptr [0x89ea],0xffff
0000:1a43  CALLF 0x0000:ffff
0000:1a48  MOV dword ptr ES:[DI + 0xe],0xfffe0000
0000:1a51  MOV byte ptr ES:[DI + 0x3b],0x0
0000:1a56  MOV word ptr ES:[DI + 0x3e],0x3e8
0000:1a5c  MOV byte ptr ES:[DI + 0x37],0xff
0000:1a61  MOV byte ptr ES:[DI + 0x3a],0x0
0000:1a66  MOV byte ptr ES:[DI + 0x2b],0x0
0000:1a6b  AND word ptr [0x8950],0xffcf
0000:1a71  JMP 0x0000:1a79
0000:1a73  MOV word ptr ES:[DI + 0x34],0xd2
0000:1a79  MOV AX,word ptr ES:[DI + 0x4]
0000:1a7d  SUB AX,BX
0000:1a7f  JS 0x0000:1a8c
0000:1a81  MOV dword ptr ES:[DI + 0xa],0x18000
0000:1a8a  POP DI
0000:1a8b  RETF
0000:1a8c  MOV dword ptr ES:[DI + 0xa],0xfffe8000
0000:1a95  POP DI
0000:1a96  RETF
0000:1a97  PUSH DI
0000:1a98  MOV DI,word ptr [0x881a]
0000:1a9c  MOV word ptr ES:[DI + 0x34],0x2bc
0000:1aa2  MOV word ptr [0x8810],0xffff
0000:1aa8  POP DI

; ---- 1B77 count=192 ----
0000:1b77  MOV [0x36f4],AX
0000:1b7a  MOV word ptr [0x36f6],BX
0000:1b7e  MOV word ptr [0x36f8],CX
0000:1b82  MOV word ptr [0x36fa],DX
0000:1b86  CALLF 0x0000:ffff
0000:1b8b  PUSH BX
0000:1b8c  MOV BX,word ptr ES:[DI + 0x4]
0000:1b90  ADD BX,word ptr [0x36f4]
0000:1b94  CMP BX,CX
0000:1b96  JGE 0x0000:1bcd
0000:1b98  ADD BX,word ptr [0x36f8]
0000:1b9c  CMP BX,AX
0000:1b9e  JLE 0x0000:1bcd
0000:1ba0  POP BX
0000:1ba1  MOV AX,word ptr ES:[DI + 0x8]
0000:1ba5  ADD AX,word ptr [0x36f6]
0000:1ba9  CMP AX,DX
0000:1bab  JGE 0x0000:1bce
0000:1bad  ADD AX,word ptr [0x36fa]
0000:1bb1  CMP AX,BX
0000:1bb3  JLE 0x0000:1bce
0000:1bb5  CMP word ptr [0x8810],0x0
0000:1bba  JZ 0x0000:1bc0
0000:1bbc  MOV AX,0x1
0000:1bbf  RETF
0000:1bc0  MOV BX,word ptr ES:[DI + 0x4]
0000:1bc4  CALLF 0x0000:ffff
0000:1bc9  MOV AX,0x2
0000:1bcc  RETF
0000:1bcd  POP BX
0000:1bce  XOR AX,AX
0000:1bd0  RETF
0000:1bd1  MOV AX,word ptr ES:[DI + 0x8]
0000:1bd5  MOV BX,word ptr ES:[DI + 0x4]
0000:1bd9  ADD AX,CX
0000:1bdb  ADD BX,DX
0000:1bdd  PUSH AX
0000:1bde  PUSH BX
0000:1bdf  PUSH CX
0000:1be0  SHR AX,0x4
0000:1be3  SHR BX,0x4
0000:1be6  SHL BX,0x1
0000:1be8  MOV DX,word ptr [0x657e]
0000:1bec  MUL DX
0000:1bee  ADD BX,AX
0000:1bf0  MOV CX,BX
0000:1bf2  MOV AX,[0x657c]
0000:1bf5  MOV AX,FS
0000:1bf7  MOV BX,word ptr [0x657a]
0000:1bfb  ADD BX,CX
0000:1bfd  MOV AX,word ptr FS:[BX]
0000:1c00  AND AX,0x1ff
0000:1c03  MOV DX,word ptr [0x30d4]
0000:1c07  MUL DX
0000:1c09  MOV BX,word ptr [0x6582]
0000:1c0d  ADD BX,AX
0000:1c0f  MOV AX,[0x6584]
0000:1c12  MOV AX,FS
0000:1c14  MOV DX,word ptr FS:[BX + 0x2]
0000:1c18  POP CX
0000:1c19  POP BX
0000:1c1a  POP AX
0000:1c1b  TEST DL,0xf
0000:1c1e  JNZ 0x0000:1c22
0000:1c20  CLC
0000:1c21  RETF
0000:1c22  TEST AL,0x8
0000:1c24  JZ 0x0000:1c39
0000:1c26  TEST BL,0x8
0000:1c29  JZ 0x0000:1c32
0000:1c2b  TEST DL,0x2
0000:1c2e  JZ 0x0000:1c20
0000:1c30  STC
0000:1c31  RETF
0000:1c32  TEST DL,0x1
0000:1c35  JZ 0x0000:1c20
0000:1c37  STC
0000:1c38  RETF
0000:1c39  TEST BL,0x8
0000:1c3c  JZ 0x0000:1c46
0000:1c3e  STC
0000:1c3f  TEST DL,0x4
0000:1c42  JZ 0x0000:1c20
0000:1c44  STC
0000:1c45  RETF
0000:1c46  TEST DL,0x8
0000:1c49  JZ 0x0000:1c20
0000:1c4b  STC
0000:1c4c  RETF
0000:1c4d  MOV CX,BX
0000:1c4f  MOV BX,word ptr ES:[DI + 0x4]
0000:1c53  CMP byte ptr ES:[DI + 0x29],0x1
0000:1c58  JZ 0x0000:1c5c
0000:1c5a  NEG AX
0000:1c5c  ADD BX,AX
0000:1c5e  MOV AX,word ptr ES:[DI + 0x8]
0000:1c62  ADD AX,CX
0000:1c64  CALLF 0x0000:ffff
0000:1c69  STC
0000:1c6a  JNZ 0x0000:1c6d
0000:1c6c  CLC
0000:1c6d  RETF
0000:1c6e  SHR AX,0x4
0000:1c71  SHR BX,0x4
0000:1c74  SHL BX,0x1
0000:1c76  MOV DX,word ptr [0x657e]
0000:1c7a  MUL DX
0000:1c7c  ADD BX,AX
0000:1c7e  MOV CX,BX
0000:1c80  MOV AX,[0x657c]
0000:1c83  MOV AX,FS
0000:1c85  MOV BX,word ptr [0x657a]
0000:1c89  ADD BX,CX
0000:1c8b  MOV AX,word ptr FS:[BX]
0000:1c8e  TEST AX,0x4000
0000:1c91  RETF
0000:1e04  MOV AX,word ptr FS:[BX]
0000:1e07  CMP AX,0xffff
0000:1e0a  JZ 0x0000:1ec3
0000:1e0e  CMP AH,0x1
0000:1e11  JZ 0x0000:1ebd
0000:1e15  MOV AH,0x1
0000:1e17  MOV word ptr FS:[BX],AX
0000:1e1a  CMP AL,0x66
0000:1e1c  JZ 0x0000:1e5c
0000:1e1e  CMP AL,0x65
0000:1e20  JZ 0x0000:1e41
0000:1e22  CMP AL,0x67
0000:1e24  JNZ 0x0000:1e77
0000:1e26  MOV DX,word ptr FS:[BX + 0x2]
0000:1e2a  ADD DX,word ptr [0x3714]
0000:1e2e  SHL EDX,0x10
0000:1e32  ADD DX,word ptr [0x3716]
0000:1e36  ADD DX,word ptr FS:[BX + 0x4]
0000:1e3a  CALLF 0x0000:ffff
0000:1e3f  JMP 0x0000:1ebd
0000:1e41  MOV DX,word ptr FS:[BX + 0x2]
0000:1e45  ADD DX,word ptr [0x3714]
0000:1e49  SHL EDX,0x10
0000:1e4d  ADD DX,word ptr [0x3716]
0000:1e51  ADD DX,word ptr FS:[BX + 0x4]
0000:1e55  CALLF 0x0000:ffff
0000:1e5a  JMP 0x0000:1ebd
0000:1e5c  MOV DX,word ptr FS:[BX + 0x2]
0000:1e60  ADD DX,word ptr [0x3714]
0000:1e64  SHL EDX,0x10
0000:1e68  ADD DX,word ptr [0x3716]
0000:1e6c  ADD DX,word ptr FS:[BX + 0x4]
0000:1e70  CALLF 0x0000:ffff
0000:1e75  JMP 0x0000:1ebd
0000:1e77  XOR AH,AH
0000:1e79  SHL AX,0x2
0000:1e7c  MOV SI,AX
0000:1e7e  ADD SI,0x81d2
0000:1e82  MOV AX,word ptr [SI]
0000:1e84  MOV DX,0x0
0000:1e87  PUSH DI
0000:1e88  PUSH BX
0000:1e89  CALLF 0x0000:ffff
0000:1e8e  MOV BL,byte ptr [SI + 0x2]
0000:1e91  MOV byte ptr ES:[DI + 0x17],BL
0000:1e95  POP BX
0000:1e96  MOV word ptr ES:[DI + 0x1a],BX
0000:1e9a  MOV AX,word ptr FS:[BX + 0x4]
0000:1e9e  ADD AX,word ptr [0x3716]
0000:1ea2  SHL EAX,0x10
0000:1ea6  MOV dword ptr ES:[DI + 0x6],EAX
0000:1eab  MOV AX,word ptr FS:[BX + 0x2]
0000:1eaf  ADD AX,word ptr [0x3714]
0000:1eb3  SHL EAX,0x10
0000:1eb7  MOV dword ptr ES:[DI + 0x2],EAX
0000:1ebc  POP DI
0000:1ebd  ADD BX,0x6
0000:1ec0  JMP 0x0000:1e04
0000:1ec3  RET
0000:321f  ENTER 0xc,0x0
0000:3223  PUSH DI
0000:3224  MOV DI,word ptr [0x881a]
0000:3228  MOV EBX,dword ptr ES:[DI + 0x6]
0000:322d  MOV EAX,dword ptr ES:[DI + 0x2]
0000:3232  POP DI
0000:3233  CALLF 0x0000:ffff
0000:3238  CALLF 0x0000:ffff
0000:323d  MOV EBX,dword ptr [0x81be]
0000:3242  MOV dword ptr [BP + -0x8],EBX
0000:3246  MOV EBX,dword ptr [0x81a6]
0000:324b  MOV dword ptr [BP + -0xc],EBX
0000:324f  MOV EAX,0xffff0000
0000:3255  XOR EBX,EBX
0000:3258  CALLF 0x0000:ffff
0000:325d  ADD dword ptr [0x81be],0x2000000
0000:3266  ADD dword ptr [0x81a6],0x2000000

; ---- 393C count=192 ----
0000:393c  PUSH DI
0000:393d  MOV DI,word ptr [0x881a]
0000:3941  CMP word ptr [0x89ea],0x0
0000:3946  JNZ 0x0000:3966
0000:3948  MOV AX,word ptr ES:[DI + 0x4]
0000:394c  MOV BX,word ptr ES:[DI + 0x8]
0000:3950  MOV CX,AX
0000:3952  MOV DX,BX
0000:3954  ADD AX,word ptr ES:[DI + 0x2c]
0000:3958  ADD CX,word ptr ES:[DI + 0x30]
0000:395c  ADD BX,word ptr ES:[DI + 0x2e]
0000:3960  ADD DX,word ptr ES:[DI + 0x32]
0000:3964  POP DI
0000:3965  RETF
0000:3966  MOV AX,0x0
0000:3969  MOV DX,AX
0000:396b  MOV CX,AX
0000:396d  MOV BX,AX
0000:396f  POP DI
0000:3970  RETF
0000:3971  MOV BX,word ptr ES:[DI + 0x4]
0000:3975  MOV AX,word ptr ES:[DI + 0x8]
0000:3979  SUB AX,0xa
0000:397c  SUB AX,word ptr ES:[DI + 0x72]
0000:3980  CALLF 0x0000:ffff
0000:3985  RET
0000:3986  MOV BX,word ptr ES:[DI + 0x4]
0000:398a  MOV AX,word ptr ES:[DI + 0x8]
0000:398e  SUB AX,word ptr ES:[DI + 0x72]
0000:3992  CALLF 0x0000:ffff
0000:3997  RET
0000:3998  ENTER 0x2,0x0
0000:399c  MOV DX,0xa
0000:399f  NEG DX
0000:39a1  CMP byte ptr ES:[DI + 0x29],0x0
0000:39a6  JL 0x0000:39aa
0000:39a8  NEG DX
0000:39aa  MOV word ptr [BP + -0x2],DX
0000:39ad  CMP byte ptr ES:[DI + 0x3a],0x0
0000:39b2  JNZ 0x0000:39c9
0000:39b4  MOV BX,word ptr ES:[DI + 0x4]
0000:39b8  ADD BX,word ptr [BP + -0x2]
0000:39bb  MOV AX,word ptr ES:[DI + 0x8]
0000:39bf  SUB AX,0x1
0000:39c2  CALLF 0x0000:ffff
0000:39c7  JNZ 0x0000:39f8
0000:39c9  MOV BX,word ptr ES:[DI + 0x4]
0000:39cd  ADD BX,word ptr [BP + -0x2]
0000:39d0  MOV AX,word ptr ES:[DI + 0x8]
0000:39d4  SUB AX,0x11
0000:39d7  CALLF 0x0000:ffff
0000:39dc  JNZ 0x0000:39f8
0000:39de  CMP word ptr ES:[DI + 0x72],0x20
0000:39e3  JLE 0x0000:39fa
0000:39e5  MOV BX,word ptr ES:[DI + 0x4]
0000:39e9  ADD BX,word ptr [BP + -0x2]
0000:39ec  MOV AX,word ptr ES:[DI + 0x8]
0000:39f0  SUB AX,0x21
0000:39f3  CALLF 0x0000:ffff
0000:39f8  LEAVE
0000:39f9  RET
0000:39fa  CMP AL,AL
0000:39fc  LEAVE
0000:39fd  RET
0000:3a1f  CMP byte ptr ES:[DI + 0x38],0x0
0000:3a24  JNZ 0x0000:3a50
0000:3a26  MOV AL,0xff
0000:3a28  CMP byte ptr ES:[DI + 0x37],AL
0000:3a2c  JZ 0x0000:3a5f
0000:3a2e  MOV BX,word ptr ES:[DI + 0x4]
0000:3a32  SUB BX,0x5
0000:3a35  MOV AX,word ptr ES:[DI + 0x8]
0000:3a39  CALLF 0x0000:ffff
0000:3a3e  JNZ 0x0000:3a50
0000:3a40  MOV BX,word ptr ES:[DI + 0x4]
0000:3a44  ADD BX,0x5
0000:3a47  MOV AX,word ptr ES:[DI + 0x8]
0000:3a4b  CALLF 0x0000:ffff
0000:3a50  JNZ 0x0000:3a57
0000:3a52  MOV byte ptr ES:[DI + 0x3b],0xff
0000:3a57  JZ 0x0000:3a5e
0000:3a59  CMP byte ptr ES:[DI + 0x3b],0x0
0000:3a5e  RET
0000:3a5f  CMP BL,BL
0000:3a61  RET
0000:3a62  CMP byte ptr ES:[DI + 0x37],0x0
0000:3a67  JNZ 0x0000:3a89
0000:3a69  TEST word ptr ES:[DI],0x1
0000:3a6e  JZ 0x0000:3a89
0000:3a70  MOV byte ptr ES:[DI + 0x2a],0xff
0000:3a75  CMP byte ptr ES:[DI + 0x28],0x0
0000:3a7a  JL 0x0000:3a83
0000:3a7c  MOV word ptr ES:[DI + 0x12],0x26
0000:3a82  RET
0000:3a83  MOV word ptr ES:[DI + 0x12],0x58
0000:3a89  RET
0000:3a8a  CMP byte ptr ES:[DI + 0x37],0x0
0000:3a8f  JLE 0x0000:3ab8
0000:3a91  MOV AX,word ptr ES:[DI + 0x8]
0000:3a95  MOV BX,word ptr ES:[DI + 0x4]
0000:3a99  CALLF 0x0000:ffff
0000:3a9e  CMP AX,0xd
0000:3aa1  JZ 0x0000:3aae
0000:3aa3  CMP AX,0xc
0000:3aa6  JZ 0x0000:3aae
0000:3aa8  CMP AX,0xb
0000:3aab  JZ 0x0000:3aae
0000:3aad  RETF
0000:3aae  CALLF 0x0000:ffff
0000:3ab3  CALLF 0x0000:ffff
0000:3ab8  RETF
0000:3ab9  TEST word ptr ES:[DI],0x4
0000:3abe  JZ 0x0000:3ac8
0000:3ac0  MOV AL,0x1
0000:3ac2  MOV byte ptr ES:[DI + 0x28],AL
0000:3ac6  JMP 0x0000:3ad5
0000:3ac8  TEST word ptr ES:[DI],0x8
0000:3acd  JZ 0x0000:3ad5
0000:3acf  MOV AL,0xff
0000:3ad1  MOV byte ptr ES:[DI + 0x28],AL
0000:3ad5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:3ada  ADD dword ptr ES:[DI + 0x2],EBX
0000:3adf  ADD EBX,dword ptr [0x8816]
0000:3ae4  CMP EBX,0x1
0000:3ae8  JZ 0x0000:3b06
0000:3aea  CMP EBX,0x0
0000:3aee  JZ 0x0000:3b06
0000:3af0  JL 0x0000:3afe
0000:3af2  MOV EAX,0x1
0000:3af8  MOV byte ptr ES:[DI + 0x29],AL
0000:3afc  JMP 0x0000:3b10
0000:3afe  MOV AL,0xff
0000:3b00  MOV byte ptr ES:[DI + 0x29],AL
0000:3b04  JMP 0x0000:3b10
0000:3b06  MOV BL,byte ptr ES:[DI + 0x28]
0000:3b0a  MOV byte ptr ES:[DI + 0x29],BL
0000:3b0e  MOV AL,BL
0000:3b10  CALL 0x0000:3998
0000:3b13  JZ 0x0000:3b44
0000:3b15  XOR EAX,EAX
0000:3b18  MOV dword ptr ES:[DI + 0xa],EAX
0000:3b1d  CMP byte ptr ES:[DI + 0x36],0x0
0000:3b22  JNZ 0x0000:3b3f
0000:3b24  CMP byte ptr ES:[DI + 0x37],0x0
0000:3b29  JNZ 0x0000:3b3f
0000:3b2b  MOV AL,0x1
0000:3b2d  MOV byte ptr ES:[DI + 0x36],AL
0000:3b31  SUB AL,0x1
0000:3b33  MOV byte ptr ES:[DI + 0x13],AL
0000:3b37  MOV SI,0x3156
0000:3b3a  CALLF 0x0000:ffff
0000:3b3f  CMP BL,BL
0000:3b41  JMP 0x0000:3c6a
0000:3b44  MOV EAX,[0x8816]
0000:3b48  ADD dword ptr ES:[DI + 0x2],EAX
0000:3b4d  XOR EAX,EAX
0000:3b50  MOV [0x8816],EAX
0000:3b54  CMP byte ptr ES:[DI + 0x3a],0x0
0000:3b59  JZ 0x0000:3b77
0000:3b5b  JL 0x0000:3b6b
0000:3b5d  CMP byte ptr ES:[DI + 0x29],0x0
0000:3b62  JGE 0x0000:3b77
0000:3b64  ADD word ptr ES:[DI + 0x8],0x2
0000:3b69  JMP 0x0000:3b77
0000:3b6b  CMP byte ptr ES:[DI + 0x29],0x0
0000:3b70  JL 0x0000:3b77
0000:3b72  ADD word ptr ES:[DI + 0x8],0x2
0000:3b77  TEST word ptr ES:[DI],0x4
0000:3b7c  MOV EAX,0x0
0000:3b82  JZ 0x0000:3bbe
0000:3b84  MOV EBX,dword ptr ES:[DI + 0xa]
0000:3b89  ADD EBX,dword ptr ES:[DI + 0x4c]
0000:3b8e  CMP EBX,dword ptr ES:[DI + 0x5c]
0000:3b93  JLE 0x0000:3b9a
0000:3b95  MOV EBX,dword ptr ES:[DI + 0x5c]
0000:3b9a  MOV dword ptr ES:[DI + 0xa],EBX
0000:3b9f  CMP byte ptr ES:[DI + 0x36],AL
0000:3ba3  JZ 0x0000:3c6a
0000:3ba7  CMP byte ptr ES:[DI + 0x37],AL
0000:3bab  JNZ 0x0000:3c6a
0000:3baf  MOV byte ptr ES:[DI + 0x36],AL
0000:3bb3  MOV SI,0x3142
0000:3bb6  CALLF 0x0000:ffff
0000:3bbb  JMP 0x0000:3c6a
0000:3bbe  TEST word ptr ES:[DI],0x8
0000:3bc3  JZ 0x0000:3c07
0000:3bc5  MOV EBX,dword ptr ES:[DI + 0x4c]
0000:3bca  MOV EAX,dword ptr ES:[DI + 0xa]
0000:3bcf  SUB EAX,EBX
0000:3bd2  MOV EBX,dword ptr ES:[DI + 0x5c]
0000:3bd7  NEG EBX
0000:3bda  CMP EBX,EAX

; ---- 44FF count=64 ----
0000:44ff  MOV word ptr [0x8808],0x4
0000:4505  XOR EAX,EAX
0000:4508  MOV [0x8806],AX
0000:450b  MOV CX,0xa
0000:450e  MOV BX,DS
0000:4510  MOV BX,ES
0000:4512  MOV DI,0x87de
0000:4515  STOSD.REP ES:DI
0000:4518  RETF
0000:45ab  MOV BX,word ptr ES:[DI + 0x2a]
0000:45af  CMP word ptr [BX + 0x87de],0x0
0000:45b4  JNZ 0x0000:45bc
0000:45b6  MOV word ptr ES:[DI + 0x18],0x470c
0000:45bc  MOV AX,word ptr ES:[DI + 0x4]
0000:45c0  SUB AX,word ptr [0x81c0]
0000:45c4  ADD AX,0x10
0000:45c7  CMP AX,0x160
0000:45ca  JA 0x0000:45df
0000:45cc  MOV AX,word ptr ES:[DI + 0x8]
0000:45d0  SUB AX,word ptr [0x81c4]
0000:45d4  ADD AX,0x10
0000:45d7  CMP AX,0xd0
0000:45da  JA 0x0000:45df
0000:45dc  CLC
0000:45dd  JMP 0x0000:45e7
0000:45df  STC
0000:45e0  MOV word ptr ES:[DI + 0x18],0x470c
0000:45e6  RET
0000:45e7  CMP dword ptr ES:[DI + 0xe],0x0
0000:45ed  JLE 0x0000:4670
0000:45f1  CMP word ptr ES:[DI + 0x2c],0x2
0000:45f6  JZ 0x0000:4681
0000:45fa  MOV CX,0x0
0000:45fd  MOV DX,0x0
0000:4600  CALLF 0x0000:ffff
0000:4605  JNC 0x0000:4609
0000:4607  JMP 0x0000:465e
0000:4609  CMP byte ptr ES:[DI + 0x29],0x0
0000:460e  JG 0x0000:4635
0000:4610  MOV AX,word ptr ES:[DI + 0x8]
0000:4614  MOV BX,word ptr ES:[DI + 0x4]
0000:4618  CALLF 0x0000:ffff
0000:461d  TEST DL,0x70
0000:4620  JNZ 0x0000:465a
0000:4622  MOV AX,word ptr ES:[DI + 0x8]
0000:4626  SUB AX,0x10
0000:4629  CALLF 0x0000:ffff
0000:462e  TEST DL,0x70
0000:4631  JNZ 0x0000:465a
0000:4633  JMP 0x0000:465c
0000:4635  MOV AX,word ptr ES:[DI + 0x8]
0000:4639  MOV BX,word ptr ES:[DI + 0x4]
0000:463d  CALLF 0x0000:ffff
0000:4642  TEST DL,0x70
0000:4645  JNZ 0x0000:465a
0000:4647  MOV AX,word ptr ES:[DI + 0x8]
0000:464b  SUB AX,0x10
0000:464e  CALLF 0x0000:ffff
0000:4653  TEST DL,0x70
0000:4656  JNZ 0x0000:465a
0000:4658  JMP 0x0000:465c
0000:465a  JMP 0x0000:465e
0000:465c  JMP 0x0000:4670
0000:465e  NEG dword ptr ES:[DI + 0xe]

; ---- 4519 count=128 ----
0000:4519  CMP byte ptr [0x88ae],0x0
0000:451e  JG 0x0000:452e
0000:4520  CMP word ptr [0x880c],0x0
0000:4525  JG 0x0000:452e
0000:4527  MOV word ptr ES:[DI + 0x18],0x0
0000:452d  RET
0000:452e  MOV AX,[0x8808]
0000:4531  CMP word ptr [0x8806],AX
0000:4535  JL 0x0000:453e
0000:4537  MOV word ptr ES:[DI + 0x18],0x0
0000:453d  RET
0000:453e  CMP byte ptr [0x88ae],0x0
0000:4543  JG 0x0000:4549
0000:4545  DEC word ptr [0x880c]
0000:4549  INC word ptr [0x8806]
0000:454d  MOV SI,0x333a
0000:4550  CALLF 0x0000:ffff
0000:4555  MOV word ptr ES:[DI + 0x18],0x45ab
0000:455b  MOV word ptr ES:[DI + 0x2c],0x0
0000:4561  MOV dword ptr ES:[DI + 0x2e],0x30000
0000:456a  MOV dword ptr ES:[DI + 0xa],0x0
0000:4573  MOV dword ptr ES:[DI + 0xe],0xfffeb000
0000:457c  MOV word ptr [0x612e],0x8
0000:4582  CALLF 0x0000:ffff
0000:4587  SUB word ptr ES:[DI + 0x8],0xf
0000:458c  XOR BX,BX
0000:458e  MOV AX,word ptr [BX + 0x87de]
0000:4592  ADD AX,word ptr [BX + 0x87e0]
0000:4596  CMP AX,0x0
0000:4599  JZ 0x0000:45a0
0000:459b  ADD BX,0x4
0000:459e  JMP 0x0000:458e
0000:45a0  MOV word ptr ES:[DI + 0x2a],BX
0000:45a4  MOV word ptr [BX + 0x87de],0x1
0000:45aa  RET
0000:45ab  MOV BX,word ptr ES:[DI + 0x2a]
0000:45af  CMP word ptr [BX + 0x87de],0x0
0000:45b4  JNZ 0x0000:45bc
0000:45b6  MOV word ptr ES:[DI + 0x18],0x470c
0000:45bc  MOV AX,word ptr ES:[DI + 0x4]
0000:45c0  SUB AX,word ptr [0x81c0]
0000:45c4  ADD AX,0x10
0000:45c7  CMP AX,0x160
0000:45ca  JA 0x0000:45df
0000:45cc  MOV AX,word ptr ES:[DI + 0x8]
0000:45d0  SUB AX,word ptr [0x81c4]
0000:45d4  ADD AX,0x10
0000:45d7  CMP AX,0xd0
0000:45da  JA 0x0000:45df
0000:45dc  CLC
0000:45dd  JMP 0x0000:45e7
0000:45df  STC
0000:45e0  MOV word ptr ES:[DI + 0x18],0x470c
0000:45e6  RET
0000:45e7  CMP dword ptr ES:[DI + 0xe],0x0
0000:45ed  JLE 0x0000:4670
0000:45f1  CMP word ptr ES:[DI + 0x2c],0x2
0000:45f6  JZ 0x0000:4681
0000:45fa  MOV CX,0x0
0000:45fd  MOV DX,0x0
0000:4600  CALLF 0x0000:ffff
0000:4605  JNC 0x0000:4609
0000:4607  JMP 0x0000:465e
0000:4609  CMP byte ptr ES:[DI + 0x29],0x0
0000:460e  JG 0x0000:4635
0000:4610  MOV AX,word ptr ES:[DI + 0x8]
0000:4614  MOV BX,word ptr ES:[DI + 0x4]
0000:4618  CALLF 0x0000:ffff
0000:461d  TEST DL,0x70
0000:4620  JNZ 0x0000:465a
0000:4622  MOV AX,word ptr ES:[DI + 0x8]
0000:4626  SUB AX,0x10
0000:4629  CALLF 0x0000:ffff
0000:462e  TEST DL,0x70
0000:4631  JNZ 0x0000:465a
0000:4633  JMP 0x0000:465c
0000:4635  MOV AX,word ptr ES:[DI + 0x8]
0000:4639  MOV BX,word ptr ES:[DI + 0x4]
0000:463d  CALLF 0x0000:ffff
0000:4642  TEST DL,0x70
0000:4645  JNZ 0x0000:465a
0000:4647  MOV AX,word ptr ES:[DI + 0x8]
0000:464b  SUB AX,0x10
0000:464e  CALLF 0x0000:ffff
0000:4653  TEST DL,0x70
0000:4656  JNZ 0x0000:465a
0000:4658  JMP 0x0000:465c
0000:465a  JMP 0x0000:465e
0000:465c  JMP 0x0000:4670
0000:465e  NEG dword ptr ES:[DI + 0xe]
0000:4663  INC word ptr ES:[DI + 0x2c]
0000:4667  SUB dword ptr ES:[DI + 0x2e],0x5000
0000:4670  MOV BX,0xfffb
0000:4673  MOV AX,0x5
0000:4676  CALLF 0x0000:ffff
0000:467b  JNC 0x0000:4681
0000:467d  NEG byte ptr ES:[DI + 0x29]
0000:4681  MOV EAX,dword ptr ES:[DI + 0xe]
0000:4686  CMP EAX,0x0
0000:468a  JG 0x0000:46b5
0000:468c  ADD EAX,0x2ee0
0000:4692  CMP EAX,0xfffd0000
0000:4698  JL 0x0000:46ac
0000:469a  CMP EAX,dword ptr ES:[DI + 0x2e]
0000:469f  JG 0x0000:46a4
0000:46a1  CLC
0000:46a2  JMP 0x0000:46b3
0000:46a4  MOV EAX,dword ptr ES:[DI + 0x2e]
0000:46a9  STC
0000:46aa  JMP 0x0000:46b3
0000:46ac  MOV EAX,0xfffd0000
0000:46b2  STC
0000:46b3  JMP 0x0000:46dc
0000:46b5  ADD EAX,0x36b0
0000:46bb  CMP EAX,0xfffd0000
0000:46c1  JL 0x0000:46d5
0000:46c3  CMP EAX,dword ptr ES:[DI + 0x2e]
0000:46c8  JG 0x0000:46cd
0000:46ca  CLC
0000:46cb  JMP 0x0000:46dc
0000:46cd  MOV EAX,dword ptr ES:[DI + 0x2e]
0000:46d2  STC
0000:46d3  JMP 0x0000:46dc
0000:46d5  MOV EAX,0xfffd0000
0000:46db  STC
0000:46dc  MOV dword ptr ES:[DI + 0xe],EAX
0000:46e1  ADD dword ptr ES:[DI + 0x6],EAX
0000:46e6  MOV AL,byte ptr ES:[DI + 0x29]

; ---- 45AB count=256 ----
0000:45ab  MOV BX,word ptr ES:[DI + 0x2a]
0000:45af  CMP word ptr [BX + 0x87de],0x0
0000:45b4  JNZ 0x0000:45bc
0000:45b6  MOV word ptr ES:[DI + 0x18],0x470c
0000:45bc  MOV AX,word ptr ES:[DI + 0x4]
0000:45c0  SUB AX,word ptr [0x81c0]
0000:45c4  ADD AX,0x10
0000:45c7  CMP AX,0x160
0000:45ca  JA 0x0000:45df
0000:45cc  MOV AX,word ptr ES:[DI + 0x8]
0000:45d0  SUB AX,word ptr [0x81c4]
0000:45d4  ADD AX,0x10
0000:45d7  CMP AX,0xd0
0000:45da  JA 0x0000:45df
0000:45dc  CLC
0000:45dd  JMP 0x0000:45e7
0000:45df  STC
0000:45e0  MOV word ptr ES:[DI + 0x18],0x470c
0000:45e6  RET
0000:45e7  CMP dword ptr ES:[DI + 0xe],0x0
0000:45ed  JLE 0x0000:4670
0000:45f1  CMP word ptr ES:[DI + 0x2c],0x2
0000:45f6  JZ 0x0000:4681
0000:45fa  MOV CX,0x0
0000:45fd  MOV DX,0x0
0000:4600  CALLF 0x0000:ffff
0000:4605  JNC 0x0000:4609
0000:4607  JMP 0x0000:465e
0000:4609  CMP byte ptr ES:[DI + 0x29],0x0
0000:460e  JG 0x0000:4635
0000:4610  MOV AX,word ptr ES:[DI + 0x8]
0000:4614  MOV BX,word ptr ES:[DI + 0x4]
0000:4618  CALLF 0x0000:ffff
0000:461d  TEST DL,0x70
0000:4620  JNZ 0x0000:465a
0000:4622  MOV AX,word ptr ES:[DI + 0x8]
0000:4626  SUB AX,0x10
0000:4629  CALLF 0x0000:ffff
0000:462e  TEST DL,0x70
0000:4631  JNZ 0x0000:465a
0000:4633  JMP 0x0000:465c
0000:4635  MOV AX,word ptr ES:[DI + 0x8]
0000:4639  MOV BX,word ptr ES:[DI + 0x4]
0000:463d  CALLF 0x0000:ffff
0000:4642  TEST DL,0x70
0000:4645  JNZ 0x0000:465a
0000:4647  MOV AX,word ptr ES:[DI + 0x8]
0000:464b  SUB AX,0x10
0000:464e  CALLF 0x0000:ffff
0000:4653  TEST DL,0x70
0000:4656  JNZ 0x0000:465a
0000:4658  JMP 0x0000:465c
0000:465a  JMP 0x0000:465e
0000:465c  JMP 0x0000:4670
0000:465e  NEG dword ptr ES:[DI + 0xe]
0000:4663  INC word ptr ES:[DI + 0x2c]
0000:4667  SUB dword ptr ES:[DI + 0x2e],0x5000
0000:4670  MOV BX,0xfffb
0000:4673  MOV AX,0x5
0000:4676  CALLF 0x0000:ffff
0000:467b  JNC 0x0000:4681
0000:467d  NEG byte ptr ES:[DI + 0x29]
0000:4681  MOV EAX,dword ptr ES:[DI + 0xe]
0000:4686  CMP EAX,0x0
0000:468a  JG 0x0000:46b5
0000:468c  ADD EAX,0x2ee0
0000:4692  CMP EAX,0xfffd0000
0000:4698  JL 0x0000:46ac
0000:469a  CMP EAX,dword ptr ES:[DI + 0x2e]
0000:469f  JG 0x0000:46a4
0000:46a1  CLC
0000:46a2  JMP 0x0000:46b3
0000:46a4  MOV EAX,dword ptr ES:[DI + 0x2e]
0000:46a9  STC
0000:46aa  JMP 0x0000:46b3
0000:46ac  MOV EAX,0xfffd0000
0000:46b2  STC
0000:46b3  JMP 0x0000:46dc
0000:46b5  ADD EAX,0x36b0
0000:46bb  CMP EAX,0xfffd0000
0000:46c1  JL 0x0000:46d5
0000:46c3  CMP EAX,dword ptr ES:[DI + 0x2e]
0000:46c8  JG 0x0000:46cd
0000:46ca  CLC
0000:46cb  JMP 0x0000:46dc
0000:46cd  MOV EAX,dword ptr ES:[DI + 0x2e]
0000:46d2  STC
0000:46d3  JMP 0x0000:46dc
0000:46d5  MOV EAX,0xfffd0000
0000:46db  STC
0000:46dc  MOV dword ptr ES:[DI + 0xe],EAX
0000:46e1  ADD dword ptr ES:[DI + 0x6],EAX
0000:46e6  MOV AL,byte ptr ES:[DI + 0x29]
0000:46ea  CBW
0000:46eb  SHL AX,0x2
0000:46ee  ADD word ptr ES:[DI + 0x4],AX
0000:46f2  MOV BX,word ptr ES:[DI + 0x2a]
0000:46f6  MOV AX,word ptr ES:[DI + 0x4]
0000:46fa  MOV word ptr [BX + 0x87de],AX
0000:46fe  MOV AX,word ptr ES:[DI + 0x8]
0000:4702  MOV word ptr [BX + 0x87e0],AX
0000:4706  CALLF 0x0000:ffff
0000:470b  RET
0000:487f  MOV SI,0x32ec
0000:4882  CALLF 0x0000:ffff
0000:4887  MOV word ptr ES:[DI + 0x18],0x489c
0000:488d  MOV byte ptr ES:[DI + 0x2a],0x0
0000:4892  MOV dword ptr ES:[DI + 0xe],0x11000
0000:489b  RET
0000:489c  CMP byte ptr ES:[DI + 0x2a],0x1
0000:48a1  JGE 0x0000:492d
0000:48a5  MOV EAX,dword ptr ES:[DI + 0xe]
0000:48aa  CMP EAX,0x3000
0000:48b0  JLE 0x0000:48b8
0000:48b2  SUB EAX,0x12c
0000:48b8  ADD dword ptr ES:[DI + 0x6],EAX
0000:48bd  MOV dword ptr ES:[DI + 0xe],EAX
0000:48c2  MOV CX,0x0
0000:48c5  MOV DX,0x0
0000:48c8  CALLF 0x0000:ffff
0000:48cd  JNC 0x0000:48d4
0000:48cf  MOV byte ptr ES:[DI + 0x2a],0x1
0000:48d4  CMP byte ptr ES:[DI + 0x29],0x0
0000:48d9  JG 0x0000:4900
0000:48db  MOV AX,word ptr ES:[DI + 0x8]
0000:48df  MOV BX,word ptr ES:[DI + 0x4]
0000:48e3  CALLF 0x0000:ffff
0000:48e8  TEST DL,0x70
0000:48eb  JNZ 0x0000:4925
0000:48ed  MOV AX,word ptr ES:[DI + 0x8]
0000:48f1  SUB AX,0x10
0000:48f4  CALLF 0x0000:ffff
0000:48f9  TEST DL,0x70
0000:48fc  JNZ 0x0000:4925
0000:48fe  JMP 0x0000:492a
0000:4900  MOV AX,word ptr ES:[DI + 0x8]
0000:4904  MOV BX,word ptr ES:[DI + 0x4]
0000:4908  CALLF 0x0000:ffff
0000:490d  TEST DL,0x70
0000:4910  JNZ 0x0000:4925
0000:4912  MOV AX,word ptr ES:[DI + 0x8]
0000:4916  SUB AX,0x10
0000:4919  CALLF 0x0000:ffff
0000:491e  TEST DL,0x70
0000:4921  JNZ 0x0000:4925
0000:4923  JMP 0x0000:492a
0000:4925  MOV byte ptr ES:[DI + 0x2a],0x1
0000:492a  JMP 0x0000:49f2
0000:492d  CMP byte ptr ES:[DI + 0x2a],0x2
0000:4932  JGE 0x0000:49eb
0000:4936  CALLF 0x0000:ffff
0000:493b  PUSH BX
0000:493c  MOV BX,word ptr ES:[DI + 0x4]
0000:4940  CMP BX,CX
0000:4942  JGE 0x0000:49f8
0000:4946  ADD BX,0x8
0000:4949  CMP BX,AX
0000:494b  JLE 0x0000:49f8
0000:494f  POP BX
0000:4950  MOV AX,word ptr ES:[DI + 0x8]
0000:4954  SUB AX,0x23
0000:4957  AND AL,0xf0
0000:4959  CMP AX,DX
0000:495b  JGE 0x0000:49f2
0000:495f  ADD AX,0x23
0000:4962  CMP AX,BX
0000:4964  JLE 0x0000:49f2
0000:4968  MOV word ptr [0x612e],0xc
0000:496e  CALLF 0x0000:ffff
0000:4973  ADD dword ptr [0x881c],0x1388
0000:497c  CMP word ptr [0x85d8],0x1
0000:4981  JZ 0x0000:499e
0000:4983  CMP word ptr [0x85d8],0x3
0000:4988  JZ 0x0000:499e
0000:498a  CMP word ptr [0x85d8],0x5
0000:498f  JZ 0x0000:499e
0000:4991  MOV byte ptr ES:[DI + 0x2a],0x2
0000:4996  MOV word ptr [0x89e6],0xffff
0000:499c  JMP 0x0000:49f2
0000:499e  PUSH DI
0000:499f  MOV AX,0x49ff
0000:49a2  XOR DX,DX
0000:49a4  CALLF 0x0000:ffff
0000:49a9  POP SI
0000:49aa  MOV byte ptr ES:[DI + 0x17],0x1
0000:49af  MOV BX,word ptr ES:[SI + 0x4]
0000:49b3  MOV word ptr ES:[DI + 0x4],BX
0000:49b7  MOV BX,word ptr ES:[SI + 0x8]
0000:49bb  MOV word ptr ES:[DI + 0x8],BX
0000:49bf  MOV DI,SI
0000:49c1  PUSH DI
0000:49c2  MOV AX,0x92f2
0000:49c5  XOR DX,DX
0000:49c7  CALLF 0x0000:ffff
0000:49cc  POP SI
0000:49cd  MOV byte ptr ES:[DI + 0x17],0x2
0000:49d2  MOV BX,word ptr ES:[SI + 0x4]
0000:49d6  MOV word ptr ES:[DI + 0x4],BX
0000:49da  MOV BX,word ptr ES:[SI + 0x8]
0000:49de  MOV word ptr ES:[DI + 0x8],BX
0000:49e2  MOV DI,SI
0000:49e4  MOV byte ptr ES:[DI + 0x2a],0x2
0000:49e9  JMP 0x0000:49f2
0000:49eb  MOV word ptr ES:[DI + 0x18],0x0
0000:49f1  RET
0000:49f2  CALLF 0x0000:ffff
0000:49f7  RET
0000:49f8  CALLF 0x0000:ffff
0000:49fd  POP BX
0000:49fe  RET
0000:49ff  MOV word ptr ES:[DI + 0x12],0x3a2
0000:4a05  MOV word ptr ES:[DI + 0x18],0x4a5e
0000:4a0b  CMP word ptr [0x85d4],0xe
0000:4a10  JNZ 0x0000:4a17
0000:4a12  MOV byte ptr [0x85da],0x32
0000:4a17  PUSH DI
0000:4a18  MOV AX,0x9614
0000:4a1b  XOR DX,DX
0000:4a1d  CALLF 0x0000:ffff
0000:4a22  POP SI
0000:4a23  MOV byte ptr ES:[DI + 0x17],0x2
0000:4a28  MOV BX,word ptr ES:[SI + 0x4]
0000:4a2c  MOV word ptr ES:[DI + 0x4],BX
0000:4a30  MOV BX,word ptr ES:[SI + 0x8]
0000:4a34  MOV word ptr ES:[DI + 0x8],BX
0000:4a38  MOV DI,SI
0000:4a3a  PUSH DI
0000:4a3b  MOV AX,0x991a
0000:4a3e  XOR DX,DX
0000:4a40  CALLF 0x0000:ffff
0000:4a45  POP SI
0000:4a46  MOV byte ptr ES:[DI + 0x17],0x2
0000:4a4b  MOV BX,word ptr ES:[SI + 0x4]
0000:4a4f  MOV word ptr ES:[DI + 0x4],BX
0000:4a53  MOV BX,word ptr ES:[SI + 0x8]
0000:4a57  MOV word ptr ES:[DI + 0x8],BX
0000:4a5b  MOV DI,SI
0000:4a5d  RET
0000:4a5e  CMP word ptr [0x85d4],0xe
0000:4a63  JNZ 0x0000:4aa0
0000:4a65  CMP byte ptr [0x85da],0x32
0000:4a6a  JNZ 0x0000:4a9b
0000:4a6c  INC word ptr ES:[DI + 0x2c]
0000:4a70  CMP word ptr ES:[DI + 0x2c],0xfa
0000:4a76  JLE 0x0000:4ab2
0000:4a78  CMP byte ptr [0x85da],0x33
0000:4a7d  JGE 0x0000:4a9b
0000:4a7f  MOV word ptr ES:[DI + 0x2c],0x0
0000:4a85  MOV AX,0x48
0000:4a88  MOV BX,0x2d
0000:4a8b  MOV CX,0x15d
0000:4a8e  CALLF 0x0000:ffff
0000:4a93  MOV word ptr [0x89ec],0xffff
0000:4a99  JMP 0x0000:4ab2
0000:4a9b  MOV byte ptr [0x85da],0x0
0000:4aa0  INC word ptr ES:[DI + 0x2c]

; ---- 470C count=64 ----
0000:470c  DEC word ptr [0x8806]
0000:4710  MOV BX,word ptr ES:[DI + 0x2a]
0000:4714  MOV word ptr [BX + 0x87de],0x0
0000:471a  MOV word ptr [BX + 0x87e0],0x0
0000:4720  MOV word ptr ES:[DI + 0x18],0x0
0000:4726  RET
0000:487f  MOV SI,0x32ec
0000:4882  CALLF 0x0000:ffff
0000:4887  MOV word ptr ES:[DI + 0x18],0x489c
0000:488d  MOV byte ptr ES:[DI + 0x2a],0x0
0000:4892  MOV dword ptr ES:[DI + 0xe],0x11000
0000:489b  RET
0000:489c  CMP byte ptr ES:[DI + 0x2a],0x1
0000:48a1  JGE 0x0000:492d
0000:48a5  MOV EAX,dword ptr ES:[DI + 0xe]
0000:48aa  CMP EAX,0x3000
0000:48b0  JLE 0x0000:48b8
0000:48b2  SUB EAX,0x12c
0000:48b8  ADD dword ptr ES:[DI + 0x6],EAX
0000:48bd  MOV dword ptr ES:[DI + 0xe],EAX
0000:48c2  MOV CX,0x0
0000:48c5  MOV DX,0x0
0000:48c8  CALLF 0x0000:ffff
0000:48cd  JNC 0x0000:48d4
0000:48cf  MOV byte ptr ES:[DI + 0x2a],0x1
0000:48d4  CMP byte ptr ES:[DI + 0x29],0x0
0000:48d9  JG 0x0000:4900
0000:48db  MOV AX,word ptr ES:[DI + 0x8]
0000:48df  MOV BX,word ptr ES:[DI + 0x4]
0000:48e3  CALLF 0x0000:ffff
0000:48e8  TEST DL,0x70
0000:48eb  JNZ 0x0000:4925
0000:48ed  MOV AX,word ptr ES:[DI + 0x8]
0000:48f1  SUB AX,0x10
0000:48f4  CALLF 0x0000:ffff
0000:48f9  TEST DL,0x70
0000:48fc  JNZ 0x0000:4925
0000:48fe  JMP 0x0000:492a
0000:4900  MOV AX,word ptr ES:[DI + 0x8]
0000:4904  MOV BX,word ptr ES:[DI + 0x4]
0000:4908  CALLF 0x0000:ffff
0000:490d  TEST DL,0x70
0000:4910  JNZ 0x0000:4925
0000:4912  MOV AX,word ptr ES:[DI + 0x8]
0000:4916  SUB AX,0x10
0000:4919  CALLF 0x0000:ffff
0000:491e  TEST DL,0x70
0000:4921  JNZ 0x0000:4925
0000:4923  JMP 0x0000:492a
0000:4925  MOV byte ptr ES:[DI + 0x2a],0x1
0000:492a  JMP 0x0000:49f2
0000:492d  CMP byte ptr ES:[DI + 0x2a],0x2
0000:4932  JGE 0x0000:49eb
0000:4936  CALLF 0x0000:ffff
0000:493b  PUSH BX
0000:493c  MOV BX,word ptr ES:[DI + 0x4]
0000:4940  CMP BX,CX
0000:4942  JGE 0x0000:49f8
0000:4946  ADD BX,0x8
0000:4949  CMP BX,AX
0000:494b  JLE 0x0000:49f8
0000:494f  POP BX
0000:4950  MOV AX,word ptr ES:[DI + 0x8]
0000:4954  SUB AX,0x23

; ---- 4C74 count=256 ----
0000:4c74  INC word ptr ES:[DI + 0x2a]
0000:4c78  CMP word ptr ES:[DI + 0x2a],0x1e
0000:4c7d  JLE 0x0000:4c85
0000:4c7f  MOV word ptr ES:[DI + 0x18],0x0
0000:4c85  CALLF 0x0000:ffff
0000:4c8a  RET
0000:5c11  MOV SI,0x646c
0000:5c14  ADD SI,word ptr [0x6468]
0000:5c18  INC word ptr [0x6468]
0000:5c1c  AND word ptr [0x6468],0xff
0000:5c22  MOV AL,byte ptr [SI]
0000:5c24  CBW
0000:5c25  RETF
0000:5c27  PUSH AX
0000:5c28  TEST AX,0xffff
0000:5c2b  PUSH BX
0000:5c2c  PUSH CX
0000:5c2d  MOV DX,word ptr [0x657e]
0000:5c31  SHR BX,0x3
0000:5c34  AND BX,0xfffe
0000:5c38  SHR AX,0x4
0000:5c3b  MUL DX
0000:5c3d  ADD BX,AX
0000:5c3f  MOV CX,BX
0000:5c41  MOV AX,[0x657c]
0000:5c44  MOV BX,word ptr [0x657a]
0000:5c48  ADD BX,CX
0000:5c4a  MOV AX,FS
0000:5c4c  MOV AX,word ptr FS:[BX]
0000:5c4f  AND AH,0x1
0000:5c52  MOV DX,word ptr [0x30d4]
0000:5c56  MUL DX
0000:5c58  MOV BX,word ptr [0x6582]
0000:5c5c  ADD BX,AX
0000:5c5e  MOV AX,[0x6584]
0000:5c61  MOV AX,FS
0000:5c63  ADD BX,0x2
0000:5c66  MOV DX,word ptr FS:[BX]
0000:5c69  POP CX
0000:5c6a  POP BX
0000:5c6b  POP AX
0000:5c6c  TEST DX,0xf
0000:5c70  JNZ 0x0000:5c74
0000:5c72  RETF
0000:5c74  TEST AX,0x8
0000:5c77  JZ 0x0000:5c8b
0000:5c79  TEST BX,0x8
0000:5c7d  JZ 0x0000:5c85
0000:5c7f  TEST DX,0x2
0000:5c83  RETF
0000:5c85  TEST DX,0x1
0000:5c89  RETF
0000:5c8b  TEST BX,0x8
0000:5c8f  JZ 0x0000:5c97
0000:5c91  TEST DX,0x4
0000:5c95  RETF
0000:5c97  TEST DX,0x8
0000:5c9b  RETF
0000:5d38  ADD SI,0x2
0000:5d3b  MOV AX,word ptr [SI + -0x2]
0000:5d3e  MOV word ptr ES:[DI + 0x1e],AX
0000:5d42  MOV word ptr ES:[DI + 0x20],AX
0000:5d46  MOV word ptr ES:[DI + 0x22],SI
0000:5d4a  MOV word ptr ES:[DI + 0x24],SI
0000:5d4e  MOV AX,word ptr [SI]
0000:5d50  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d55  JNZ 0x0000:5d5a
0000:5d57  ADD AX,0x32
0000:5d5a  MOV word ptr ES:[DI + 0x12],AX
0000:5d5e  RETF
0000:5d60  CMP word ptr ES:[DI + 0x20],0x0
0000:5d65  JZ 0x0000:5d6d
0000:5d67  DEC word ptr ES:[DI + 0x20]
0000:5d6b  RETF
0000:5d6d  ADD word ptr ES:[DI + 0x24],0x2
0000:5d72  MOV SI,word ptr ES:[DI + 0x24]
0000:5d76  MOV AX,word ptr [SI]
0000:5d78  CMP AX,0x0
0000:5d7b  JGE 0x0000:5d89
0000:5d7d  NEG AX
0000:5d7f  ADD AX,AX
0000:5d81  SUB SI,AX
0000:5d83  SUB word ptr ES:[DI + 0x24],AX
0000:5d87  JMP 0x0000:5d76
0000:5d89  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d8e  JNZ 0x0000:5d93
0000:5d90  ADD AX,0x32
0000:5d93  MOV word ptr ES:[DI + 0x12],AX
0000:5d97  MOV AX,word ptr ES:[DI + 0x1e]
0000:5d9b  MOV word ptr ES:[DI + 0x20],AX
0000:5d9f  RETF
0000:6616  MOV SI,0x34d4
0000:6619  CALLF 0x0000:ffff
0000:661e  MOV word ptr ES:[DI + 0x18],0x66e1
0000:6624  MOV word ptr ES:[DI + 0x2a],0x14
0000:662a  MOV AL,byte ptr ES:[DI + 0x29]
0000:662e  MOV byte ptr ES:[DI + 0x28],AL
0000:6632  MOV dword ptr ES:[DI + 0xa],0xfffeb000
0000:663b  CMP AL,0x0
0000:663d  JL 0x0000:6644
0000:663f  NEG dword ptr ES:[DI + 0xa]
0000:6644  MOV word ptr ES:[DI + 0x30],0x0
0000:664a  MOV word ptr ES:[DI + 0x32],0x0
0000:6650  RET
0000:92a9  MOV word ptr [0x89e6],0xffff
0000:92af  POP DI
0000:92b0  RET
0000:92b3  MOV SI,0x30e2
0000:92b6  CALLF 0x0000:ffff
0000:92bb  MOV word ptr ES:[DI + 0x18],0x9313
0000:92c1  MOV word ptr ES:[DI + 0x2a],0x5
0000:92c7  MOV word ptr ES:[DI + 0x35],0x0
0000:92cd  MOV word ptr ES:[DI + 0x33],0x0
0000:92d3  MOV byte ptr ES:[DI + 0x32],0x0
0000:92d8  MOV dword ptr ES:[DI + 0xa],0xfffee000
0000:92e1  MOV byte ptr ES:[DI + 0x2c],0xff
0000:92e6  MOV word ptr ES:[DI + 0x2d],0x14
0000:92ec  MOV byte ptr ES:[DI + 0x2f],0xff
0000:92f1  RET
0000:92f2  MOV byte ptr ES:[DI + 0x29],0xff
0000:92f7  MOV byte ptr ES:[DI + 0x28],0xff
0000:92fc  CALL 0x0000:92b3
0000:92ff  RET
0000:9313  MOV BX,0xffd8
0000:9316  MOV AX,0x28
0000:9319  CALLF 0x0000:ffff
0000:931e  JNC 0x0000:9325
0000:9320  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9325  CMP byte ptr ES:[DI + 0x29],0x0
0000:932a  JG 0x0000:9340
0000:932c  MOV AX,word ptr ES:[DI + 0x8]
0000:9330  MOV BX,word ptr ES:[DI + 0x4]
0000:9334  SUB BX,0x26
0000:9337  CALLF 0x0000:ffff
0000:933c  JZ 0x0000:9354
0000:933e  JMP 0x0000:9359
0000:9340  MOV AX,word ptr ES:[DI + 0x8]
0000:9344  MOV BX,word ptr ES:[DI + 0x4]
0000:9348  ADD BX,0x26
0000:934b  CALLF 0x0000:ffff
0000:9350  JZ 0x0000:9354
0000:9352  JMP 0x0000:9359
0000:9354  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9359  CMP byte ptr ES:[DI + 0x32],0x1
0000:935e  JGE 0x0000:94a8
0000:9362  CMP byte ptr ES:[DI + 0x2f],0x0
0000:9367  JLE 0x0000:943f
0000:936b  CMP byte ptr ES:[DI + 0x2c],0x0
0000:9370  JGE 0x0000:93e5
0000:9372  CMP word ptr ES:[DI + 0x2d],0x14
0000:9377  JNZ 0x0000:9379
0000:9379  MOV EBX,dword ptr ES:[DI + 0xa]
0000:937e  MOV AL,byte ptr ES:[DI + 0x29]
0000:9382  CBW
0000:9383  CWDE
0000:9385  SHL EAX,0xc
0000:9389  SUB EBX,EAX
0000:938c  CMP EBX,0xfffee000
0000:9393  JL 0x0000:93aa
0000:9395  CMP EBX,0x12000
0000:939c  JG 0x0000:93a1
0000:939e  CLC
0000:939f  JMP 0x0000:93b1
0000:93a1  MOV EBX,0x12000
0000:93a7  STC
0000:93a8  JMP 0x0000:93b1
0000:93aa  MOV EBX,0xfffee000
0000:93b0  STC
0000:93b1  MOV dword ptr ES:[DI + 0xa],EBX
0000:93b6  ADD dword ptr ES:[DI + 0x2],EBX
0000:93bb  DEC word ptr ES:[DI + 0x2d]
0000:93bf  JGE 0x0000:943c
0000:93c1  NEG byte ptr ES:[DI + 0x29]
0000:93c5  NEG byte ptr ES:[DI + 0x28]
0000:93c9  NEG byte ptr ES:[DI + 0x2c]
0000:93cd  MOV AL,byte ptr ES:[DI + 0x29]
0000:93d1  CBW
0000:93d2  CWDE
0000:93d4  SHL EAX,0x9
0000:93d8  MOV dword ptr ES:[DI + 0xa],EAX
0000:93dd  MOV word ptr ES:[DI + 0x2d],0x14
0000:93e3  JMP 0x0000:943c
0000:93e5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:93ea  MOV AL,byte ptr ES:[DI + 0x29]
0000:93ee  CBW
0000:93ef  CWDE
0000:93f1  SHL EAX,0xa
0000:93f5  ADD EBX,EAX
0000:93f8  CMP EBX,0xfffee000
0000:93ff  JL 0x0000:9416
0000:9401  CMP EBX,0x12000
0000:9408  JG 0x0000:940d
0000:940a  CLC
0000:940b  JMP 0x0000:941d
0000:940d  MOV EBX,0x12000
0000:9413  STC
0000:9414  JMP 0x0000:941d
0000:9416  MOV EBX,0xfffee000
0000:941c  STC
0000:941d  MOV dword ptr ES:[DI + 0xa],EBX
0000:9422  ADD dword ptr ES:[DI + 0x2],EBX
0000:9427  DEC word ptr ES:[DI + 0x2d]
0000:942b  JGE 0x0000:943c
0000:942d  NEG byte ptr ES:[DI + 0x2c]
0000:9431  MOV byte ptr ES:[DI + 0x2f],0xff
0000:9436  MOV word ptr ES:[DI + 0x2d],0x14
0000:943c  JMP 0x0000:95c1
0000:943f  MOV EAX,dword ptr ES:[DI + 0xa]
0000:9444  ADD dword ptr ES:[DI + 0x2],EAX
0000:9449  INC word ptr ES:[DI + 0x35]
0000:944d  CMP word ptr ES:[DI + 0x35],0x82
0000:9453  JLE 0x0000:9478
0000:9455  MOV SI,0x646c
0000:9458  ADD SI,word ptr [0x6468]
0000:945c  INC word ptr [0x6468]
0000:9460  AND word ptr [0x6468],0xff
0000:9466  MOV AL,byte ptr [SI]
0000:9468  SHR AL,0x2
0000:946b  CBW
0000:946c  MOV word ptr ES:[DI + 0x35],AX
0000:9470  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9475  JMP 0x0000:95c1
0000:9478  INC word ptr ES:[DI + 0x2a]
0000:947c  CMP word ptr ES:[DI + 0x2a],0x50
0000:9481  JLE 0x0000:95c1
0000:9485  MOV SI,0x646c
0000:9488  ADD SI,word ptr [0x6468]
0000:948c  INC word ptr [0x6468]
0000:9490  AND word ptr [0x6468],0xff
0000:9496  MOV AL,byte ptr [SI]
0000:9498  SHR AL,0x2
0000:949b  CBW
0000:949c  MOV word ptr ES:[DI + 0x2a],AX
0000:94a0  MOV byte ptr ES:[DI + 0x32],0x1
0000:94a5  JMP 0x0000:95c1
0000:94a8  CMP byte ptr ES:[DI + 0x2f],0x0
0000:94ad  JLE 0x0000:94bd
0000:94af  MOV byte ptr ES:[DI + 0x32],0x0
0000:94b4  MOV word ptr ES:[DI + 0x2a],0x32
0000:94ba  JMP 0x0000:95c1
0000:94bd  CMP byte ptr ES:[DI + 0x32],0x2
0000:94c2  JZ 0x0000:9541
0000:94c4  CMP byte ptr ES:[DI + 0x32],0x3
0000:94c9  JZ 0x0000:955f
0000:94cd  MOV EBX,dword ptr ES:[DI + 0xa]
0000:94d2  MOV AL,byte ptr ES:[DI + 0x29]
0000:94d6  CBW
0000:94d7  CWDE
0000:94d9  SHL EAX,0xb
0000:94dd  SUB EBX,EAX
0000:94e0  CMP EBX,0xfffee000
0000:94e7  JL 0x0000:94fe
0000:94e9  CMP EBX,0x12000
0000:94f0  JG 0x0000:94f5
0000:94f2  CLC
0000:94f3  JMP 0x0000:9505

; ---- 5C11 count=96 ----
0000:5c11  MOV SI,0x646c
0000:5c14  ADD SI,word ptr [0x6468]
0000:5c18  INC word ptr [0x6468]
0000:5c1c  AND word ptr [0x6468],0xff
0000:5c22  MOV AL,byte ptr [SI]
0000:5c24  CBW
0000:5c25  RETF
0000:5c27  PUSH AX
0000:5c28  TEST AX,0xffff
0000:5c2b  PUSH BX
0000:5c2c  PUSH CX
0000:5c2d  MOV DX,word ptr [0x657e]
0000:5c31  SHR BX,0x3
0000:5c34  AND BX,0xfffe
0000:5c38  SHR AX,0x4
0000:5c3b  MUL DX
0000:5c3d  ADD BX,AX
0000:5c3f  MOV CX,BX
0000:5c41  MOV AX,[0x657c]
0000:5c44  MOV BX,word ptr [0x657a]
0000:5c48  ADD BX,CX
0000:5c4a  MOV AX,FS
0000:5c4c  MOV AX,word ptr FS:[BX]
0000:5c4f  AND AH,0x1
0000:5c52  MOV DX,word ptr [0x30d4]
0000:5c56  MUL DX
0000:5c58  MOV BX,word ptr [0x6582]
0000:5c5c  ADD BX,AX
0000:5c5e  MOV AX,[0x6584]
0000:5c61  MOV AX,FS
0000:5c63  ADD BX,0x2
0000:5c66  MOV DX,word ptr FS:[BX]
0000:5c69  POP CX
0000:5c6a  POP BX
0000:5c6b  POP AX
0000:5c6c  TEST DX,0xf
0000:5c70  JNZ 0x0000:5c74
0000:5c72  RETF
0000:5c74  TEST AX,0x8
0000:5c77  JZ 0x0000:5c8b
0000:5c79  TEST BX,0x8
0000:5c7d  JZ 0x0000:5c85
0000:5c7f  TEST DX,0x2
0000:5c83  RETF
0000:5c85  TEST DX,0x1
0000:5c89  RETF
0000:5c8b  TEST BX,0x8
0000:5c8f  JZ 0x0000:5c97
0000:5c91  TEST DX,0x4
0000:5c95  RETF
0000:5c97  TEST DX,0x8
0000:5c9b  RETF
0000:5d38  ADD SI,0x2
0000:5d3b  MOV AX,word ptr [SI + -0x2]
0000:5d3e  MOV word ptr ES:[DI + 0x1e],AX
0000:5d42  MOV word ptr ES:[DI + 0x20],AX
0000:5d46  MOV word ptr ES:[DI + 0x22],SI
0000:5d4a  MOV word ptr ES:[DI + 0x24],SI
0000:5d4e  MOV AX,word ptr [SI]
0000:5d50  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d55  JNZ 0x0000:5d5a
0000:5d57  ADD AX,0x32
0000:5d5a  MOV word ptr ES:[DI + 0x12],AX
0000:5d5e  RETF
0000:5d60  CMP word ptr ES:[DI + 0x20],0x0
0000:5d65  JZ 0x0000:5d6d
0000:5d67  DEC word ptr ES:[DI + 0x20]
0000:5d6b  RETF
0000:5d6d  ADD word ptr ES:[DI + 0x24],0x2
0000:5d72  MOV SI,word ptr ES:[DI + 0x24]
0000:5d76  MOV AX,word ptr [SI]
0000:5d78  CMP AX,0x0
0000:5d7b  JGE 0x0000:5d89
0000:5d7d  NEG AX
0000:5d7f  ADD AX,AX
0000:5d81  SUB SI,AX
0000:5d83  SUB word ptr ES:[DI + 0x24],AX
0000:5d87  JMP 0x0000:5d76
0000:5d89  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d8e  JNZ 0x0000:5d93
0000:5d90  ADD AX,0x32
0000:5d93  MOV word ptr ES:[DI + 0x12],AX
0000:5d97  MOV AX,word ptr ES:[DI + 0x1e]
0000:5d9b  MOV word ptr ES:[DI + 0x20],AX
0000:5d9f  RETF
0000:6616  MOV SI,0x34d4
0000:6619  CALLF 0x0000:ffff
0000:661e  MOV word ptr ES:[DI + 0x18],0x66e1
0000:6624  MOV word ptr ES:[DI + 0x2a],0x14
0000:662a  MOV AL,byte ptr ES:[DI + 0x29]
0000:662e  MOV byte ptr ES:[DI + 0x28],AL
0000:6632  MOV dword ptr ES:[DI + 0xa],0xfffeb000
0000:663b  CMP AL,0x0
0000:663d  JL 0x0000:6644
0000:663f  NEG dword ptr ES:[DI + 0xa]
0000:6644  MOV word ptr ES:[DI + 0x30],0x0

; ---- 5C27 count=128 ----
0000:5c27  PUSH AX
0000:5c28  TEST AX,0xffff
0000:5c2b  PUSH BX
0000:5c2c  PUSH CX
0000:5c2d  MOV DX,word ptr [0x657e]
0000:5c31  SHR BX,0x3
0000:5c34  AND BX,0xfffe
0000:5c38  SHR AX,0x4
0000:5c3b  MUL DX
0000:5c3d  ADD BX,AX
0000:5c3f  MOV CX,BX
0000:5c41  MOV AX,[0x657c]
0000:5c44  MOV BX,word ptr [0x657a]
0000:5c48  ADD BX,CX
0000:5c4a  MOV AX,FS
0000:5c4c  MOV AX,word ptr FS:[BX]
0000:5c4f  AND AH,0x1
0000:5c52  MOV DX,word ptr [0x30d4]
0000:5c56  MUL DX
0000:5c58  MOV BX,word ptr [0x6582]
0000:5c5c  ADD BX,AX
0000:5c5e  MOV AX,[0x6584]
0000:5c61  MOV AX,FS
0000:5c63  ADD BX,0x2
0000:5c66  MOV DX,word ptr FS:[BX]
0000:5c69  POP CX
0000:5c6a  POP BX
0000:5c6b  POP AX
0000:5c6c  TEST DX,0xf
0000:5c70  JNZ 0x0000:5c74
0000:5c72  RETF
0000:5c74  TEST AX,0x8
0000:5c77  JZ 0x0000:5c8b
0000:5c79  TEST BX,0x8
0000:5c7d  JZ 0x0000:5c85
0000:5c7f  TEST DX,0x2
0000:5c83  RETF
0000:5c85  TEST DX,0x1
0000:5c89  RETF
0000:5c8b  TEST BX,0x8
0000:5c8f  JZ 0x0000:5c97
0000:5c91  TEST DX,0x4
0000:5c95  RETF
0000:5c97  TEST DX,0x8
0000:5c9b  RETF
0000:5d38  ADD SI,0x2
0000:5d3b  MOV AX,word ptr [SI + -0x2]
0000:5d3e  MOV word ptr ES:[DI + 0x1e],AX
0000:5d42  MOV word ptr ES:[DI + 0x20],AX
0000:5d46  MOV word ptr ES:[DI + 0x22],SI
0000:5d4a  MOV word ptr ES:[DI + 0x24],SI
0000:5d4e  MOV AX,word ptr [SI]
0000:5d50  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d55  JNZ 0x0000:5d5a
0000:5d57  ADD AX,0x32
0000:5d5a  MOV word ptr ES:[DI + 0x12],AX
0000:5d5e  RETF
0000:5d60  CMP word ptr ES:[DI + 0x20],0x0
0000:5d65  JZ 0x0000:5d6d
0000:5d67  DEC word ptr ES:[DI + 0x20]
0000:5d6b  RETF
0000:5d6d  ADD word ptr ES:[DI + 0x24],0x2
0000:5d72  MOV SI,word ptr ES:[DI + 0x24]
0000:5d76  MOV AX,word ptr [SI]
0000:5d78  CMP AX,0x0
0000:5d7b  JGE 0x0000:5d89
0000:5d7d  NEG AX
0000:5d7f  ADD AX,AX
0000:5d81  SUB SI,AX
0000:5d83  SUB word ptr ES:[DI + 0x24],AX
0000:5d87  JMP 0x0000:5d76
0000:5d89  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d8e  JNZ 0x0000:5d93
0000:5d90  ADD AX,0x32
0000:5d93  MOV word ptr ES:[DI + 0x12],AX
0000:5d97  MOV AX,word ptr ES:[DI + 0x1e]
0000:5d9b  MOV word ptr ES:[DI + 0x20],AX
0000:5d9f  RETF
0000:6616  MOV SI,0x34d4
0000:6619  CALLF 0x0000:ffff
0000:661e  MOV word ptr ES:[DI + 0x18],0x66e1
0000:6624  MOV word ptr ES:[DI + 0x2a],0x14
0000:662a  MOV AL,byte ptr ES:[DI + 0x29]
0000:662e  MOV byte ptr ES:[DI + 0x28],AL
0000:6632  MOV dword ptr ES:[DI + 0xa],0xfffeb000
0000:663b  CMP AL,0x0
0000:663d  JL 0x0000:6644
0000:663f  NEG dword ptr ES:[DI + 0xa]
0000:6644  MOV word ptr ES:[DI + 0x30],0x0
0000:664a  MOV word ptr ES:[DI + 0x32],0x0
0000:6650  RET
0000:92a9  MOV word ptr [0x89e6],0xffff
0000:92af  POP DI
0000:92b0  RET
0000:92b3  MOV SI,0x30e2
0000:92b6  CALLF 0x0000:ffff
0000:92bb  MOV word ptr ES:[DI + 0x18],0x9313
0000:92c1  MOV word ptr ES:[DI + 0x2a],0x5
0000:92c7  MOV word ptr ES:[DI + 0x35],0x0
0000:92cd  MOV word ptr ES:[DI + 0x33],0x0
0000:92d3  MOV byte ptr ES:[DI + 0x32],0x0
0000:92d8  MOV dword ptr ES:[DI + 0xa],0xfffee000
0000:92e1  MOV byte ptr ES:[DI + 0x2c],0xff
0000:92e6  MOV word ptr ES:[DI + 0x2d],0x14
0000:92ec  MOV byte ptr ES:[DI + 0x2f],0xff
0000:92f1  RET
0000:92f2  MOV byte ptr ES:[DI + 0x29],0xff
0000:92f7  MOV byte ptr ES:[DI + 0x28],0xff
0000:92fc  CALL 0x0000:92b3
0000:92ff  RET
0000:9313  MOV BX,0xffd8
0000:9316  MOV AX,0x28
0000:9319  CALLF 0x0000:ffff
0000:931e  JNC 0x0000:9325
0000:9320  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9325  CMP byte ptr ES:[DI + 0x29],0x0
0000:932a  JG 0x0000:9340
0000:932c  MOV AX,word ptr ES:[DI + 0x8]
0000:9330  MOV BX,word ptr ES:[DI + 0x4]
0000:9334  SUB BX,0x26
0000:9337  CALLF 0x0000:ffff
0000:933c  JZ 0x0000:9354
0000:933e  JMP 0x0000:9359
0000:9340  MOV AX,word ptr ES:[DI + 0x8]
0000:9344  MOV BX,word ptr ES:[DI + 0x4]
0000:9348  ADD BX,0x26
0000:934b  CALLF 0x0000:ffff
0000:9350  JZ 0x0000:9354

; ---- 5D38 count=96 ----
0000:5d38  ADD SI,0x2
0000:5d3b  MOV AX,word ptr [SI + -0x2]
0000:5d3e  MOV word ptr ES:[DI + 0x1e],AX
0000:5d42  MOV word ptr ES:[DI + 0x20],AX
0000:5d46  MOV word ptr ES:[DI + 0x22],SI
0000:5d4a  MOV word ptr ES:[DI + 0x24],SI
0000:5d4e  MOV AX,word ptr [SI]
0000:5d50  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d55  JNZ 0x0000:5d5a
0000:5d57  ADD AX,0x32
0000:5d5a  MOV word ptr ES:[DI + 0x12],AX
0000:5d5e  RETF
0000:5d60  CMP word ptr ES:[DI + 0x20],0x0
0000:5d65  JZ 0x0000:5d6d
0000:5d67  DEC word ptr ES:[DI + 0x20]
0000:5d6b  RETF
0000:5d6d  ADD word ptr ES:[DI + 0x24],0x2
0000:5d72  MOV SI,word ptr ES:[DI + 0x24]
0000:5d76  MOV AX,word ptr [SI]
0000:5d78  CMP AX,0x0
0000:5d7b  JGE 0x0000:5d89
0000:5d7d  NEG AX
0000:5d7f  ADD AX,AX
0000:5d81  SUB SI,AX
0000:5d83  SUB word ptr ES:[DI + 0x24],AX
0000:5d87  JMP 0x0000:5d76
0000:5d89  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d8e  JNZ 0x0000:5d93
0000:5d90  ADD AX,0x32
0000:5d93  MOV word ptr ES:[DI + 0x12],AX
0000:5d97  MOV AX,word ptr ES:[DI + 0x1e]
0000:5d9b  MOV word ptr ES:[DI + 0x20],AX
0000:5d9f  RETF
0000:6616  MOV SI,0x34d4
0000:6619  CALLF 0x0000:ffff
0000:661e  MOV word ptr ES:[DI + 0x18],0x66e1
0000:6624  MOV word ptr ES:[DI + 0x2a],0x14
0000:662a  MOV AL,byte ptr ES:[DI + 0x29]
0000:662e  MOV byte ptr ES:[DI + 0x28],AL
0000:6632  MOV dword ptr ES:[DI + 0xa],0xfffeb000
0000:663b  CMP AL,0x0
0000:663d  JL 0x0000:6644
0000:663f  NEG dword ptr ES:[DI + 0xa]
0000:6644  MOV word ptr ES:[DI + 0x30],0x0
0000:664a  MOV word ptr ES:[DI + 0x32],0x0
0000:6650  RET
0000:92a9  MOV word ptr [0x89e6],0xffff
0000:92af  POP DI
0000:92b0  RET
0000:92b3  MOV SI,0x30e2
0000:92b6  CALLF 0x0000:ffff
0000:92bb  MOV word ptr ES:[DI + 0x18],0x9313
0000:92c1  MOV word ptr ES:[DI + 0x2a],0x5
0000:92c7  MOV word ptr ES:[DI + 0x35],0x0
0000:92cd  MOV word ptr ES:[DI + 0x33],0x0
0000:92d3  MOV byte ptr ES:[DI + 0x32],0x0
0000:92d8  MOV dword ptr ES:[DI + 0xa],0xfffee000
0000:92e1  MOV byte ptr ES:[DI + 0x2c],0xff
0000:92e6  MOV word ptr ES:[DI + 0x2d],0x14
0000:92ec  MOV byte ptr ES:[DI + 0x2f],0xff
0000:92f1  RET
0000:92f2  MOV byte ptr ES:[DI + 0x29],0xff
0000:92f7  MOV byte ptr ES:[DI + 0x28],0xff
0000:92fc  CALL 0x0000:92b3
0000:92ff  RET
0000:9313  MOV BX,0xffd8
0000:9316  MOV AX,0x28
0000:9319  CALLF 0x0000:ffff
0000:931e  JNC 0x0000:9325
0000:9320  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9325  CMP byte ptr ES:[DI + 0x29],0x0
0000:932a  JG 0x0000:9340
0000:932c  MOV AX,word ptr ES:[DI + 0x8]
0000:9330  MOV BX,word ptr ES:[DI + 0x4]
0000:9334  SUB BX,0x26
0000:9337  CALLF 0x0000:ffff
0000:933c  JZ 0x0000:9354
0000:933e  JMP 0x0000:9359
0000:9340  MOV AX,word ptr ES:[DI + 0x8]
0000:9344  MOV BX,word ptr ES:[DI + 0x4]
0000:9348  ADD BX,0x26
0000:934b  CALLF 0x0000:ffff
0000:9350  JZ 0x0000:9354
0000:9352  JMP 0x0000:9359
0000:9354  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9359  CMP byte ptr ES:[DI + 0x32],0x1
0000:935e  JGE 0x0000:94a8
0000:9362  CMP byte ptr ES:[DI + 0x2f],0x0
0000:9367  JLE 0x0000:943f
0000:936b  CMP byte ptr ES:[DI + 0x2c],0x0
0000:9370  JGE 0x0000:93e5
0000:9372  CMP word ptr ES:[DI + 0x2d],0x14
0000:9377  JNZ 0x0000:9379
0000:9379  MOV EBX,dword ptr ES:[DI + 0xa]
0000:937e  MOV AL,byte ptr ES:[DI + 0x29]
0000:9382  CBW

; ---- 5D60 count=128 ----
0000:5d60  CMP word ptr ES:[DI + 0x20],0x0
0000:5d65  JZ 0x0000:5d6d
0000:5d67  DEC word ptr ES:[DI + 0x20]
0000:5d6b  RETF
0000:5d6d  ADD word ptr ES:[DI + 0x24],0x2
0000:5d72  MOV SI,word ptr ES:[DI + 0x24]
0000:5d76  MOV AX,word ptr [SI]
0000:5d78  CMP AX,0x0
0000:5d7b  JGE 0x0000:5d89
0000:5d7d  NEG AX
0000:5d7f  ADD AX,AX
0000:5d81  SUB SI,AX
0000:5d83  SUB word ptr ES:[DI + 0x24],AX
0000:5d87  JMP 0x0000:5d76
0000:5d89  CMP byte ptr ES:[DI + 0x28],0xff
0000:5d8e  JNZ 0x0000:5d93
0000:5d90  ADD AX,0x32
0000:5d93  MOV word ptr ES:[DI + 0x12],AX
0000:5d97  MOV AX,word ptr ES:[DI + 0x1e]
0000:5d9b  MOV word ptr ES:[DI + 0x20],AX
0000:5d9f  RETF
0000:6616  MOV SI,0x34d4
0000:6619  CALLF 0x0000:ffff
0000:661e  MOV word ptr ES:[DI + 0x18],0x66e1
0000:6624  MOV word ptr ES:[DI + 0x2a],0x14
0000:662a  MOV AL,byte ptr ES:[DI + 0x29]
0000:662e  MOV byte ptr ES:[DI + 0x28],AL
0000:6632  MOV dword ptr ES:[DI + 0xa],0xfffeb000
0000:663b  CMP AL,0x0
0000:663d  JL 0x0000:6644
0000:663f  NEG dword ptr ES:[DI + 0xa]
0000:6644  MOV word ptr ES:[DI + 0x30],0x0
0000:664a  MOV word ptr ES:[DI + 0x32],0x0
0000:6650  RET
0000:92a9  MOV word ptr [0x89e6],0xffff
0000:92af  POP DI
0000:92b0  RET
0000:92b3  MOV SI,0x30e2
0000:92b6  CALLF 0x0000:ffff
0000:92bb  MOV word ptr ES:[DI + 0x18],0x9313
0000:92c1  MOV word ptr ES:[DI + 0x2a],0x5
0000:92c7  MOV word ptr ES:[DI + 0x35],0x0
0000:92cd  MOV word ptr ES:[DI + 0x33],0x0
0000:92d3  MOV byte ptr ES:[DI + 0x32],0x0
0000:92d8  MOV dword ptr ES:[DI + 0xa],0xfffee000
0000:92e1  MOV byte ptr ES:[DI + 0x2c],0xff
0000:92e6  MOV word ptr ES:[DI + 0x2d],0x14
0000:92ec  MOV byte ptr ES:[DI + 0x2f],0xff
0000:92f1  RET
0000:92f2  MOV byte ptr ES:[DI + 0x29],0xff
0000:92f7  MOV byte ptr ES:[DI + 0x28],0xff
0000:92fc  CALL 0x0000:92b3
0000:92ff  RET
0000:9313  MOV BX,0xffd8
0000:9316  MOV AX,0x28
0000:9319  CALLF 0x0000:ffff
0000:931e  JNC 0x0000:9325
0000:9320  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9325  CMP byte ptr ES:[DI + 0x29],0x0
0000:932a  JG 0x0000:9340
0000:932c  MOV AX,word ptr ES:[DI + 0x8]
0000:9330  MOV BX,word ptr ES:[DI + 0x4]
0000:9334  SUB BX,0x26
0000:9337  CALLF 0x0000:ffff
0000:933c  JZ 0x0000:9354
0000:933e  JMP 0x0000:9359
0000:9340  MOV AX,word ptr ES:[DI + 0x8]
0000:9344  MOV BX,word ptr ES:[DI + 0x4]
0000:9348  ADD BX,0x26
0000:934b  CALLF 0x0000:ffff
0000:9350  JZ 0x0000:9354
0000:9352  JMP 0x0000:9359
0000:9354  MOV byte ptr ES:[DI + 0x2f],0x1
0000:9359  CMP byte ptr ES:[DI + 0x32],0x1
0000:935e  JGE 0x0000:94a8
0000:9362  CMP byte ptr ES:[DI + 0x2f],0x0
0000:9367  JLE 0x0000:943f
0000:936b  CMP byte ptr ES:[DI + 0x2c],0x0
0000:9370  JGE 0x0000:93e5
0000:9372  CMP word ptr ES:[DI + 0x2d],0x14
0000:9377  JNZ 0x0000:9379
0000:9379  MOV EBX,dword ptr ES:[DI + 0xa]
0000:937e  MOV AL,byte ptr ES:[DI + 0x29]
0000:9382  CBW
0000:9383  CWDE
0000:9385  SHL EAX,0xc
0000:9389  SUB EBX,EAX
0000:938c  CMP EBX,0xfffee000
0000:9393  JL 0x0000:93aa
0000:9395  CMP EBX,0x12000
0000:939c  JG 0x0000:93a1
0000:939e  CLC
0000:939f  JMP 0x0000:93b1
0000:93a1  MOV EBX,0x12000
0000:93a7  STC
0000:93a8  JMP 0x0000:93b1
0000:93aa  MOV EBX,0xfffee000
0000:93b0  STC
0000:93b1  MOV dword ptr ES:[DI + 0xa],EBX
0000:93b6  ADD dword ptr ES:[DI + 0x2],EBX
0000:93bb  DEC word ptr ES:[DI + 0x2d]
0000:93bf  JGE 0x0000:943c
0000:93c1  NEG byte ptr ES:[DI + 0x29]
0000:93c5  NEG byte ptr ES:[DI + 0x28]
0000:93c9  NEG byte ptr ES:[DI + 0x2c]
0000:93cd  MOV AL,byte ptr ES:[DI + 0x29]
0000:93d1  CBW
0000:93d2  CWDE
0000:93d4  SHL EAX,0x9
0000:93d8  MOV dword ptr ES:[DI + 0xa],EAX
0000:93dd  MOV word ptr ES:[DI + 0x2d],0x14
0000:93e3  JMP 0x0000:943c
0000:93e5  MOV EBX,dword ptr ES:[DI + 0xa]
0000:93ea  MOV AL,byte ptr ES:[DI + 0x29]
0000:93ee  CBW
0000:93ef  CWDE
0000:93f1  SHL EAX,0xa
0000:93f5  ADD EBX,EAX
0000:93f8  CMP EBX,0xfffee000
0000:93ff  JL 0x0000:9416
0000:9401  CMP EBX,0x12000
0000:9408  JG 0x0000:940d
0000:940a  CLC
0000:940b  JMP 0x0000:941d
0000:940d  MOV EBX,0x12000
0000:9413  STC
0000:9414  JMP 0x0000:941d
0000:9416  MOV EBX,0xfffee000

