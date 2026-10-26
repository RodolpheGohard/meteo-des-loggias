Optimiser la consommation d’un ESP8266 côté **code** repose surtout sur trois axes : réduire le temps actif, limiter le Wi-Fi, et exploiter les modes basse consommation. Voici ce qui fait vraiment la différence en pratique.

---

## 1. Utiliser les modes de sommeil (crucial)

Le levier principal, c’est le **deep sleep**.

* `ESP.deepSleep(us);` → met le module en sommeil profond (≈ 20–100 µA seulement)
* Le module redémarre après le temps défini (ou via reset externe)

👉 Exemple :

```cpp
ESP.deepSleep(10e6); // 10 secondes
```

💡 Important :

* Il faut relier **GPIO16 (D0)** à **RST** pour réveil automatique
* Tout redémarre à zéro → stocker les données critiques (RTC memory ou flash si nécessaire)

⚠️ **Piège : D0-RST bloque le flash**

Câblé directement, D0-RST empêche le circuit auto-reset de l'adaptateur USB-série (ou le bouton reset) de faire entrer la carte en mode bootloader : l'état de D0 (haut en fonctionnement normal, tiré bas seulement au réveil) vient contredire ces impulsions, donc `esptool`/l'IDE Arduino n'arrive plus à flasher.

Solution retenue : une **diode en série** entre D0 et RST, **anode côté D0, cathode côté RST**. Elle laisse D0 tirer RST vers le bas pour réveiller la carte, mais bloque le courant dans l'autre sens quand le circuit auto-reset USB tire RST bas pour le flash — plus besoin de débrancher le fil à chaque upload.

---

## 2. Réduire le temps de Wi-Fi actif

Le Wi-Fi est de loin le plus énergivore.

### a) Se connecter plus vite

* Utiliser une IP fixe :

```cpp
WiFi.config(ip, gateway, subnet);
```

* Désactiver scan inutile :

```cpp
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
```

### b) Couper le Wi-Fi dès que possible

```cpp
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
```

---

## 3. Éviter les boucles actives inutiles

Un code mal optimisé tourne en permanence → consommation max.

👉 Mauvais :

```cpp
while (WiFi.status() != WL_CONNECTED) {
  delay(100);
}
```

👉 Mieux :

* Timeout + sleep
* Ou événementiel (`WiFi.onEvent`)

---

## 4. Réduire la fréquence CPU

Le ESP8266 peut tourner à 80 MHz au lieu de 160 MHz.

```cpp
system_update_cpu_freq(80);
```

→ gain modéré mais utile si actif longtemps

---

## 5. Désactiver ce qui ne sert pas

* LED onboard (souvent oubliée)
* Serial debug :

```cpp
Serial.end();
```

* ADC si inutilisé

---

## 6. Utiliser le modem sleep / light sleep

Moins radical que le deep sleep :

```cpp
WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
```

→ utile si connexion maintenue mais trafic faible

---

## 7. Minimiser les transmissions

* Envoyer des données **regroupées**
* Éviter les connexions répétées HTTP → préférer MQTT ou keep-alive

---

## 8. Optimiser la logique applicative

Le bon pattern :

1. Réveil
2. Mesure capteur
3. Connexion Wi-Fi
4. Envoi données
5. Sleep

👉 Exemple simplifié :

```cpp
void setup() {
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    sendData();
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  ESP.deepSleep(60e6); // 1 min
}

void loop() {}
```

---

## 9. Bonus : stockage RTC

Pour éviter des recalculs ou reconnexions :

```cpp
system_rtc_mem_write(...)
system_rtc_mem_read(...)
```

---

## Conclusion

Le gain ne vient pas d’une micro-optimisation mais d’une stratégie globale :

* **Deep sleep + réveil périodique → gain énorme**
* **Wi-Fi actif le moins longtemps possible**
* **CPU + périphériques réduits au strict minimum**

---

Si tu veux, je peux analyser ton code actuel et te dire exactement où tu perds de l’énergie (souvent il y a des erreurs classiques).

