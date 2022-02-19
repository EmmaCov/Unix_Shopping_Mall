/* Simulateur d'un magasin de puericulture dont les echanges sont geres par tubes. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>

/***************************
  Definition de constantes
****************************/
//#define DEBUG 

#define MESSAGE_SIZE 256
#define N 256
#define MAX_RAND 3

// Pour les echanges par tubes
#define START_CONVERSATION '0'
#define HELLO '1'
#define ARTICLE '2'
#define GIVE '3'
#define TOTAL '4'
#define PAY '5'
#define RECEIPT '6'
#define THX_BYE '7'


/************************************
  Declaration des variables globales
*************************************/
typedef struct{
  char name_article[N];
  int price;
}Article;

Article body = {"body", 10};
Article brassiere = {"brassiere", 15};
Article pyjama = {"pyjama", 20};
Article *articles[3]={&body,&brassiere,&pyjama};
Article article;

char *salesmen[3] = {"Pierre", "Paul", "Jacques"};
char *clients[3] = {"Chloe", "Elise", "Lea"};
char *cashers[3] = {"Lilou", "Laura", "Nadia"};

char *name_salesman = NULL;
char *name_client = NULL;
char *name_casher = NULL;

char readMsg[MESSAGE_SIZE];
char writeMsg[MESSAGE_SIZE];

pid_t pid_client, pid_salesman, pid_casher;
int salesman_to_client[2], client_to_salesman[2], casher_to_client[2], client_to_casher[2];

/**********************************
  Signature des fonctions utilisees
***********************************/

void passPIDClient();          /* Fonction uniquement employée par le processus Client qui transmet son PID aux autres processus (Vendeur et Caissier) */
void fetchPIDClientSalesman(); /* Le processus Vendeur recupere le PID du processus Client */
void fetchPIDClientCasher();   /* Le processus Caissier recupere le PID du processus Client */
void salesman();               /* Fonction du processus Vendeur pour communiquer avec le processus Client */
void casher();                 /* Fonction du processus Caissier pour communiquer avec le processus Client */
void clientAndSalesman();      /* Fonction du processus Client pour communiquer avec le processus Vendeur */
void clientAndCasher();        /* Fonction du processus Client pour communiquer avec le processus Caissier */

char* pick(char **array);             /* Fonction qui renvoie aleatoirement un des elements du tableau passé en paramètres */
Article* pickArticle(Article **tab);  /* Fonction qui renvoie aleatoirement un des elements du tableau d'Articles passé en paramètres */
char* getArticleName(Article *a);     /* Retourne le nom d'un article */
int getArticlePrice(Article a);       /* Retourne le prix d'un article */


int main(void){

  /* Creation des pipes */
  if(pipe(salesman_to_client) != 0 || pipe(client_to_salesman) != 0){
      fprintf(stderr, "Erreur de creation du tube client/salesman.\n");
      return EXIT_FAILURE;
  }
  if(pipe(casher_to_client) != 0 || pipe(client_to_casher) != 0){
      fprintf(stderr, "Erreur de creation du tube client/casher.\n");
      return EXIT_FAILURE;
  }

  /* Choix de la cliente, du vendeur, de la caissiere et de l'article */
  article = *pickArticle(articles);
  name_casher = pick(cashers);
  name_client = pick(clients);
  name_salesman = pick(salesmen);

  printf("\n\nLe scenario va mettre en situation %s qui demande à %s de l'aide pour choisir un(e) %s, et qui va passer en caisse avec %s.\n\n",name_client,name_salesman,getArticleName(&article),name_casher);

  if((pid_salesman = fork()) == -1){
    perror("fork");
    exit(1);
  }
  if(pid_salesman == 0){
    /*****************
      BLOC DU VENDEUR 
    ******************/

    /* Pour que le processus Vendeur recupere le PID du processus Client */
    signal(SIGUSR1, fetchPIDClientSalesman);
    pause();

    #ifdef DEBUG
      printf("\nJe suis le processus vendeur (%d)\npid_casher = %d; pid_salesman = %d; pid_client = %d\n\n",getpid(),pid_casher,pid_salesman,pid_client);
    #endif
    
    /* Pour la communication avec la cliente */
    for(int i=0; i<3; i++){
      signal(SIGUSR1, salesman);
      pause();
    }

    return EXIT_SUCCESS;
  }
  else{
    sleep(1);
    if((pid_casher = fork()) == -1){
      perror("fork");
      exit(1);
    }
    if(pid_casher == 0){
      /***********************
         BLOC DE LA CAISSIERE 
      ************************/

      /* Pour que le processus Caissier recupere le PID du processus Client */
      signal(SIGUSR1, fetchPIDClientCasher);
      pause();

      #ifdef DEBUG
        printf("Je suis le processus caissiere (%d)\npid_casher = %d; pid_salesman = %d; pid_client = %d\n\n",getpid(),pid_casher,pid_salesman,pid_client);
      #endif
      
      /* Pour la communication avec la cliente */
      for(int i=0; i < 3; i++){
        signal(SIGUSR1, casher);
        pause();
      }

      return EXIT_SUCCESS;
    }
    else{
      sleep(1);
      if((pid_client = fork()) == -1){
        perror("fork");
        exit(1);
      }
      if(pid_client == 0){
        /*********************
           BLOC DE LA CLIENTE
        **********************/

        /* Le processus Client translet son PID aux processus Vendeur et Caissier */
        signal(SIGUSR1, passPIDClient);
        pause();

        #ifdef DEBUG
          printf("Je suis le processus client (%d)\npid_casher = %d; pid_salesman = %d; pid_client = %d\n\n",getpid(),pid_casher,pid_salesman,pid_client);
        #endif
        
        /* Pour la communication avec le vendeur et la caissiere */
        for(int i=0; i < 3; i++){
          signal(SIGUSR1, clientAndSalesman);
          pause();
        }
        for(int i=0; i < 3; i++){
          signal(SIGUSR1, clientAndCasher);
          pause();
        }

        return EXIT_SUCCESS;
      }
      else{
        /*****************************
           BLOC DU SYSTEME CENTRALISE 
        ******************************/
        sleep(1);

        printf("\n%s entre dans le magasin...\n\n", name_client);
        /* Transmission du PID du processus Client aux autres processus */
        #ifdef DEBUG
          printf("Main: Envoi signal SIGUSR1 au client (%d) | Reponse attendue : appel passPIDClient\n", pid_client);
        #endif
        kill(pid_client, SIGUSR1);
        sleep(1);

        #ifdef DEBUG
          printf("Main: Envoi signal SIGUSR1 au caissier (%d) | Reponse attendue : appel fetchPIDClientCasher\n", pid_casher);
        #endif
        kill(pid_casher, SIGUSR1);
        sleep(1);

        #ifdef DEBUG
          printf("Main: Envoi signal SIGUSR1 au vendeur (%d) | Reponse attendu : appel fetchPIDClientSalesman\n", pid_salesman);
        #endif
        kill(pid_salesman, SIGUSR1);
        sleep(1);

        /* Communication entre le vendeur et la cliente */
        close(client_to_salesman[0]);
        write(client_to_salesman[1], "0", sizeof("0"));
        for(int i=0; i<3; i++){
          #ifdef DEBUG
            printf("\nMain: Envoi signal SIGUSR1 au vendeur %d\n", pid_salesman);
          #endif
          kill(pid_salesman, SIGUSR1);
          sleep(1);

          #ifdef DEBUG
            printf("\nMain: Envoi signal SIGUSR1 a la cliente %d\n", pid_client);
          #endif
          kill(pid_client, SIGUSR1);
          sleep(1);
        }

        /* Communication entre la caissiere et la cliente */
        printf("\n%s se dirige vers la caisse...\n\n", name_client);
        close(casher_to_client[0]);
        write(casher_to_client[1], "0", sizeof("0"));
        for(int i=0; i<3; i++){
          #ifdef DEBUG
            printf("\nMain: Envoi signal SIGUSR1 a la caissiere %d\n", pid_casher);
          #endif
          kill(pid_client, SIGUSR1);
          sleep(1);

          #ifdef DEBUG
            printf("\nMain: Envoi signal SIGUSR1 a la cliente %d\n", pid_client);
          #endif
          kill(pid_casher, SIGUSR1);
          sleep(1);
        }

        return EXIT_SUCCESS;
      }
    }
  }
  return 0;
}



/**********************************
            Fonctions
***********************************/

/* Fonction uniquement employée par le processus Client qui transmet son PID aux autres processus (Vendeur et Caissier) */
void passPIDClient(){
  #ifdef DEBUG
    printf("  On est dans transmetPID\n");
  #endif
  
  int pid = getpid();
  sprintf(writeMsg, "%d", pid);
  #ifdef DEBUG
    printf("  Nous sommes dans le processus %d, nous transmettons le pid %s\n", getpid(), writeMsg);
  #endif
  close(client_to_casher[0]);
  close(client_to_salesman[0]);

  write(client_to_casher[1], writeMsg, sizeof(writeMsg));   // PID client vers casher
  write(client_to_salesman[1], writeMsg, sizeof(writeMsg)); // PID client vers salesman

  #ifdef DEBUG
    printf("  On sort de transmetPID\n\n");
  #endif
  return;
}

/* Le processus Vendeur recupere le PID du processus Client */
void fetchPIDClientSalesman(){
  #ifdef DEBUG
    printf("  On est dans fetchPIDClientSalesman\n");
  #endif
  close(client_to_salesman[1]);
  read(client_to_salesman[0], readMsg, sizeof(readMsg));
  int pid = atoi(readMsg);
  #ifdef DEBUG
    printf("  Nous sommes dans le processus %d, nous recevons le pid %d\n",getpid(),pid);
  #endif
  pid_client = pid;
  #ifdef DEBUG
    printf("  Je suis le processus vendeur (%d)\n pid_casher = %d;  pid_salesman = %d; pid_client = %d\n",getpid(),pid_casher,pid_salesman,pid_client);
    printf("  On sort de recupPIDVendeur\n\n");
  #endif
  return;
}

/* Le processus Caissier recupere le PID du processus Client */
void fetchPIDClientCasher(){
  #ifdef DEBUG
    printf("  On est dans fetchPIDClientCasher\n");
  #endif
  close(client_to_casher[1]);
  read(client_to_casher[0], readMsg, sizeof(readMsg));
  #ifdef DEBUG
    printf("  Le message recu est : %s\n",readMsg);
  #endif
  int pid = atoi(readMsg);
  #ifdef DEBUG
    printf("  Nous sommes dans le processus %d, nous recevons le pid %d\n",getpid(),pid);
  #endif
  pid_client = pid;
  #ifdef DEBUG
    printf("  Je suis le processus caissiere (%d)\n pid_casher = %d;  pid_salesman = %d; pid_client = %d\n",getpid(),pid_casher,pid_salesman,pid_client);
    printf("  On sort de recupPIDCaissiere\n\n");
  #endif

  return;
}

/* Fonction du processus Vendeur pour communiquer avec le processus Client */
void salesman(){
  char client_message[2];
  char message_to_return[2];
  message_to_return[1] = '\0';

  
  close(client_to_salesman[1]);
  close(salesman_to_client[0]);
  
  /* On lit l'entree du tube client_to_salesman et on stocke le msg dans client_message */
  read(client_to_salesman[0], &client_message, sizeof(client_message));

  /* On evalue le premier caractere du message lu */
  switch(client_message[0]){
    case START_CONVERSATION:
      message_to_return[0] = HELLO;
      write(salesman_to_client[1], message_to_return, sizeof(message_to_return));
      printf("Vendeur %s : Bonjour !\n", name_salesman);
      break;
    case HELLO:
      message_to_return[0] = ARTICLE;
      write(salesman_to_client[1], message_to_return, sizeof(message_to_return));
      printf("Vendeur %s : Que puis-je faire pour vous ?\n", name_salesman);
      break;
    case ARTICLE:
      message_to_return[0] = GIVE;
      write(salesman_to_client[1], message_to_return, sizeof(message_to_return));
      printf("Vendeur %s : Tenez voici un(e) %s, c'est l'un des meilleurs produits que nous proposons.\n", name_salesman, getArticleName(&article));
      break;
  }
  return;
}

/* Fonction du processus Caissier pour communiquer avec le processus Client */
void casher(){
  char client_message[2];
  char message_to_return[2];
  message_to_return[1] = '\0';

  close(client_to_casher[1]);
  close(casher_to_client[0]);

  read(client_to_casher[0], &client_message, sizeof(client_message));

  switch(client_message[0]){
    case GIVE:
      message_to_return[0] = TOTAL;
      write(casher_to_client[1], message_to_return, sizeof(message_to_return));
      printf("Caissiere %s : Bonjour ! Très bien je vais le doucher  **bip**. Ca fera %d€ s'il vous plait.\n",name_casher, getArticlePrice(article));
      break;
    case PAY:
      message_to_return[0] = RECEIPT;
      write(casher_to_client[1], message_to_return, sizeof(message_to_return));
      printf("Caissiere %s : Merci **tringggg** c'est encaissé. Et voilà pour vous, votre %s et j'ai mis le ticket de caisse dans votre sac.\n",name_casher, getArticleName(&article));
      break;
    case THX_BYE:
      /* dernier message échangé entre les processus donc pas besoin de renvoyer un message dans le pipe */
      printf("Caissiere %s : Merci à vous, bonne journée !\n\n",name_casher);
      break;
  }

  return;
}

/* Fonction du processus Client pour communiquer avec le processus Vendeur */
void clientAndSalesman(){
  char message[2];
  char message_to_return[2];
  message_to_return[1] = '\0';

  close(salesman_to_client[1]);
  close(client_to_salesman[0]);

  read(salesman_to_client[0], &message, sizeof(message));
  switch(message[0]){
    case HELLO:
      message_to_return[0] = HELLO;
      write(client_to_salesman[1], message_to_return, sizeof(message_to_return));
      printf("Cliente %s : Bonjour !\n",name_client);
      break;
    case ARTICLE:
      message_to_return[0] = ARTICLE;
      write(client_to_salesman[1], message_to_return, sizeof(message_to_return));
      printf("Cliente %s : Je cherche un(e) %s s'il vous plait !\n",name_client,getArticleName(&article));
      break;
    case GIVE:
      /* dernier message échangé entre les processus donc pas besoin de renvoyer un message dans le pipe */
      printf("Cliente %s : Merci, je vais à la caisse pour payer.\n",name_client);
      break;
    default:
      break;
  }
  return ;
}

/* Fonction du processus Client pour communiquer avec le processus Caissier */
void clientAndCasher(){
  char message[2];
  char message_to_return[2];
  message_to_return[1] = '\0';

  close(casher_to_client[1]);
  close(client_to_casher[0]);

  read(casher_to_client[0], &message, sizeof(message));
  switch(message[0]){
    case START_CONVERSATION:
      message_to_return[0] = GIVE;
      write(client_to_casher[1], message_to_return, sizeof(message_to_return));
      printf("Cliente %s : Bonjour ! Tenez voilà l'article que je souhaite acheter.\n", name_client);
      break;
    case TOTAL:
      message_to_return[0] = PAY;
      write(client_to_casher[1], message_to_return, sizeof(message_to_return));
      printf("Cliente %s : Entendu, voilà les %d€ demandés.\n", name_client, getArticlePrice(article));
      break;
    case RECEIPT:
      message_to_return[0] = THX_BYE;
      write(client_to_casher[1], message_to_return, sizeof(message_to_return));
      printf("Cliente %s : Merci beaucoup, au revoir !\n", name_client);
      break;
    default:
      break;
  }
  return ;
}

/* Fonction qui renvoie aleatoirement un des elements du tableau passé en paramètres */
char* pick(char** array){
    srand(time(NULL));
    int r = rand()%MAX_RAND;
    return array[r];
}

/* Fonction qui renvoie aleatoirement un des elements du tableau d'Articles passé en paramètres */
Article* pickArticle(Article** array){
    srand(time(NULL));
    int r = rand()%MAX_RAND;
    return array[r];
}

/* Retourne le nom d'un article */
char* getArticleName(Article *a){
  return a->name_article;
}

/* Retourne le prix d'un article */
int getArticlePrice(Article a){
  return a.price;
}