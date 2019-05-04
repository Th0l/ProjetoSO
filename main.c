#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

//Estrutura usada para Guardar os offsets iniciais e finais dos resultados dos execs
struct offStr {
	int offsetInicial; //Offset antes de escrever o Output
	int offsetFinal; //Offset depois de escrever o Output
};

typedef struct offStr* OFF;
//----------------------------------------------------------------------------//
void sig_usr(int signo){
    if(signo == SIGINT){
      printf("\nControl-C apanhado, ficheiros fechados, exiting...\n");
      exit(0);}
    return;}
//----------------------------------------------------------------------------//
int powi(int num)
{
  int ret = 1;
  for(int x = 0; x<num ;x++)
    ret = ret * 10;

  return ret;
}
//----------------------------------------------------------------------------//
int getNum(char* string)
{
  int lt = strlen(string)-2;
  int ret = 0;
  int power = 0;
  for(int i = 1;string[i] != '|';i++){
    power = powi(lt-1);
    lt--;
    ret += (string[i] - '0') * power;}

  return ret;
}
//----------------------------------------------------------------------------//
int readln2(int fildes, char *buf, int size){
	int x = 0, i = 1;
	char c = 0;

	while (i && x < size && c!='\n'){
		i = read(fildes, &c, 1);
		if (i){
			buf[x] = c;
			x++;
		}
		if( c=='\0') break;
	}
	return x;
}

//----------------------------------------------------------------------------//

/**
 * @param (fildes) File Descriptor;
 * @param (buf) Buffer para armazenar;
 * @param (size) Tamanho do buffer;
 *
 * @return Numero de bytes lidos
 */

int readln(int fildes, char *buf, int size){
	int x = 0, i = 1;
	char c = 0;

	while (i && x < size && c!='\n'){
		i = read(fildes, &c, 1);
		if (i){
			buf[x] = c;
			x++;
		}
		if( c=='\0') break;
	}
	buf[x-1]='\0';
	return x;
}
//----------------------------------------------------------------------------//
void replaceFiles(char* nome)
{
	int n,f;
	char buff[1024];
	memset(buff,'\0',1024);
	n = open("temp.txt",O_RDONLY);
	f = open(nome,O_TRUNC|O_WRONLY);
	while(readln(n,buff,1024)){
		write(f,buff,strlen(buff));
		write(f,"\n",1);
	}
	close(n);n = -1;
	close(f);f = -1;
}
//----------------------------------------------------------------------------//

void runCommands(char *nome)
{
    int n,f,execTotal = 0,arrC = 0;
    OFF offsets[256];
    char buffer[1024];
    char *strArr[15];
    char *command,*insert;
    const char *delet = " ";
    int check = 0;

    f = open(nome,O_RDONLY);
    n = open("temp.txt",O_CREAT | O_TRUNC | O_RDWR );

    if(f >= 0)
    {
        printf("Beggining\n");
        memset(buffer,'\0',1024);
        while(readln(f,buffer,1024)) // Vai ler o ficheiro linha a linha
        {
            if(buffer[0] == '>')//Verifica se é inicio de input
                check = 1;
            if(check == 0) {
                write(n,buffer,strlen(buffer));

                write(n,"\n",1);

                if(buffer[0] == '$') // Se detetar um comando vai executar
                {

                    if(buffer[1] != ' ') //Se for um comando que tem o StdIn igual ao StdOut do comando anterior
                    {
                        int i = 0;
                        insert = strtok(buffer,delet);
                        while(insert != NULL) //Ciclo while que vai separar os diferentes comandos
                        {
                            insert = strtok(NULL,delet);

                            if(i == 0) //Para sacar o comando
                            {
                                command = malloc(sizeof(strlen(insert)+1));
                                strcpy(command,insert);
                                strArr[arrC] = malloc(sizeof(strlen(insert)+1));
                                strcpy(strArr[arrC],insert);
                                arrC++;
                            }
                            else // Para ter os argumentos
                            {
                                if(insert != NULL) {
                                    strArr[arrC] = malloc(sizeof(strlen(insert)));
                                    strcpy(strArr[arrC],insert);
                                    arrC++;
                                }
                            }
                            i++;
                        }

                        if(i == 2)
                        {
                            if(buffer[1] == '|')
                            {
                                int fd[2];
                                pipe(fd);
                                char buff[1024];

                                OFF set = offsets[execTotal-1];
                                int tmp = lseek(n,0,SEEK_CUR);
                                lseek(n,set->offsetInicial,SEEK_SET);
                                while(lseek(n,0,SEEK_CUR) < set->offsetFinal) { //Vai escrever o output do anterior no pipe
                                    memset(buff,'\0',1024);
                                    readln2(n,buff,1024);
                                    write(fd[1],buff,strlen(buff));
                                }
                                lseek(n,tmp,SEEK_SET); //Vai voltar a meter a posição correta de escrita no ficheiro

                                write(n,">>>\n",4);

                                OFF ff = malloc(sizeof(struct offStr));
                                ff->offsetInicial = lseek(n,0,SEEK_CUR);

                                if(!fork())//Filho
                                {
                                    close(fd[1]);

                                    dup2(fd[0],0);//StdIn vai para o Pipe
                                    dup2(n,1);//StdOut vai para a file

                                    close(fd[0]);

                                    execlp(command,command,NULL);
                                    _exit(1);
                                }
                                else//Pai
                                {
                                    close(fd[1]);
                                    close(fd[0]);

																		int status;
		                                wait(&status);
																		if(status!=0){
																			close(n);
																			close(f);
																			printf("Finished\n");
																			_exit(1);}

                                    ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                    offsets[execTotal] = ff;

                                    write(n,"<<<\n",4); //O Pai Escreve pois o filho vai fazer o Exec

                                    execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                    for(int k = 0; k<arrC; k++)
                                        memset(strArr[k],'\0',15);
                                    arrC = 0;
                                }
                            }
                            else//Codigo para correr que ñ requer o output do anterior mas sim um mais antigo
                            {
                                int num;
                                num = getNum(strtok(buffer,delet));

                                int fd[2];
                                pipe(fd);
                                char buff[1024];

                                OFF set = offsets[execTotal-num];
                                int tmp = lseek(n,0,SEEK_CUR);
                                lseek(n,set->offsetInicial,SEEK_SET);
                                while(lseek(n,0,SEEK_CUR) < set->offsetFinal) { //Vai escrever o output do anterior no pipe
                                    memset(buff,'\0',1024);
                                    readln2(n,buff,1024);
                                    write(fd[1],buff,strlen(buff));
                                }
                                lseek(n,tmp,SEEK_SET); //Vai voltar a meter a posição correta de escrita no ficheiro

                                write(n,">>>\n",4);

                                OFF ff = malloc(sizeof(struct offStr));
                                ff->offsetInicial = lseek(n,0,SEEK_CUR);

                                if(!fork())//Filho
                                {
                                    close(fd[1]);

                                    dup2(fd[0],0);//StdIn vai para o Pipe
                                    dup2(n,1);//StdOut vai para a file

                                    close(fd[0]);

                                    execlp(command,command,NULL);
                                    _exit(1);
                                }
                                else//Pai
                                {
                                    close(fd[1]);
                                    close(fd[0]);

																		int status;
		                                wait(&status);
																		if(status!=0){
																			close(n);
																			close(f);
																			printf("Finished\n");
																			_exit(1);}

                                    ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                    offsets[execTotal] = ff;

                                    write(n,"<<<\n",4); //O Pai Escreve pois o filho vai fazer o Exec

                                    execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                    for(int k = 0; k<arrC; k++)
                                        memset(strArr[k],'\0',15);
                                    arrC = 0;
                                }

                            }
                        }
                        else//Codigos para correr que contêm argumentos
                        {
                            if(buffer[1] == '|')
                            {
                                char *args[arrC+1];

                                int x;

                                for(x = 0; x < arrC; x++) { //Passar os argumentos do strArr para o args que vai ter tamanho correto
                                    args[x] = malloc(sizeof(strlen(strArr[x])));
                                    strcpy(args[x],strArr[x]);
                                }

                                args[x] = NULL; //Meter a NULL para indicar que nao tem mais argumentos

                                int fd[2];
                                pipe(fd);
                                char buff[1024];

                                OFF set = offsets[execTotal-1];
                                int tmp = lseek(n,0,SEEK_CUR);
                                lseek(n,set->offsetInicial,SEEK_SET);
                                while(lseek(n,0,SEEK_CUR) < set->offsetFinal) { //Vai escrever o output do anterior no pipe
                                    memset(buff,'\0',1024);
                                    readln2(n,buff,1024);
                                    write(fd[1],buff,strlen(buff));
                                }
                                lseek(n,tmp,SEEK_SET); //Vai voltar a meter a posição correta de escrita no ficheiro

                                write(n,">>>\n",4);

                                OFF ff = malloc(sizeof(struct offStr));
                                ff->offsetInicial = lseek(n,0,SEEK_CUR);

                                if(!fork())//Filho
                                {
                                    close(fd[1]);

                                    dup2(fd[0],0);//StdIn vai para o Pipe
                                    dup2(n,1);//StdOut vai para a file

                                    close(fd[0]);

                                    execvp(command,args);
                                    _exit(1);
                                }
                                else//Pai
                                {
                                    close(fd[1]);
                                    close(fd[0]);

																		int status;
		                                wait(&status);
																		if(status!=0){
																			close(n);
																			close(f);
																			printf("Finished\n");
																			_exit(1);}

                                    ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                    offsets[execTotal] = ff;

                                    write(n,"<<<\n",4); //O Pai Escreve pois o filho vai fazer o Exec

                                    execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                    for(int k = 0; k<arrC; k++)
                                        memset(strArr[k],'\0',15);
                                    arrC = 0;
                                }
                            }
                            else
                            {
                                int num;
                                num = getNum(strtok(buffer,delet));

                                char *args[arrC+1];

                                int x;

                                for(x = 0; x < arrC; x++) { //Passar os argumentos do strArr para o args que vai ter tamanho correto
                                    args[x] = malloc(sizeof(strlen(strArr[x])));
                                    strcpy(args[x],strArr[x]);
                                }

                                args[x] = NULL; //Meter a NULL para indicar que nao tem mais argumentos

                                int fd[2];
                                pipe(fd);
                                char buff[1024];

                                OFF set = offsets[execTotal-num];
                                int tmp = lseek(n,0,SEEK_CUR);
                                lseek(n,set->offsetInicial,SEEK_SET);
                                while(lseek(n,0,SEEK_CUR) < set->offsetFinal) { //Vai escrever o output do anterior no pipe
                                    memset(buff,'\0',1024);
                                    readln2(n,buff,1024);
                                    write(fd[1],buff,strlen(buff));
                                }
                                lseek(n,tmp,SEEK_SET); //Vai voltar a meter a posição correta de escrita no ficheiro

                                write(n,">>>\n",4);

                                OFF ff = malloc(sizeof(struct offStr));
                                ff->offsetInicial = lseek(n,0,SEEK_CUR);

                                if(!fork())//Filho
                                {
                                    close(fd[1]);

                                    dup2(fd[0],0);//StdIn vai para o Pipe
                                    dup2(n,1);//StdOut vai para a file

                                    close(fd[0]);

                                    execvp(command,args);
                                    _exit(1);
                                }
                                else//Pai
                                {
                                    close(fd[1]);
                                    close(fd[0]);

																		int status;
		                                wait(&status);
																		if(status!=0){
																			close(n);
																			close(f);
																			printf("Finished\n");
																			_exit(1);}

                                    ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                    offsets[execTotal] = ff;

                                    write(n,"<<<\n",4); //O Pai Escreve pois o filho vai fazer o Exec

                                    execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                    for(int k = 0; k<arrC; k++)
                                        memset(strArr[k],'\0',15);
                                    arrC = 0;
                                }
                            }
                        }
                    }
                    else //Se for um comando que nao tem como StdIn os inputs dos anteriores
                    {
                        int i = 0;
                        insert = strtok(buffer,delet);
                        while(insert != NULL) //Ciclo while que vai separar os diferentes comandos
                        {
                            insert = strtok(NULL,delet);

                            if(i == 0) //Para sacar o comando
                            {
                                command = malloc(sizeof(strlen(insert)+1));
                                strcpy(command,insert);
                                strArr[arrC] = malloc(sizeof(strlen(insert)+1));
                                strcpy(strArr[arrC],insert);
                                arrC++;
                            }
                            else // Para ter os argumentos
                            {
                                if(insert != NULL) {
                                    strArr[arrC] = malloc(sizeof(strlen(insert)));
                                    strcpy(strArr[arrC],insert);
                                    arrC++;
                                }
                            }
                            i++;
                        }

                        if(i == 2) //Se o comando nao tiver argumentos faz execlp
                        {
                            write(n,">>>\n",4);

                            OFF ff = malloc(sizeof(struct offStr));
                            ff->offsetInicial = lseek(n,0,SEEK_CUR);

                            if(!fork())//FIlho
                            {

                                dup2(n,1); //StdOut redirecionado para a File Temporaria (espero)

                                execlp(command,command,NULL);
                                _exit(-1);
                            }
                            else//Pai
                            {
																int status;
                                wait(&status);
																if(status!=0){
																	close(n);
																	close(f);
																	printf("Finished\n");
																	_exit(1);}

                                ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                offsets[execTotal] = ff;

                                write(n,"<<<\n",4); // O Pai Escreve pois o filho vai fazer o Exec

                                execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                for(int k = 0; k<arrC; k++)
                                    memset(strArr[k],'\0',15);
                                arrC = 0;
                            }
                        }
                        else //Se o comando tiver argumentos faz execvp
                        {
                            char *args[arrC+1];

                            int x;

                            for(x = 0; x < arrC; x++) { //Passar os argumentos do strArr para o args que vai ter tamanho correto
                                args[x] = malloc(sizeof(strlen(strArr[x])));
                                strcpy(args[x],strArr[x]);
                            }

                            args[x] = NULL; //Meter a NULL para indicar que nao tem mais argumentos

                            write(n,">>>\n",4);

                            OFF ff = malloc(sizeof(struct offStr));
                            ff->offsetInicial = lseek(n,0,SEEK_CUR);

                            if(!fork())//Filho
                            {

                                dup2(n,1); //StdOut redirecionado para a File Temporaria (espero)

                                execvp(command,args);
                                _exit(1);
                            }
                            else//Pai
                            {
																int status;
																wait(&status);
																if(status!=0){
																	close(n);
																	close(f);
																	printf("Finished\n");
																	_exit(1);}

                                ff->offsetFinal = lseek(n,0,SEEK_CUR);

                                offsets[execTotal] = ff;

                                write(n,"<<<\n",4); // O Pai Escreve pois o filho vai fazer o Exec

                                execTotal++; //Aumenta o execTotal para se saber quantas vezes ja se executou um comando

                                for(int k = 0; k<arrC; k++)
                                    memset(strArr[k],'\0',15);
                                arrC = 0;
                            }
                        }
                    }
                }
            }
            else
            {
                if(buffer[0] == '<')
                    check = 0;
            }
        }
        strcpy(buffer,"");
        close(f);f = -1;
        close(n);n = -1;
        replaceFiles(nome);
    }
    else {
        printf("Esse NoteBook não existe.\n");
    }

		printf("Finished\n");
    return;
}
//----------------------------------------------------------------------------//
int main(int argc,char *argv[])
{

	signal(SIGINT,sig_usr);

	if(argc < 2){printf("Insira o nome do NoteBook a processar\n");}

	else{runCommands(argv[1]);}

	return 0;
}
