#pragma once
#include "storage.h"

// Traitement non-bloquant des commandes série.
// Appeler à chaque itération de loop().
// Les commandes SET_* mettent à jour cfg en mémoire ET en EEPROM.
// SET_WIFI déclenche une reconnexion WiFi immédiate.
void handleSerialCommands(AppConfig& cfg);
