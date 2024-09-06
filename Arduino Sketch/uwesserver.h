//uwesserver.h
WiFiServer server(80);
WiFiClient client1;
#define MAX_PACKAGE_SIZE 2048
char HTTP_Header[110];
int Aufruf_Zaehler = 0, switch_color=0;
#define ACTION_Tor 1
#define ACTION_Wallbox 2
#define ACTION_Refresh 3
int action; //, leistung, solar;
//*********************************************************************************************************************
int  wifi_traffic() ;
int Pick_Parameter_Zahl(const char*, char*);
void make_HTML02() ;
void make_HTML_tabelle();
void send_bin(const unsigned char * , int, const char * , const char * ) ;
void send_not_found() ;
void send_HTML() ;
void set_colgroup( int, int, int, int, int) ;
void set_colgroup1( int ) ;
void strcati(char* , int ) ;
void strcati2(char*, int) ;
int Find_End(const char *, const char *) ;
int Find_Start(const char *, const char *) ;
int Pick_Dec(const char *, int ) ;
int Pick_N_Zahl(const char *, char, byte) ;
int Pick_Hex(const char * , int) ;
void Pick_Text(char *, char  *, int) ;
char HexChar_to_NumChar( char) ;
void exhibit(const char *, int) ;
void exhibit(const char *, unsigned int) ;
void exhibit(const char *, unsigned long) ;
void exhibit(const char *, const char *) ;
char mrk(int, int); 
//*********************************************************************************************************************
int wifi_traffic() {
  char my_char;
  int htmlPtr = 0,myIdx, myIndex;
  unsigned long my_timeout;
  client1 = server.available();  // Check if a client1 has connected
  if (!client1) return(-1);
  my_timeout = millis() + 250L;
  while (!client1.available() && (millis() < my_timeout) ) delay(10);
  delay(10);
  if (millis() > my_timeout) return(-1);
  htmlPtr = 0;
  my_char = '\0';
  while (client1.available() && my_char != '\r') { //\r = Return empfangen?
    my_char = client1.read();
    puffer[htmlPtr++] = my_char;
  }
  client1.flush();
  puffer[htmlPtr] = '\0';
  //Serial.print("Empfangen: ");Serial.println(puffer);
  if ( (Find_Start ("/X?", puffer) < 0 && Find_Start ("/t", puffer) < 0) && Find_Start ("/?r", puffer) <0  && Find_Start ("GET / HTTP", puffer) < 0 ) {
    send_not_found();
    return(-1);
  }
//  if( Find_Start ("/t", puffer) > 0 || Find_Start ("/X?r=R", puffer) > 0 || Find_Start ("/?r=R", puffer) ) { //Tabelle wurde gedrueckt
  if (Find_Start ("/t", puffer) > 0) { //Tabelle wurde gedrueckt
    //Serial.println("Tabelle");
    make_HTML_tabelle();
  }
  else {
    if (Find_Start ("/X?r=R", puffer) > 0 /*|| Find_Start ("/?r=R", puffer) */ ) { //Refresh wurde gedrückt
      //Serial.println("REFRESH");
    }
    else {
      if (Find_Start ("/X?", puffer) > 0) {
        html_s = Pick_Parameter_Zahl("s=", puffer);         // Benutzereingaben einlesen und verarbeiten
        //Serial.print("Start_h: "); Serial.println(html_s);
        html_e = Pick_Parameter_Zahl("e=", puffer);         // Benutzereingaben einlesen und verarbeiten
        //Serial.print("Ende_h: "); Serial.println(html_e);
        html_m = Pick_Parameter_Zahl("m=", puffer);         // Benutzereingaben einlesen und verarbeiten
        //Serial.print("radio: "); Serial.println(html_m);
        if (html_m != 3 ) html_h = 0; //die Anzahl Stunden wird bei den Mittelwert (Varianten) nicht gebraucht und verwirrt...
        else html_h = Pick_Parameter_Zahl("h=", puffer);         // Benutzereingaben einlesen und verarbeiten
        //Serial.print("Anz_h: "); Serial.println(html_h);
        html_z = Pick_Parameter_Zahl("z=", puffer);         // Benutzereingaben einlesen und verarbeiten
        //Serial.print("Check_B: "); Serial.println(html_z);
        myIndex = Find_End("i=", puffer);
        if (myIndex >= 0) Pick_Text(html_i, &puffer[myIndex], 15);
        //Serial.print("IP: "); Serial.println(html_i);
      }
    }
    make_HTML02();  //Antwortseite aufbauen
  }
  strcpy(HTTP_Header , "HTTP/1.1 200 OK\r\n"); // Header aufbauen
  strcat(HTTP_Header, "Content-Length: ");
  strcati(HTTP_Header, strlen(puffer));
  strcat(HTTP_Header, "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  client1.print(HTTP_Header);
  delay(20);
  send_HTML();
  return(1);
}
//*********************************************************************************************************************
void make_HTML02() {
  static boolean farbe=LOW;
  strcpy(puffer,"<!DOCTYPE html>");
  strcat(puffer,"<html>");
  strcat( puffer, "<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); //für Schriftgroesse Smartphone
  strcat( puffer, "<head><title>Tibber-Relais></title></head>");
  strcat( puffer, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
  if(farbe==LOW) strcat( puffer, "<body bgcolor=\"#adcede\">");    //Farbe hin und her schalten, als Quittung fürs absenden
  else strcat( puffer, "<body bgcolor=\"#decead\">");
  farbe=!farbe;
  strcat(puffer,"<body>");
  strcat(puffer,"<h2>Tibber Relais</h2>");
  strcat(puffer,"<form action=\"/X\">");
  strcat( puffer, "<form>");
//*********************** Fehlermeldung Beginn ***********************************
  if(strlen(fehlermeldung) > 0 ) sprintf(puffer+strlen(puffer),"<table><tr><p style=\"font-size:15px\"><font color=\"ff0000\">%s</font></tr></table>",fehlermeldung);
//*********************** Fehlermeldung Ende *************************************
//*********************** Start und Ende Stunde  Beginn **************************
  strcat( puffer, "<table>");
  set_colgroup(225, 30, 57, 0, 0);
  //set_colgroup(190, 40, 60, 0, 0);
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><label for=\"s\"> Zeitfenster (0 ... 23) von : </label></td>");
  strcat(puffer,"<td><input type=\"number\" style= \"width:40px\" id=\"s\" name=\"s\" min=\"0\" max=\"23\" value=\"");strcati(puffer,html_s);strcat(puffer,"\"></td>");
  strcat(puffer,"<td><label for=\"e\">&nbsp;&nbsp;  bis: </label></td>"); //&nbsp Leerzeichen
  strcat(puffer,"<td><input type=\"number\" style= \"width:40px\" id=\"e\" name=\"e\" min=\"0\" max=\"23\" value=\"");strcati(puffer,html_e);strcat(puffer,"\"></td>");
  strcat(puffer,"</tr>");
  strcat(puffer,"<br>");
  strcat( puffer, "</table>");
//*********************** Start und Ende Stunde Ende ***************************
//*********************** radio + preisw. Stunden Beginn ***********************
  //strcat(puffer,"<th><p>Einschalten, wenn besser als: </p></th>");
  strcat(puffer,"<br>Einschalten, wenn besser als: ");
  strcat( puffer, "<table>");
  set_colgroup(30, 305, 0, 0, 0);
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><input type=\"radio\" name=\"m\" id=\"0\" name=\"m\" value=\"0\"");if(html_m==0) strcat(puffer," CHECKED");strcat(puffer,"></td>");
  strcat(puffer,"<td><label for=\"0\"> (Mittelwert+Minimum)/2 </label><br></td>");
  strcat(puffer,"</tr>");
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><input type=\"radio\" name=\"m\" id=\"1\" name=\"m\" value=\"1\"");if(html_m==1) strcat(puffer," CHECKED");strcat(puffer,"></td>"); //CHECKED = Standardauswahl
  strcat(puffer,"<td><label for=\"1\"> Mittelwert</label><br></td>");
  strcat(puffer,"</tr>");
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><input type=\"radio\" name=\"m\" id=\"2\" name=\"m\" value=\"2\"");if(html_m==2) strcat(puffer," CHECKED");strcat(puffer,"></td>");
  strcat(puffer,"<td><label for=\"2\"> (Mittelwert+Maximum)/2 </label></td>");
  strcat(puffer,"</tr>");
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><input type=\"radio\" name=\"m\" id=\"3\" name=\"m\" value=\"3\"");if(html_m==3) strcat(puffer," CHECKED");strcat(puffer,"></td>");
  strcat(puffer,"<td><label for=\"3\"> Preiswerteste Stunden (Anzahl): </label></td>");
  strcat(puffer,"<td><input type=\"number\" style= \"width:40px\" id=\"h\" name=\"h\" min=\"1\" max=\"23\"value=\"");if(html_h>0) strcati(puffer,html_h);strcat(puffer,"\"></td>");
  strcat(puffer,"</tr>");
  strcat( puffer, "</table>");
//*********************** radio + preisw. Stunden Ende ***************
//*********************** Checkbox  Stunden im Stück Beginn **********
  strcat( puffer, "<table>");
  set_colgroup(30, 335, 0, 0, 0);
  strcat(puffer,"<tr>");
  strcat( puffer, "<td><p> </p></td>");
  strcat( puffer, "<td> <label for=\"z\"> Stunden im Stueck: </label></td>");
  strcat( puffer, "<td> <input type=\"checkbox\" id=\"z\" name=\"z\" value=\"1\""); if(html_z==1) strcat(puffer," checked "); strcat(puffer,"></td>");
  strcat(puffer,"</tr>");
  strcat( puffer, "</table>");
//*********************** Checkbox  Stunden im Stück Ende ************
//*********************** Textfeld IP-Adresse eingeben Beginn ********
  strcat(puffer,"<br>");
  strcat( puffer, "<table>");
  set_colgroup(277, 0, 0, 0, 0);
  strcat(puffer,"<tr>");
  strcat(puffer,"<td><label for=\"i\">IP-Adr Shelly: (z.B. 192.168.178.45) </label></td>");
  strcat(puffer,"<td><input type=\"text\" style= \"width:100px\" id=\"i\" name=\"i\"value=\"");strcat(puffer,html_i);strcat(puffer,"\"></td>");
  strcat(puffer,"<br>");
  strcat(puffer,"</tr>");
  strcat( puffer, "</table>");
//*********************** Textfeld IP-Adresse eingeben Ende*****************
//*********************** Senden Button Start ******************************
  strcat( puffer, "<table>");
  set_colgroup(315, 0, 0, 0, 0);
  strcat(puffer,"<tr>");
  strcat(puffer,"<br><td><input type=\"submit\" </td>");
  strcat( puffer, "</form>");
//*********************** Senden Button Ende *******************************
// *********************** Refresh Knopf ***********************************
  strcat( puffer, "<form><td><button style= \"width:70px\" name=\"r\" value=\"R\"><font color=\"000000\">Refresh</button></font></td></td>"); 
  strcat(puffer,"</tr>");
  strcat( puffer, "</table>");
// *********************** Refresh Knopf Ende*******************************
//*********************** graue Statuszeilee *******************************
  sprintf(puffer+strlen(puffer),"<br><p style=\"font-size:11px\"><font color=\"7f7f7f\"> %s, RSSI: %d, %s %02d.%02d.%04d %02d:%02d:%02d, %d<br></font>",
                                 vers, wifi_station_get_rssi(), wochentag[tm.tm_wday],tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_hour,tm.tm_min,tm.tm_sec, Aufruf_Zaehler++);
//*********************** graue Statuszeilee Ende **************************
  strcat(puffer,"</form>");
  strcat(puffer,"</body>");
  strcat(puffer,"</html> "); 
} // Ende make_HTML02
//*********************************************************************************************************************
char mrk(int t, int h) { //prueft ob h innerhalb des Schaltfensters und wenn ja ob Relais an (*) oder aus(x) ist
  if ( schalttabelle[t][h]==1 ) return('*');
  else {
    if(html_s<html_e && h>=html_s && h<=html_e && t==0 ) return('x');
    if(html_s>html_e && ( (h>=html_s && t==0) || (h<=html_e && t==1))) return('x');
    //return('.');
  }
  return('.');
}
//*********************************************************************************************************************
void make_HTML_tabelle () {
  static boolean farbe1=LOW;
  strcpy(puffer,"<!DOCTYPE html>");
  strcat(puffer,"<html>");
  strcat( puffer, "<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); //für Schriftgroesse Smartphone
  strcat( puffer, "<head><title>Tibber-Preise></title></head>");
  strcat( puffer, "<font color=\"#000000\" face=\"COURIER NEW\">");
  if(farbe1==LOW) strcat( puffer, "<body bgcolor=\"#adcede\">");    //Farbe hin und her schalten, als Quittung fürs absenden
  else strcat( puffer, "<body bgcolor=\"#decead\">");
  farbe1=!farbe1;
  strcat(puffer,"<body>");
  strcat(puffer,"<h2>Tibber-Relais-Preise</h2>");
  strcat(puffer,"<form action=\"/t\">");
  strcat( puffer, "<form>");

  strcat( puffer, "<table>");
  set_colgroup(250, 600, 800, 0, 0);
  sprintf(puffer+strlen(puffer),"<b>__________%s____________||||___________%s___________<br>", wochentag[pr_wo_tag],wochentag[pr_wo_tag+1]);
  strcat( puffer, "</table>");
  for(int h1=0;h1<12;h1++) {
    sprintf(puffer+strlen(puffer),"%02dh %c %04d || %02dh %c %04d |||| %02dh %c %04d || %02dh %c %04d<br>", h1, mrk(0,h1), preis[0][h1], h1+12, mrk(0,h1+12),preis[0][h1+12], h1,mrk(1,h1), preis[1][h1], h1+12,mrk(1,h1+12), preis[1][h1+12]); 
  }
  sprintf(puffer+strlen(puffer),"</b><br><p style=\"font-size:11px\"><font color=\"7f7f7f\"> * = ON, x = OFF, . = ausserhalb der Schaltzeit, Preise brutto in 1/100 Cent<br>"); 
  sprintf(puffer+strlen(puffer)," WICHTIG: Die Preise werden nur zur Stunde der Startzeit aktualisiert!<br>"); 
  sprintf(puffer+strlen(puffer)," Bei einem Zeitfenster über Mitternacht ist die Tabelle erst ab 14:00 Uhr<br>"); 
  sprintf(puffer+strlen(puffer)," und dem erreichen der Startzeit gueltig!<br>"); 
  sprintf(puffer+strlen(puffer)," %s, RSSI: %d, %s %02d.%02d.%04d %02d:%02d:%02d, Counter: %d<br>",
                                 vers, wifi_station_get_rssi(), wochentag[tm.tm_wday],tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_hour,tm.tm_min,tm.tm_sec, Aufruf_Zaehler++);
  
  // nur fuer Tests: static unsigned long minfreeheap1=100000;
  // nur fuer Tests: if(minfreeheap1 > ESP.getFreeHeap()) minfreeheap1 = ESP.getFreeHeap();
  // nur fuer Tests: sprintf(puffer+strlen(puffer),"FREEHEAP: ESP.getFreeHeap(): %d, min heap: %d, ESP.getResetReason(): %s, ESP.getHeapFragmentation()(>50 ist kritisch): %d, ESP.getMaxFreeBlockSize(): %d", 
  // nur fuer Tests:                ESP.getFreeHeap(), minfreeheap1, ESP.getResetReason().c_str(),ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize() ); 
  strcat(puffer,"<br></font></form>");
  strcat(puffer,"</body>");
  strcat(puffer,"</html> "); 
  //Serial.print("LAENGE: ");Serial.println(strlen(puffer));
}
//*********************************************************************************************************************
void send_not_found() {
  client1.print("HTTP/1.1 404 Not Found\r\n\r\n");
  delay(20);
  client1.stop();
}
//*********************************************************************************************************************
void send_HTML() {
  char my_char;
  int  my_len = strlen(puffer);
  int  my_ptr = 0;
  int  my_send = 0;
  while ((my_len - my_send) > 0) {            // in Portionen senden
    my_send = my_ptr + MAX_PACKAGE_SIZE;
    if (my_send > my_len) {
      client1.print(&puffer[my_ptr]);
      delay(20);
      my_send = my_len;
    } else {
      my_char = puffer[my_send];
      // Auf Anfang eines Tags positionieren
      while ( my_char != '<') my_char = puffer[--my_send];
      puffer[my_send] = 0;
      client1.print(&puffer[my_ptr]);
      delay(20);
      puffer[my_send] =  my_char;
      my_ptr = my_send;
    }
  }
  client1.stop();
}
//*********************************************************************************************************************
void set_colgroup(int w1, int w2, int w3, int w4, int w5) {
  strcat( puffer, "<colgroup>");
  set_colgroup1(w1);
  set_colgroup1(w2);
  set_colgroup1(w3);
  set_colgroup1(w4);
  set_colgroup1(w5);
  strcat( puffer, "</colgroup>");
}
//*********************************************************************************************************************
void set_colgroup1(int ww) {
  if (ww == 0) return;
  strcat( puffer, "<col width=\"");
  strcati( puffer, ww);
  strcat( puffer, "\">");
}
//*********************************************************************************************************************
void strcati(char* tx, int i) {
  char tmp[8];
  itoa(i, tmp, 10);
  strcat (tx, tmp);
}
//*********************************************************************************************************************
void strcati2(char* tx, int i) {
  char tmp[8];
  itoa(i, tmp, 10);
  if (strlen(tmp) < 2) strcat (tx, "0");
  strcat (tx, tmp);
}
//*********************************************************************************************************************
int Pick_Parameter_Zahl(const char * par, char * str) {
  int myIdx = Find_End(par, str);
  if (myIdx >= 0) return  Pick_Dec(str, myIdx);
  else return -1;
}
//*********************************************************************************************************************
int Find_Start(const char * such, const char * str) {
  int tmp = -1;
  int ll = strlen(such);
  int ww = strlen(str) - ll;
  for (int i = 0; i <= ww && tmp == -1; i++) {
    if (strncmp(such, &str[i], ll) == 0) tmp = i;
  }
  return tmp;
}
//*********************************************************************************************************************
int Find_End(const char * such, const char * str) {
  int tmp = Find_Start(such, str);
  if (tmp >= 0)tmp += strlen(such);
  return tmp;
}
//*********************************************************************************************************************
int Pick_Dec(const char * tx, int idx ) {
  int tmp = 0;
  for (int p = idx; p < idx + 5 && (tx[p] >= '0' && tx[p] <= '9') ; p++) {
    tmp = 10 * tmp + tx[p] - '0';
  }
  return tmp;
}
//*********************************************************************************************************************
int Pick_N_Zahl(const char * tx, char separator, byte n) {
  int ll = strlen(tx), tmp = -1;
  byte anz = 1, i = 0;
  while (i < ll && anz < n) {
    if (tx[i] == separator)anz++;
    i++;
  }
  if (i < ll) return Pick_Dec(tx, i);
  else return -1;
}
//*********************************************************************************************************************
int Pick_Hex(const char * tx, int idx ) {
  int tmp = 0;
  for (int p = idx; p < idx + 5 && ( (tx[p] >= '0' && tx[p] <= '9') || (tx[p] >= 'A' && tx[p] <= 'F')) ; p++) {
    if (tx[p] <= '9')tmp = 16 * tmp + tx[p] - '0';
    else tmp = 16 * tmp + tx[p] - 55;
  }
  return tmp;
}
//*********************************************************************************************************************
void Pick_Text(char * tx_ziel, char  * tx_quelle, int max_ziel) {
  int p_ziel = 0;
  int p_quelle = 0;
  int len_quelle = strlen(tx_quelle);
  while (p_ziel < max_ziel && p_quelle < len_quelle && tx_quelle[p_quelle] && tx_quelle[p_quelle] != ' ' && tx_quelle[p_quelle] !=  '&') {
    if (tx_quelle[p_quelle] == '%') {
      tx_ziel[p_ziel] = (HexChar_to_NumChar( tx_quelle[p_quelle + 1]) << 4) + HexChar_to_NumChar(tx_quelle[p_quelle + 2]);
      p_quelle += 2;
    } else if (tx_quelle[p_quelle] == '+') {
      tx_ziel[p_ziel] = ' ';
    }
    else {
      tx_ziel[p_ziel] = tx_quelle[p_quelle];
    }
    p_ziel++;
    p_quelle++;
  }
  tx_ziel[p_ziel] = 0;
}
//*********************************************************************************************************************
char HexChar_to_NumChar( char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 55;
  return 0;
}
//*********************************************************************************************************************
