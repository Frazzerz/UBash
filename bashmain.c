#include "bash.h"

int main(){
	char input[MAXINPUT]; 																		// buffer input
	char **cmds[MAXCMD];																		// Array di array di stringhe (struttura dati usata)
	int n;																						// numero di comandi
	pid_t pid=-1;																				

	for(;;){
		printDir();																				// Stampa directory corrente
		if(takeinput(input)==-1)																// takeinput ritorna -1 se l'input è vuoto
			continue;
		
		if((n=parsePipe(input, cmds))==1){														// se ho un solo comando
			if(strcmp(cmds[1][0], "cd")==0){													// se è cd
				if(chdir(cmds[1][1])==-1)														// cambio directory corrente
					printf("cd: %s: File o directory non esistente\n", cmds[1][1]);				// segnalo se non esiste
			}
			else
				execcmd(cmds[1], pid);															// chiamo funzione esegui comando singolo
		}
			
		else if(n>1){
			execpipe(cmds, n, pid);																// chiamo funzione esegui + comandi
		}

		for(int i=0; i<=n; i++)																	// liberare memoria allocata per i vari comandi, dopo aver eseguito
			free(cmds[i]);	

	}	
}
