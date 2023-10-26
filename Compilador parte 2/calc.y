%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nodes.h"
#include "gramatica.h"
int yyerror(const char *s);
int yylex(void);
class Node;
%}


%define parse.error verbose

%union{
	char *str;
	int itg;
	double flt;
	Node *node;
}

%token TOK_PRINT TOK_e TOK_ou TOK_si TOK_sinaum TOK_repete 
%token TOK_Igual TOK_Diferente TOK_MenorQ TOK_MaiorQ
%token TOK_IDENT TOK_FLOAT TOK_INT

%type<str> TOK_IDENT
%type<itg> TOK_INT
%type<flt> TOK_FLOAT
%type<node> factor expr term stmt stmts 
%type<node> IF While logical Iterm Ifactor 

%start program

%%

program : stmts {
	Program *p = new Program();
	p->addChild($stmts);
	
	//analise semántica (imprime a arvore)
	PrintTree pt;
	pt.print(p);
	
	//verificação semantica 
	CheckUndeclaredVar ck;
	ck.check(p);
	
	Check_vars_used cv;
	cv.check(p);
	
	if (errorcount > 0){
	   printf("%d erro(s) encontrados. \n",errorcount);
	   exit(errorcount);
	}else {
	   CodeGen cg;
	   cg.generate(p);
	}
}
	
	
stmts : stmts[ss] stmt {
	$ss->addChild($stmt);
	$$ = $ss;
}
stmts: stmt {
	Node *stmts = new Node();
	stmts->addChild($stmt);
	$$ = stmts;
}
      
stmt : TOK_IDENT[id] '=' expr ';'{
	$$ = new Attr($id, $expr);
} 
  	
stmt : TOK_PRINT expr ';'{
	$$ = new Print($expr);
}

stmt : IF {
	$$ = $IF;
}
stmt : While {
	$$ = $While;
}	

stmt : logical {
	$$ = $logical;
}


/*If e Else*/

IF: TOK_si '('logical')' '{' stmts'}'{
  $$ = new IF($logical,$stmts,NULL); 
}
 
IF: TOK_si '('logical')' '{' stmts[stm] '}' TOK_sinaum '{' stmts[st] '}'{
   $$ = new IF($logical,$stm,$st); 
 }
 

/*While*/ 
While : TOK_repete '(' logical ')''{' stmts '}'';'{
       $$ = new While($logical,$stmts); 
}

/* AND e OR*/ 
  
logical : logical[And] TOK_e Iterm[it]{
         $$ = new AndOr($And,$it, '&');           
}

logical : Iterm {
	$$ = $Iterm;
	}
	
Iterm   : Iterm[Or] TOK_ou Ifactor[ifa]{
        $$ = new AndOr($Or,$ifa, '|'); 
   
}
Iterm	: Ifactor {
        $$ = $Ifactor;
}

/*Expressao logica*/		 	            	
Ifactor : '(' logical ')' {
        $$ = $logical;
}

Ifactor : expr[le] '<' expr[re] {         
            $$ = new Logical($le, "<", $re);
            }
          
Ifactor : expr[le] '>' expr[re] {
             $$ = new Logical($le,">",$re);
            }
            
Ifactor : expr[le] TOK_Igual expr[re] {
             $$ = new Logical($le, "==",$re );
            }
            
 Ifactor : expr[le] TOK_Diferente expr[re] {
             $$ = new Logical($le, "!=",$re);
            }
            
Ifactor : expr[le] TOK_MenorQ expr[re] {
             $$ = new Logical($le, "<=",$re);
            }
            
Ifactor : expr[le] TOK_MaiorQ expr[re]{
             $$ = new Logical($le, ">=",$re);
            }
 
/*Expressao aritimetica*/

expr : expr[ex] '+' term{
	$$ = new BinaryOp($ex,$term, '+');
}

expr : expr[ex] '-' term{
	$$ = new BinaryOp($ex,$term, '-');
}

expr : term {
	$$ = $term;
}
   
term : term[te] '*' factor{
	$$ = new BinaryOp($te,$factor, '*');
}

term : term[te] '/' factor {
	$$ = new BinaryOp($te,$factor, '/');
}
                
term : factor {
	$$ = $factor;
} 
     
factor : '(' expr ')'{
	$$ = $expr;
}
factor : TOK_INT[itg]{
	$$ = new Int($itg);

} 
factor : TOK_FLOAT[flt]{
	$$ = new Float($flt);
} 
factor : TOK_IDENT[id]{
	$$ = new Ident($id);
}
                 
%%

extern int yylineno;


int yyerror(const char *s){

    printf("erro da linha %d: %s\n", yylineno, s);
    errorcount++;
    return 1;

}

