/* parsc.c    Pascal Parser      Gordon S. Novak Jr.    02 Nov 05    */

/* This file contains a parser for a Pascal subset using the techniques of
   recursive descent and operator precedence.  The Pascal subset is equivalent
   to the one handled by the Yacc version pars1.y .  */

/* Copyright (c) 2005 Gordon S. Novak Jr. and
   The University of Texas at Austin. */

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


/* To use:
                     make pars1c
                     pars1c
                     i:=j .

                     pars1c
                     begin i:=j; if i+j then x:=a+b*c else x:=a*b+c; k:=i end.

                     pars1c
                     if x+y then if y+z then i:=j else k:=2.
*/

/* You may copy this file to be parsc.c, expand it for your assignment,
   then use    make parsec    as above.  */

#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "lexan.h"
#include "symtab.h"
#include "parse.h"
#include <string.h>

TOKEN parseresult;
TOKEN savedtoken;
int labelnumber = -1;

#define DEBUG       127             /* set bits here for debugging, 0 = off  */
#define DB_CONS       1             /* bit to trace cons */
#define DB_BINOP      2             /* bit to trace binop */
#define DB_MAKEIF     4             /* bit to trace makeif */
#define DB_MAKEPROGN  8             /* bit to trace makeprogn */
#define DB_PARSERES  16             /* bit to trace parseresult */
#define DB_GETTOK    32             /* bit to trace gettok */
#define DB_EXPR      64             /* bit to trace expr */

TOKEN cons(TOKEN item, TOKEN list)           /* add item to front of list */
  { item->link = list;
    if (DEBUG & DB_CONS)
       { printf("cons\n");
         dbugprinttok(item);
         dbugprinttok(list);
       };
    return item;
  }

TOKEN binop(TOKEN op, TOKEN lhs, TOKEN rhs)       /* reduce binary operator */
            /* operator, left-hand side, right-hand side */
  { op->operands = lhs;         /* link operands to operator       */
    lhs->link = rhs;            /* link second operand to first    */
    rhs->link = NULL;           /* terminate operand list          */
    if (DEBUG & DB_BINOP)
       { printf("binop\n");
         dbugprinttok(op);      /*       op         =  (op lhs rhs)      */
         dbugprinttok(lhs);     /*      /                                */
         dbugprinttok(rhs);     /*    lhs --- rhs                        */
       };
    return op;
  }

TOKEN copytok(TOKEN origtok)
{
  int i;
  TOKEN cpy = talloc();
  cpy->tokentype = origtok->tokentype;
  cpy->datatype = origtok->datatype;
   // if(origtok->datatype == STRINGTYPE)
    strcpy(cpy->stringval, origtok->stringval);
  return cpy;
}
TOKEN makeif(TOKEN tok, TOKEN exp, TOKEN thenpart, TOKEN elsepart)
  {  tok->tokentype = OPERATOR;  /* Make it look like an operator   */
     tok->whichval = IFOP;
     if (elsepart != NULL) elsepart->link = NULL;
     thenpart->link = elsepart;
     exp->link = thenpart;
     tok->operands = exp;
     if (DEBUG & DB_MAKEIF)
        { printf("makeif\n");
          dbugprinttok(tok);
          dbugprinttok(exp);
          dbugprinttok(thenpart);
          dbugprinttok(elsepart);
        };
     return tok;
   }

TOKEN makeprogn(TOKEN tok, TOKEN statements)
  {  tok->tokentype = OPERATOR;
     tok->whichval = PROGNOP;
     tok->operands = statements;
     if (DEBUG & DB_MAKEPROGN)
       { printf("makeprogn\n");
         dbugprinttok(tok);
         dbugprinttok(statements);
       };
     return tok;
   }

TOKEN makeprogram(TOKEN tok, TOKEN name, TOKEN args, TOKEN statements)
  {
      tok->tokentype = OPERATOR;
      tok->whichval = PROGRAMOP;
      tok->operands = name;
      name->link = args;
      args->link = statements;
      return tok;
  }

TOKEN findid(TOKEN tok) { /* the ID token */
SYMBOL sym, typ;

sym = searchst(tok->stringval);
tok->symentry = sym;
typ = sym->datatype;
tok->symtype = typ;
if ( typ->kind == BASICTYPE ||
     typ->kind == POINTERSYM)
    tok->datatype = typ->basicdt;
return tok;
}

TOKEN makegoto(int label)
{
  TOKEN nok = talloc();
  TOKEN tok = copytok(nok);
  nok->tokentype = NUMBERTOK;
  nok->datatype = INTEGER;
  nok->intval = label;
  tok->tokentype = OPERATOR;
  tok->whichval = GOTOOP;
  tok->operands = nok;
  return tok;

}

TOKEN makelabel(TOKEN tok)
{ 
  TOKEN nok = talloc();
  nok->tokentype = NUMBERTOK;
  nok->datatype = INTEGER;
  nok->intval = ++labelnumber;
  tok->tokentype = OPERATOR;
  tok->whichval = LABELOP;
  tok->operands = nok;
  return tok;
}

TOKEN makeop (int opnum)
{
  TOKEN tok = talloc();
  tok->tokentype = OPERATOR;
  tok->whichval = opnum;
  return tok;
}

TOKEN makefloat(TOKEN tok)
{
  TOKEN ret = talloc();
  ret->tokentype = OPERATOR;
  ret->whichval = FLOATOP;
  ret->operands = tok;
  ret->operands->link = NULL;
  return ret;
}

TOKEN makefuncall(TOKEN tok, TOKEN fn, TOKEN args)
{
  tok->tokentype = OPERATOR;
  tok->whichval = FUNCALLOP;
  tok->operands = fn;
  fn->link = args;
  args->link = NULL;
  return tok;
}
yyerror(s)
  char * s;
  { 
  fputs(s,stderr); putc('\n',stderr);
  }

TOKEN gettok()       /* Get the next token; works with peektok. */
  {  TOKEN tok;
     if (savedtoken != NULL)
       { tok = savedtoken;
	 savedtoken = NULL; }
       else tok = gettoken();
     if (DEBUG & DB_GETTOK)
       { printf("gettok\n");
         dbugprinttok(tok);
       };
     return(tok);
   }

TOKEN peektok()       /* Peek at the next token */
  { if (savedtoken == NULL)
       savedtoken = gettoken();
     if (DEBUG & DB_GETTOK)
       { printf("peektok\n");
         dbugprinttok(savedtoken);
       };
     return(savedtoken);
   }

int reserved(TOKEN tok, int n)          /* Test for a reserved word */
  { return ( tok->tokentype == RESERVED
	    && (tok->whichval + RESERVED_BIAS ) == n);
  }

TOKEN parseprogram(TOKEN keytok)  /* Parse a Program */
  {  TOKEN name, args, rest, end, tok;
     TOKEN statement();
     int finish = 1;
     name = NULL;
     args = NULL;
     rest = NULL;
     while(finish == 1)
     {
        if(name == NULL)  /* Get the name of the program */
        {
          tok = gettok();
          if (tok->tokentype != IDENTIFIERTOK)  /* Checks if the program name is valid */
            yyerror("Program name not specified.");
          else
          {
          name = tok;
          }

        }

        else if (args == NULL)
        {
          tok = gettok();  /* Consume the left paren; error otherwise. */
          if( (tok->whichval + DELIMITER_BIAS) == LPAREN )
          {
            tok = gettok();
            args = tok;
            tok = gettok();  /* Consume the right paren; error otherwise. */
          if( (tok->whichval + DELIMITER_BIAS) != RPAREN )
            yyerror("Missing Right paren.");
            args = makeprogn(tok, args);
            gettok(); /* Consume semicolon */
          }

        }

        else
        {
            tok = peektok();
            if (tok->tokentype == OPERATOR && tok->whichval == DOTOP)
              finish = 0;
            else
            {
            tok = statement();
            if(tok!=NULL)
            {
            if (rest == NULL)
              rest = tok;
            else
            {
              end->link = tok;
            }
              tok->link = NULL;
              end = tok;
              tok = peektok();
          if (tok->tokentype == OPERATOR && tok->whichval == DOTOP)
              finish = 0;
          else
              gettok();
            }
            }
          
        }
     }

     return makeprogram(keytok, name, args, rest);
     

  }

void parsevars(TOKEN keytok)
{ TOKEN tok, idlist;
  SYMBOL var;
  int donezo = 1;
  while(peektok()->tokentype == IDENTIFIERTOK){
    idlist = NULL;
    donezo = 1;
  while(donezo == 1)
  {
    tok = gettok();
    if(tok->tokentype == DELIMITER && (tok->whichval + DELIMITER_BIAS == COLON))
      donezo = 0;
    else
    {
      if(tok->tokentype == DELIMITER && (tok->whichval + DELIMITER_BIAS == COMMA))
        tok = gettok();
      if(tok->tokentype!= IDENTIFIERTOK)
        yyerror("Invalid variable declaration.");
    var = insertsym(tok->stringval);
    if(idlist==NULL)
      idlist = tok;
    else{
      idlist->link = tok;
      }
       tok->link = NULL;
    }

  }
  tok = gettok();
   instvars(idlist, tok);
   tok = gettok();
 }

}

void parseconst(TOKEN keytok)
{ TOKEN idtok, consttok;
  while(peektok()->tokentype == IDENTIFIERTOK)
  {
     idtok = gettok();
     keytok = gettok();
     if(keytok->tokentype!= OPERATOR || keytok->whichval!=EQOP)
       yyerror("Invalid constant declaration");
     consttok = gettok();
     keytok = gettok();
     if ( (keytok->whichval + DELIMITER_BIAS) != SEMICOLON)
       yyerror("Bad item in parseconst.") ;
     instconst(idtok,consttok);


  }
}

void  instconst(TOKEN idtok, TOKEN consttok)
{
  SYMBOL var;
  var = insertsym(idtok->stringval);
  var->kind = CONSTSYM;
  if(consttok->datatype == REAL){
   var->basicdt = REAL;
  var->datatype = searchlev("real", 0);
  var->constval.realnum = consttok->tokenval.realnum;
  }
  if(consttok->datatype == INTEGER){
    var->basicdt = INTEGER;
    var->datatype = searchlev("integer",0);
    var->constval.intnum = consttok->tokenval.intnum;
  }
}

 int offs = 0;
 void  instvars(TOKEN idlist, TOKEN typetok)
 {
   
    SYMBOL sym = NULL;
    if(!strcmp(typetok->stringval,"integer"))
    {
      while(idlist!=NULL)
      {
        sym = searchst(idlist->stringval);
        if(sym==NULL)
          yyerror("NULL symbol");
        sym->kind = VARSYM;
        sym->basicdt = 0;
        sym->datatype = searchlev(typetok->stringval, 0);
        sym->size = 4;
        sym->offset = offs;
        offs+=4;
        idlist = idlist->link;
       }
    }
    if(!strcmp(typetok->stringval,"real"))
    {
      while(idlist!=NULL)
      {
        sym = searchst(idlist->stringval);
        if(sym==NULL)
          yyerror("NULL symbol");
        sym->kind = VARSYM;
        sym->basicdt = REAL;
        sym->datatype = searchlev(typetok->stringval, 0);
        sym->size = 8;
        sym->offset = offs;
        offs+=8;
        idlist = idlist->link;
      }
    }

 }

TOKEN parsebegin(TOKEN keytok)  /* Parse a BEGIN ... END statement */
  {  TOKEN front, end, tok;
     TOKEN statement();
     int done;
     int count = 0;
     front = NULL;
     done = 0;
     while ( done == 0 )
       { tok = statement();        /* Get a statement */
          count++;
	 if ( front == NULL )      /* Put at end of list */
	    front = tok;
	    else end->link = tok;
	 tok->link = NULL;
	 end = tok;
	 tok = gettok();           /* Get token: END or semicolon */
	 if ( reserved(tok, END) )
	   done = 1;
	   else if (tok->tokentype != DELIMITER
		    || (tok->whichval + DELIMITER_BIAS) != SEMICOLON)
	     yyerror("Bad item in begin - end.") ;
       };
       if(count>1)
     return (makeprogn(keytok,front));
    return front;
  }

TOKEN parseif(TOKEN keytok)  /* Parse an IF ... THEN ... ELSE statement */
  {  TOKEN expr, thenpart, elsepart, tok;
     TOKEN parseexpr();
     TOKEN statement();
     expr = parseexpr();
     tok = gettok();
     if ( reserved(tok, THEN) == 0 ) yyerror("Missing THEN");
     thenpart = statement();
     elsepart = NULL;
     tok = peektok();
     if ( reserved(tok, ELSE) )
       { tok = gettok();        /* consume the ELSE */
	 elsepart = statement();
       };
     return ( makeif(keytok, expr, thenpart, elsepart));
  }

TOKEN parseassign(TOKEN lhs)  /* Parse an assignment statement */
  {  TOKEN tok, rhs; TOKEN parseexpr();
     tok = gettok();
     if ( tok->tokentype != OPERATOR || tok->whichval != ASSIGNOP )
       printf("Unrecognized statement\n");
     rhs = parseexpr();
     return ( binop(tok, lhs, rhs) );
  }


/* *opstack and *opndstack allow reduce to manipulate the stacks of parseexpr */
void reduce(TOKEN *opstack, TOKEN *opndstack)  /* Reduce an op and 2 operands */
  {  TOKEN op, lhs, rhs;
      TOKEN constant;
     if (DEBUG & DB_EXPR)
       { printf("reduce\n");
       };
     op = *opstack;               /* pop one operator from op stack */
     *opstack = op->link;
     rhs = *opndstack;            /* pop two operands from opnd stack */
     lhs = rhs->link;
     *opndstack = lhs->link;
     if(lhs->tokentype == NUMBERTOK && lhs->datatype == REAL)  /* lhs is real; rhs is integer.  Make rhs a floating point */
     {
        if(rhs->tokentype == IDENTIFIERTOK && rhs->symentry->basicdt == 0)
          rhs = makefloat(rhs);
      }
      if(lhs->tokentype == NUMBERTOK && lhs->datatype == INTEGER)
      {
        if(rhs->tokentype == IDENTIFIERTOK && rhs->symentry->basicdt == 1) /*lhs is integer; rhs is real.  Make lhs a floating point */
        {

           constant = talloc();
           constant = copytok(lhs);
           constant->tokentype = NUMBERTOK;
           constant-> datatype = REAL;
           constant->tokenval.realnum = lhs->tokenval.intnum;
          // constant->tokentype = NUMBERTOK;
          // constant-> datatype = REAL;
          // constant->tokenval.realnum = lhs->symentry->constval.intnum;
           *opndstack = cons( binop(op,constant,rhs), *opndstack);  /* push result opnd */

        }
      }
      else
     *opndstack = cons( binop(op,lhs,rhs), *opndstack);  /* push result opnd */
   }

TOKEN makefor(int sign, TOKEN tok, TOKEN asg, TOKEN tokb, TOKEN endexpr, TOKEN tokc, TOKEN statement)
{
  tok = makeprogn(tok,asg);
  asg->link = endexpr;
  endexpr->link = statement;
  return tok;
}

TOKEN makerepeat(TOKEN tok, TOKEN statements, TOKEN tokb, TOKEN expr)
{ TOKEN lab, gotovar, nul;
  lab = makelabel(talloc());
  gotovar = makegoto(labelnumber);
  tok = makeprogn(tok,NULL);
  tok->link = gotovar;
  expr = makeif(talloc(),expr, tok, gotovar);
  lab->link = statements;
  statements->link = tokb;
  tokb->link = expr;
  return makeprogn(talloc(),lab);
}

TOKEN parserepeat(TOKEN keytok)
{ TOKEN parseexpr(); TOKEN statement();
  TOKEN expr, incre, tok;
  expr = statement();
  gettok();  /* Consume semicolon; can add error checking later */
  incre = statement();
  keytok = gettok();  /* Consume until reserved word; can add error checking later */
  keytok = parseexpr();
  // keytok = makeif(talloc(),keytok, makeprogn(talloc(),NULL), NULL);
  // gettok();  Consume semicolon at end 
  return makerepeat(talloc(),expr,incre,keytok);
}

TOKEN parsefor(TOKEN keytok)  /* Parses a FOR TO DO loop */
{ TOKEN parseexpr(); TOKEN statement();
  TOKEN tok, vari, vari2, vari3, tovar, lab, op, op2,op3, one, expr, dostuff, gotovar, front, end;
  TOKEN constant = talloc();
 
   vari = copytok(peektok()); /* Get incrementing variable  (normally i ) */
   vari2 = copytok(vari);      /* Create copies of the incrementing variable to create increment expression */
   vari3 = copytok(vari);
   expr = parseexpr();
   tok = gettok();  /* Get TO reserved word; error otherwise; DOWNTO is not implemented yet; add later when needed */
   if( reserved(tok, TO) == 0 ) yyerror("Missing TO");
   tovar = gettok();  /* Get the variable that i is incrementing to */
   tovar = findid(tovar);
   if(tovar->symentry->kind == CONSTSYM)
   {
      constant->tokentype = NUMBERTOK;
      constant-> datatype = tovar->symentry->basicdt;
      if(constant->datatype == INTEGER)
        constant->tokenval.intnum = tovar->symentry->constval.intnum;
      else
        constant->tokenval.realnum = tovar->symentry->constval.realnum;
   }


    op = makeop(LEOP);
    if(tovar->symentry->kind == CONSTSYM)
      tovar = cons(constant, vari);
    else
    tovar = cons(tovar, vari);  /* Produces the condition for the if statement */
    reduce(&op, &tovar);
    op2 = makeop(PLUSOP);  /* This makeop's args will change later, depending on whether or not there is a TO or DOWNTO */
    one = copytok(vari2);   /* Make incrementer with a value of one */
    one->tokentype = NUMBERTOK;
    one->datatype = INTEGER;
    one->intval = 1;
    one = cons(one, vari2);
    reduce(&op2, &one);
    one = cons(one, vari3);
    op3 = makeop(ASSIGNOP);
    // op2 = cons(op3, op2);
    reduce(&op3, &one);

    // return makeprogn(keytok,one);
   tok = gettok();
   if( reserved(tok, DO) == 0) yyerror("Missing DO");
   tok = statement();
   lab = makelabel(talloc());
 gotovar = makegoto(labelnumber);
   one->link = gotovar;
   dostuff = tok;
   dostuff->link = one;
   dostuff = makeprogn(keytok,dostuff);  /* End expression */
    dostuff = makeif(talloc(), tovar, dostuff, NULL);
  return makefor(1, talloc(), expr, talloc(), lab, talloc(), dostuff);


}

TOKEN parsefunc(TOKEN keytok)
{TOKEN parseexpr();
  TOKEN tok, args;
  args = NULL;
  tok = gettok();  /* Remove lparen */
  while(peektok()->whichval + DELIMITER_BIAS!= RPAREN)
  {
  if(peektok()->tokentype == IDENTIFIERTOK)
  {

    args = parseexpr();
  }
  else
  {
  while(peektok()->whichval+ DELIMITER_BIAS!=RPAREN)
  {
  tok = gettok();  /* Get args */
    if(args == NULL)
      args = tok;
    else
      args->operands = tok;
  }
  }
  }
  tok = gettok(); /* Consume rparen */
  return makefuncall(tok, keytok, args);

}

/*                             +     *                                     */
static int precedence[] = { 0, 1, 0, 3    };  /* **** trivial version **** */

TOKEN parseexpr()   /* Parse an expression using operator precedence */
                    /* Partial implementation -- handles +, *, ()    */
  {  TOKEN tok, op, lhs, rhs, constant; int state, done, gotparen;
     TOKEN opstack, opndstack;
     gotparen = 0;
     if (DEBUG & DB_EXPR)
       { printf("parseexpr\n");
       };
     done = 0;
     state = 0;
     opstack = NULL;
     opndstack = NULL;
     while ( done == 0 )
       { tok = peektok();
	 switch ( tok->tokentype )
	   { case IDENTIFIERTOK: 
         tok = gettok();
         if((peektok()->whichval + DELIMITER_BIAS) == LPAREN )
         {
          tok = parsefunc(tok);
          opndstack = cons(tok,opndstack);
         }
         else
         {
         tok = findid(tok);  // If identifier is a constant
        if(tok->symentry->kind == CONSTSYM)
        {
          constant = talloc();
          constant->tokentype = NUMBERTOK;
          constant-> datatype = tok->symentry->basicdt;
      if(constant->datatype == INTEGER)
        constant->tokenval.intnum = tok->symentry->constval.intnum;
      else
        constant->tokenval.realnum = tok->symentry->constval.realnum;
        opndstack = cons(constant, opndstack);
         }
         else
         opndstack = cons(tok, opndstack);
        }
         break;
       case NUMBERTOK: /* operand: push onto stack */
	       tok = gettok();
	       opndstack = cons(tok, opndstack);
	       break;
	     case DELIMITER:
	       if ( (tok->whichval + DELIMITER_BIAS) == LPAREN )
		 {   gotparen = 1;
        tok = gettok();
		   opstack = cons(tok, opstack);
		 }
	         else if ( (tok->whichval + DELIMITER_BIAS) == RPAREN )
		   {  if(gotparen==0)
            done = 1;
          else
          {
          gotparen = 0;
          tok = gettok();
		     while ( opstack != NULL
			    && (opstack->whichval + DELIMITER_BIAS)
                                != LPAREN )
		        reduce(&opstack, &opndstack);
		     opstack = opstack->link;  /* discard the left paren */
          }
		   }
	         else done = 1;
	       break;
	     case OPERATOR:
	       if ( tok->whichval != DOTOP )   /* special case for now */
		 { tok = gettok();
		   while ( opstack != NULL && opstack->tokentype != DELIMITER
			  && (precedence[opstack->whichval]
			       >= precedence[tok->whichval]))
		     reduce(&opstack, &opndstack);
		   opstack = cons(tok,opstack);
		 }
	         else done = 1;
	       break;
	     default: done = 1;
	     }
       }
     while ( opstack != NULL ) reduce(&opstack, &opndstack);
     return (opndstack);
  }



TOKEN statement ()    /* Parse a Pascal statement: the "big switch" */
  { TOKEN tok, pok, result;
    result = NULL;
    tok = gettok();
    pok = peektok();
    if (tok->tokentype == RESERVED)
       switch (  tok->whichval + RESERVED_BIAS )   /* the big switch */
	 { case BEGINBEGIN: result = parsebegin(tok);
	        break;
	   case IF:         result = parseif(tok);
	        break;      
     case PROGRAM:    result = parseprogram(tok);
          break;
     case VAR:        parsevars(tok); 
          break;
     case CONST:      parseconst(tok);
          break;
     case FOR:        result = parsefor(tok);
          break;
     case REPEAT:     result = parserepeat(tok);
          break;
	 }
       else if (tok->tokentype == IDENTIFIERTOK)
                if ( (pok->whichval + DELIMITER_BIAS) == LPAREN )
                      result = parsefunc(tok);
                else
	                    result = parseassign(tok);
    return (result);
  }

int yyparse ()             /* program = statement . */
  {  TOKEN dottok;
     savedtoken = NULL;
     parseresult = statement();    /* get the statement         */
     dottok = gettok();            /* get the period at the end */
     if (dottok->tokentype == OPERATOR && dottok->whichval == DOTOP)
        return (0);
        else return(1);
   }


main()          /* Call yyparse repeatedly to test */
  { int res;
    initscanner();
    init_charclass();   /* initialize character class array */
    initsyms(); //added; meant to initialize the symbol table
    printf("Started parser test.\n");
    res = yyparse();
    printf("yyparse result = %8d\n", res);
    printst();   //added; meant to print out the symbol table
    if (DEBUG & DB_PARSERES) dbugprinttok(parseresult);
    ppexpr(parseresult);
    }
