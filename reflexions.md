# **Reflexions Projet Unix**

## Gestion des processus
* Comment le main va gerer tous les kill ?
* ou ping-pong entre les processus (signal, kill)
* comment transformer un int en chaine et vice versa ?

### Remarques
* pause() bloque un processus dans l'attente de la delivrance d'un signal
* On doit ecrire autant de fois signal(SIGUSR1, ...) que de kill vers le proc.
* Attention, un proc ne passe pas au dela de read si aucun msg a lire dans pipe, il attend de recevoir un msg.

## Gestion des articles (prix, encaissement...)
* Comment associer un article a son prix ? Un struct def
* Ticket de caisse :
    * Nom de l'article 
    * Prix
    * Type de Paiement
    * Montant
    * Rendu
* Sac ??

## Pistes d'amelioration

* Total à payer = petite somme pondérée 
* Cliente peut acheter plrs articles ?
