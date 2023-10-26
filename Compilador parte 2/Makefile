LLVM_FLAGS=`llvm-config-16 --cxxflags --ldflags --libs`

all: 
	flex -o calc.yy.c calc.l
	bison -Hgramatica.h calc.y
	clang++ -o0 -ggdb $(LLVM_FLAGS) calc.tab.c calc.yy.c -o compilador
	
clean:
	rm -f compilador gramatica.h calc.tab.c calc.yy.c	
