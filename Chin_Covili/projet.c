/*
    Simulateur d'un magasin de puericulture dont les echanges sont geres par tubes.
    -- Elise CHIN, Emma COVILI
    -- 26/04/2020
*/

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

#define STRING_SIZE 256
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
#define CANTPAY '8'


/************************************
  Declaration des variables globales
*************************************/
typedef struct{
  char name_article[256];
  int price;
}Article;

typedef struct{
  Article content[3];
  int receipt[3];
}Bag;

Article body = {"body", 10};
Article brassiere = {"brassiere", 15};
Article pyjama = {"pyjama", 20};
Article *articles[3]={&body,&brassiere,&pyjama};
Article article;

Bag bag;

char *salesmen[3] = {"Pierre", "Paul", "Jacques"};
char *clients[3] = {"Chloe", "Elise", "Lea"};
char *cashers[3] = {"Lilou", "Laura", "Nadia"};

char *name_salesman = NULL;
char *name_client = NULL;
char *name_casher = NULL;

int money;

pid_t pid_client, pid_salesman, pid_casher;
int salesman_to_client[2], client_to_salesman[2], casher_to_client[2], client_to_casher[2];


/**********************************
  Signature des fonctions utilisees
***********************************/

void safePipe(int *pipe_name);  /* Creation d'un tube */
int safeFork();                 /* Duplication d'un processus */

int isInteger(char *input);                                   /* Verifie si l'entree est un nombre entier positif */
char* pick(char **array);                                     /* Fonction qui renvoie aleatoirement un des elements du tableau passé en paramètres */
void scenario_choice(Article *article, int *client_wallet);   /* Fontion qui gere le choix des scenarios */

void writeInPipe(char message, int *pipe_name); /* Ecrit dans un tube */
void salesman();                                /* Fonction du processus Vendeur pour communiquer avec le processus Client */
void casher();                                  /* Fonction du processus Caissier pour communiquer avec le processus Client */
void clientAndSalesman();                       /* Fonction du processus Client pour communiquer avec le processus Vendeur */
void clientAndCasher();                         /* Fonction du processus Client pour communiquer avec le processus Caissier */

Article* pickArticle(Article **tab);            /* Fonction qui renvoie aleatoirement un des elements du tableau d'Articles passé en paramètres */
Article* getArticleByName(char *article_name);  /* Retourne l'article dont le nom est renseigne en parametre */
char* getArticleName(Article *a);               /* Retourne le nom d'un article */
int getArticlePrice(Article a);                 /* Retourne le prix d'un article */

int payement(int article_price, int money_available);           /* Retourne difference entre le montant de l'article et le contenu du porte monnaie */
void setBagReceipt(Bag *bag, int total, int money, int change); /* Fonction qui le ticket de caisse */
void printReceipt(Bag bag);                                     /* Fonction qui affiche le ticket de caisse*/


/**********************************
        Fonction principale
 *********************************/

int main(void){

    /* Creation des pipes */
    safePipe(salesman_to_client);
    safePipe(client_to_salesman);
    safePipe(casher_to_client);
    safePipe(client_to_casher);

    /* Initialisation du scenario */
    printf("----- CHOIX DU SCENARIO -----\n\n");
    name_casher = pick(cashers);
    name_client = pick(clients);
    name_salesman = pick(salesmen);
    scenario_choice(&article, &money);

    printf("\n\n----- DEBUT DE LA SIMULATION ----- \n");
    printf("\n\nLe scenario va mettre en situation %s qui demande à %s de l'aide pour choisir un(e) %s, et qui va passer en caisse avec %s.\n\n",name_client,name_salesman,getArticleName(&article),name_casher);

    pid_salesman = safeFork();
    if(pid_salesman == 0){
        /**********************
            BLOC DU VENDEUR
        ***********************/

        /* Pour la communication avec la cliente */
        for(int i=0; i<3; i++){
            signal(SIGUSR1, salesman);
            pause();
        }

        exit(EXIT_SUCCESS);
    }
    else{
        sleep(1);
        pid_casher = safeFork();
        if(pid_casher == 0){
            /***************************
                BLOC DE LA CAISSIERE
            ****************************/

            /* Pour la communication avec la cliente */
            for(int i=0; i < 3; i++){
                signal(SIGUSR1, casher);
                pause();
            }

            exit(EXIT_SUCCESS);
        }
        else{
            /**********************************
                    BLOC DE LA CLIENTE
            ***********************************/
            /* Processus parent qui communique avec processus enfants (vendeur et cliente) */

            sleep(1);
            printf("\n%s entre dans le magasin...\n\n", name_client);

            /* Communication entre le vendeur et la cliente */
            close(client_to_salesman[0]);
            write(client_to_salesman[1], "0", sizeof("0"));
            for(int i=0; i<3; i++){
                kill(pid_salesman, SIGUSR1);
                sleep(1);

                clientAndSalesman();
                sleep(1);
            }

            /* Communication entre la caissiere et la cliente */
            printf("\n%s se dirige vers la caisse...\n\n", name_client);

            write(casher_to_client[1], "0", sizeof("0"));
            for(int i=0; i<3; i++){
                clientAndCasher();
                sleep(1);

                kill(pid_casher, SIGUSR1);
                sleep(1);
            }

            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}


/**********************************
            Fonctions
***********************************/

/* Creation d'un tube */
void safePipe(int *pipe_name){
    if(pipe(pipe_name) != 0){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

 /* Duplication d'un processus */
int safeFork(){
    int f = fork();
    if(f == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return f;
}

/* Verifie si l'entree est un nombre entier positif */
int isInteger(char *input){
    int val;
    char c;
    if(sscanf(input, "%d%c", &val, &c) != 1){
        printf("Valeur incorrecte. Veuillez saisir un nombre ENTIER.\n");
        return -1;
    }
    if(val < 0){
        printf("Valeur incorrecte. Veuillez saisir un nombre entier POSITIF.\n");
        return -1;
    }
    return val;
}

/* Fonction qui renvoie aleatoirement un des elements du tableau passé en paramètres */
char* pick(char** array){
    srand(time(NULL));
    int r = rand()%MAX_RAND;
    return array[r];
}

/* Fonction qui gere le choix des scenarios */
void scenario_choice(Article *article, int *client_wallet){
    printf("Entrez l'entier associé au scénario de votre choix.\n");
    printf("1. Si vous souhaitez etre spectateur/spectatrice de la simulation.\n");
    printf("2. Si vous desirez vous-meme choisir l'article de la cliente et le montant disponible dans son porte-monnaire.\n\n");
    printf("Option : ");

    char str[STRING_SIZE];
    int money = 0;
    int option;
    while(1){
        scanf("%d", &option);
        if(option == 1 || option == 2)
            break;
        printf("Valeur incorrecte. Veuillez saisir 1 ou 2 : ");
    }

    switch(option){
        case 1:
            *article = *pickArticle(articles);
            *client_wallet = 20;
            break;
        case 2:
            printf("\n * Choix nom de l'article * \n");
            while(1){
                printf("\nEntrez le nom de l'article (body, brassiere, pyjama) : ");
                scanf("%s", str);
                if(strcmp(str, "body") == 0 || strcmp(str, "brassiere") == 0 || strcmp(str, "pyjama") == 0)
                    break;
                printf("Valeur incorrecte. Veuillez reessayer s'il vous plait.\n");
            }
            *article = *getArticleByName(str);

            printf("\n * Choix argent de la cliente * \n");
            while(1){
                printf("\nEntrez le montant disponible dans le porte-monnaie de la cliente (entier) : ");
                scanf("%s", str);
                if((money = isInteger(str)) != -1)
                    break;
            }
            *client_wallet = money;
            break;
    }
}

/* Ecrit dans un tube */
void writeInPipe(char message, int *pipe_name){
    char message_to_return[2];
    message_to_return[0] = message;
    message_to_return[1] = '\0';

    close(pipe_name[0]);
    write(pipe_name[1], message_to_return, sizeof(message_to_return));

    return;
}

/* Fonction du processus Vendeur pour communiquer avec le processus Client */
void salesman(){
    char client_message[2];

    close(client_to_salesman[1]);
    read(client_to_salesman[0], &client_message, sizeof(client_message));

    /* On evalue le premier caractere du message lu */
    switch(client_message[0]){
        case START_CONVERSATION:
            writeInPipe(HELLO, salesman_to_client);
            printf("Vendeur %s : Bonjour !\n", name_salesman);
            break;
        case HELLO:
            writeInPipe(ARTICLE, salesman_to_client);
            printf("Vendeur %s : Que puis-je faire pour vous ?\n", name_salesman);
            break;
        case ARTICLE:
            writeInPipe(GIVE, salesman_to_client);
            printf("Vendeur %s : Tenez voici un(e) %s, c'est l'un des meilleurs produits que nous proposons.\n", name_salesman, getArticleName(&article));
            break;
        default:
            break;
    }
    return;
}

/* Fonction du processus Caissier pour communiquer avec le processus Client */
void casher(){
    char client_message[2];

    close(client_to_casher[1]);
    read(client_to_casher[0], &client_message, sizeof(client_message));

    switch(client_message[0]){
        case GIVE:
            writeInPipe(TOTAL, casher_to_client);
            printf("Caissiere %s : Bonjour ! Très bien je vais le doucher  **bip**. Ca fera %d€ s'il vous plait.\n", name_casher, getArticlePrice(article));
            break;
        case PAY:
            writeInPipe(RECEIPT, casher_to_client);
            printf("Caissiere %s : Merci **tringggg** c'est encaissé. Et voilà pour vous, votre %s et j'ai mis le ticket de caisse dans votre sac.\n",name_casher, getArticleName(&article));
            bag.content[0] = article;
            setBagReceipt(&bag, getArticlePrice(article), money, payement(getArticlePrice(article), money));
            printReceipt(bag);
            break;
        case CANTPAY:
            writeInPipe(THX_BYE, casher_to_client);
            printf("Caissiere %s : Oh tant pis, à demain !\n\n",name_casher);
            break;
        case THX_BYE:
            /* dernier message échangé entre les processus donc pas besoin de renvoyer un message dans le pipe */
            printf("Caissiere %s : Merci à vous, bonne journée !\n\n",name_casher);
            break;
        default:
            break;
    }

    return;
}

/* Fonction du processus Client pour communiquer avec le processus Vendeur */
void clientAndSalesman(){
    char message[2];

    close(salesman_to_client[1]);
    read(salesman_to_client[0], &message, sizeof(message));

    switch(message[0]){
        case HELLO:
            writeInPipe(HELLO, client_to_salesman);
            printf("Cliente %s : Bonjour !\n",name_client);
            break;
        case ARTICLE:
            writeInPipe(ARTICLE, client_to_salesman);
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

    close(casher_to_client[1]);
    read(casher_to_client[0], &message, sizeof(message));

    switch(message[0]){
        case START_CONVERSATION:
            writeInPipe(GIVE, client_to_casher);
            printf("Cliente %s : Bonjour ! Tenez voilà l'article que je souhaite acheter.\n", name_client);
            break;
        case TOTAL:
            if(payement(getArticlePrice(article), money)>=0){
                writeInPipe(PAY, client_to_casher);
                printf("Cliente %s : Entendu, voilà les %d€ demandés.\n", name_client, getArticlePrice(article));
            }
            else{
                writeInPipe(CANTPAY, client_to_casher);
                printf("Cliente %s : Ah mince, je n'ai pas assez d'argent sur moi. Je reviendrai demain. Au revoir ! \n", name_client);

            }
            break;
        case RECEIPT:
            writeInPipe(THX_BYE, client_to_casher);
            printf("Cliente %s : Merci beaucoup, au revoir !\n", name_client);
            break;
        default:
            break;
    }
    return ;
}

/* Fonction qui renvoie aleatoirement un des elements du tableau d'Articles passé en paramètres */
Article* pickArticle(Article** array){
    srand(time(NULL));
    int r = rand()%MAX_RAND;
    return array[r];
}

/* Retourne l'article dont le nom est renseigne en parametre */
Article* getArticleByName(char *article_name){
    for(int i=0; i<3; i++){
        if(strcmp(getArticleName(articles[i]), article_name) == 0){
          return articles[i];
        }
    }
    return NULL;
}

/* Retourne le nom d'un article */
char* getArticleName(Article *a){
    return a->name_article;
}

/* Retourne le prix d'un article */
int getArticlePrice(Article a){
    return a.price;
}

/* Fonction qui cree le ticket de caisse */
void setBagReceipt(Bag *bag, int total,int money, int change){
    bag->receipt[0] = total;
    bag->receipt[1] = money;
    bag->receipt[2] = change;
}

/* Fonction qui affiche le ticket de caisse*/
void printReceipt(Bag bag){
    printf("\tTicket de caisse :\n");
    printf("\t\t Montant : %d\n", bag.receipt[0]);
    printf("\t\t Liquide : %d\n", bag.receipt[1]);
    printf("\t\t Rendu monnaie : %d\n", bag.receipt[2]);

}

/* Retourne difference entre le montant de l'article et le contenu du porte monnaie */
int payement(int article_price, int money_available){
    return money_available - article_price;
}
