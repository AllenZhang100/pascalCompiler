/* Allen Zhang
atz227
CS375-Compilers
/*
/* lex1.c         14 Feb 01; 31 May 12       */

/* This file contains code stubs for the lexical analyzer.
   Rename this file to be lexanc.c and fill in the stubs.    */

/* Copyright (c) 2001 Gordon S. Novak Jr. and
   The University of Texas at Austin. */

/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "token.h"
#include "lexan.h"

/* This file will work as given with an input file consisting only
   of integers separated by blanks:
   make lex1
   lex1
   12345 123    345  357
   */

/* Skip blanks, whitespace, and comments.  When in a comment, skipblanks consumes everything
within the comment, then calls itself again to make sure anything following it is not meant to
be skipped over, ie, a comment following another comment. */
void skipblanks ()
  {
      int c,d;
        while ((c = peekchar()) != EOF
             && (c == ' ' || c == '\n' || c == '\t'))
          getchar();
       if((c=peekchar())=='{')
        {
          getchar();
          while((c=peekchar())!='}')
          getchar();
          if((c=peekchar())=='}')
          getchar();
          skipblanks();
        }




      
      if(((c=peekchar())=='(')&&((d=peek2char())=='*'))
      {
          getchar();
          getchar();
        while((c=peek2char())!=EOF && !(((c=peekchar())=='*')&&((d=peek2char())==')')))
          getchar();
        if((((c=peekchar())=='*')&&((d=peek2char())==')')))
        {
          getchar();
          getchar();
        }
        skipblanks();
      }  
    }

/* Get identifiers and reserved words.  Makes use of a char array of reserved words to
compare whether the input was or was not a reserved word.  If nothing matches, then it
has to be an identifier. */
TOKEN identifier (TOKEN tok)
  {
    int c,i;
    char e;
    char d[15];
    int counter = 0;
    int length = 15;
    int boole= 1;
    const char *resA[6];
    const char *resB[29];
    resA[0] = "and";
    resA[1] = "or";
    resA[2] = "not";
    resA[3] = "div";
    resA[4] = "mod";
    resA[5] = "in";

    resB[0]="array";resB[1]="begin";resB[2]="case";resB[3]="const";resB[4]="do";
    resB[5]="downto";resB[6]="else";resB[7]="end";resB[8]="file";resB[9]="for";
    resB[10]="function";resB[11]="goto";resB[12]="if";resB[13]="label";resB[14]="nil";
    resB[15]="of";resB[16]="packed";resB[17]="procedure";resB[18]="program";resB[19]="record";
    resB[20]="repeat";resB[21]="set";resB[22]="then";resB[23]="to";resB[24]="type";
    resB[25]="until";resB[26]="var";resB[27]="while";resB[28]="with";
    for(i=0;i<15;i++)
    {
      d[i]='\0';
    }
    while((c=peekchar())!=EOF && c!= ' '&& c!= '\n'&&c!= '\t'&& (CHARCLASS[c] == ALPHA 
      ||CHARCLASS[c]==NUMERIC))
    {
        e = getchar();
        if(length>0)
        {
        d[counter] = e;
        counter++;
        length--;
      }
    }

    for(i=0;i<6;i++)
    {
      if(strcmp(d,resA[i])==0)
      {
        tok->tokentype = OPERATOR;
        tok->whichval = i+14;
         tok->datatype = STRINGTYPE;
        boole = 0;
      }
    }
    for(i=0;i<29;i++)
    {
      if(strcmp(d,resB[i])==0)
      {
        tok->tokentype = RESERVED;
        tok->whichval = i+1;
         tok->datatype = STRINGTYPE;
        boole = 0;
      }
    }
    if(boole ==1)
    {
      tok->tokentype = IDENTIFIERTOK;
      strcpy(tok->stringval,d);
    }

    return (tok);
    }

/* Uses a while loop with nested if statements to identify special cases, namely that of EOF
and the case where an apostrophe appears within a string */
TOKEN getstring (TOKEN tok)
  {
    char a;
    int c,d,i;
    int stay = 1;
    i = 0;
    int length = 15;
    if((c = peekchar())== '\'')
    {
      getchar();
      while(stay==1)
      {
        if ((c=peekchar())== EOF)
          stay = 0;
        else if ((c=peekchar())=='\'')
        {
          if ((c=peekchar())==(d=peek2char()))
          {
            a = getchar();
            getchar();
            if(length>0)
            {
            tok->stringval[i]=a;
            i++;
            length--;
            }
          }
          else
          {
            stay = 0;
            getchar();
          }

        }
        else
        {
          a = getchar();
          if(length>0)
          {
          tok->stringval[i]=a;
          i++;
          length--;
          }
        }
      }
    }
    tok->stringval[i]='\0';
    tok->tokentype = STRINGTOK;
    // tok->datatype = STRINGTYPE;
    return (tok);
    }

TOKEN special (TOKEN tok)
  {
    int i,j,c;
    char e;
    int counter = 0;
    char d[4];
    const char *a[13];
    const char *b[8];
    int stop = 1;
    a[0] = "+";
    a[1] = "-";
    a[2] = "*";
    a[3] = "/";
    a[4] = ":="; 
    a[5] = "=";
    a[6] = "<>";
    a[7] = "<";
    a[8] = "<=";
    a[9] = ">=";
    a[10] = ">";
    a[11] = "^";
    a[12] = ".";

    b[0] = ",";
    b[1] = ";";
    b[2] = ":";
    b[3] = "(";
    b[4] = ")";
    b[5] = "[";
    b[6] = "]";
    b[7] = "..";
    for(i=0;i<4;i++)
    {
      d[i]='\0';
    }

    while((c=peekchar())!=EOF && c!= ' '&& c!= '\n'&&c!= '\t'&& CHARCLASS[c] == SPECIAL && stop==1)
    {
        if(counter==0)
        {
        e = getchar();
        d[counter] = e;
        counter++;
        }
        else if (strcmp(d,b[2])==0)
        {
          if((c=peekchar())=='=')
          {
            e = getchar();
            d[counter] = e;
            counter++;
          }
          stop = 0;
        }
        else if (strcmp(d,a[7])==0)
        {
          if((c=peekchar())=='>' || (c=peekchar())=='=')
          {
            e = getchar();
            d[counter] = e;
            counter++;
          }
          stop = 0;
        }
        else if (strcmp(d,a[10])==0)
        {
          if((c=peekchar())=='=')
          {
            e = getchar();
            d[counter] = e;
            counter++;
          }
          stop = 0;
        }
        else if (strcmp(d,a[12])==0)
        {
          if((c=peekchar())=='.')
          {
            e = getchar();
            d[counter] = e;
            counter++;
          }
          stop = 0;
        }
        else
          stop = 0;
    }


    for(i=0;i<13;i++)
    {
      if(strcmp(d,a[i])==0)
      {
        tok->tokentype = OPERATOR;
        tok->whichval = i+1;
      }
    }

      for(j = 0; j<8; j++)
    {
      if(strcmp(d,b[j])==0)
      {
        tok->tokentype = DELIMITER;
        tok->whichval = j+1;
      }
    }

     return (tok);

  }

/* Get and convert unsigned numbers of all types. */
TOKEN number (TOKEN tok)
   { long num;
    int  c,i, charval;
    num = 0;
    int isReal = 0;
    int power = 0;
    int length = 0;
    int powermult = 0;
    double mult = 1;
    while ( (c = peekchar()) != EOF
            && CHARCLASS[c] == NUMERIC)
      {   c = getchar();
          charval = (c - '0');
          if(charval==0 && num ==0)
          isReal = 0;
          else{
          if(length<11){
          num = num * 10 + charval;
          length++;}
          else
            power++;
          }
        }
    if((c=peekchar())=='.' && CHARCLASS[peek2char()]== NUMERIC)
    {
      isReal = 1;
      getchar();
      while ( (c = peekchar()) != EOF
            && CHARCLASS[c] == NUMERIC)
      {   c = getchar();
          charval = (c - '0');
          if(charval==0 && num==0)
          power--; 
          else if(length<8){
          num = num * 10 + charval;
          length++;
          power--;}
          
        }

    }
    if((c=peekchar())=='e' && ((c=peek2char())=='+' || CHARCLASS[peek2char()]==NUMERIC))
    {
      isReal=1;
      getchar();
      if((c=peekchar())=='+')
        getchar();

      while((c=peekchar()!=EOF) && CHARCLASS[peekchar()]==NUMERIC)
      {
        c = getchar();
        charval = (c- '0');
        powermult = powermult*10 + charval;
      }
    }

    if((c=peekchar())=='e' && ((c=peek2char())=='-'))
    {
      isReal=1;
      getchar();
      getchar();

      while((c=peekchar()!=EOF) && CHARCLASS[peekchar()]==NUMERIC)
      {
        c = getchar();
        charval = (c- '0');
        powermult = powermult*10 + charval;
      }
        powermult*= -1;
    }
      if(powermult+power+length-1>38 || powermult+power+length-1 <-38){
      powermult = 0;
      power = -50;}

    if(isReal==0){
    if(num > 2147483647 || length>10)
      printf("%s\n","Integer number out of range");
    tok->tokentype = NUMBERTOK;
    tok->datatype = INTEGER;
    tok->intval = num;}
    else
    {
      power+=powermult;
      if(power==-50)
        printf("%s\n","Float number out of range");
      if(power<0)
      {
        for(i = power; i<0; i++)
          mult*=10;
        tok->tokentype = NUMBERTOK;
        tok -> datatype = REAL;
        tok ->realval = num/mult;
      }
      else if(power>0)
      {
        for(i = power; i>0; i--)
          mult*=10;
        tok->tokentype = NUMBERTOK;
        tok -> datatype = REAL;
        tok ->realval = num*mult;
      }

     }
    return (tok);
  }

