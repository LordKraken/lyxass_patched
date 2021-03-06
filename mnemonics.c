/*
    mnemonics.c

    Handle 65C02 mnemonics

*/

#include <stdio.h>

#include "my.h"
#include "error.h"
#include "label.h"
#include "global_vars.h"
#include "parser.h"

#include "mnemonics.h"
#include "jaguar.h"

int Expression(long *);

void saveCurrentLine();

extern void writeByte(char );
extern void writeWordLittle(short );

// this must be provided by any mnemonic.c file
int Endian(void)
{
  if ( sourceMode == LYNX )
    return targetLITTLE_ENDIAN;
  else
    return targetBIG_ENDIAN;
}

// one-byte commands
int op0(int op)
{
  writeByte( (char) op );
  CYCLES += 2;
  if ( op == 0x48 || op == 0xda || op == 0x5a || op == 0x08 ) ++CYCLES;
  if ( op == 0x68 || op == 0xfa || op == 0x7a || op == 0x28 ) CYCLES += 2;
  if ( op == 0x40 || op == 0x66 ) CYCLES += 6;
  return 0;
}

// branch
int op1(int op)
{
  long l;
  int err;

  if ( (err = Expression(&l)) == EXPR_ERR) return 1;

  if ( err == EXPR_OK ){
    int dist = l - Global.pc - 2;
    if ( dist > 127 || dist < -128 )return Error(DISTANCE_ERR,"");
    writeByte(op);
    writeByte((char)dist);
  } else {
    saveCurrentLine();
    writeByte(op);
    writeByte(0);
  }
  CYCLES += 2;
  if ( (l & ~255) != ( Global.pc & ~255) ) ++CYCLES;
  return 0;
}

// jsr => absolute adress

int op2(int op)
{
  long l;
  int err;
  
  if ( (err = Expression(&l)) == EXPR_ERR) return 1;
  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");

  writeByte(op);
  writeWordLittle((short) l);

  if ( err == EXPR_UNSOLVED ) saveCurrentLine();

  CYCLES += 6;

  return 0;
}

// ora,and,eor,adc,sta,lda,cmp,sbc

int op3(int op)
{
  int err;
  long l;
  int komma = 0;

  if ( ! KillSpace() ){
    return Error(SYNTAX_ERR, __FUNCTION__ );
  }

  // immediate 

  if ( TestAtom('#') ){

    if ( (err = Expression(&l)) == EXPR_ERR) return 1;

    if ( l < -128 || l > 255 ) return Error(BYTE_ERR,"");
    if ( op == 0x81 ) return Error(SYNTAX_ERR, __FUNCTION__ ); // STA #0 not possible
    writeByte(op | 0x08);
    writeByte((char)l);
    if ( err == EXPR_UNSOLVED ) saveCurrentLine();

    CYCLES += 2;

    if ( (Global.pc & 255) == 1) ++CYCLES;

    return 0;
  }

  // indirect
  
  if ( TestAtom('(') ){

    if ( (err = Expression(&l)) == EXPR_ERR) return 1;
    if ( l < 0 || l > 255 ) return Error(BYTE_ERR,"");

    if ( TestAtom(',') ){
      if ( !TestAtomOR('x','X') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      komma = 1;
      CYCLES += 6;
    }
    if ( !TestAtom(')') ) return Error(SYNTAX_ERR, __FUNCTION__ );

    if ( !komma ) {
      if ( TestAtom(',') ){
	CYCLES += 5;
	if ( !TestAtomOR('y','Y') ) return Error(SYNTAX_ERR, __FUNCTION__ );
	op |= 0x10;
      } else {
	op |= 0x12;
	--op;
	CYCLES++;
      }
    }
    if ( op == 0x91 ) ++CYCLES;
    writeByte(op);
    writeByte((char)l);
    if ( err == EXPR_UNSOLVED ) saveCurrentLine();
    return 0;
  }

  // absolute

  if ( (err = Expression(&l)) == EXPR_ERR) return 1;

  if ( TestAtom(',') ){
    CYCLES += 4;

    if ( TestAtomOR('X','x') ){
      op |= 0x14;
    } else if ( TestAtomOR('Y','y') ) {
      op |= 0x18;
      writeByte(op);
      writeWordLittle((short)l);
      if ( err == EXPR_UNSOLVED ) saveCurrentLine();
      return 0;
    } else return Error(SYNTAX_ERR, __FUNCTION__ );
  } else {
    op |= 0x04;
  }
  if ( l < 0 || l > 65535 ) Error(WORD_ERR,"");
  
  if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2) {
    CYCLES+=4;
    op |= 0x08;
    writeByte(op);
    writeWordLittle((short)l);
  } else {
    CYCLES+=3;
    writeByte(op);
    writeByte((char)l);
  }
  if ( op == 0x99 || op == 0x9d ) ++CYCLES;

  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}

// trb,tsb

int op4(int op)
{
  int err;
  long l;

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;

  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");

  if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
    CYCLES += 6;
    writeByte(op | 0x08);
    writeWordLittle((short)l);
  } else {
    CYCLES += 5;
    writeByte(op);
    writeByte((char)l);
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}

// stz 
  
int op5(int op)
{
  int err;
  long l;

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
  
  if ( TestAtom(',') ) {

    if ( !TestAtomOR('x','X') ) Error(SYNTAX_ERR, __FUNCTION__ );

    if (l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
      writeByte( op | 0x9a );
      writeWordLittle((short)l);
      CYCLES += 5;
    } else {
      writeByte( op | 0x70 );
      writeByte((char)l);
      CYCLES += 4;
    }
    if ( err == EXPR_UNSOLVED ) saveCurrentLine();
    return 0;
  }

  if (l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
    writeByte( op | 0x98 );
    writeWordLittle((short)l);
    CYCLES += 4;
  } else {
    writeByte( op | 0x60 );
    writeByte((char)l);
    CYCLES += 3;
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}

// stx
int op6(int op)
{
  int err;
  long l;

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
  
  if ( TestAtom(',') ) {
    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ) return Error(BYTE_ERR,"");

    if ( !TestAtomOR('y','Y') ) Error(SYNTAX_ERR, __FUNCTION__ );
    CYCLES += 4;
    writeByte(op | 0x10);
    writeByte((char)l);
  } else {
    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
      CYCLES += 4;
      writeByte(op | 0x08 );
      writeWordLittle((short)l);
    } else {
      CYCLES += 3;
      writeByte(op);
      writeByte((char)l);
    }
  }

  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}

// sty

int op7(int op)
{
  int err;
  long l;

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
  
  if ( TestAtom(',') ) {

    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ) return Error(BYTE_ERR,"");

    if ( !TestAtomOR('x','X') ) Error(SYNTAX_ERR, __FUNCTION__ );
    CYCLES += 4;
    writeByte(op | 0x10);
    writeByte((char)l);
  } else {
    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
      CYCLES += 4;
      writeByte(op | 0x08 );
      writeWordLittle((short)l);
    } else {
      CYCLES += 3;
      writeByte(op);
      writeByte((char)l);
    }
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}

// bit,ldy,cpy

int op8(int op)
{
  int err;
  long l;
  int flag = 0;

  if ( TestAtom('#') ){
    
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < -128 || l > 255 ) return Error(BYTE_ERR,"");
    CYCLES += 2;
    writeByte(op == 0x20 ? 0x89 : op);
    writeByte((char)l);
  } else {
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
    op |= 0x04;
    CYCLES += 3;
    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
      op |= 0x08;
      flag = 1;
      CYCLES++;
    }
    if ( TestAtom(',') ){
      if ( (op & 0xc0) == 0xc0) return Error(SYNTAX_ERR, __FUNCTION__ );                     // no cpy ABCD,x !!!
      if ( !TestAtomOR('x','X') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      op |= 0x10;
      CYCLES++;
    }
    
    writeByte(op);
    if ( flag ){
      writeWordLittle((short)l);
    } else {
      writeByte((char)l);
    }
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
  
}
/*
  ldx
*/
int op9(int op)
{
  int err;
  long l;
  int flag = 0;

  if ( TestAtom('#') ){
    
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < -128 || l > 255 ) return Error(BYTE_ERR,"");
    CYCLES += 2;
    writeByte(op);
    writeByte((char)l);
  } else {
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
    CYCLES += 3;
    op |= 0x04;
    if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
      op |= 0x08;
      flag = 1;
      ++CYCLES;
    }
    if ( TestAtom(',') ){
      if ( !TestAtomOR('y','Y') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      op |= 0x10;
      ++CYCLES;
    }
    
    writeByte(op);
    if ( flag ){
      writeWordLittle((short)l);
    } else {
      writeByte((char)l);
    }
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;    
}

/* asl/rol/lsr/ror/dec/inc */

int opa(int op)
{
  int err;
  long l;

  KillSpace();
  if ( !atom ){
    op |= 8;
    if ( op >= 0xca ) op ^= 0xf0;
    writeByte(op);
    CYCLES += 2;
    return 0;
  }

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");

  CYCLES += 5;
  op |= 0x04;
  if ( l > 255 || err == EXPR_UNSOLVED || Current.pass2 ){
    ++CYCLES;
    op |= 0x0a;
    if ( TestAtom(',') ){
      if ( !TestAtomOR('x','X') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      op |= 0x10;
    }
    writeByte(op);
    writeWordLittle((short)l);
  } else {
    if ( TestAtom(',') ){
      if ( !TestAtomOR('x','X') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      op |= 0x10;
      ++CYCLES;
    }
    writeByte(op);
    writeByte((char)l);
  }
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}
/*
  rmb / smb
*/
int opb(int op)
{
  int err;
  long l;

  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < 0 || l > 255 ) return Error(BYTE_ERR,"");

  writeByte(op);
  writeByte((char)l);

  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}
/*
  JMP
*/
int opc(int op)
{
  int err;
  long l;

  if ( TestAtom('(') ){
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
    op |= 0x20;
    if ( TestAtom(',') ){
      if ( !TestAtomOR('x','X') ) return Error(SYNTAX_ERR, __FUNCTION__ );
      op |= 0x10;
    }
    if ( !TestAtom(')') ) return Error(SYNTAX_ERR, __FUNCTION__ );
    CYCLES += 6;
  } else {
    if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
    if ( l < 0 || l > 65535 ) return Error(WORD_ERR,"");
    if ( err != EXPR_UNSOLVED ){
      int dist = Global.pc - l - 2;
      if ( dist >= -128 && dist <= 127 ) Warning("JMP can be changed into BRA !\n");
    }
    CYCLES += 3;
  }
  writeByte(op);
  writeWordLittle((short)l);
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  return 0;
}
// brk
// this opcode has an immediate-value on the lynx
int opd(int op)
{
  int err;
  long l;

  if ( !TestAtom('#') ) return Error(SYNTAX_ERR, __FUNCTION__ );
    
  if ( (err = Expression( &l )) == EXPR_ERROR ) return 1;
  if ( l < -128 || l > 255 ) return Error(BYTE_ERR,"");
  writeByte(op);
  writeByte((char)l);
  if ( err == EXPR_UNSOLVED ) saveCurrentLine();
  CYCLES += 7;
  return 0;
}

int CheckMnemonic(char *s)
{  
  if ( Current.ifFlag && Current.switchFlag){
    if ( sourceMode == LYNX ){
      if (SearchOpcode(OpCodes,s) < 0 ) return -1;
    } else {
      if (SearchOpcode(JaguarOpcodes,s) < 0 ) return -1;
    }

    if ( GetComment() ){
      Error(GARBAGE_ERR,"");
      return 1;
    }
  }

  return 0;
}
