# Piloter la température intérieure en utilisant une vanne motorisée sur chaudière fioul et avec thermostat intelligent (Netatmo)

## Problématique
* Le thermostat intelligent Netatmo permet de piloter une chaudière en envoyant des ordres simples avec un contact sec binaire : ON ou OFF.
* Le choix retenu est d’utiliser le statut du contact sec pour augmenter ou fermer la vanne motorisée de la chaudière plutôt que de piloter la température de la chaudière, plus compliqué car il faut maintenir une température minimale de 30°. Plusieurs types de vannes motorisées existent : 3 points ou proportionnelle. Une Vanne motorisée “proportionnelle” est plus adaptée à ce projet car elle permet de positionner la vanne sur une position cible quelle que soit sa position initiale. Une vanne 3 points ne permet  uniquement de la tourner dans un sens ou dans l'autre sans connaissance de sa position absolue, ce qui rendrait le pilotage plus compliqué. 
*  La vanne motorisée est pilotée avec un signal analogique 2-10V. 
*  Le choix a été fait d’utiliser un Arduino Due avec son port DAC (Digital to Analog Converter) pour produire le signal de pilotage. Ce dernier ne pouvant générer un signal analogique que de 0.55V à 2.75V, il a fallu rajouter un module amplificateur de tension en sortie afin d’arriver à un signal 2V à 10V.

## [Code du programme Arduino](chaudiere.ino)
L’arduino n’a pas connaissance de l’écart entre la température cible et la température actuelle, nous ne pouvons donc pas utiliser un programme PID classique. L’arduino ne connait que le statut du contact sec. A partir de cela, l'objectif du code du programme est de limiter l’oscillation de la température autour de la température cible. Quelques fonctionnalités implémentées :
* Limitation de l’ouverture à 80% pour minimiser le choc thermique au retour chaudière
* Ouvre ou ferme la vanne de X % toutes les 10 minutes selon le statut du contact sec du thermostat
* Compensation lors des changements d'état (ON vs OFF) pour contrer les oscillations de température (ex: après une longue phase de fermeture de la vanne, on rouvre la vanne plus fort et on attend plus longtemps que 10 min pour la prochaine variation)

## Intérêt du projet
* Economique par rapport aux modules constructeur de la chaudière
* Le thermostat intelligent permet de piloter la température à distance (utile pour maison secondaire peu occupée)
* Planning de chauffe précis et personnalisable à volonté sur l’application Netatmo

## Idées d’amélioration pour les prochaines versions
* Affiner le paramétrage du code et régler la vitesse de d’ouverture/fermeture de la vanne dynamiquement
* En plus du circuit chauffage, la chaudière chauffe également l’eau sanitaire et lorsque c’est le cas la température monte à 80°C et transmet donc cette chaleur au circuit chauffage en même temps. Idée de fermer la vanne lorsque ce cas se produit (nécessite un thermomètre à connecter à l’arduino) pour maintenir une température constante dans le circuit chauffage indépendamment du chauffe-eau qui s'active ou non.


## Equipement et budget
* Netatmo Thermostat Intelligent 170€ (occasion 50€)
* Arduino Due (avec DAC 0.55-2.75V) 42€
* Chargeur micro usb 5V 10€
* Amplificateur de tension (pour atteindre 2-10V) 12€
* Vanne motorisée ESBE ARA639 158€
* Vis boulon M6 x 4cm pour fixer le moteur à la vanne 6€
* Chargeur 24V 26€

Budget total environ 300€



## Photos dispositif 

![alt tag](photos/0.%20Vue%20d'ensemble.jpg)
![alt tag](photos/2.%20Planche.jpg)
