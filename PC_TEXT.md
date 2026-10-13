# The texts said as a computer's

The game's lines about the PlayStation 2 (its memory cards and their slots, the controller ports, switching the console off)
are said as a computer's, the way the Crash Twinsanity native port does it. The replacements are made when the game is loaded,
before it starts (`src/pctext.cpp`): each line of the game's text table that names the console is pointed at a new line,
after its original has been checked (an executable that differs anywhere is left as it is). The new lines take the old lines'
own space, which nothing else uses. A line with `%i` keeps it where the original had it (the game prints the free space it
needs there).

Kept as they are: the controller's button names in the tutorials' hints (L1, R2, the cross glyph) and the vibration option:
they're the controller's, and a gamepad has them.

The six languages: English, French, German, Italian, Spanish, Dutch. Ä, Ö, Ü and á are the font's own codes.

### no controller

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | INSERT CONTROLLER IN | PLEASE CONNECT |
| EN | 2 | CONTROLLER PORT 1 | A CONTROLLER |
| FR | 1 | INSERER UNE MANETTE DANS | VEUILLEZ BRANCHER |
| FR | 2 | LE PORT DE MANETTE 1 | UNE MANETTE |
| DE | 1 | CONTROLLER IN CONTROLLER- | BITTE EINEN CONTROLLER |
| DE | 2 | ANSCHLUSS EINSTECKEN | ANSCHLIESSEN |
| IT | 1 | INSERISCI CONTROLLER  | COLLEGA UN |
| IT | 2 | NELL'INGRESSO CONTROLLER | CONTROLLER |
| ES | 1 | INSERTA EL CONTROLADOR | CONECTA UN |
| ES | 2 | EN EL PUERTO 1 | CONTROLADOR |
| NL | 1 | SLUIT CONTROLLER AAN | SLUIT EEN |
| NL | 2 | OP CONTROLLER-AANSLUITING | CONTROLLER AAN |

### checking saves

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | CHECKING MEMORY CARD | CHECKING SAVE DATA |
| EN | 2 | (8MB) (FOR p) | (empty) |
| FR | 1 | VERIFICATION DE LA MEMORY | VERIFICATION DES |
| FR | 2 | CARD (8MO) (p) | SAUVEGARDES |
| DE | 1 | ÜBERPRÜFE MEMORY CARD | SPIELSTÄNDE WERDEN |
| DE | 2 | (8MB) (p)... | ÜBERPRÜFT |
| IT | 1 | CONTROLLO MEMORY CARD (8MB) | CONTROLLO DEI |
| IT | 2 | (p) IN CORSO | SALVATAGGI IN CORSO |
| ES | 1 | COMPROBANDO TARJETA DE | COMPROBANDO |
| ES | 2 | MEMORIA (8MB) (p) | DATOS GUARDADOS |
| NL | 1 | GEHEUGENKAART WORDT GECONTROLEERD | OPGESLAGEN GEGEVENS |
| NL | 2 | (8MB) (p) | WORDEN GECONTROLEERD |

### no save storage

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | PLEASE INSERT MEMORY CARD | SAVE DATA |
| EN | 2 | (8MB) (FOR p) | IS NOT AVAILABLE |
| EN | 3 | IN MEMORY CARD SLOT 1 | (empty) |
| FR | 1 | INSERER UNE MEMORY CARD | SAUVEGARDES |
| FR | 2 | (8MO) (p) | INDISPONIBLES |
| FR | 3 | DANS LA FENTE N°1 | (empty) |
| DE | 1 | BITTE MEMORY CARD | SPEICHERN IST |
| DE | 2 | (8MB) (p) IN MEMORY | NICHT MÖGLICH |
| DE | 3 | CARD-STECKPLATZ 1 EINLEGEN | (empty) |
| IT | 1 | INSERISCI LA MEMORY CARD | SALVATAGGI |
| IT | 2 | (8MB) (p) | NON DISPONIBILI |
| IT | 3 | NELL'INGRESSO MEMORY CARD 1 | (empty) |
| ES | 1 | INSERTE LA TARJETA DE MEMORIA | LOS DATOS GUARDADOS |
| ES | 2 | (8MB) (p) | NO ESTáN DISPONIBLES |
| ES | 3 | EN LA RANURA 1 | (empty) |
| NL | 1 | VOER GEHEUGENKAART | OPSLAAN IS |
| NL | 2 | (8MB) (p) | NIET MOGELIJK |
| NL | 3 | IN AANSLUITING 1 IN | (empty) |

### save data unreadable

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | MEMORY CARD | THE SAVE DATA |
| EN | 2 | (8MB) (FOR p) | COULD NOT BE READ |
| EN | 3 | IS NOT FORMATTED | (empty) |
| FR | 1 | LA MEMORY CARD (8MO) (p) | LES SAUVEGARDES |
| FR | 2 | INSEREE DANS LA FENTE N°1 | SONT ILLISIBLES |
| FR | 3 | N'EST PAS FORMATEE | (empty) |
| DE | 1 | MEMORY CARD (8MB) (p) | DIE SPIELSTÄNDE |
| DE | 2 | IN STECKPLATZ 1 | SIND NICHT LESBAR |
| DE | 3 | IST NICHT FORMATIERT | (empty) |
| IT | 1 | LA MEMORY CARD (8MB) (p) | IMPOSSIBILE LEGGERE |
| IT | 2 | NELL'INGRESSO MEMORY CARD 1 | I SALVATAGGI |
| IT | 3 | NON è FORMATTATA | (empty) |
| ES | 1 | LA TARJETA DE MEMORIA | NO SE PUEDEN LEER |
| ES | 2 | (8MB) (p) DE LA | LOS DATOS GUARDADOS |
| ES | 3 | RANURA 1 NO ESTáFORMATEADA | (empty) |
| NL | 1 | DE GEHEUGENKAART (8MB) | DE OPGESLAGEN GEGEVENS |
| NL | 2 | (p) IN GEHEUGENKAART- | ZIJN NIET LEESBAAR |
| NL | 3 | AANSLUITING 1 IS NIET GEFORMATTEERD | (empty) |

### not enough space

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | NOT ENOUGH SPACE ON | NOT ENOUGH SPACE |
| EN | 2 | MEMORY CARD | ON THE COMPUTER |
| EN | 3 | (8MB) (FOR p) | (empty) |
| EN | 4 | (empty) | (empty) |
| FR | 1 | ESPACE INSUFFISANT SUR LA MEMORY | ESPACE INSUFFISANT |
| FR | 2 | CARD (8MO) (p) POUR | SUR L'ORDINATEUR POUR |
| FR | 3 | SAUVEGARDER LA PARTIE | (kept) |
| FR | 4 | (empty) | (empty) |
| DE | 1 | NICHT GENÜGEND PLATZ AUF MEMORY | NICHT GENÜGEND PLATZ |
| DE | 2 | CARD (8MB) (p) ZUM | AUF DEM COMPUTER ZUM |
| DE | 3 | SPEICHERN DES SPIELS | (kept) |
| DE | 4 | (empty) | (empty) |
| IT | 1 | SPAZIO INSUFFICIENTE SULLA MEMORY | SPAZIO INSUFFICIENTE |
| IT | 2 | CARD (8MB) (p) PER | SUL COMPUTER PER |
| IT | 3 | SALVARE LA PARTITA | (kept) |
| IT | 4 | (empty) | (empty) |
| ES | 1 | NO HAY ESPACIO SUFICIENTE EN LA | NO HAY ESPACIO SUFICIENTE |
| ES | 2 | TARJETA DE MEMORIA (8MB) (p) | EN EL ORDENADOR |
| ES | 3 | PARA GRABAR LA PARTIDA | (kept) |
| ES | 4 | (empty) | (empty) |
| NL | 1 | ONVOLDOENDE RUIMTE OP DE | ONVOLDOENDE RUIMTE |
| NL | 2 | GEHEUGENKAART (8MB) (p) | OP DE COMPUTER |
| NL | 3 | OM SPEL OP TE SLAAN | (kept) |
| NL | 4 | (empty) | (empty) |

### don't switch off

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | PLEASE DO NOT REMOVE MEMORY | PLEASE DON'T TURN |
| EN | 2 | CARD (8MB) (FOR p) | OFF YOUR COMPUTER |
| EN | 3 | OR SWITCH OFF POWER | (empty) |
| FR | 1 | NE PAS RETIRER LA MEMORY | N'ETEIGNEZ PAS |
| FR | 2 | CARD (8MO) (p) | VOTRE ORDINATEUR |
| FR | 3 | NI ETEINDRE LA CONSOLE | (empty) |
| DE | 1 | BITTE MEMORY CARD (8MB) | BITTE DEN COMPUTER |
| DE | 2 | (p) NICHT ENTFERNEN | NICHT AUSSCHALTEN |
| DE | 3 | UND STROM NICHT AUSSCHALTEN | (empty) |
| IT | 1 | NON RIMUOVERE MEMORY CARD | NON SPEGNERE |
| IT | 2 | (8MB) (p) | IL COMPUTER |
| IT | 3 | E NON SPEGNERE CONSOLE | (empty) |
| ES | 1 | NO RETIRE LA TARJETA DE | NO APAGUES |
| ES | 2 | MEMORIA (8MB) (p) | EL ORDENADOR |
| ES | 3 | NI CORTE LA ALIMENTACIóN | (empty) |
| NL | 1 | VERWIJDER GEHEUGENKAART | SCHAKEL DE COMPUTER |
| NL | 2 | (8MB) (p) NIET EN | NIET UIT |
| NL | 3 | SCHAKEL DE STROOM NIET UIT | (empty) |

### free space needed (two messages)

| Language | Line | Original | Replacement |
|---|---|---|---|
| EN | 1 | YOU NEED A MEMORY CARD | (empty) |
| EN | 2 | (8MB) (FOR p) | YOU NEED AT LEAST |
| EN | 3 | WITH AT LEAST %iKB OF FREE SPACE TO | %iKB OF FREE SPACE TO |
| EN | 4 | SAVE YOUR PROGRESS | (kept) |
| EN | 5 | PLEASE INSERT A MEMORY CARD | (empty) |
| EN | 6 | (8MB) (FOR p) | PLEASE FREE UP AT LEAST |
| EN | 7 | WITH AT LEAST %iKB OR YOU WILL | %iKB OF SPACE OR YOU WILL |
| EN | 8 | BE UNABLE TO SAVE | (kept) |
| FR | 1 | NECESSITE CARTE MEMOIRE | (empty) |
| FR | 2 | (8MO) (p) | IL VOUS FAUT AU MOINS |
| FR | 3 | AVEC AU MOINS %iKO D'ESPACE LIBRE | %iKO D'ESPACE LIBRE |
| FR | 4 | POUR SAUVEGARDER LA PARTIE | (kept) |
| FR | 5 | VEUILLEZ INSERER CARTE MEMOIRE | (empty) |
| FR | 6 | (8MO) (p) | VEUILLEZ LIBERER AU MOINS |
| FR | 7 | AVEC AU MOINS %iKO SINON IL SERA | %iKO SINON IL SERA |
| FR | 8 | IMPOSSIBLE DE SAUVEG | (kept) |
| DE | 1 | SIE BENöTIGEN EINE MEMORY CARD | (empty) |
| DE | 2 | (8MB) (p) | SIE BENÖTIGEN MINDESTENS |
| DE | 3 | MIT MIND. %iKB FREIEM SPEICHER, UM IHRE | %iKB FREIEN SPEICHER, UM IHRE |
| DE | 4 | SPIELDATEN ZU SPEICHERN | (kept) |
| DE | 5 | SETZEN SIE DAHER FÜR DEN SPEICHERVORGANG | (empty) |
| DE | 6 | EINE MEMORY CARD | UM SPEICHERN ZU KÖNNEN, |
| DE | 7 | (8MB) (p) | GEBEN SIE BITTE MINDESTENS |
| DE | 8 | MIT MIND. %iKB EIN | %iKB SPEICHER FREI |
| IT | 1 | USA UNA MEMORY CARD | (empty) |
| IT | 2 | (8MB) (p) | SERVE SPAZIO SUL COMPUTER: |
| IT | 3 | CON ALMENO %iKB DI SPAZIO LIBERO PER | ALMENO %iKB LIBERI PER |
| IT | 4 | SALVARE PROGRESSI | (kept) |
| IT | 5 | INSERISCI UNA MEMORY CARD | (empty) |
| IT | 6 | (8MB) (p) | LIBERA SPAZIO SUL COMPUTER: |
| IT | 7 | CON ALMENO %iKB PER | ALMENO %iKB PER |
| IT | 8 | POTER SALVARE | (kept) |
| ES | 1 | USA UNA TARJETA DE MEMORIA | (empty) |
| ES | 2 | (8MB) (p) | NECESITAS AL MENOS |
| ES | 3 | CON %iKB DE ESPACIO LIBRE PARA | %iKB DE ESPACIO LIBRE PARA |
| ES | 4 | GRABAR TU PROGRESO | (kept) |
| ES | 5 | INTRODUCE UNA TARJETA DE MEMORIA | (empty) |
| ES | 6 | (8MB) (p) | LIBERA AL MENOS |
| ES | 7 | CON AL MENOS %iKB O | %iKB DE ESPACIO O |
| ES | 8 | NO PODRáS GRABAR | (kept) |
| NL | 1 | ER IS EEN MEMORY CARD | (empty) |
| NL | 2 | (8MB) (p) | JE HEBT MINSTENS |
| NL | 3 | MET MINSTENS %iKB RUIMTE NODIG OM | %iKB VRIJE RUIMTE NODIG OM |
| NL | 4 | JE GAME OP TE SLAAN | (kept) |
| NL | 5 | PLAATS EEN MEMORY CARD | (empty) |
| NL | 6 | (8MB) (p) | MAAK MINSTENS |
| NL | 7 | MET MINSTENS %iKB OF JE KUNT | %iKB RUIMTE VRIJ OF JE KUNT |
| NL | 8 | NIET OPSLAAN | (kept) |
