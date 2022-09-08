uB=bash
MAIN=bashmain
FUNCT=bashfunz
Comp=gcc -std=gnu11
Cflag=-Wall -pedantic -Werror -g -fsanitize=address 
LIBS=-lreadline

$(uB): $(MAIN).o $(FUNCT).o
	$(Comp) $(Cflag) $^ -o $@ $(LIBS)

$(MAIN).o: $(MAIN).c 
	$(Comp) $(Cflag) $^ -c $(LIBS)

$(FUNCT).o: $(FUNCT).c
	$(Comp) $(Cflag) $^ -c $(LIBS)

clean: 
	rm -f $(uB) *.o a.out

.PHONY: clean
