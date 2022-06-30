#include <SIM800L.h>

#include <TinyGPS++.h> //GPS LIBRERIA - SE AÑADE MANUALMENTE

#include <TinyGPSPlus.h> //GPS LIBRERIA - SE AÑADE MANUALMENTE

#include <SoftwareSerial.h> //LIBRERÍA PUERTO SERIAL

#include <LiquidCrystal.h> //LCD LIBRERIA

#include <AltSoftSerial.h>


#define rxPin 11
#define txPin 10
SoftwareSerial sim800L(rxPin,txPin); 

AltSoftSerial neogps;

SoftwareSerial ss(9,8); //Pines GPS

TinyGPSPlus gps;

LiquidCrystal lcd(7,6,5,4,3,2); //Pines LCD

unsigned long previousMillis = 0;
long interval = 60000;


void setup()
{
  Serial.begin(9600);
  
  sim800L.begin(9600);

  ss.begin(9600);
  
  lcd.begin(16,2); //Tamaño Pantalla

  Serial.println("Iniciando...");
  lcd.setCursor(0,0); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ARRIBA
            lcd.print("Iniciando");
          lcd.setCursor(0,1); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ABAJO
            lcd.print("GePOS");
  delay(10000);

  //Envía comando AT y devuelve AT OK cuando conecta el SIM
  sendATcommand("AT", "OK", 2000);
  delay(15000);
  sendATcommand("AT+CMGF=1", "OK", 2000);
  //sim800L.print("AT+CMGR=40\r");
  lcd.setCursor(0,0); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ARRIBA
            lcd.print("Conectando");
          lcd.setCursor(0,1); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ABAJO
            lcd.print("GePOS");
}

void loop()
{
  while(sim800L.available()){
    Serial.println(sim800L.readString());
  }
  while(Serial.available())  {
    sim800L.println(Serial.readString());
  }

    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval) {
       previousMillis = currentMillis;
       sendGpsToServer();
    }
    while (ss.available() > 0){
    gps.encode(ss.read());
    if (gps.location.isUpdated()){ //COMENTAR PARA ACTUALIZAR 
        
        delay(2500);

          lcd.setCursor(0,0); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ARRIBA
            lcd.print("LONG: ");
            lcd.print(gps.location.lng(),5); //PRINT LONG. EN PANTALLA CON 5 DIGITOS
          lcd.setCursor(0,1); //POSICIÓN DE ESCRITURA EN PANTALLA, PARTE DE ABAJO
            lcd.print("LAT: ");
            lcd.println(gps.location.lat(),5); //PRINT LAT. EN PANTALLA CON 5 DIGITOS
      
      }//COMENTAR PARA ACTUALIZAR
  }
}

 


int sendGpsToServer()
{
    //PUEDE TOMAR HASTA 60 SEGUNDOS EN ENVIAR DATOS
    boolean newData = false;
    for (unsigned long start = millis(); millis() - start < 2000;){
      while (neogps.available()){
        if (gps.encode(neogps.read())){
          newData = true;
          break;
        }
      }
    }
  
    //IF HAY NUEVOS DATOS
    if(true){
      newData = false;
      
      String latitude, longitude;
      float altitude;
      unsigned long date, time, speed, satellites;
  
      latitude = String(gps.location.lat(), 6); // LATITUD, CON 6 DIGITOS
      longitude = String(gps.location.lng(), 6); // LONG, CON 6 DIGITOS
      
      Serial.print("Longitud: ");               Serial.println(gps.location.lng(),8);     // ► PRINT LONGITUD CON 8 DIGITOS
      Serial.print("Latitud: ");                Serial.println(gps.location.lat(),8);     // ► PRINT LATITUD CON 8 DIGITOS
      Serial.print("Velocidad: ");              Serial.println(gps.speed.kmph());         // ► PRINT VELOCIDAD
      Serial.print("Satélites conectados: ");   Serial.println(gps.satellites.value());   // ► SATELITES EN USO
  
      //if (latitude == 0) {return 0;}
      
      Serial.println("SINCRONIZANDO..."); //Sincronización de los datos para generar la URL
      
      String url, temp;
      url = "http://gepos.ddns.net/gpsdata.php?lat=";
      url += latitude;
      url += "&lng=";
      url += longitude;

      Serial.println(url);    
      delay(5000);
          
    sendATcommand("AT+CFUN=1", "OK", 2000);
    //AT+CGATT = 1 SI ESTÁ CONECTADO || AT+CGATT = 0 NO ESTÁ CONECTADO
    sendATcommand("AT+CGATT=1", "OK", 2000);
    //CONEXIÓN GPRS
    sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);
    //Configuración del APN
    sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet\"", "OK", 2000);
    //ACTIVA GPRS
    sendATcommand("AT+SAPBR=1,1", "OK", 2000);
    //Servicio HTTP
    sendATcommand("AT+HTTPINIT", "OK", 2000); 
    sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 1000);
    sim800L.print("AT+HTTPPARA=\"URL\",\"");
    sim800L.print(url);
    sendATcommand("\"", "OK", 1000);
    //Configura HTTP
    sendATcommand("AT+HTTPACTION=0", "0,200", 1000);
    //TERMINA EL SERVICIO HTTP
    sendATcommand("AT+HTTPTERM", "OK", 1000);
    //TERMINA EL GPRS.
    sendATcommand("AT+CIPSHUT", "SHUT OK", 1000);

  }
  return 1;    
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);
    delay(100);
    
    //LIMPIA EL BUFFER
    while( sim800L.available() > 0) sim800L.read();
    
    if (ATcommand[0] != '\0'){
      //ENVÍA EL COMANDO AT PARA EL SIM
      sim800L.println(ATcommand);
    }

    x = 0;
    previous = millis();

    //TIME OUT EN CASO DE QUE NO FUNCIONE XD
    do{
        if(sim800L.available() != 0){
            response[x] = sim800L.read();
            x++;
            if(strstr(response, expected_answer) != NULL){
                answer = 1;
            }
        }
    }while((answer == 0) && ((millis() - previous) < timeout));

  Serial.println(response);
  return answer;
}

//AT+CFUN=1
//AT+CGATT=1
//AT+SAPBR=3,1,"Contype","GPRS"
//AT+SAPBR=3,1,"APN","internet"
//AT+SAPBR=1,1
