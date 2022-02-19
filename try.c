#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define N 256

typedef struct{
  char name_article[N];
  int price;
}article;

int main(int argc, char const *argv[]) {
  printf("1ere Methode\n");
  article body = {"body", 10};
  article brassiere = {"brassiere", 15};
  article pyjama = {"pyjama", 20};
  article *articles[3]={&body,&brassiere,&pyjama};
  for(int i=0;i<sizeof(articles);i++){
    printf("%s, %d\n", articles[i]->name_article,articles[i]->price);
  }
/*
  printf("2eme Methode\n");
  article *articles_2[3]={{"body",10},{"brassiere",15},{"pyjama",20}};
  for(int i=0;i<sizeof(articles_2);i++){
    printf("%s, %d\n", articles_2[i]->name_article,articles_2[i]->price);
  }
*/

  return 0;
}
