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
int labels[50];

#define DEBUG       0             /* set bits here for debugging, 0 = off  */
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
if(tok->stringval == NULL)
  return tok;
sym = searchst(tok->stringval);
if(sym == NULL)
return tok;
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

TOKEN dopoint(TOKEN var, TOKEN tok)
{
  tok->tokentype = OPERATOR;
  tok->whichval = POINTEROP;
  tok->operands = var;
  // if(searchst(var->stringval)->datatype->datatype->datatype != NULL && searchst(var->stringval)->datatype->datatype->datatype->kind == RECORDSYM)
  //   tok->symtype = searchst(var->stringval)->datatype->datatype->datatype;
  // else
  if(var->tokentype == OPERATOR && var->whichval == AREFOP){
    // yyerror("GOT IN");
    tok->symtype = var->symtype->datatype;
    }
  else
  tok->symtype = searchst(var->stringval)->datatype->datatype;
  

  var->link = NULL;
  return tok; 
}

TOKEN makearef(TOKEN var, TOKEN off, TOKEN tok)
{
  tok->tokentype = OPERATOR;
  tok->whichval = AREFOP;
  tok->operands = var;
  var->link = off;
  off->link = NULL;
  return tok;
}

TOKEN addoffs(TOKEN expr, TOKEN off)
{
  TOKEN offs = expr->operands->link;
  if(offs->tokentype!= NUMBERTOK)
  {
    offs->operands->intval +=off->intval;
    // printf("WHAT AM I %d \n", offs->operands->intval);
  }
  else
  expr->operands->link->intval = offs->intval + off->intval;
  return expr; 
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

TOKEN nconc(TOKEN lista, TOKEN listb)
{
  TOKEN tok = lista;
  if(lista == NULL)
    return listb;
  while(1)
  {
  if(lista->link == NULL)
    {
      lista->link = listb;
      return tok;
      break;
    }
  else
    lista = lista->link;
  }
}

void parsevars(TOKEN keytok)
{ TOKEN tok, idlist,end;
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
      end->link = tok;
      }
       tok->link = NULL;
       end = tok;
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

int wordaddress(int n, int wordsize)
{
    int address,pad = 0;
    pad = n%wordsize;
    if(pad!=0)
    pad = wordsize - pad;
    address = n+pad;
    return address;
}

TOKEN instfields(TOKEN idlist, TOKEN typetok)
{
  SYMBOL s,prev;
  prev = NULL;
  TOKEN tok = idlist;
  while(tok!=NULL)
  {
    s = makesym(tok->stringval);
    if(typetok->symtype->kind == RECORDSYM) 
    s->datatype = typetok->symtype->datatype;
  else if((typetok->symtype->kind == TYPESYM && typetok->symtype->datatype!=NULL && typetok->symtype->datatype->kind == POINTERSYM))
    s->datatype = typetok->symtype->datatype;
    else
    s->datatype = typetok->symtype;
    s->size = typetok->symtype->size;
    tok->symentry = s;
    // if(prev==NULL)
    //   prev = s;
    // else
    // {
    //   prev->link = s;
    //   prev = s;
    // }
    tok = tok->link;
  }
  return idlist;
}

TOKEN makesubrange(TOKEN tok, int low, int high)
{
  SYMBOL s;
  s = symalloc();
  s->kind = SUBRANGE;
  s->size = 4;
  s->lowbound = low;
  s->highbound = high;
  tok->symtype = s;
  return tok;
}

TOKEN instenum(TOKEN idlist)
{
  SYMBOL e;
  int max = -1;
  while(idlist!=NULL)
  {
    max++;
    e = searchins(idlist->stringval);
    e->kind = CONSTSYM;
    e->basicdt = INTEGER;
    e->datatype = searchlev("integer",0);
    e->constval.intnum = max;
    idlist = idlist->link;
  
  }

  return makesubrange(talloc(), 0, max);
}

void instlabel(TOKEN tok)
{
  int done = 0;
  while((peektok()->whichval + DELIMITER_BIAS) != SEMICOLON)
  {
    tok = gettok();
    labelnumber++;
    labels[labelnumber] = tok->intval;
    if((peektok()->whichval + DELIMITER_BIAS) == COMMA)
      gettok();
  }
  gettok();  // Take care of semicolon
}

TOKEN instpoint(TOKEN tok, TOKEN typename)
{
  SYMBOL p;
  p = symalloc();
  p->kind = POINTERSYM;
  p->datatype = searchins(typename->stringval);
  p->size = basicsizes[POINTER];
  p->basicdt = POINTER;
  tok->symtype = p;
  return tok;
}

void parsetype(TOKEN keytok)
{ TOKEN idtok, typet, fields, end, fieldtype, front, rectok, idlist, enumtok, eend, pointertok;
  SYMBOL fieldsym, fieldtypesym,id;
  int off = 0;
  int enu = 0;
  while(peektok()->tokentype == IDENTIFIERTOK)
  {
    idtok = gettok();  //Get the next identifier
    keytok = gettok();
    if(keytok->tokentype!= OPERATOR || keytok->whichval!=EQOP)
      yyerror("Invalid constant declaration");
    typet = gettok();  //Get kind of type, whether it be enum, record, array, etc.

    if(typet->tokentype == RESERVED && typet->whichval + RESERVED_BIAS == RECORD)
    {
      fields = NULL;
      front = NULL;
      end = NULL;

      id = searchins(idtok->stringval);
      id->kind = TYPESYM;

      while(peektok()->tokentype!= RESERVED && peektok()->whichval + RESERVED_BIAS!= END)
      {
        if(peektok()->tokentype == IDENTIFIERTOK)
        {
        keytok = gettok();
        // if(front == NULL){
        //   front = keytok;
        // }
        if(fields == NULL)
          fields = keytok;
          
        else
           end->link = keytok;
           keytok->link = NULL;
           end = keytok;

        }
        else if(peektok()->tokentype == DELIMITER && peektok()->whichval + DELIMITER_BIAS == COMMA)
          gettok();
        else if(peektok()->tokentype == DELIMITER && peektok()->whichval + DELIMITER_BIAS == COLON)
        {
          gettok(); //get rid of colon
          fieldtype = gettok();
          fieldtypesym = searchst(fieldtype->stringval);
          fieldtype->symtype = fieldtypesym;
          instfields(fields,fieldtype);
          front = nconc(front,fields);
          fields = NULL;

        }
        if(peektok()->tokentype == DELIMITER && peektok()->whichval + DELIMITER_BIAS == SEMICOLON)
          gettok();
      }
      gettok();  // Get the end token
      rectok = instrec(talloc(), front);

      id->size = rectok->symentry->size;
      id->datatype = rectok->symentry;

    }
    else if(typet->tokentype == DELIMITER && typet->whichval + DELIMITER_BIAS == LPAREN)
    {
      idlist = NULL;
      enumtok = NULL;
      eend = NULL;
      while(peektok()->tokentype!= DELIMITER && peektok()->whichval + DELIMITER_BIAS != RPAREN)
      {
        if(peektok()->tokentype == IDENTIFIERTOK)
        {
          keytok = gettok();
          if(idlist == NULL)
            idlist = keytok;
          else
            eend->link = keytok;
            keytok->link = NULL;
            eend = keytok;
        }
        if(peektok()->tokentype == DELIMITER && peektok()->whichval + DELIMITER_BIAS == COMMA)
          gettok();

      }
      gettok(); //Remove the r paren
      enumtok = instenum(idlist);
      id = searchins(idtok->stringval);
      id->kind = TYPESYM;
      id->size = 4;
      id->datatype = enumtok->symtype;
      
    }
    else if(typet->tokentype == OPERATOR && typet->whichval + OPERATOR_BIAS == POINT)
    {
      pointertok = NULL;
      pointertok = instpoint(talloc(), gettok());
      id = searchins(idtok->stringval);
      id->kind = TYPESYM;
      id->size = 8;
      id->datatype = pointertok->symtype;
      id->basicdt = pointertok->symtype->basicdt;
    }
      if(peektok()->tokentype == DELIMITER && peektok()->whichval + DELIMITER_BIAS == SEMICOLON)
      gettok();

  }
}

TOKEN instrec(TOKEN rectok, TOKEN argstok)
{ SYMBOL rec;
  TOKEN iterate = argstok;
  TOKEN offsize = argstok;
  int size = 0;
  int isize;
  while(iterate->link!=NULL)
  {
    iterate->symentry->link = iterate->link->symentry;
    iterate = iterate->link;  
  }
  while(offsize!=NULL)
  {
    isize = offsize->symentry->size;
    size = wordaddress(size, isize);
    offsize->symentry->offset = size;
    size+=isize;
    offsize = offsize->link;

  }
  rec = symalloc();
  rec->kind = RECORDSYM;
  rec->size = size;
  rec->datatype = argstok->symentry;
  rectok->symtype = rec->datatype;
  rectok->symentry = rec;
  return rectok;



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

TOKEN instarray(TOKEN bounds, TOKEN typetok)
{
  SYMBOL bound,bound1, arra1;
  TOKEN arr = talloc();
  SYMBOL arra = symalloc();
  if(bounds->link == NULL)
  {
  bound = bounds->symentry;
  arra->kind = ARRAYSYM;

  arra->datatype = searchst(typetok->stringval);
  arra->lowbound = bound->lowbound;
  arra->highbound = bound->highbound;
  arra->size = ((arra->highbound-arra->lowbound+1)*(arra->datatype->size));
  arr->symentry = arra;

  }
  else
  {
    arra1 = symalloc();
    bound1 = searchst(bounds->link->stringval);
    arra1->kind = ARRAYSYM;
    arra1->datatype = searchst(typetok->stringval);
    arra1->lowbound = bound1->datatype->lowbound;
    arra1->highbound = bound1->datatype->highbound;
    arra1->size = ((arra1->highbound-arra1->lowbound+1)*(arra1->datatype->size));

    bound = bounds->symentry;
    arra->kind = ARRAYSYM;
    arra->datatype = arra1;
    arra->lowbound = bound->lowbound;
    arra->highbound = bound->highbound;
    arra->size = ((arra->highbound-arra->lowbound+1)*(arra->datatype->size));
    arr->symentry = arra;
  }

  return arr;
}

 int offs = 0;
 void  instvars(TOKEN idlist, TOKEN typetok)
 {
    SYMBOL type, sub;
    SYMBOL sym = NULL;
    TOKEN arr, bound, keytok, end;
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
        offs = wordaddress(offs,4);
        sym->offset = offs;
        offs+=4;
        idlist = idlist->link;
       }
    }
    else if(!strcmp(typetok->stringval,"real"))
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
        offs = wordaddress(offs,8);
        sym->offset = offs;
        offs+=8;
        idlist = idlist->link;
      }
    }
    else if(typetok->tokentype == RESERVED && typetok->whichval + RESERVED_BIAS == ARRAY)
    {
      gettok(); //Get LBRACKET
      bound = NULL;
      while(peektok()->whichval + DELIMITER_BIAS != RBRACKET)
      {
        if(peektok()->tokentype == NUMBERTOK)
        {
          sub = symalloc();
          sub->kind = SUBRANGE;
          sub->lowbound = gettok()->intval;
          gettok(); //get rid of ..
          sub->highbound = gettok()->intval;
          if(bound==NULL)
          {
            bound = talloc();
            bound->symentry = sub;
            bound->link = NULL;
          }

        }
        else if(peektok()->tokentype == IDENTIFIERTOK)
          bound->link = gettok();
        else
          gettok();
      }
        gettok(); //Consume RBRACKET
        gettok();  //Consume of reserved word
        offs = wordaddress(offs,16);
        arr = gettok();
        arr = instarray(bound, arr);

        sym = searchins(idlist->stringval);
        sym->kind = VARSYM;
        sym->datatype = arr->symentry;
        sym->size = arr->symentry->size;
        sym->offset = offs;
        offs+=sym->size;

    }
    else
    {
      type = searchst(typetok->stringval);
      while(idlist!=NULL)
      {
        sym = searchins(idlist->stringval);
        sym->kind = VARSYM;
        sym->datatype = type->datatype;
        sym->size = type->size;
        sym->offset = offs;
        sym->basicdt = type->basicdt;
        offs+= sym->size;
        idlist = idlist->link;
      }
    }
    blockoffs[blocknumber] = offs;
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

TOKEN reducedot(TOKEN var, TOKEN dot, TOKEN field)
  {
    
    SYMBOL sym,test;
    if(var->symtype->datatype->kind == RECORDSYM)
    sym = var->symtype->datatype->datatype;
    else if(var->symtype->datatype->kind == TYPESYM)
      sym = var->symtype->datatype->datatype->datatype;
    else if(var->symtype->kind == RECORDSYM)
      sym = var->symtype->datatype;
    else
      sym = var->symtype;

    while(sym!=NULL)
    {
      if(!strcmp(sym->namestring,field->stringval))
      {
        dot->tokentype = NUMBERTOK;
        dot->intval = sym->offset;
        // if(!strcmp(sym->namestring,"location"))
        // printf("%s\n",sym->datatype->namestring);
        if(sym->datatype->kind == POINTERSYM)
          field->symtype = sym->datatype->datatype;
        else
        field->symtype = sym;
      // printf("%d\n", field->symtype->size);
      //   printf("%s\n",field->symtype->namestring);
        if(var->tokentype == OPERATOR && var->whichval == AREFOP)
          return addoffs(var,dot);
        return makearef(var, dot, field);
      }
      sym = sym->link;

    }
    return var;
  }

// TOKEN addoffs(TOKEN expr, TOKEN off)
// {
//   TOKEN offs = expr->operands->link;
//   if(offs->tokentype!= NUMBERTOK)
//       yyerror("AREF OFFSET IS NOT A NUMBER");
//   expr->operands->link->intval = offs->intval + off->intval;
//   return expr; 
// }
TOKEN arrayref(TOKEN arr, TOKEN tok, TOKEN subs, TOKEN tokb)
{ void reduce();
  TOKEN of = NULL;
  TOKEN opnd, op, lower, aref, lhs, rhs,expr, constant;
  aref = NULL;
  opnd = NULL;
  op = NULL;
  // tok->tokentype = NUMBERTOK;
  // tok->intval = 0;
   SYMBOL arra = searchst(arr->stringval)->datatype;

   while(subs!=NULL && arra!=NULL)
   {
      // printf("HELLOTHERE%s\n",subs->stringval);
     if(subs->tokentype == NUMBERTOK)
     {
       if(of==NULL)
       {
         of = talloc();
         of->tokentype = NUMBERTOK;
         of->intval = (subs->intval - arra->lowbound) * arra->datatype->size;

       }

     }

     else if(subs->tokentype == IDENTIFIERTOK)
     {
        subs = findid(subs);
        if(subs->symentry->kind!=CONSTSYM)  //Create expression to deal with VARSYMS
        {

            rhs = copytok(subs);
          of = talloc();
          of->tokentype = NUMBERTOK;
          of->datatype = INTEGER;
          of->intval = arra->datatype->size;
            lhs = of;
          op = makeop(TIMESOP);
          expr = binop(op,lhs,rhs);
          op = makeop(PLUSOP);
          lower = talloc();
          lower->tokentype = NUMBERTOK;
          lower->datatype = INTEGER;
          lower->intval = (0-arra->lowbound)*arra->datatype->size;
          expr = binop(op, lower, expr);
          tokb->symtype = arra;
          aref = makearef(arr,expr,tokb);

        }
        else
        {
          constant = talloc();
          constant->tokentype = NUMBERTOK;
          constant-> datatype = subs->symentry->basicdt;
        if(constant->datatype == INTEGER)
          constant->intval = subs->symentry->constval.intnum * arra->datatype->size;
        // printf("MY VALUE IS %d\n", constant->intval);
        if(aref!=NULL)
          aref = addoffs(aref, constant);

        }
     }
       subs = subs->link;
       arra = arra->datatype;

      
   }
   if(aref!=NULL)
    return aref;
    tokb->symtype = arra;
  return makearef(arr,of,tokb);

}

TOKEN parseassign(TOKEN lhs)  /* Parse an assignment statement */
  {  TOKEN tok, rhs, field, subs, end; TOKEN parseexpr();
    lhs = findid(lhs);
    while(peektok()->tokentype != OPERATOR || peektok()->whichval != ASSIGNOP)
    {
     tok = gettok();
     // if ( tok->tokentype != OPERATOR || tok->whichval != ASSIGNOP )
     //   printf("Unrecognized statement\n");
     if(tok->tokentype == OPERATOR && tok->whichval + OPERATOR_BIAS == POINT)  //adding code
      lhs = dopoint(lhs, tok);
     else if(tok->tokentype == OPERATOR && tok->whichval + OPERATOR_BIAS == DOT)
     {
        field = gettok();  //Get the field
        lhs = reducedot(lhs, tok, field);
     }
     else if(tok->tokentype == DELIMITER && tok->whichval + DELIMITER_BIAS == LBRACKET)
     {
        subs = NULL;

        while(peektok()->tokentype!= DELIMITER || peektok()->whichval + DELIMITER_BIAS != RBRACKET)
        {
          tok = gettok();
          if(tok->tokentype == IDENTIFIERTOK || tok->tokentype == NUMBERTOK)
          {
            if(subs == NULL)
              subs = tok;
            else
              end->link = tok;
              tok->link = NULL;
              end = tok;
          }


        }
        tok = gettok(); //Get RBRACKET
        // if(subs->link!=NULL)
        // printf("%s\n",subs->link->stringval);
        lhs = arrayref(lhs,tok, subs, talloc());
     }
    }
      tok = gettok(); //Get ASSIGNOP
     rhs = parseexpr(); //stopped adding above this current line
     if(lhs->tokentype == IDENTIFIERTOK)
      findid(lhs);
     if(rhs->tokentype == IDENTIFIERTOK)
      findid(rhs);
      tok->datatype = lhs->datatype; // Added recently
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



TOKEN parsewhile(TOKEN keytok)
{ TOKEN statement(); TOKEN parseexpr();
  TOKEN expr, lab, ifstatement, thenexpr, got,end;
  lab = makelabel(keytok);
  expr = parseexpr();  //Get expr for if statement
  keytok = gettok();
  if(keytok->tokentype!= RESERVED && keytok->whichval + RESERVED_BIAS != DO)
    yyerror("Missing DO in while statement");
  thenexpr = statement();
  end = thenexpr->operands;
  got = makegoto(lab->operands->intval);
  while(end!=NULL)
  {
    if(end->link == NULL){
      end->link = got;
      break;}
    end = end->link;
  }
  ifstatement = makeif(talloc(), expr, thenexpr, NULL);
  lab->link = ifstatement;
  
  return makeprogn(talloc(),lab);

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
  TOKEN tok, args, id;
  args = NULL;
  tok = gettok();  /* Remove lparen */
  // if(!strcmp(keytok->stringval,"write"))
  //   strcpy(keytok->stringval,"writei");
  while(peektok()->whichval + DELIMITER_BIAS!= RPAREN)
  {
  if(peektok()->tokentype == IDENTIFIERTOK)
  {
    if(!strcmp(keytok->stringval,"write")|| !strcmp(keytok->stringval,"writeln"))
    {
      id = findid(peektok());
      if(id->symtype->basicdt == INTEGER)
      {
        if(!strcmp(keytok->stringval,"write"))
          strcpy(keytok->stringval,"writei");
        else if(!strcmp(keytok->stringval,"writeln"))
          strcpy(keytok->stringval,"writelni");
      }
      else
      {
        if(!strcmp(keytok->stringval,"write"))
          strcpy(keytok->stringval,"writef");
        else if(!strcmp(keytok->stringval,"writeln"))
          strcpy(keytok->stringval,"writelnf");
      }
    }
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
  {  TOKEN tok, op, lhs, rhs, constant, opnd; int state, done, gotparen;
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
     {  case RESERVED:
          if(tok->whichval + RESERVED_BIAS == NIL)
          {
            tok = gettok();
            tok->tokentype = NUMBERTOK;
            tok->datatype = INTEGER;
            tok->intval = 0;
            opndstack = cons(tok, opndstack);
          }
          else
            done = 1;
          break;
        case IDENTIFIERTOK: 
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
         if ( tok->whichval == DOTOP )   /* special case for now */
       {
          tok = gettok();
          opnd = opndstack;
          opndstack = opndstack->link;
          tok = reducedot(opnd, tok, gettok());
          opndstack = cons(tok, opndstack);
       }
        else if( tok->whichval == POINTEROP)
        {
          tok = gettok();
          opnd = opndstack;
          opndstack = opndstack->link;
          tok = dopoint(opnd,tok);
          opndstack = cons(tok, opndstack);
        }
       else{ 
        tok = gettok();
       while ( opstack != NULL && opstack->tokentype != DELIMITER
        && (precedence[opstack->whichval]
             >= precedence[tok->whichval]))
         reduce(&opstack, &opndstack);
       opstack = cons(tok,opstack);
     }
           // else done = 1;
         break;
       default: done = 1;
       }
       }
     while ( opstack != NULL ) reduce(&opstack, &opndstack);
     return (opndstack);
  }


TOKEN parsenew(TOKEN keytok)
{
  TOKEN name, assign, varn, opnds;
  SYMBOL sym;
  gettok(); //Get rid of LPAREN
  name = gettok();  //Get name of variable
  gettok();  //Get rid of RPAREN;
  assign = makeop(ASSIGNOP);
  varn = talloc();
  varn->tokentype = NUMBERTOK;
  varn->intval = searchst(name->stringval)->datatype->datatype->size;
  opnds = makefuncall(talloc(),keytok, varn);
  opnds->link = name;
  reduce(&assign, &opnds);
  return opnds;

}

TOKEN dolabel(TOKEN labeltok, TOKEN tok, TOKEN statement)
{ 
  TOKEN nok= talloc();
  int i;
  for(i=0; i<50; i++)
  {
    if(labels[i]==labeltok->intval)
      break;
  }
  nok->tokentype = NUMBERTOK;
  nok->datatype = INTEGER;
  nok->intval = i;
  tok->tokentype = OPERATOR;
  tok->whichval = LABELOP;
  tok->operands = nok;
  nok->link = NULL;
  tok->link = statement;
  return makeprogn(talloc(),tok);

}
TOKEN parsegoto (TOKEN keytok)
{
  TOKEN got = gettok();
  int i;
  for(i=0; i<50; i++)
  {
    if(labels[i]==got->intval)
      break;
  }
  return makegoto(i);

}
TOKEN parselabel (TOKEN keytok)
{ TOKEN statement();
  TOKEN tok = gettok();
  TOKEN stat;
  if(tok->tokentype!=DELIMITER && tok->whichval+DELIMITER_BIAS!= COLON)
    yyerror("Invalid label statement");
  stat = statement();
  return dolabel(keytok, tok, stat);
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
     case TYPE:      parsetype(tok);
          break;
     case LABEL:     instlabel(tok);
          break;
     case WHILE:      result = parsewhile(tok);
          break;
     case GOTO:       result = parsegoto(tok);
          break;
   }
       else if (tok->tokentype == IDENTIFIERTOK)
                if(!strcmp(tok->stringval,"new"))
                      result = parsenew(tok);
                else if ( (pok->whichval + DELIMITER_BIAS) == LPAREN )
                      result = parsefunc(tok);
                else
                      result = parseassign(tok);
       else if (tok->tokentype == NUMBERTOK)
                      result = parselabel(tok);
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
    // printf("Started parser test.\n");
    res = yyparse();
    // printf("yyparse result = %8d\n", res);
    // printst();   //added; meant to print out the symbol table
    if (DEBUG & DB_PARSERES) dbugprinttok(parseresult);
    // ppexpr(parseresult);
    gencode(parseresult, blockoffs[blocknumber], labelnumber);
    }