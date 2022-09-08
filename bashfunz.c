#include "bash.h"

//////////////// Funzione che stampa directory corrente ///////////////

void printDir(){												
	char cwd[1024];												// buffer per la directory
	if(getcwd(cwd, sizeof(cwd))==NULL){
		perror("can't find current directory, setting to default");
		strcpy(cwd, "default_dir");
	}
	char* user;
	if((user=getenv("USER"))==NULL){								// controllo che la variabile esista
		perror("can't find USER variable, setting to default");
		user="default_user";
	}
	printf("User:%s-path:%s", user, cwd);						// uso getenv per mettere il nome dell'utente
}

/////////////// Funzione che prende la stringa da stdi ////////////////

int takeinput(char* str){										
	char* buf;							// Buffer per la stringa inserita
	
	buf=readline("$ ");					// readline alloca la memoria, ma in caso di SIGINT/SIGTERM non viene liberata: abbiam notato il problema usando valgrind, ma non abbiam saputo rimediare
	if(strlen(buf)!=0){					// Controllo se input è vuoto
		strcpy(str, buf);										
        free(buf);                      // libero il buffer
		return 0;
	}	
	else
		return -1;
}

////////////// Funzione che divide comandi dagli argomenti //////////////////

int redircheck(char** tokens[], int cmdno, int cmd, int arg){
    int inputredir=0, outputredir=0;
	if(tokens[cmd][arg][0]=='<'){													// Controlli se si usa giustamente la ridirezione
		if(cmd!=1){
			printf("redirezione input possibile solo sul primo comando\n");
			return -1;
		}
		else if(tokens[cmd][arg][1]=='\0'){											// in ubash la redirezione è attaccata (<bar) 
			printf("redirezione errata, non usare lo spazio tra < e il file\n");	// se staccato, errore
			return -1;
		}
		inputredir++;																// se la redirezione è sul primo comando, la considero...
	}
	if(inputredir>1){																// ...ma può averne una sola
		printf("una sola redirezione dell'input permessa\n");
		return -1;
	}
	if(tokens[cmd][arg][0]=='>'){														
		if(cmd!=cmdno){
			printf("redirezione output possibile solo sull'ultimo comando\n");
			return -1;
		}
		else if(tokens[cmd][arg][1]=='\0'){											// in ubash la redirezione è attaccata (>bar) 
			printf("redirezione errata, non usare lo spazio tra > e il file\n");	// se staccato, errore
			return -1;
		}
		outputredir++;																// se la redirezione è sull'ultimo comando, la considero...
	}
	if(outputredir>1){																// ...ma può averne una sola
		printf("una sola redirezione dell'output permessa\n");
		return -1;
	}
	return 0;
}

///////////////////////

int parseSpace(char** tokens[], int cmdno){													
	int i;																					 									
	for(i=1; i<=cmdno; i++){																// inizio da [1], perchè in [0] c'è la lista di cmd
		if((tokens[i]=(char **) malloc(sizeof(char *)*MAXARG))==NULL){						// alloco memoria per MAXARG args per ogni comando
			perror("malloc");
			exit(1);
		}
		for(int j=0; (tokens[i][j]=strtok_r(tokens[0][i-1], " ", &tokens[0][i-1])); j++){	// faccio il parsing spezzando sullo spazio
            if(redircheck(tokens, cmdno, i, j)==-1)
                return -1;
		}
	}
																						    // espansione variabili d'ambiente precedute da $
	for(int j=1; j<=cmdno; j++){															// ciclo su tutti i comandi
		for(int h=1; tokens[j][h]!=NULL; h++){												// ciclo su ogni argomento del comando		
			if(tokens[j][h][0]=='$')
				if((tokens[j][h]=getenv(tokens[j][h]+sizeof(char)))==NULL){
					perror("getenv");
					return -1;
				}	
		}
	}

	return i-1;																				// ritorno numero di comandi per controllo (i-1, perchè ho iniziato da i=1)
}

///////////////////////////// Funzione che divide i comandi /////////////////////////////////////// 

int checkcd(char** tokens[], int cmdno){                                    // controlli per cd: 																			
    if((strcmp(tokens[1][0], "cd")==0)){								    // -cd come primo, ma:
		if(cmdno>1){														// --ci sono altri comandi
		    printf("cd può essere solo l'unico comando!\n");
            return -1;
    	}
		else{
			for(int j=0; tokens[1][j]!=NULL; j++){
				if(*tokens[1][j]=='<' || *tokens[1][j]=='>'){				// --tentata ridirezione i/o
					printf("cd non può redirezionare i/o\n");
					return -1;
				}
				if(j>1){													// --troppi argomenti
					printf("cd può avere un solo argomento\n");
					return -1;
				}	
			}
		}
	}
    for(int j=2; j<=cmdno; j++)
		if(strcmp(tokens[j][0], "cd")==0){									// -cd non primo
			printf("cd può essere solo il primo comando!\n");
			return -1;
		}

    return 0;
}

///////////////////////////

bool checkEmptycmd(char* token){										// funzione che controlla se il comando è vuoto
	char* prova;
	if((prova=malloc(sizeof(char)*MAXCMD))==NULL){
		perror("malloc");
		exit(1);
	}
	char* aux=prova;													// punta l'inizio per poi fare la free()
	strcpy(prova, token);
	while(*prova!='\0'){
		if(isspace(*prova))												// uso isspace per individuare anche \t, \n,...
			prova++;
		else{
			free(aux);
			return true;
		}
	}
	free(aux);
	return false;
}

//////////////////////////

int parsePipe(char* input, char** tokens[]){							
	if((tokens[0]=(char **) malloc(sizeof(char *)*MAXCMD))==NULL){		// creo spazio per la lista di comandi
		perror("malloc");
		exit(1);
	}
	int i;																// salvo il numero di comandi
	for(i=0; (tokens[0][i]=strtok_r(input, "|", &input)); i++){			// divido i comandi con | e li metto nell'indice 0 dell'array
		if(!checkEmptycmd(tokens[0][i])){								// controllo che non ci siano comandi vuoti
			printf("hai inserito un comando vuoto\n");
			return 0;
		}
	}

	if(parseSpace(tokens, i)!=i){										// controllo che il numero di cmd rimanga lo stesso
		return 0;
    }

    if(checkcd(tokens, i)==-1){
        return 0;
	}
    

	return i;															// ritorno il numero di cmd
}

/////////////////////////////////// Funzione che determina le azioni del processo padre //////////////////////////////////////////////

void padre(){
	int* w8check;
	if((w8check=malloc(sizeof(int)))==NULL){
		perror("malloc");
		exit(1);
	}															
	wait(w8check);
	if(!WIFEXITED(*w8check))												// controlli sulla wait
		printf("figlio terminato in modo anomalo\n");
	else if(WEXITSTATUS(*w8check)!=0)
			printf("exit status==%d\n", WEXITSTATUS(*w8check));
	if(WIFSIGNALED(*w8check))
		printf("processo figlio terminato da un segnale: %d %s\n", WTERMSIG(*w8check), strsignal(WTERMSIG(*w8check)));
		
	free(w8check);
}

/////////////////////////////////// Funzione che esegue comando singolo ////////////////////////////////////////////////////////////// 

void execcmd(char** cmd, pid_t pid){									                                
	if((pid=fork())<0){													                                // Se fallisce la fork, errore					
		perror("fork");
		return;
	}
    if(pid==0){																							// Dentro il figlio:
		for(int i=1; (cmd[i]!=NULL); i++){																// ciclo sugli argomenti esistenti
			if(cmd[i][0]=='<'){																			// controllo se c'è una redirezione
				int filein;																				// fd per il file
				if((filein=open((char *) cmd[i]+sizeof(char), O_RDONLY))==-1){							// controllo la open, usata in read only
					perror("open");
					return;
				}
				dup2(filein, STDIN_FILENO);																// reindirizzo l'input
				cmd[i]=NULL;																			// elimino l'argomento 
				close(filein);																			// chiudo il fd
			}
            else if(cmd[i][0]=='>'){																	// controllo se c'è una redirezione
				int fileout;																			// fd per il file
				if((fileout=open((char *) cmd[i]+sizeof(char), O_CREAT | O_WRONLY, 0777))==-1){			// controllo la open, usata con tutti i permessi 
					perror("open");																		// ed in grado di creare un file inesistente
					return;
				}
				dup2(fileout, STDOUT_FILENO);															// reindirizzo l'output
				cmd[i]=NULL;																			// elimino l'argomento 
				close(fileout);																			// chiudo il fd
			}
		}
        if(execvp(cmd[0], cmd)<0){
			perror("exec");
			return;
		}
	}
	else{
		padre();
		return;
	}
}

//////////////////////////// Funzione esegue + comandi //////////////////////////////////////

void closepipe(int i, int** arrpipe, int n){
	if(i==0){
		close(arrpipe[i][0]);
		close(arrpipe[i][1]);
	}

	else if(i==n-1){
		for(int j=0; j<i; j++){
			close(arrpipe[j][0]);
			close(arrpipe[j][1]);	
		}
	}

	else{
		for(int j=0; j<i+1; j++){
			close(arrpipe[j][0]);
			close(arrpipe[j][1]);	
		}
	}
}

////////////////////////////

void figliopipe(char** cmd[], int** arrpipe, int n, int i){
	if(i==0){																			// se è il primo comando
		for(int j=1; j<MAXARG && (cmd[i+1][j]!=NULL); j++){								// ciclo sugli argomenti esistenti
			if(cmd[i+1][j][0]=='<'){													// controllo se c'è una redirezione
				int filein;																// fd per il file
				if((filein=open((char *) cmd[i+1][j]+sizeof(char), O_RDONLY))==-1){		// controllo la open
					perror("open");
					return;
				}
				dup2(filein, STDIN_FILENO);												// reindirizzo l'input
				cmd[i+1][j]=NULL;														// elimino l'argomento 
				close(filein);															// chiudo il fd
			}
		}
		dup2(arrpipe[i][1], STDOUT_FILENO);												// reindirizzo l'output sulla pipe
	}
	else{
		if(i==n-1){																							// se è l'ultimo comando
			for(int j=1; j<MAXARG && (cmd[i+1][j]!=NULL); j++){												// ciclo sugli argomenti esistenti
				if(cmd[i+1][j][0]=='>'){																	// controllo se c'è una redirezione
					int fileout;																			// fd per il file
					if((fileout=open((char *) cmd[i+1][j]+sizeof(char), O_CREAT | O_WRONLY, 0777))==-1){	// controllo la open
						perror("open");
						return;
					}
					dup2(fileout, STDOUT_FILENO);															// reindirizzo l'output
					cmd[i+1][j]=NULL;																		// elimino l'argomento 
					close(fileout);																			// chiudo il fd
				}
			}
			dup2(arrpipe[i-1][0], STDIN_FILENO);															// reindirizzo solo l'input prendendo l'output del comando precedente
		}	

		else{
			dup2(arrpipe[i-1][0], STDIN_FILENO);															// altrimenti reindirizzo entrambi
			dup2(arrpipe[i][1], STDOUT_FILENO);		
		}
	}
	closepipe(i, arrpipe, n);

	if(execvp(cmd[i+1][0], cmd[i+1])<0){																	// eseguo
		perror("exec");
		return;
	}
		
}

///////////////////////////////

void execpipe(char** cmd[], int n, pid_t pid){													
	int** arrpipe;
	if((arrpipe=(int **) malloc(sizeof(int*)*(n-1)))==NULL){									// creo n-1 pipe e le alloco in un array di array
		perror("malloc");
		exit(1);
	}
	for(int i=0; i<(n-1); i++){
		if((arrpipe[i]=(int *) malloc(sizeof(int)*2))==NULL){									// alloco i/o in ogni pipe	
			perror("malloc");
			exit(1);
		}
	}	
	
	for(int i=0; i<(n-1); i++){
		if(pipe(arrpipe[i])<0){																	// attivo le pipe e faccio i controlli
			perror("pipe");
			return;
		}
	}

	for(int i=0; i<n; i++){																		// ciclo per ogni comando
		if((pid=fork())<0){									
			perror("fork");
			return;
		}

		if(pid==0){
			figliopipe(cmd, arrpipe, n, i);
		}	
	}
	
	for(int j=0; j<n-1; j++){
		close(arrpipe[j][0]);
		close(arrpipe[j][1]);	
	}
	
	for(int j=0; j<n; j++){
		padre();
	}
	
	for(int j=0; j<(n-1); j++)											
		free(arrpipe[j]);																		// libero la memoria allocata per le pipe

	free(arrpipe);

	return;
}

///////////////////////////////////////////////////////////////////////////////////////
