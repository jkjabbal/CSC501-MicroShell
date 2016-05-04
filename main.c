
#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
static int firstCmd;
static int lastCmd = 0;
int fd[2];
int fdx[2];
int cmds=0;
int single=0;
extern char **environ;
int initialfd_stdout=-1;
int initialfd_stdin=-1;
int initialfd_stderr=-1;
int nicetobeset=0;
int nicevalue=4;
int searchforpath=0;
char *host;
char *msg;
char *path;
int abortme=0;
int childrunning=0;
void handleTin(Cmd c){
   int fd;
   initialfd_stdin=dup(0);
   fd=open(c->infile,O_RDONLY,0777);
   if(fd == -1){
    fprintf(stderr,"%s\n",strerror(errno));
    exit(0);
    }
   else{
   if(dup2(fd,0) == -1){
    fprintf(stderr,"%s\n",strerror(errno));
    exit(0);
    }
   else{
    close(fd);
    close(fd);
    close(fd);
   }
   }
}

void handleTout(Cmd c){
  int fd;
  initialfd_stdout=dup(1);
  fd=open(c->outfile,O_CREAT|O_WRONLY|O_TRUNC,0777);
  if(fd == -1)
    fprintf(stderr,"%s\n",strerror(errno));
  else{
  if(dup2(fd,1) == -1){
    fprintf(stderr,"%s\n",strerror(errno));
    }
  else
    close(fd);
  }
}

void handleTapp(Cmd c){
  int fd;
  initialfd_stdout=dup(1);
  fd=open(c->outfile,O_CREAT|O_WRONLY|O_APPEND,0777);
  if(fd == -1)
   fprintf(stderr,"%s\n",strerror(errno)); 
  else{  
  if(dup2(fd,1)==-1)
   fprintf(stderr,"%s\n",strerror(errno)); 
  else
   close(fd);
  }
}

void handleToutErr(Cmd c){
  int fd;
  initialfd_stdout=dup(1);
  initialfd_stderr=dup(2);
  fd=open(c->outfile,O_CREAT|O_WRONLY|O_TRUNC,0777);
  if(fd == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else{
  if(dup2(fd,1) == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else{
  if(dup2(fd,2) == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else 
   close(fd);
  }
  }
}

void handleTappErr(Cmd c){
  int fd;
  initialfd_stdout=dup(1);
  initialfd_stderr=dup(2);
  fd=open(c->outfile,O_CREAT|O_WRONLY|O_APPEND,0777);
  if(fd == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else{
  if(dup2(fd,1) == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else{
  if(dup2(fd,2) == -1)
   fprintf(stderr,"%s\n",strerror(errno));
  else
   close(fd);
  }
  }
}

void handleTpipe(Cmd c){
 if(cmds%2 == 0){
  if(lastCmd == 1){
    close(fdx[1]);
    initialfd_stdin=dup(0);
    dup2(fdx[0],0);
   }else if(firstCmd == 1 ){
   // firstCmd=0;
    close(fd[0]);
    dup2(fd[1],1);
  }else{ 
    close(fdx[1]);
    close(fd[0]); 
    initialfd_stdin=dup(0);
    dup2(fdx[0],0);
    dup2(fd[1],1);
  }
  }else{
   if(firstCmd == 1 ){
   // firstCmd=0;
    close(fdx[0]);
    dup2(fdx[1],1);
  }else if(lastCmd == 1){
    //lastCmd=0;
    close(fd[1]);
    initialfd_stdin=dup(0);
    dup2(fd[0],0);
  }else{
    close(fd[1]);
    close(fdx[0]);
    dup2(fd[0],0);
    dup2(fdx[1],1);
  }
  }
}

void handleTpipeErr(Cmd c){
 if(cmds%2 == 0){
  if(lastCmd == 1){
    close(fdx[1]);
    initialfd_stdin=dup(0);
    dup2(fdx[0],0);
    }else if(firstCmd == 1 ){
    close(fd[0]);
    dup2(fd[1],1);
    dup2(fd[2],2);
  }else{
    close(fdx[1]);
    close(fd[0]);
    initialfd_stdin=dup(0);
    dup2(fdx[0],0);
    dup2(fd[1],1);
    initialfd_stderr=dup(2);
    dup2(fd[2],2);
  }
  }else{
   if(firstCmd == 1 ){
    close(fdx[0]);
    dup2(fdx[1],1);
    dup2(fdx[1],2);
  }else if(lastCmd == 1){
    close(fd[1]);
    initialfd_stdin=dup(0);
    dup2(fd[0],0);
    initialfd_stderr=dup(2);
    dup2(fd[2],2);
  }else{
    close(fd[1]);
    close(fdx[0]);
    dup2(fd[0],0);
    dup2(fdx[1],1);
    initialfd_stderr=dup(2);
    dup2(fdx[2],2);
  }
  }
}


void revertdup(){
 if(initialfd_stdout != -1){
  dup2(initialfd_stdout,STDOUT_FILENO);
  initialfd_stdout=-1;
 }
 if(initialfd_stdin != -1){
  dup2(initialfd_stdin,STDIN_FILENO);
  initialfd_stdin=-1;
 }
 if(initialfd_stderr != -1){
  dup2(initialfd_stderr,STDERR_FILENO);
  initialfd_stderr=-1;
 }
}

void handleCdBuiltIn(Cmd c){
   int status;
   char arr[500];
   if(strcmp(c->args[0],"cd") == 0){
    if(c->args[1]){
      status = chdir(c->args[1]);
      if(status <  0){
        fprintf(stderr,"%s\n",strerror(errno));
      }
    }else{
      char *home;
      home =(char *)malloc(500);
      home=getenv("HOME");
      if(home == NULL){
       home = getpwuid(getuid())->pw_dir;
      }
      status = chdir(home);
      if(status < 0){
       fprintf(stderr,"%s\n",strerror(errno));
      }
    } 
  }
  revertdup();
}

void  handlePwdBuiltIn(Cmd c){
  char *cwd = (char *)malloc(1000);
  cwd = getcwd(cwd,(size_t)1000);
  if(cwd == NULL)
   fprintf(stderr,"%s\n",strerror(errno));
  else
   fprintf(stdout,"%s\n",cwd);

  revertdup();
}

void handleEchoBuiltIn(Cmd c){
  int num_args;
  for(num_args=1;num_args<c->nargs;num_args++){
   printf("%s",c->args[num_args]);
   printf(" ");
  }
  printf("\n");
  revertdup();
}

void handleSetenvBuiltIn(Cmd c){
  if(c->nargs < 2){ 
  int i;
  for(i=0;environ[i];i++)
    printf("%s\n",environ[i]);
  }else{
    char *envvar;
    envvar = (char *)malloc(1000);
    strcpy(envvar,c->args[1]);
    strcat(envvar,"=");
    if(c->args[2])
     strcat(envvar,c->args[2]);
    else
     strcat(envvar,"");
    if(putenv(envvar) == -1)
    {
     fprintf(stderr,"%s\n",strerror(errno));
    }
  }
  revertdup();
}

void handleUnsetenvBuiltIn(Cmd c){
 if(c->args[1] != NULL){
  if(unsetenv(c->args[1]) == -1)
   fprintf(stderr,"%s\n",strerror(errno));
 }
 revertdup();
}

void handleNiceBuiltIn(Cmd c){
 int i;
 int k=0;
  Cmd c1;
   c1=c;
  int c_ke_nargs;
  c_ke_nargs=c->nargs; 
  if(c1->args[1][0] == 45 || c1->args[1][0] == 43)
  {
    if(isdigit(c1->args[1][1])){
    nicevalue = atoi(c->args[1]);
    for(i=2;i<c->nargs;i++){
     c1->args[k]=c->args[i];
     k++;
     }  
     c1->nargs = k;
     c1->maxargs = k;  
     }
  }else if(isdigit(c1->args[1][0])){
   nicevalue = atoi(c->args[1]);
    for(i=2;i<c->nargs;i++){
     c1->args[k]=c->args[i];  
     k++;
     }
     c1->nargs = k;
     c1->maxargs = k; 
  }else{
   nicevalue = 4;
    for(i=1;i<c->nargs;i++){
     c1->args[k]=c->args[i];  
     k++;
     }
     c1->nargs = k;
     c1->maxargs = k; 
  }
  nicetobeset=1;

  for(k=c1->nargs;k<c_ke_nargs;k++){
  c1->args[k]=NULL;
  }
  executeCmd(c1); 
  revertdup();
}

void *handleWhereBuiltIn(Cmd c){
  if(c->args[1])
   {
  char *pathvar;
  pathvar=(char *)malloc(1000);
  pathvar=getenv("PATH");
  
  int fd;
  char *token;
  char *matchme;
  char *pseudopath;
  token = (char *)malloc(1000);
  matchme =(char *)malloc(1000);
  pseudopath = (char *)malloc(1000);

  strcpy(pseudopath,pathvar);
  token=strtok(pseudopath,":");
  
  while(token != NULL){
  strcpy(matchme,token);
  strcat(matchme,"/");
  strcat(matchme,c->args[1]);
  fd=open(matchme,O_RDONLY,555);
  if(fd != -1){
   printf("%s\n",matchme);
   }
  token=strtok(NULL,":");
  }
  }
  revertdup();
}

static void prCmd(Cmd c)
{
  int i;

  if ( c ) {
    if ( c->in == Tin ){
       handleTin(c);
       } 
    if ( c->out != Tnil )
      switch ( c->out ) {
      case Tout:
        handleTout(c);
        break;
      case Tapp:
        handleTapp(c);
        break;
      case ToutErr:
        handleToutErr(c);
        break;
      case TappErr:
        handleTappErr(c);
        break;
      case Tpipe:
        handleTpipe(c);
        break;
      case TpipeErr:
        handleTpipeErr(c);
        break;
      default:
        fprintf(stderr, "Shouldn't get here\n");
        exit(-1);
      }
    if ( !strcmp(c->args[0], "end") )
      exit(0);
   }
 
  
  if(single != 1 && strcmp(c->args[0],"nice") != 0 && lastCmd ==1 )
  {
   handleTpipe(c);
  }
}

int checkiffile(char *path){
 struct stat path_stat;
 stat(path,&path_stat);
 return S_ISREG(path_stat.st_mode);
}

int checkifpermissions(char *path){
 struct stat path_stat;
 stat(path,&path_stat);
  if(path_stat.st_mode & S_IXUSR &&  path_stat.st_mode & S_IXGRP &&  path_stat.st_mode & S_IXOTH)
    return 1;
  else
   return 0;
}

char *resolveCmdPath(Cmd c){
  char *cwd = (char *)malloc(1000);
  cwd = getcwd(cwd,(size_t)1000);
  
  char *path;
  path = (char *)malloc(500);

  if(c->args[0][0] == 47){
   strcpy(path,c->args[0]);
   }else if(strchr(c->args[0],'/')){
   strcpy(path,cwd);
   strcat(path,"/");
   strcat(path,c->args[0]);
   }else{
  char *pathvar;
  pathvar=(char *)malloc(1000);
  pathvar=getenv("PATH");

  int fd;
  char *token;
  char *matchme;
  char *pseudopath;
  token = (char *)malloc(1000);
  matchme =(char *)malloc(1000);
  pseudopath = (char *)malloc(1000);
  
  strcpy(pseudopath,pathvar);
  token=strtok(pseudopath,":");
  
  while(token != NULL){
  strcpy(matchme,token);
  strcat(matchme,"/");
  strcat(matchme,c->args[0]);
  fd=open(matchme,O_RDONLY,555);
  if(fd != -1){
   strcpy(path,matchme);
   }
  token=strtok(NULL,":");
  }
   }
  return path;
}

int checkPermissions(char *path){
  int file;
  int perm;
  file = checkiffile(path);
  if(file != 0){
  perm = checkifpermissions(path);
   if(perm == 0){
     return 0;
   }
  }else
   return 0;  
 return 1;
}

void executeCmd(Cmd c){
  pid_t pid;
  int p;
  if(lastCmd != 1 && single !=1 ){
  if(cmds%2 == 0){
  p=pipe(fd);
  }
  else{
  p=pipe(fdx);
  }
  }
  if( p == -1 )
   fprintf(stderr,"%s\n",strerror(errno));
  else{
     if(strcmp(c->args[0],"cd") == 0 && lastCmd == 1 && nicetobeset !=1 ){
      prCmd(c);
      handleCdBuiltIn(c);
     }else if(strcmp(c->args[0],"pwd")==0 && lastCmd == 1 && nicetobeset !=1 ){
       prCmd(c);
       handlePwdBuiltIn(c);
     }else if(strcmp(c->args[0],"echo")==0 && lastCmd == 1 && nicetobeset !=1){
       prCmd(c);
       handleEchoBuiltIn(c);
     }else if(strcmp(c->args[0],"setenv")==0 && lastCmd == 1 && nicetobeset !=1){
       prCmd(c);
       handleSetenvBuiltIn(c);
     }else if(strcmp(c->args[0],"unsetenv")==0 && lastCmd == 1 && nicetobeset !=1){
       prCmd(c);
       handleUnsetenvBuiltIn(c);
     }else if(strcmp(c->args[0],"nice")==0 && lastCmd == 1 && nicetobeset !=1){
       prCmd(c);
       handleNiceBuiltIn(c);
     }else if(strcmp(c->args[0],"where")==0 && lastCmd == 1 && nicetobeset !=1){
       prCmd(c);
       handleWhereBuiltIn(c);
     }else{
  pid = fork();
  if(pid < 0){
    fprintf(stderr,"%s\n",strerror(errno));
    exit(0);
  }
  else if (pid == 0){
    if(nicetobeset == 1)
     {
       if(setpriority(PRIO_PROCESS,getpid(),nicevalue) == -1)
        fprintf(stderr,"%s\n",strerror(errno));
     }
     prCmd(c);
     if(strcmp(c->args[0],"cd") == 0){
      handleCdBuiltIn(c);
      exit(0);
      }else if(strcmp(c->args[0],"pwd")==0){
      handlePwdBuiltIn(c);
      exit(0);
      }else if(strcmp(c->args[0],"echo")==0){
       handleEchoBuiltIn(c);
      exit(0);
      }else if(strcmp(c->args[0],"setenv")==0){
       handleSetenvBuiltIn(c);
      exit(0);
      }else if(strcmp(c->args[0],"unsetenv")==0){
       handleUnsetenvBuiltIn(c);
       exit(0);
      }else if(strcmp(c->args[0],"nice")==0){
       handleNiceBuiltIn(c);
       exit(0);
      }else if(strcmp(c->args[0],"where")==0){
       handleWhereBuiltIn(c);
       exit(0);
      }else{
          path=(char *)malloc(1000);
          path = resolveCmdPath(c);
          if(strcmp(path,"")!=0){
           if(checkPermissions(path)!=1){
             fprintf(stderr,"Permission Denied\n");
             exit(0);
           }
           }else{
             fprintf(stderr,"command not found\n");
             exit(0);
           }
          execvpe(path,c->args,environ);
        }
      }else{
      if(firstCmd != 1 && single != 1){
        if(cmds%2 == 0){
          close(fdx[0]);
          close(fdx[1]);
          close(fdx[2]);
        }else{
          close(fd[0]);
          close(fd[1]);
          close(fd[2]);
        }
       }  
       wait(NULL);  
       /*int statusval;
       waitpid(pid,&statusval,NULL);
       if(WIFEXITED(statusval)==1){
         printf("exit status of child : %d\n",WEXITSTATUS(statusval));
         if(WEXITSTATUS(statusval)==1){
          abortme=1;
         }
       }*/
       }
   }
  }
}

static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;
   if ( p == NULL )
    return;
   firstCmd = 1;
   for ( c = p->head; c != NULL; c = c->next ) {
     if ( !strcmp(c->args[0], "end") || !strcmp(c->args[0],"logout"))
     exit(0);
   cmds++;
   if(c == p->head && c->next == NULL){
    single=1; 
   }
   if(c->next == NULL){
    lastCmd=1;
    }
   if(strcmp(c->args[0],"cd")!=0 && strcmp(c->args[0],"pwd")!=0 && strcmp(c->args[0],"echo")!=0 && strcmp(c->args[0],"setenv")!=0 && strcmp(c->args[0],"unsetenv")!=0 && strcmp(c->args[0],"nice")!=0 && strcmp(c->args[0],"where")!=0 ){
   path=(char *)malloc(1000);
   path = resolveCmdPath(c);
   if(strcmp(path,"")!=0){
    if(checkPermissions(path)!=1){
      fprintf(stderr,"Permission Denied\n");
      break;
     }
    }else{
      fprintf(stderr,"command not found\n");
      break;
    }
   }
   executeCmd(c);
   firstCmd = 0;
   nicetobeset=0;
   nicevalue=4;
  }
  lastCmd = 0;
  cmds=0;
  single=0;
  nicetobeset=0;
  nicevalue=4;
  prPipe(p->next);
}

void initializeShell(){
 Pipe p;
 int fd;
 char *home;
 char *duphome;
 home =(char *)malloc(1000);
 duphome=(char *)malloc(1000);
 home=getenv("HOME");
 strcpy(duphome,home);
 if(duphome == NULL){
  duphome = getpwuid(getuid())->pw_dir;
 }
 strcat(duphome,"/.ushrc");

 int initialfd;
 initialfd=dup(0);
 fd = open(duphome,O_RDONLY,0777);
 if(fd == -1){
   //fprintf(stderr,"%s\n",strerror(errno));
   close(fd);
   close(initialfd); 
 }else{
   struct stat buf;
   stat(duphome,&buf);
   int size = buf.st_size;
   if(size != 0){
   if(dup2(fd,0) == -1)
    fprintf(stderr,"%s\n",strerror(errno));
   else
   {
    close(fd);
    p=parse();
   if(p!=NULL){
    while(strcmp(p->head->args[0],"end")!=0){
    prPipe(p);
    freePipe(p);
    p=parse(); 
    if(p==NULL)
      break;
    }
   }
    dup2(initialfd,STDIN_FILENO);
    close(initialfd);
   }
   } 
  }
}

void signalhandler(int signalnum){
 switch(signalnum){
   case SIGQUIT :  if(signal(SIGQUIT,signalhandler)==SIG_IGN)
                   signal(SIGQUIT,SIG_IGN);
                   fflush(STDIN_FILENO);
                   printf("\n%s%% ",host);
                   fflush(STDIN_FILENO);
                   signal(SIGQUIT,signalhandler);
                   break;
   case SIGTERM :  fflush(STDIN_FILENO);
                   printf("\n%s%% ",host);
                   fflush(STDIN_FILENO);
                   return;
                 break;
   case SIGHUP : if(signal(SIGHUP,signalhandler)==SIG_IGN)
                   signal(SIGHUP,SIG_IGN);
                   fflush(STDIN_FILENO);
                   printf("\n%s%% ",host);
                   fflush(STDIN_FILENO);
                   signal(SIGHUP,signalhandler);
                   break;
   case SIGINT :   if(childrunning == 0){
                   fflush(STDIN_FILENO);
                   printf("\n%s%% ",host);
                   fflush(STDIN_FILENO);
                   }else{
                   fflush(STDIN_FILENO);
                   printf("\n");
                   fflush(STDIN_FILENO);
                   }
                   return;
                   break;
   default : printf("Default Case\n");
             return;
 }
 return;
}

void registerSigHandlers(){
 if(signal(SIGQUIT,signalhandler)==SIG_ERR)
  printf("couldn't ignore SIGQUIT\n");
 if(signal(SIGHUP,signalhandler)==SIG_ERR)
  printf("couldn't ignore SIGHUP\n");
 if(signal(SIGINT,signalhandler)==SIG_ERR)
  printf("couldn't ignore SIGINT\n");
 if(signal(SIGTERM,signalhandler)==SIG_ERR)
  printf("couldn't handle SIGTERM\n");
 return;
}

int main(int argc, char *argv[],char *envp[])
{
  Pipe p;
  host=(char *)malloc(100);
  gethostname(host,100);

  initializeShell();
  
  registerSigHandlers();

  while ( 1 ) {
    printf("%s%% ", host);
    fflush(stdout);
    p = parse();
    childrunning=1;    
    prPipe(p);
    childrunning=0;
    freePipe(p);
  }
}

/*........................ end of main.c ....................................*/
