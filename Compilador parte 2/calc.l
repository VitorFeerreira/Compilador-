%{
#include <stdio.h>
#include <stdlib.h>

class Node;
#include "gramatica.h"
%}

%option yylineno

A [a-zA-Z_]
D [0-9]

%%
[ \t\n\r]	{/* ignora espaços*/}

"print"		{ return TOK_PRINT;}
"+"		{ return '+';}
"-"		{ return '-';}
"*"		{ return '*';}
"/"		{ return '/';}
";"		{ return ';';}
"="		{ return '=';}
"("             { return '(';}
")"             { return ')';}
"^"		{ return '^';}
"=="		{return TOK_Igual;}
"!="		{return TOK_Diferente;}
"<="		{return TOK_MenorQ;}
">="		{return TOK_MaiorQ;}

[<>!{}]		{ return yytext[0]; }
"&"		{ return TOK_e; }  /*AND*/
"|"		{ return TOK_ou; } /*OR*/
"si"		{ return TOK_si; } /*if*/ 
"sinaum"	{ return TOK_sinaum; } /*esle*/
"repete"	{ return TOK_repete; } /*while*/
":"             { return ':';}

{A}({A}|{D})*	{yylval.str = strndup(yytext, yyleng); return TOK_IDENT;}
{D}*\.{D}+	{yylval.flt = atof(yytext); return TOK_FLOAT;}
{D}+		{yylval.itg = atoi(yytext); return TOK_INT;}



.		{ printf("Erro: caracter inválido \%c\n", yytext[0]);}
 
%%

int yywrap(){
return 1;
}

char *build_filename = NULL;

int main(int argc, char *argv[]){

if (argc <2){
	printf("Syntaxe: %s arquivo \n", argv[0]);
	return 1;
}
	build_filename = argv[1];
	yyin = fopen(build_filename, "r");
	if (!yyin){
	printf("Não foi possivel abrir o arquivo %s\n", build_filename);
	return 1;
	}
	
	yyparse();
	if (yyin)
	fclose(yyin);
return 0;
}
