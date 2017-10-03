/* codgen.c       Generate Assembly Code for x86         15 May 13   */

/* Copyright (c) 2013 Gordon S. Novak Jr. and The University of Texas at Austin
    */

/* Starter file for CS 375 Code Generation assignment.           */
/* Written by Gordon S. Novak Jr.                  */

/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License (file gpl.text) for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "token.h"
#include "symtab.h"
#include "genasm.h"
#include "codegen.h"

void genc(TOKEN code);

/* Set DEBUGGEN to 1 for debug printouts of code generation */
#define DEBUGGEN 0

int nextlabel;    /* Next available label number */
int stkframesize;   /* total stack frame size */
static int loc_reg[4];    /* Keeps track of which registers are in use */
static int float_reg[16];

/* Top-level entry for code generator.
   pcode    = pointer to code:  (program foo (output) (progn ...))
   varsize  = size of local storage in bytes
   maxlabel = maximum label number used so far

Add this line to the end of your main program:
    gencode(parseresult, blockoffs[blocknumber], labelnumber);
The generated code is printed out; use a text editor to extract it for
your .s file.
         */

void gencode(TOKEN pcode, int varsize, int maxlabel)
  {  TOKEN name, code;
     name = pcode->operands;
     code = name->link->link;
     nextlabel = maxlabel + 1;
     stkframesize = asmentry(name->stringval,varsize);
     genc(code);
     asmexit(name->stringval);
  }

/* Get a register.   */
/* Need a type parameter or two versions for INTEGER or REAL */
int getreg(int kind)
  {
    int i;
    if(kind<=WORD)
    {
      for(i = 0; i<4; i++)
      {
        if(loc_reg[i]==0)
        {
          used(RBASE+i);
          return RBASE + i;
        }
      }
    }
    else
    {
      for(i = 0; i<16; i++)
      {
        if(float_reg[i]==0)
        {
          used(XMM0+i);
          return XMM0 + i;
        }
      }
    }
     return RBASE;
  }

void clearreg()
{
  int i;
  for(i = 0; i<16; i++)
  {
    if(i<4)
      loc_reg[i] = 0;
    float_reg[i] = 0;
  }
}

void unused(int reg)
{
  if(reg<4)
    loc_reg[reg] = 0;
  else
    float_reg[XMM0-reg]= 0;
}

void used(int reg)
{
  if(reg<4)
    loc_reg[reg]=1;
  else
    float_reg[reg-XMM0]=1;
}

int getlabnum()
{
  int num = nextlabel;
  nextlabel++;
  return num;
}

int convertop(int comp)
{
  if(comp == EQOP)
    return JE;
  if(comp == NEOP)
    return JNE;
  if(comp == LTOP)
    return JL;
  if(comp == LEOP)
    return JLE;
  if(comp == GEOP)
    return JGE;
  if(comp == GTOP)
    return JG;
  return 0;
}

int funcallin(TOKEN code)
{
  TOKEN k = code;
  while(k!=NULL){
  if(k->whichval == FUNCALLOP)
    return 1;
  k=k->operands;
  }
  return 0;
}

void savereg(int reg)
{
  asmsttemp(reg);
  unused(reg);
}

void restorereg(int reg)
{
  asmldtemp(reg);
}

int genaref(TOKEN code, int storereg)
{
    TOKEN lhs, rhs;
    int reg, reg2;
    if(code->whichval == AREFOP)
    {

            lhs = code->operands; //the reference
             if(lhs->tokentype!=IDENTIFIERTOK){
              reg = genarith(lhs);
            rhs = lhs->link;  //the offset
            if(storereg>=0){
              reg2 = storereg;
              if(reg2>=XMM0)
                asmldr(MOVSD,rhs->intval,reg,reg2, lhs->stringval);
              }
            else{

             reg2 = getreg(WORD);
              asmldr(MOVQ,rhs->intval, reg, reg2, lhs->stringval );
             }
              unused(reg);
              reg = reg2;
              
              }
              else
              {
                reg = genarith(lhs->link);
                asmop(CLTQ);
              }
            

              
    }
    else if(code->whichval == POINTEROP)
          {
            if(code->operands->tokentype!=IDENTIFIERTOK)
            reg = genarith(code->operands);
            else
            {
            if(code->operands->tokentype==IDENTIFIERTOK)
              {
                findid(code->operands);
              }
            reg = getreg(WORD);
            
            if(code->operands->symentry==NULL)
              findid(code->operands);
            
             asmld(MOVQ, code->operands->symentry->offset-stkframesize,reg, code->operands->stringval);
            }
          }
          return reg;
}

/* Generate code for arithmetic expression, return a register number */
int genarith(TOKEN code)
  {   int num, reg, reg2, temp, tf;
      tf = 0;
      double real;
      TOKEN lhs, rhs, fn, args;
     if (DEBUGGEN)
       { printf("genarith\n");
	 dbugprinttok(code);
       };
      switch ( code->tokentype )
       { case NUMBERTOK:
           switch (code->datatype)
             { case INTEGER:
		 num = code->intval;
		 reg = getreg(WORD);
		 if ( num >= MINIMMEDIATE && num <= MAXIMMEDIATE )
		   asmimmed(MOVL, num, reg);
		 break;
	       case REAL:
         real = code->realval;
         reg = getreg(FLOAT);
         num = getlabnum();
         makeflit(real, num);
         asmldflit(MOVSD, num, reg);
         break;

		 break;
	       }
	   break;
       case IDENTIFIERTOK:
       {
          if(code->symentry==NULL)
          findid(code);
          if(code->symentry->basicdt==INTEGER)
          {
            reg = getreg(WORD);
            asmld(MOVL, code->symentry->offset-stkframesize, reg,code->stringval );
          }
          else if(code->symentry->basicdt==POINTER)
          {
            reg = getreg(WORD);
            asmld(MOVQ, code->symentry->offset-stkframesize, reg, code->stringval);
          }
          else
          {
            reg = getreg(FLOAT);
            asmld(MOVSD, code->symentry->offset-stkframesize, reg, code->stringval);
          }
       }

	   break;
       case OPERATOR:
          if(code->whichval >= EQOP && code->whichval <= GTOP) //Comparison statement; used in ifs
          {
          lhs = code->operands;
          rhs = lhs->link;
          reg = genarith(lhs);
          reg2 = genarith(rhs);
          asmrr(CMPL, reg2, reg);
          }
          else if(code->whichval == PLUSOP)
          {
            lhs = code->operands;
            rhs = lhs->link;
            reg = genarith(lhs);
            reg2 = genarith(rhs);
            if(reg<4 && reg2<4)
            {
            asmrr(ADDL,reg2,reg);
            unused(reg2);
            }
            else if(reg>4 && reg2<4) //One is float one is int
            {
              temp = getreg(FLOAT);
              asmfloat(reg2, temp);
              asmrr(ADDSD, temp, reg);
              unused(reg2);
              unused(temp);
            }
          }
          else if(code->whichval == TIMESOP)
          {
            lhs = code->operands;
            rhs = lhs->link;
            reg = genarith(lhs);
            if(funcallin(rhs))
            {
              tf = 1;
              savereg(reg);

            }
            reg2 = genarith(rhs);
            if(tf)
            {
              reg = getreg(FLOAT);
              restorereg(reg);
              if(reg>4)
                asmrr(MULSD, reg2, reg);
            }
            else if(reg>4) //We know its float operation 
            {
              asmrr(MULSD, reg2, reg);
            }
            else
              asmrr(IMULL, reg2, reg);

          }
          else if(code->whichval == MINUSOP)
          {
            lhs = code->operands;
            rhs = lhs->link;
            if(rhs==NULL)
            {
              if(lhs->symentry==NULL)
                findid(lhs);
              if(lhs->datatype == REAL)
              {
                reg = getreg(FLOAT);
                reg2 = getreg(FLOAT);
                asmld(MOVSD, lhs->symentry->offset - stkframesize, reg, lhs->stringval);
                asmfneg(reg, reg2);
              }
            }
            else
            {
              if(lhs->tokentype==IDENTIFIERTOK)
                findid(lhs);
              if(rhs->tokentype == IDENTIFIERTOK)
                findid(rhs);
              if(lhs->datatype == INTEGER)
              {
                reg = genarith(lhs);
                reg2 = genarith(rhs);
                asmrr(SUBL,reg2,reg);
                unused(reg2);

              }
            }
          }
          else if(code->whichval == FLOATOP)
          {
            lhs = code->operands;
            if(lhs->symentry == NULL)
              findid(lhs);
            reg = getreg(WORD);
            asmld(MOVL,lhs->symentry->offset - stkframesize, reg, lhs->stringval);
            reg2 = getreg(FLOAT);
            asmfloat(reg, reg2);
            unused(reg);
            reg = reg2;
          }
          else if(code->whichval == FUNCALLOP)
          {
            fn = code->operands;
            args = fn->link;
            reg = genarith(args);
            if(!strcmp(fn->stringval,"new"))
              asmrr(MOVL,reg,EDI);
            asmcall(fn->stringval);
          }
          else if(code->whichval == AREFOP || POINTEROP)
          {
            reg = genaref(code, -1);
          }


	   break;
       };
     return reg;
    }


/* Generate code for a Statement from an intermediate-code form */
void genc(TOKEN code)
  {  TOKEN tok, lhs, rhs, labnum, cond, thenpart, elsepart, fn, args;
     int reg, offs, thenlab, elselab, reg1, lab;
     SYMBOL sym;
     clearreg();
     if (DEBUGGEN)
       { printf("genc\n");
	 dbugprinttok(code);
       };
     if ( code->tokentype != OPERATOR )
        { printf("Bad code token");
	  dbugprinttok(code);
	};
     switch ( code->whichval )
       { case PROGNOP:
	   tok = code->operands;
	   while ( tok != NULL )
	     {  genc(tok);
		tok = tok->link;
	      };
	   break;
	 case ASSIGNOP:                   
	   lhs = code->operands;
	   rhs = lhs->link;
     if(lhs->tokentype == OPERATOR && rhs->tokentype == OPERATOR && rhs->whichval == AREFOP)
      {
        used(XMM0);
        reg = genaref(rhs,XMM0);
        reg1 = genarith(lhs);
        findid(lhs->operands);
        asmstrr(MOVSD,reg,lhs->operands->symentry->offset-stkframesize,reg1, lhs->operands->stringval);
        break;
      }
      else
	   reg = genarith(rhs);              /* generate rhs into a register */
     if(lhs->tokentype == OPERATOR)
     {
        if(lhs->operands->tokentype == IDENTIFIERTOK){
        findid(lhs->operands);
        if(lhs->operands->symtype->kind == ARRAYSYM){
          reg1 = genarith(lhs);

        }
        else
          reg1 = genarith(lhs->operands);
          }
        else
          reg1 = genarith(lhs->operands);
        findid(rhs);
        if(rhs->tokentype == IDENTIFIERTOK && rhs->symentry->basicdt == POINTER)
          asmstr(MOVQ, reg, lhs->operands->link->intval, reg1, lhs->operands->stringval);
        else if(reg>4)
          asmstr(MOVSD, reg, lhs->operands->link->intval, reg1, lhs->operands->stringval);
        else
        asmstr(MOVL, reg, lhs->operands->link->intval, reg1, lhs->operands->stringval);
     }
     else
     {
     if(lhs->symentry == NULL)
      findid(lhs);
	   sym = lhs->symentry;              /* assumes lhs is a simple var  */
	   offs = sym->offset - stkframesize; /* net offset of the var   */
    code->datatype = lhs->datatype;
           switch (code->datatype)            /* store value into lhs  */
             { case INTEGER:
                 if(reg>4) /
                 {
                  reg1 = getreg(WORD);
                  asmfix(reg, reg1);
                  reg = reg1;
                 }

                 asmst(MOVL, reg, offs, lhs->stringval);
                 break;
               case REAL:
                  asmst(MOVSD, reg, offs, lhs->stringval);
                  break;
               case POINTER:
                  asmst(MOVQ, reg, offs, lhs->stringval); 
                  break;
             };
      }
           break;
   case GOTOOP:
        lab = code->operands->intval;
        asmjump(JMP, lab);
        break;
   case LABELOP:
        labnum = code->operands;
        asmlabel(labnum->intval);
        break;
   case IFOP:
        thenlab = getlabnum();
        elselab = getlabnum();
        cond = code->operands; /* Condition statement */
        thenpart = cond->link;
        elsepart = thenpart->link;
        reg = genarith(cond);
        asmjump(convertop(cond->whichval),thenlab);
        if(elsepart!=NULL)
        genc(elsepart);
        asmjump(JMP, elselab);
        asmlabel(thenlab);
        genc(thenpart);
        asmlabel(elselab);
        break;
   case FUNCALLOP:
        fn = code->operands;
        args = fn->link;
        if(!strcmp(fn->stringval,"writelni"))
        {
          reg = genarith(args);
           asmrr(MOVL,reg,EDI);

        }
        if(!strcmp(fn->stringval,"writelnf"))
        {
          used(XMM0);
          reg = genaref(args,XMM0);

        }
        if(args->tokentype == STRINGTOK)
        {
          lab = getlabnum();
          makeblit(args->stringval,lab);
          asmlitarg(lab, EDI);

        }
        asmcall(fn->stringval);
        break; 
	 };
  }
