#define vers "Vers.: 10.08.2024 tibber Relais"

const char *ssid = "Name des WLANs"; 
const char *password = "Passwort des WLANs";  
const char *token = "5K4MVS-OjfWhK_4yrjOlFe1F6kJXPVf7eQYggo8ebAE"; // API demo token Tibber, durch eigenen Token ersetzen

const char *tibberApi = "https://api.tibber.com/v1-beta/gql";
char puffer[2500],fehlermeldung[100];
int html_s=11,html_e=15,html_m=3,html_h=1,html_z=-1;//s=start,e=ende,m=0=(mw+min/2),m=1=mw,m=2=(mw+max/2),m=3=Auswahl der Stundenzahl, h=Stundenzahl, z=1=zusammenhängend 
char html_i[16]="192.168.178.126";
int cnt, html_min=30000, html_max=0, html_preis[24], html_preis_sort[24]; //Minimum, maximum, sortierung der Stunden gemaess html_s - html_e
long html_mw=0; //Mittelwert der Stunden gemaess html_s - html_e
int preis[2][24] ; //jeweils für den aktuellen und den folgenden Tag
long preismittel[2]={0, 0}; //jeweils für den aktuellen und den folgenden Tag
int min1[2]={0,0}, max1[2]={0,0}; //jeweils für den aktuellen und den folgenden Tag
// ZUM TESTEN        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19   20   21   22   23    Uhr
//int preis[2][24]={{2703,2685,2686,2674,2678,2687,2875,3020,2946,2774,2574,2508,2148,1939,2177,2368,2552,2745,2885,2840,2739,2661,2663,2601},//Preise zu Testzwecken vom 8.+9.3.2024
//                  {2562,2542,2528,2514,2467,2457,2489,2520,2520,2461,2281,1891,1875,1829,1888,2224,2515,2716,2817,2781,2689,2608,2609,2570}};//Preise zu Testzwecken vom 8.+9.3.2024
//   Min h: 13, 0.1939,          |||| Min h: 13, 0.1829,      
//   Max h:  7, 0.3020,          |||| Max h: 18, 0.2817,      
//  Mittelwert: 0.2630           |||| Mittelwert: 0.2431      
boolean schalttabelle[2][24]={  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}  };
int mini=30000, maxi=0, min_abgerundet, max_aufgerundet, spreizung;
long mittelwert[2]={0,0};
int tagz, m2, tag_alt, m2_alt, pr_wo_tag=0;
long html_min_zu=0, html_min_zu_alt = 10000000000;
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h> 
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>  
#include "time.h"
#include "ota.h" 
#include "Internetzeit.h"
#include "uwesserver.h"       // erledigt den Webseiteaufbau und Abfrage
#include <EEPROM.h>
#define relay 16 //D0
#define LED 2 //D4
#define AN LOW
#define AUS HIGH
StaticJsonDocument<1500> doc; //Speicher für die json-Daten vor dem Deserialisieren
ADC_MODE(ADC_VCC);
WiFiClientSecure client_sec;  
//*****************************************************************************************
void setup() {
  // Initialize serial and WiFi
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW); 
  pinMode(LED, OUTPUT);
  digitalWrite(LED, !LOW); //bei digital 0 leuchtet blaue Bord LED bei 1 nicht 
  WiFi.hostname("TR94");
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi...");  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  server.begin();
  sprintf(puffer,"\nconnected, address=%s; Hostname=%s, Version= %s\r\n",WiFi.localIP().toString().c_str(),WiFi.hostname().c_str(),vers);  Serial.print(puffer);
  eeprom_mng(0); //Werte der Eingabemaske aus EEPROM holen
  if(html_s>=html_e && html_s < 14) fehler(0); 
  delay(100);
}
//*****************************************************************************************
void loop() {
  static boolean flag13=false, fl_erster_durchlauf=false, fl_h_akt = false;
  static int h_akt=99;
  uwes_ota();
  static unsigned long t_alt = 0; 
  if(millis() > t_alt ) { 
    t_alt = millis() + 10*1000; // 1 Sekunden Pause
    showTime(); //Internetzeit aktualisieren
    //sprintf(puffer,"Zeit (loop-Schleife): %s, %02d.%02d.%04d %02d:%02d:%02d\n",wochentag[tm.tm_wday],tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_hour,tm.tm_min,tm.tm_sec);Serial.print(puffer);
  }
  int test_h=teststunde();
  if(test_h!=0) tm.tm_hour=test_h; // ###NUR ZUM TESTEN, Eingabe einer gewuenschten Stunde über Tastatur
  ////if( (tm.tm_hour == html_s && fl_h_akt == false) || fl_erster_durchlauf==false) {
  if( (  (tm.tm_hour == html_s || ( tm.tm_hour==14 && html_s<html_e  ))&& fl_h_akt == false) || fl_erster_durchlauf==false) {
    //bei aktueller Stunde = html_swird aktualisiert. Wenn die Schaltzeiten Html_s und Html_e am gleichen Tag sind auch um 14.oo Uhr,
    //wennn es über Mitternacht geht nicht, da z.B. die Zeit von 22-20Uhr gehen könnte und dann die Werte nicht mehr stimmen
    fl_erster_durchlauf=true;
    fl_h_akt = true;
    hole_tibber_preise();
    if(ermittele_schaltzeiten()<0) fehler(2); 
  }
  if( tm.tm_hour != html_s ) fl_h_akt = false;
  if(tm.tm_hour != h_akt) { //nach Beginn einer jeden vollen Stunde durchlaufen . . . 
    h_akt=tm.tm_hour;
    preisabhaengig_schalten();
    preisabhaengig_stunden_tabelle(); //fuer die Anzeige bei Eingabe mit ip-adr/t
  }
  if( wifi_traffic() > 0 ) { // es hat eine Eingabe ueber die html-Maske gegeben
    if(html_s>=html_e && html_s < 14) fehler(3); //bei Schaltzeiten ueber Mitternacht muss html_s>=14 sein
    else   fehlermeldung[0]='\0';
    eeprom_mng(1); //in 60 Sekunden werden die Aenderungen im EEPROM gespeichert
    if(html_m!=3) html_z=-1; //Stunden im Stueck wird nur  bei preiswerteste Stunden bewertet
    if(ermittele_schaltzeiten()<0) fehler(4); 
    preisabhaengig_schalten();
    preisabhaengig_stunden_tabelle(); //zum Testen
    //freeheap(); // zum Testen 
  }
  eeprom_mng(2); //gibt es etwas im EEPROM zu speichern
  delay(100); 
} //loop ende
//*****************************************************************************
void fehler(int f) {
  sprintf(fehlermeldung,"Bei Schaltzeiten über Mitternacht, muss<br>die Startzeit mindestens 14:00 betragen (%d)", f);
}
//*****************************************************************************
int ermittele_schaltzeiten() { //wird aufgerufen beim starten des Programms,wenn aktuelle Stunde == html_s und nach Aufruf der html_maske
  int cnt=0; // Zahl der Stunden die es zu beruecksihtigen gilt
  if(html_s > html_e && html_s <14) return(-1); //html_s muss < html_e sein oder html_s muss >13 sein, da es vorher keine Preise für den naechsten Tag gibt
  if(html_s > html_e) cnt =24-html_s+html_e+1; //mögliche Schaltzeit geht über 2 Tage...
  else cnt=html_e - html_s + 1;    
  //sprintf(puffer,"ermittele_schaltzeiten html_s=%d, html_e=%d, cnt=%d, strlen(html_i): %d\n", html_s, html_e, cnt, strlen(html_i));Serial.println(puffer);
  int tag=0, n1=0, anz=0, loop=0;
  html_min=30000;
  html_max=0;
  html_mw=0;
  html_min_zu_alt=10000000000; //Minimum der Summe der zusammenhaengenden Stunden 
  m2_alt=0;
  for(int n = html_s; n<= html_s+cnt-1; n++){
    if(n>23) {
      tag=1;
      n1=n-24;
    }
    else {
      tag=0;
      n1=n;
    }
    if(html_z<=0) { //html_z <=0: stunden müssen nicht zusammenhaengen
      if(preis[tag][n1] <html_min) html_min = preis[tag][n1]; //Minimalwert für die in der html-Maske gewählten Stunden berechnen
      if(preis[tag][n1] >html_max) html_max = preis[tag][n1]; //wie vor nur Maximalwert
      html_mw=html_mw+preis[tag][n1]; //wie vor "Mittelwert, Teil1"
      html_preis_sort[anz++]=preis[tag][n1]; //benoetigte Anzahl Elemente in ein "sort-Array" kopieren
      //sprintf(puffer,"ermittele_schaltzeiten, hier for-schleife: anz=%d, preis[tag][n1]=%d, tag=%d, n1=%d, cnt=%d\n", anz, preis[tag][n1], tag, n1, cnt);Serial.print(puffer);
    }
    else {  //html_z > 0: stunden müssen zusammenhaengen
      if(html_h>=cnt) return (-1);//Zahl der zusammenhaengenden Stunden muss kleiner sein als die Zahl der Stunden im betrachtetem Zeitraum
      tagz=0;
      m2=0;
      html_min_zu=0;
      if( (html_s+cnt-html_h-n) < 0) break;
      for(int m=n1;m<=n1+html_h-1;m++) {
        if(m>23) {
          tagz=1;
          m2=m-24;
        }
        else {
          if(tag==1)tagz=1; //in diesem Fall ist die aeussere for-Schleife "n" schon einen Tag weiter und die innere Forschleife"m" muss nachziehen
          else tagz=0;
          m2=m;
        }
        html_min_zu = html_min_zu + preis[tagz][m2]; //aufaddieren (wie "Mittelwert Teil1"
        //sprintf(puffer,"ermittele_schaltzeiten, aus der For n1=%d, html_min_zu: %d, m=%d, m2=%d, tagz=%d, html_s+cnt-html_h-n=%d\n",n1, html_min_zu,m, m2, tagz,html_s+cnt-html_h-n); Serial.print(puffer);
      }
      //sprintf(puffer,"ermittele_schaltzeiten, html_min_zu=%d,\n", html_min_zu);Serial.print(puffer);
      if(html_min_zu < html_min_zu_alt) {//kleinsten Wert merken
        html_min_zu_alt = html_min_zu;
        tag_alt=tag;
        m2_alt=n1; // m2_alt ist die Stunde, wo die preiswertesten zusammenhaengenden Stunden starten
      }
      //sprintf(puffer,"ermittele_schaltzeiten, hier zusammenhaengend: tag=%d, m2=%d, m2_alt=%d, html_min_zu_alt=%d\n", tag, m2, m2_alt, html_min_zu_alt); Serial.print(puffer);
    }
  }
  if(html_z<=0) {
    html_mw=html_mw/cnt; //Mittelwert, Teil2
    bubbleSort(html_preis_sort, cnt);  // "sort"-Array sortieren
  }
  //for(int m=0;m<cnt;m++) { sprintf(puffer,"ermittele_schaltzeiten, m=%d, html_min= %d, html_max=%d, html_mw=%d  cnt=%d, html_preis_sort[m]=%d\n",m, html_min,html_max, html_mw, cnt, html_preis_sort[m]); Serial.print(puffer);  }
  return(1);
} //ermittele_schaltzeiten Ende   
//*****************************************************************************
void bubbleSort(int arr[], int n) { //aufsteigend sortieren eines Arrays
  for (int i = 0; i < n - 1; i++)  // das Letzte i Element ist bereits am Platz 
     for (int j = 0; j < n - i - 1; j++) 
      if (arr[j] > arr[j + 1]) 
        swap(arr[j], arr[j + 1]); 
} 
//*****************************************************************************
void swap(int &a, int &b) { //Werte a und b vertauschen
  int c = a;
  a = b;
  b = c;
}
//*****************************************************************************
void preisabhaengig_schalten() {
  //html_m  0= (mittelwert+minimum)/2
  //        1= mittelwert
  //        2= (mittelwert+maximum)/2
  //        3= Anzahl Stunden
  int tg=tag(tm.tm_hour); //wenn die Schaltzeit über Mitternacht geht, wird in tg der jeweiliege Tag gespeichert um auf das Array richtig zugreifen zu können.
  //sprintf(puffer,"preisabhängig schalten: html_s=%d, html_e=%d, tg=%d, html_m=%d, html_h=%d, html_z=%d\n",html_s, html_e, tg, html_m,html_h, html_z); Serial.print(puffer);
  if(tg==-1) {
    schalten(LOW);
    return;
  }
  if( (html_s<html_e && tm.tm_hour >= html_s && tm.tm_hour <=html_e) || (html_s>html_e && (tm.tm_hour >= html_s || tm.tm_hour <=html_e))) {
    switch (html_m) {    
      case 0:
        if( (html_mw+html_min)/2 >= preis[tg][tm.tm_hour]) schalten(HIGH);
        else schalten(LOW);
        break;
      case 1:
        if( html_mw >= preis[tg][tm.tm_hour]) schalten(HIGH);
        else schalten(LOW);
        break;
      case 2:
        if( (html_mw+html_max)/2 >= preis[tg][tm.tm_hour]) schalten(HIGH);
        else schalten(LOW);
        break;
      case 3:
        if(html_z <=0) {
          if(preis[tg][tm.tm_hour]<=html_preis_sort[html_h-1]) schalten(HIGH);
          else schalten(LOW);
        }
        else { //in m2_alt steht die erste Stunde des zusammenhaengenden Bereichs
          int x=0;
          if(m2_alt + html_h >23 &&tg==1) x=m2_alt+html_h-23; //muss hier html_h ggf. gelöscht werden?
          else x=m2_alt;
          //sprintf(puffer,"preisabhängig schalten: tm.tm_hour=%d, m2_alt=%d, tg=%d, html_h=%d, x=%d\n", tm.tm_hour, m2_alt, tg, html_h, x); Serial.print(puffer);
          if(tm.tm_hour >= m2_alt && tm.tm_hour<x + html_h) schalten(HIGH);
          else schalten(LOW);
        }
        break;
      default: // tue nichts
      break;
    }
  }
} // preisabhaengig_schalten Ende
//*****************************************************************************
void preisabhaengig_stunden_tabelle() {
  //html_m  0= (mittelwert+minimum)/2
  //        1= mittelwert
  //        2= (mittelwert+maximum)/2
  //        3= Anzahl Stunden
  for(int h=0;h<=23;h++) { 
    schalttabelle[0][h]=0; //Tabelle vom Vortag loeschen
    schalttabelle[1][h]=0;
    int tg=tag(h); //wenn die Schaltzeit über Mitternacht geht, wird in tg der jeweiliege Tag gespeichert um auf das Array richtig zugreifen zu können.
    if (tg>=0) {
    //sprintf(puffer,"preisabhaengig_stunden_tabelle: html_s=%d, html_e=%d, tg=%d, html_m=%d, html_h=%d, html_z=%d, h=%d, tg=%d\n",html_s, html_e, tg, html_m,html_h, html_z,h,tg); Serial.print(puffer);
      switch (html_m) {
        case 0:
          if( (html_mw+html_min)/2 >= preis[tg][h]) schalttabelle[tg][h]=HIGH;
          else schalttabelle[tg][h]=LOW;
          break;
        case 1:
          if( html_mw >= preis[tg][h]) schalttabelle[tg][h]=HIGH;
          else schalttabelle[tg][h]=LOW;
          break;
        case 2:
          if( (html_mw+html_max)/2 >= preis[tg][h]) schalttabelle[tg][h]=HIGH;
          else schalttabelle[tg][h]=LOW;
          break;
        case 3:
          if(html_z <=0) {
            if(preis[tg][h]<=html_preis_sort[html_h-1]) schalttabelle[tg][h]=HIGH;
            else schalttabelle[tg][h]=LOW;
          }
          else {
            int x=0;
            if(m2_alt + html_h >23 &&tg==1) x=m2_alt+html_h-23; //muss hier html_h ggf. gelöscht werden?
            else x=m2_alt;
            //sprintf(puffer,"preisabhaengig_stunden_tabelle, Zeile 325   h=%d, m2_alt=%d, tg=%d, html_h=%d, x=%d\n", h, m2_alt, tg, html_h, x); Serial.print(puffer);
             if(h >= m2_alt && h<x + html_h) schalttabelle[tg][h]=HIGH;
            else schalttabelle[tg][h]=LOW;
          }
          break;
        default: // tue nichts
          break;
      }
    }
  }
  //for(int t=0;t<=1;t++) {
  //  for(int h=0;h<=23;h++) {sprintf(puffer,"t=%d, h=%2d, on/off=%d\n",t, h, schalttabelle[t][h]);Serial.print(puffer);}
  //}
} // preisabhaengig_stunden_tabelle Ende
//*****************************************************************************************
int tag(int h) { // return -1 wenn aktuelle Stunde nicht im Zeitfenster, wenn im Zeitfenster dann return 0 wenn Start < Ende, return 1 wenn Start > Ende (geht also über Folgetag, 
  if(html_s > html_e) {
    if(h>=html_s) return(0);
    else {
      if(h<=html_e) return(1);
      return(-1);
    }
  }
  if(h<html_s || h > html_e) return(-1); //liegt die aktuelle Stunde im Schaltfenster
  return(0);
}
//*****************************************************************************************
void schalten (boolean ein) {
  //sprintf(puffer,"Relais: %d, aktuelle Stunde: %2d\n", ein, tm.tm_hour); Serial.print(puffer);
  digitalWrite(LED, !ein); //LED hat umgekehrte Logik
  digitalWrite(relay, ein); 
  if(strlen(html_i)>=7) shelly(ein); // shelly via WLAN ein, bei kleiner 7 liegt definitiv keine gültige IP-Adr vor
}
//*****************************************************************************************
void shelly(boolean ein) {
  WiFiClient client;
  HTTPClient http;
  //Serial.print("[HTTP] begin...\n");
  strcpy(puffer,"http://");    //ip-Adresse aus Eingabemaske kopieren, 
  strcat(puffer,html_i);       //Bsp: fertiger String: "http://192.168.178.133/relay/0?turn=on"
  strcat(puffer,"/relay/0?turn=");
  if(ein==HIGH) strcat(puffer,"on");
  else strcat(puffer,"off");
    if (http.begin(client, puffer)) {  // HTTP
    //Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();    // start connection and send HTTP header
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        //Serial.println(payload);
      }
    } 
    else {
      //Serial.printf("hier shelly Funktion: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    //Serial.println("hier shelly Funktion: [HTTP] Unable to connect");
  }
}
//*****************************************************************************************
void hole_tibber_preise() { // wird direkt nach dem Start  und wenn aktuelle Stunde == html_s ist aufgerufen
  WiFiClientSecure client_sec; //HTTPS !!!
  client_sec.setInsecure(); //the magic line, use with caution
  HTTPClient https;
  strcpy(puffer, "Bearer ");
  strcat(puffer, token);
  https.begin(client_sec, tibberApi);
  https.addHeader("Content-Type", "application/json");  // add necessary headers
  https.addHeader("Authorization",  puffer);           // add necessary headers
  const char *anfrage = "{\"query\": \"{viewer { homes { currentSubscription{ priceInfo{ today{ total  } tomorrow { total  }}}}}}\"} ";
  int httpCode = https.POST(anfrage);
  if (httpCode == HTTP_CODE_OK) {
    //String response = https.getString();//Serial.println(response);// DeserializationError error = deserializeJson(doc, response);
    DeserializationError error = deserializeJson(doc,  https.getString());
    preise_aus_json(); 
  } 
  else {
    //Serial.println("something went wrong");
    //Serial.println(httpCode);
  }
  https.end();
  client_sec.stop();  // Disconnect from the server
} // hole_tibber_preise Ende
//*****************************************************************************************
void preise_aus_json() {
  //Serial.println("preise_aus_json    :");
  char tag[2][9]={"today", "tomorrow"};
  double preis_in_doublefloat;
  for(int to=0;to<2;to++) {
    for(int h=0;h<24;h++) {
      preis_in_doublefloat = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][tag[to]][h]["total"];
      preis[to][h] = int (10000*preis_in_doublefloat);
      //sprintf(puffer,"Tag= %9s, to=%2d, h=%2d, Preis= %4d\n", tag[to], to, h, preis[to][h]); Serial.print (puffer);
      int h1; if(h==23) h1=0; else h1=h;
      if(h>0) {
        if(preis[to][ min1[to] ]>preis[to][h1])  min1[to]=h1;
        if(preis[to][ max1[to] ]<preis[to][h1])  max1[to]=h1;
      }
      preismittel[to] =preis[to][h] + preismittel[to]; //mittelwert berechnen Teil 1
    }
    preismittel[to] = preismittel[to] / 24;  //mittelwert berechnen Teil 2    
    //sprintf(puffer,"\nMin= %2d, Minpreis= %6.4f, max= %2d,  Maxpreis= %4d, Mittelwert %4d\n\n",min1[to],preis[to][min1[to]],max1[to],preis[to][max1[to]],preismittel[to]);Serial.print (puffer);
  }
  pr_wo_tag=tm.tm_wday;
  //Serial.println("\n            Heute        ||||             Morgen"); // in kompakter Tabelle ausgeben
  //for(int h1=0;h1<12;h1++) {
  //  sprintf(puffer,"h=%2d: %4d || h=%2d: %4d |||| h=%2d: %4d || h=%2d: %4d ||\n", h1, preis[0][h1], h1+12, preis[0][h1+12], h1, preis[1][h1], h1+12, preis[1][h1+12]); Serial.print (puffer);
  //}
  //sprintf(puffer,            " Min h: %2d, %4d,        |||| Min h: %2d, %4d, \n", min1[0], preis[0][min1[0]],min1[1], preis[1][min1[1]]);
  //sprintf(puffer+strlen(puffer)," Max h: %2d, %4d,        |||| Max h: %2d, %4d, \n",max1[0], preis[0][max1[0]],max1[1], preis[1][max1[1]]);
  //sprintf(puffer+strlen(puffer),"Mittelwert: %4d         |||| Mittelwert: %4d\n\n",preismittel[0], preismittel[1]); Serial.print (puffer);
  //sprintf(puffer,"   <h:%2d, >h:%2d, =:%4d  ||||   <h:%2d, >h:%2d, =:%4d \n", min1[0], max1[0], preismittel[0], min1[1], max1[1] ,preismittel[1]); Serial.print (puffer);
}
//*****************************************************************************************
void eeprom_mng(int akt) { //0=lesen, 1=schreiben_init in 60 Sekunden, 2=schreiben
  static boolean fl_eeprom_save=true;
  static unsigned long t_eeprom_save = 0;   
  switch (akt) {
    case 0:
      EEPROM.begin(512); 
      html_s=int (EEPROM.read(0));// um beim naechsten Einschalten den vorherigen Wert zu haben.
      html_e=int (EEPROM.read(1));
      html_m=int (EEPROM.read(2));
      html_h=int (EEPROM.read(3));
      html_z=byte (EEPROM.read(4));
      if(html_z == 255) html_z = -1; // //-1 = hex FF FF wird als hex FF im Epromgespeichert und als 255 (dec)  zurückgeholt
      for(int n=0;n<16;n++) html_i[n]=char (EEPROM.read(n+5)); //ip-adr fuer shelly lesen
      EEPROM.commit();
      break;
    case 1:
      t_eeprom_save = millis() + 60 * 1000; //nach 1 Minute Werte im EEPROM sichern
      fl_eeprom_save=false;
      break;
    case 2: // es wird nur bei einer Aenderung gespeichert
      if(millis() >t_eeprom_save && fl_eeprom_save==false ) { 
        //Serial.println("**************** Hier eeprom_save");delay(100);
        fl_eeprom_save=true;
        EEPROM.begin(512); 
        if (EEPROM.read(0) != html_s) EEPROM.write(0,html_s); 
        if (EEPROM.read(1) != html_e) EEPROM.write(1,html_e); 
        if (EEPROM.read(2) != html_m) EEPROM.write(2,html_m); 
        if (EEPROM.read(3) != html_h) EEPROM.write(3,html_h); 
        int tz=EEPROM.read(4);
        if (tz == 255) tz= -1;
        if (tz != html_z) EEPROM.write(4,html_z);
        for(int n = 0 ; n < 16 ; n++) {
          if( html_i[n] != EEPROM.read(n+5) )  EEPROM.write(n+5,html_i[n]);
        }
        EEPROM.commit();
      }
      break;
    default: 
      break;
  }
}
//*****************************************************************************************
int teststunde() { //Eingabe eines beliebigen STundenwertes über die Tastatur im IDE-Monitorfenster
  int h_test=0;
  while (Serial.available() > 0) {// read the incoming byte:
    char incomingByte = Serial.read();
    if( incomingByte != 0x0A ) {
      h_test=h_test*10+incomingByte-0x30;
      return(h_test);  
    }
  }
  return (0);
}  
//************ nur fuer Tests !!! *********************************************
void freeheap() {
  static unsigned long minfreeheap=100000;
  if(minfreeheap > ESP.getFreeHeap()) minfreeheap = ESP.getFreeHeap();
   sprintf(puffer,"FREEHEAP: ESP.getFreeHeap(): %d, min heap: %d, ESP.getResetReason(): %s, ESP.getHeapFragmentation()(>50 ist kritisch): %d, ESP.getMaxFreeBlockSize(): %d\n", 
                 ESP.getFreeHeap(), minfreeheap, ESP.getResetReason().c_str(),ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize() ); 
   Serial.print(puffer);
}


