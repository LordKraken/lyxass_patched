#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "my.h"
#include "error.h"
#include "label.h"
#include "global_vars.h"
#include "parser.h"

//#include "mnemonics.h"
//#include "jaguar.h"

int Expression(long *);

void saveCurrentLine();

extern void writeByte(char );
extern void writeWordBig(short );
extern void writeLongBig(long );

int direct(int );
int one_reg(int );
int two_reg(int );
int imm_reg(int );  // immediate 1..32
int imm2_reg(int ); // immediate 0..31
int imm3_reg(int ); // immediate -16..15
int imm4_reg(int ); // immediate long
int cond_rel(int ); // condition, relative
int cond_abs(int ); // condition, absolute
int load_reg(int );
int load2_reg(int );
int store_reg(int );
int store2_reg(int );

extern int getdec(long *);

int GetRegisterOrPC(int *reg)
{
  extern int LabelTable[256];

  LABEL regL,*regL2;
  long solved;
  char * save_srcLinePtr = srcLinePtr-1;
  
  *reg = 99;

  KillSpace();

  if ( (atom == 'r' || atom == 'R') && isdigit(next_atom) ){
   long l;
    GetAtom();
    if ( getdec(&l) == EXPR_ERR ) return Error(REG_ERR,"");
    if ( l > 31 ) return Error(REG_ERR,"");    
    *reg = (int)l;
    return 0;
  }

  if ( (atom == 'p' || atom == 'P') &&
       (next_atom == 'c' || next_atom == 'C') ){
    *reg = 64;
    GetAtom();
    GetAtom();
    if ( !LabelTable[atom] ) return 0;
    srcLinePtr = save_srcLinePtr;
    atom = *srcLinePtr++;
  }
  
  if ( GetLabel(&regL) ) return 1;

  if ( (regL2 = FindLabel(&regL, &solved)) == NULL ) return Error(REG_ERR,"");

  if (regL2->type != REGISTER ) return Error(REG_ERR,"");

  *reg = regL2->value;
  
  return 0;
}

int GetRegister(int *reg)
{
  LABEL regL;
  LABEL *regL2;
  long solved;

  *reg = 99;

  KillSpace();

  if ( (atom == 'r' || atom == 'R') && isdigit(next_atom) ){
    long l;
    GetAtom();
    if ( getdec(&l) == EXPR_ERR ) return Error(REG_ERR,"");
    if ( l > 31 ) return Error(REG_ERR,"");
    *reg = (int)l;
    return 0;
  }
  
  if ( GetLabel(&regL) ) return 1;

  if ( (regL2 = FindLabel(&regL, &solved)) == NULL ) return Error(REG_ERR,"");

  if (regL2->type != REGISTER ) return Error(REG_ERR,"");

  *reg = regL2->value;

  return 0;
}

int direct(int op)
{
  writeWordBig(op << 10);

  return 0;
}
/*
  blub Rn
*/
int one_reg(int op)
{
  int reg;

  if ( GetRegister( &reg) ) return 1;

  // handle UNPACK specially
  if ( op == 64 ){ 
    op = ( 63 << 10 ) | 32 | reg;
  } else {
    op = (op << 10) | reg;
  }
  writeWordBig( op );

  return 0;
}
/*
  blab Rn,Rm
*/
int two_reg(int op)
{
  int reg1,reg2;
  if ( GetRegisterOrPC( &reg1 )) return 1;
  if ( !TestAtom(',') )return Error(SYNTAX_ERR,"");
  if ( GetRegister( &reg2 )) return 1;

  if ( reg1 == 64 && op == 34){ // special case: mov pc,rn
    op = 51;
    reg1 = 0;
  }
    
  if ( reg1 > 31 ) return Error(REG_ERR,"");

  op = ( op << 10 ) | ( reg1 << 5 ) | reg2;

  writeWordBig( op );
  return 0;
}
/*
  #1..32,Rn
*/
int imm_reg(int op)
{
  long imm;
  int reg;

  if ( !TestAtom('#') ) return Error(SYNTAX_ERR,"");

  if ( Expression( &imm ) ) return 1;

  if ( imm < 1 || imm > 32 ) Error(IMM_ERR,"1<=x<=32");

  if ( op == 24 ) imm = 32-imm; // shlq

  if ( imm == 32 ) imm = 0;

  if ( !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg ) ) return 1;



  op = ( op << 10 ) | ( imm << 5) | reg;

  writeWordBig( op );

  return 0;
}
/*
  #0..31,Rn
*/
int imm2_reg(int op)
{
  long imm;
  int reg;

  if ( !TestAtom('#') ) return Error(SYNTAX_ERR,"");

  if ( Expression( &imm ) ) return 1;

  if ( imm < 0 || imm > 31 ) Error(IMM_ERR,"0<=n<=31");
  
  if ( !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg ) ) return 1;

  op = ( op << 10 ) | ( imm << 5) | reg;

  writeWordBig( op );

  return 0;
}
/*
  #-16..15,Rn
*/
int imm3_reg(int op)
{
  long imm;
  int reg;

  if ( !TestAtom('#') ) return Error(SYNTAX_ERR,"");

  if ( Expression( &imm ) ) return 1;

  if ( imm < -16 || imm > 15 ) Error(IMM_ERR,"-16<=n<=15");
  
  imm &= 31;

  if ( !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg ) ) return 1;

  op = ( op << 10 ) | ( imm << 5) | reg;

  writeWordBig( op );

  return 0;
}

/*
  #absolute,Rn
*/
int imm4_reg(int op)
{
  long imm;
  int reg;
  int err;

  if ( !TestAtom('#') ) return Error(SYNTAX_ERR,"");

  if ( (err = Expression( &imm )) == EXPR_ERR ) return 1;

  if ( err == EXPR_UNSOLVED ) saveCurrentLine();

  if ( !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg ) ) return 1;

  op = ( op << 10 ) | reg;

  writeWordBig( op );
  writeWordBig( imm & 0xffff);
  writeWordBig( imm >> 16 );

  return 0;
}

struct cond_s{
  char ascCond[6];
  int cond;
}conditions[] = { 
  {"Z",2},{"EQ",2},
  {"NZ",1},{"NE",1},
  {"NC",4},{"CC",4},
  {"C",8},{"CS",8},
  {"MI",0x18},{"N",0x18},
  {"PL",0x14},{"NN",0x14},
  {"GT",0x15},
  {"NN_NZ",0x15},{"NN_Z",0x16},{"N_NZ",0x19},{"N_Z",0x1a},
  {"\0",0}
};

int GetCondition(int *cond)
{
  LABEL condition;
  LABEL *l;
  long solved;
  int i;
  char *p;
  char *save_srcLinePtr = srcLinePtr-1;

  *cond = 0;

  if ( GetLabel(&condition) ) return 0;

  if ( condition.type != (UNSURE|NORMAL) ){
    srcLinePtr = save_srcLinePtr;
    atom = *srcLinePtr++;
    return 0;
  }

  //printf("Cond:%s\n",condition.name);

  if ( (l = FindLabel(&condition, &solved) ) != NULL ){
    srcLinePtr = save_srcLinePtr;
    atom = *srcLinePtr++;
    return 0;
  }

  if ( condition.len > 5 )  return Error(SYNTAX_ERR,"");

  for ( p = condition.name; *p ; ++p ){
    if ( *p >= 'a' && *p <= 'z' ) *p &= 0xdf;
  }

  //*(long *)l->name &= 0xdfdfdfdf;
  //*(short *)&l->name[4] &= 0xdfdf;
  
  for( i = 0 ; conditions[i].cond ; ++i ){
    if ( !strcmp(conditions[i].ascCond,condition.name) ){
      *cond = conditions[i].cond;
      return 0;
    }
  }

  *cond = 0;
  return 1;
}
int cond_rel(int op )
{
  int cond;
  long dest;
  int err;

  if ( GetCondition( & cond ) ) return 1;

  if ( cond && !TestAtom(',') ) return Error(SYNTAX_ERR,""); 

  if ( (err = Expression(&dest)) == EXPR_ERR) return 1;

  op = ( op << 10 ) | cond;

  if ( err == EXPR_UNSOLVED ){
    saveCurrentLine();
  } else {
    long dist = dest - (Global.pc + 2);
    if ( dist < -32 || dist > 30 ) return Error(DISTANCE_ERR,"");
    dist >>= 1;
    op  |= (dist & 31) << 5;
  }

  if ( (Global.pc < 0xf03000) && (Global.pc & 3) ){
    Warning("JUMP not long-aligned ! NOP inserted !");
    writeWordBig((short)0xe400);
  }
  writeWordBig( op );
  return 0;
}

int cond_abs(int op )
{
  int cond;
  int reg;
  
  if ( GetCondition( &cond ) ) return 1;

  if ( cond && !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  op = ( op << 10 ) | cond;

  if ( !TestAtom('(') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg ) ) return 1;

  if ( !TestAtom(')') ) return Error(SYNTAX_ERR,"");

  op |= reg << 5;

  if ( (Global.pc < 0xf03000) && (Global.pc & 3) ){
    Warning("JR not long-aligned ! NOP inserted !");
    writeWordBig((short)0xe400);
  }

  writeWordBig( op );
  return 0;
}

int load_reg(int op )
{
  int reg1,reg2;

  if ( !TestAtom('(') ) return Error(SYNTAX_ERR,"");
  
  if ( GetRegister( &reg1 ) ) return 1;
  
  if ( TestAtom(')') ){
    op = 41;
  } else {
    if ( atom != '+' ) return Error(SYNTAX_ERR,"");
    GetAtom();
    op = 58;
    if ( (reg1 -= 14) < 0 || reg1 > 1 ) return Error(SYNTAX_ERR,"");
    op += reg1;
    if ( GetRegister( &reg1 ) ){
      long l;
      int err;
      if ( (err = Expression( &l )) == EXPR_ERR ) return 1;
      if ( err == EXPR_UNSOLVED ) saveCurrentLine();
      if ( (l & 3) ) return Error(IMM_ERR,"x % 4 != 0");
      l >>= 2;
      if ( l < 1 || l > 32 ) return Error(IMM_ERR,"4<=x<=128");
      reg1 = l == 32 ? 0 : l;
      op -= 15;
    }
    if ( !TestAtom(')') ) return Error(SYNTAX_ERR,"");
  }

  if ( !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg2 ) ) return 1;

  op = ( op << 10 ) | ( reg1 << 5 ) | reg2;

  writeWordBig( op );

  return 0;
}

int load2_reg(int op )
{
  int reg1,reg2;

  if ( !TestAtom('(') ) return Error(SYNTAX_ERR,"");
  
  if ( GetRegister(&reg1) ) return 1;

  if ( !TestAtom(')') || !TestAtom(',') ) return Error(SYNTAX_ERR,"");

  if ( GetRegister( &reg2 ) ) return 1;

  op = ( op << 10 ) | ( reg1 << 5 ) | reg2;

  writeWordBig( op );

  return 0;
}
int store_reg(int op )
{
  int reg1,reg2;

  if ( GetRegister( &reg1 ) ) return 1;

  if ( !TestAtom(',') || !TestAtom('(') ) return Error(SYNTAX_ERR,"");
  
  if ( GetRegister( &reg2 ) ) return 1;
  
  if ( TestAtom(')') ){
    op = 47;
  } else {
    if ( atom != '+' ) return Error(SYNTAX_ERR,"");
    GetAtom();
    op = 49;
    if ( (reg2 -= 14) < 0 || reg2 > 1 ) return Error(SYNTAX_ERR,"");
    op += reg2 +11;
    
    if ( GetRegister( &reg2 ) ){
      long l;
      int err;
      if ( (err = Expression( &l )) == EXPR_ERR ) return 1;
      if ( err == EXPR_UNSOLVED ) saveCurrentLine();
      if ( (l & 3) ) return Error(IMM_ERR,"x % 4 != 0");
      l >>= 2;
      if ( l < 1 || l > 32 ) return Error(IMM_ERR,"4<=x<=128");
      reg2 = l == 32 ? 0 : l;
      op -= 11;
    }
    if ( !TestAtom(')')) return Error(SYNTAX_ERR,"");
  }
  op = ( op << 10 ) | ( reg2 << 5 ) | reg1;

  writeWordBig( op );

  return 0;
}

int store2_reg(int op )
{
  int reg1,reg2;

  if ( GetRegister( &reg1 ) ) return 1;

  if ( !TestAtom(',') || !TestAtom('(') ) return Error(SYNTAX_ERR,"");
  
  if ( GetRegister( &reg2 ) ) return 1;

  if ( !TestAtom(')') ) return Error(SYNTAX_ERR,"");

  op = ( op << 10 ) | ( reg2 << 5 ) | reg1;

  writeWordBig( op );

  return 0;
}
