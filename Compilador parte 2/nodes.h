
#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "backllvm.h"

class Node;
#include "gramatica.h"
using namespace std;

int errorcount = 0;

// symbol table
map<string, Value*> symbols;

class Node {
protected:
	vector<Node*> children;

public:
	void addChild(Node *n) {
		children.push_back(n);
	}

	vector<Node*> const& getChildren() {
		return children;
	}

	virtual string toStr() {
		return "node";
	}

	virtual Value* codegen() {
		for(Node *n : children) {
			n->codegen();
		}
		return NULL;
	}
};

class Program: public Node {
public:
	virtual string toStr() override {
		return "program";
	}
};

class Attr: public Node {
protected:
	string ident;
public:
	Attr(string id, Node *expr) {
		ident = id;
		children.push_back(expr);
	}

	virtual string toStr() override {
		string r(ident);
		r.append("=");
		return r;
	}

	const string getName() {
		return ident;
	}

	virtual Value* codegen() override {
		Value *exprv = children[0]->codegen();

		Value *address = NULL;
		if (symbols.count(ident) == 0) {
			address = backend.CreateAlloca(
				exprv->getType(), 0, NULL,
				ident);
			symbols[ident] = address;
		} else {
			address = symbols[ident];
		}

		return backend.CreateStore(exprv, address);
	}
};

class Print: public Node {
public:
	Print(Node *expr) {
		children.push_back(expr);
	}

	virtual string toStr() override {
		return "print";
	}

	virtual Value* codegen() override {
		Value *exprv = children[0]->codegen();
		vector<Value*> args;
		args.push_back(exprv);
		return backend.CreateCall(printfloat, args);
	}
};

/*IF e ELSY*/
class IF : public Node {
public:
    IF(Node *logical, Node *stmts, Node *elsy) {
        children.push_back(logical);
        children.push_back(stmts);
        if (elsy != NULL) {
            children.push_back(elsy);
        }
    }

    virtual string toStr() override {
        return "if";
    }

    virtual Value* codegen() override {
        BasicBlock *if_condition = BasicBlock::Create(ctx, "if_condition", current_func);
        BasicBlock *body_if = BasicBlock::Create(ctx, "body_if", current_func);
        BasicBlock *else_block = BasicBlock::Create(ctx, "else_block", current_func);
        BasicBlock *contin_if = BasicBlock::Create(ctx, "contin_if", current_func);

        // setup entry block, goto condition
        backend.CreateBr(if_condition);

        // setup condition block
        backend.SetInsertPoint(if_condition);
        Value *expr = children[0]->codegen();
        backend.CreateCondBr(expr, body_if, else_block);

        // setup body block
        backend.SetInsertPoint(body_if);
        children[1]->codegen();
        backend.CreateBr(contin_if);

        // setup else block
        backend.SetInsertPoint(else_block);
        if (children.size() == 3) {
            children[2]->codegen();
        }
        backend.CreateBr(contin_if);

        backend.SetInsertPoint(contin_if);
        return contin_if;
    }
};

		
/*---------------*/


/*And---------Or--------*/

class AndOr : public Node {
protected:
	int op;
public: 
	AndOr(Node *left, Node *rigth, int oper){
	  children.push_back(left);
	  children.push_back(rigth);
	  op = oper;	
	}
	virtual string toStr() override {
	string r;
	r.push_back(op);
	return r;
	}
	virtual Value* codegen() override {
		Value *lv = children[0]->codegen();
		Value *rv = children[1]->codegen();
		switch (op) {
			case '&': return backend.CreateAnd(lv, rv);
			case '|': return backend.CreateOr(lv,rv);

			default: cerr << "Fail! Operador não implementado: " << (char)op << endl;
		}
		return NULL;
	}
};
/*--------------------*/


/*-----------------------*/


/*Expressão aritimetica*/
class BinaryOp : public Node {
protected:
	char op;
public:
	BinaryOp(Node *left, Node *right,
		char oper) {
		children.push_back(left);
		children.push_back(right);
		op = oper;
	}

	virtual string toStr() override {
		string r;
		r.push_back(op);
		return r;
	}

	virtual Value* codegen() override {
		Value *lv = children[0]->codegen();
		Value *rv = children[1]->codegen();
		switch (op) {
			case '+': return backend.CreateFAdd(lv, rv);
			case '-': return backend.CreateFSub(lv, rv);
			case '*': return backend.CreateFMul(lv, rv);
			case '/': return backend.CreateFDiv(lv, rv);

			default: cerr << "Fail! Operador não implementado: " << op << endl;
		}
		return NULL;
	}
};

class Ident: public Node {
protected:
	string name;
public:
	Ident(string name) {
		this->name = name;
	}
	
	virtual string toStr() override {
		return name;
	}

	const string getName() {
		return name;
	}

	virtual Value* codegen() override {
		Value *symbol = symbols[name];
		AllocaInst* ai = dyn_cast<AllocaInst>(symbol);
		Type *st = ai->getAllocatedType();
		return backend.CreateLoad(st, symbol, name);
	}
};

class Float: public Node {
protected:
	double value;
public:
	Float(double v) {
		value = v;
	}
	virtual string toStr() override {
		return to_string(value);
	}
	virtual Value* codegen() override {
		return ConstantFP::get(ctx, APFloat(value));
	}
};

class Int: public Node {
protected:
	int value;
public:
	Int(int v) {
		value = v;
	}
	virtual string toStr() override {
		return to_string(value);
	}
	virtual Value* codegen() override {
		return ConstantFP::get(ctx, APFloat((double)value));
	}
};

class While: public Node {
public:
	While(Node *logical, Node *stmts) {
		children.push_back(logical);
		children.push_back(stmts);
	}

	virtual string toStr() override {
		return "while";
	}

	virtual Value* codegen() override {
		BasicBlock *condition = BasicBlock::Create(ctx, "cond", current_func);
		BasicBlock *body = BasicBlock::Create(ctx, "body", current_func);
		BasicBlock *contin = BasicBlock::Create(ctx, "contin", current_func);

		// setup entry block, goto condition
		backend.CreateBr(condition);

		// setup condition block
		backend.SetInsertPoint(condition);
		Value *expr = children[0]->codegen();
		backend.CreateCondBr(expr, body, contin);

		// setup body block
		backend.SetInsertPoint(body);
		children[1]->codegen();
		backend.CreateBr(condition);

		backend.SetInsertPoint(contin);
		return contin;
	}

};
/*Expressão logica*/
class Logical : public Node {
protected:
    string oper; // Representa o operador composto, como ">="

public:
    Logical(Node *le, const string &op, Node *re) {
        children.push_back(le);
        children.push_back(re);
        oper = op;
    }

    virtual string toStr() override {
        return oper;
    }

    virtual Value *codegen() override {
        Value *lv = children[0]->codegen();
        Value *rv = children[1]->codegen();

        if (oper == ">") {
            return backend.CreateFCmpOGT(lv, rv);
        } else if (oper == "<") {
            return backend.CreateFCmpOLT(lv, rv);
        } else if (oper == ">=") {
            return backend.CreateFCmpOGE(lv, rv);
        } else if (oper == "<=") {
            return backend.CreateFCmpOLE(lv, rv);
        } else if (oper == "!=") {
            return backend.CreateFCmpUNE(lv, rv);
        } else if (oper == "==") {
            return backend.CreateFCmpOEQ(lv, rv);
        } else {
            cerr << "Fail! Operador não implementado: " << oper << endl;
        }

        return NULL;
    }
};

class PrintTree {
public:
	void printRecursive(Node *n) {
		for(Node *c : n->getChildren()) {
			printRecursive(c);
		}

		cout << "n" << (long)n;
		cout << "[label=\"" << n->toStr() << "\"]";
		cout << ";" << endl;

		for(Node *c : n->getChildren()) {
			cout << "n" << (long)n << " -- " <<
					"n" << (long)c << ";" << endl;
		}
	}

	void print(Node *n) {
		cout << "graph {\n";
		printRecursive(n);
		cout << "}\n";
	}
};

//verificação semantica de declaração de variavel 
class CheckUndeclaredVar {
private:
	set<string> vars;
public:
	void checkRecursive(Node *n) {
		// visit left and right
		for(Node *c : n->getChildren()) {
			checkRecursive(c);
		}

		// visit root
		Attr *a = dynamic_cast<Attr*>(n);
		if (a) {
			// visiting an Attr node, new var
			vars.insert(a->getName());
		} else {
			// visiting an Ident(load) node,
			// check if var exists
			Ident *i = dynamic_cast<Ident*>(n);
			if (i) {
				if (vars.count(i->getName()) == 0) {
					// undeclared var
					cout << "Undeclared var " <<
						i->getName() << endl;
					errorcount++;
				}
			}
		}
		
	}

	void check(Node *n) {
		checkRecursive(n);
	}
};
class Check_vars_used{
private:
	set<string> vars;
	set<string> vars_used;
	set<string> difference;
public:	
	void check_vars_used(Node *n){
	//visit  left and rigth
	    for(Node *c : n->getChildren()){
	       check_vars_used(c); 
	    }
	    //visit root
	    Attr *a= dynamic_cast<Attr*>(n);
	    if (a){
	    	//visiting a Attr node, new var
	    	vars.insert(a->getName());
	    	
	    }else {
	    	//visiting a Ident(load) node,
	    	//check if var exists
	    	Ident *i = dynamic_cast<Ident*>(n);
	    	if(i){ 
	    	vars_used.insert(i->getName());	    	      	       	        	    	
	    	  }
	    	 }
	    	}
	     
	    
	void check(Node *n){
	check_vars_used(n);
	Ident *i = dynamic_cast<Ident*>(n);
	
	 for (auto &v : vars) {
	     
	     //cout << "vars created "<< v << endl;
	      if (vars_used.find(v) == vars_used.end()) {
               difference.insert(v);
	   }
	  }
	  cout << endl;
	  
	for (auto &v : vars_used ) {
	     
	    // cout << "used var "<< v  << endl;  	
	  	 
	  }
	  
	  for (const auto &v : difference) {
    	      cout << "Variable created but not used: " << v << endl;
    	      errorcount++;
		}	   
	}
	
};



class CodeGen {
public:
	void generate(Node *p) {
		setup_llvm();
		p->codegen();

		// terminate main function
		Value *retv = ConstantInt::get(ctx, APInt(16, 0));
		backend.CreateRet(retv);

		module->print(outs(), nullptr);
		print_llvm_ir();
	}
};

