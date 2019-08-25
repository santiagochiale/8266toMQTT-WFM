#include <Arduino.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//se definen las variables para los valores y las etiquetas que se usaran en el archivo de configuración config.json
char mqttServer[40];
char mqttPort[40];
char mqttUser[40];
char mqttPass[40];
char topicoRaizPublicacion[40];

//estas etiquetas se usan en el main, WifiManager.cpp y WifiManager.h para leer y escribir el archivo config.json
String etParametro1 = "Servidor_MQTT";
String etParametro2 = "Puerto_MQTT";
String etParametro3 = "User_MQTT";
String etParametro4 = "Pass_MQTT";
String etParametro5 = "Topico_Raiz_Publicacion";


#define DHTTYPE DHT11
#define DHTPIN 2
DHT dht(DHTPIN, DHTTYPE);
ADC_MODE(ADC_VCC);


String mac;
String topicoRecepcion = "";
char topicoPublicacion[50] = "";


WiFiClient espClient;           //se declara una conexion de tipo wifi
PubSubClient client(espClient); //se genera una instancia llamada client de PubSubClient y se le pasa como parametro el medio de conexion que va a usar dicha instancia mqtt, en este caso la conexion wifi del esp

long lastMsg = 0;
char msg[25];

float temperatura = 0;
float humedad = 0;
float vdd = 0;
int beacon = 0;
boolean SPIFFSinit = false;

//git desde platformio
//******************************
//****DECLARACION FUNCIONES*****
//******************************

void setupWifi();                                               //solo se declara la funcion para luego escribirla debajo del codigo
void callback(char *topic, byte *payload, unsigned int length); //funcion que se llama para el procesamiento de los mensajes que arriban al esp, payload es el mensaje en si
void reconnect();                                               //funcion para reconectar con el broker
void sensate(); //funcion que sensa la temperatura y humedad y la muestra por el puerto serial
void configWiFi();
void readDataSPIFFS();
void configMQTT();
void SPIFFSbegin();

//*****************************

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  //dht.begin();
  delay(100);
  Serial.begin(115200);
  randomSeed(micros());//planta una semilla para la generacion de numeros aleatorios.

  digitalWrite(LED_BUILTIN,HIGH);

  SPIFFSbegin();//funcion para iniciar el sistema de archivos y verifacar que el config.json no este corrupto

  setupWifi();//funcion que conecta a wifi via autoconnect. si no lo consigue llama a configWifi que abre el portal de configuracion
  configMQTT();

  int puertoMQTT = atoi(mqttPort);
  Serial.println(puertoMQTT);
  client.setServer(mqttServer, puertoMQTT); //seteo del cliente mqtt
  client.setCallback(callback);           //listener de mensajes. cuando llegue un mensaje se ejecutara la funcion que le pasamos entre parentesis, en este caso callback
}

void loop(){
  if (!client.connected())
  {              //esto controla si la conexion al broker esta activa o se cayo, si se cayo la reconecta
    reconnect(); //funcion que reconecta con el broker
  }
  client.loop(); //este metodo tiene que ser llamado muy frecuentemente. esto es lo que vigila que todo funcione correctamente NO PONER NUNCA UN DELAY AQUI.

  long now = millis();      //esta linea junto con el if reemplaza al delay. toma el valor en milsegundos del tiempo transcurrido desde que arranco la placa y lo guarda en la varible now, luego lo compara con una varible que se sobre escribe y lo topea en el valor del delay, en este caso 1 seg. luego reemplaza la varible y procesa las variables
  if (now - lastMsg > 5000) //periodo
  {
    lastMsg = now;
    sensate();
    //con la variable beacon se genera un variacion para que se vea en el dashboard a fines de depuracion y se acumula para la generacion del grabado de la base de datos
    //la cantidad que se setee aca debe ser la misma que de setee en node js
    //este punto se ejecuta cada T=perido. ej si periodo es 5000 ms (5s) y queremos generar un registro en la bd cada 10 min (10min*60seg*1000 = 600.000 ms) debemos generar un contador de beacon hasta 120 (600.000 ms / 5.000 ms = 120)
    if (beacon == 120)
    {
      beacon = 0;
    }
    else
    {
      beacon++;
    }

    //el proximo string genera el formato que devemos enviar a la plataforma (temp,hum,bat)
    String toSend = String(temperatura) + "," + String(humedad) + "," + String(vdd) + "," + String(beacon); //transforma las varibles a string y concatena para enviar. Pero la libreria pide un char array para enviar y nosotros tenemos un string. para eso se usa el msg[25]
    toSend.toCharArray(msg, 25);
    Serial.print("Publicamos mensaje --> ");
    Serial.println(msg);

    mac = WiFi.macAddress();
    (String(topicoRaizPublicacion) + "/" + mac).toCharArray(topicoPublicacion,50);

    Serial.println(topicoPublicacion);
    client.publish(topicoPublicacion, msg); //a esta funcion publish se le pasa el topico bajo el cual se envian los valores y el char array de los valores. SOLO SE PUEDEN ENVIAR CHAR ARRAY, es por esto que se convierten las variables a string y luego a un char array.
  }
}

//******************************
//*****FUNCIONES****************
//******************************

void setupWifi(){
  delay(10);

  WiFiManager wifiManager;

  if (!wifiManager.autoConnect("AutoConnectAP")) {
    configWiFi();
    Serial.println("Entrando a config WiFi");
  }else{
    //if you get here you have connected to the WiFi
    Serial.println("Conectado. Usando los ultimos parametros guardados)");
  }

}

void callback(char *topic, byte *payload, unsigned int length){
  String incoming = "";
  topicoRecepcion = String(topic);
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i]; //esta funcion convierte a caracter el contenido de payload (uno por uno) y se lo vamos sumando a la varible de tipo string incoming.
  }
  incoming.trim(); //esto limpia de espacios en blanco o caracteres extraños

  /*

  */
   //reemplazar el codigo siguiente por switch case

  if (topicoRecepcion=="act3" && incoming=="on") {
    Serial.println("Actuador on");
    digitalWrite(LED_BUILTIN, LOW);
    readDataSPIFFS(); //para propositos de debug se leen los valores guardados en el config.json cada vez que se acciona el actuador
  }
  if (topicoRecepcion=="act3" && incoming=="off") {
    Serial.println("Actuador off");
    digitalWrite(LED_BUILTIN, HIGH);

  }
  if (topicoRecepcion=="wiFiManager3" && incoming=="on") {
    Serial.println("Programador on");
    configWiFi();
  }

  if (topicoRecepcion=="wiFiManager3" && incoming=="off") {
    Serial.println("Programador off");
    ESP.reset();
  }
}

void reconnect(){
  int contIntentos = 0;
  while (!client.connected())
  {
    Serial.print("Intentando conexión Mqtt...");
    //Creamos el cliente ID
    String clientID = "dht11/33:33:33:33:33:33:33:33/"; //esto puede ser la mac del dispositivo, va a depender de la lista de accesos
    clientID += String(random(0xffff), HEX);
    //Intentamos reconectar
    if (client.connect(clientID.c_str(), mqttUser, mqttPass))
    { //esta linea genera la conexion al broker y devuelve un true si lo logra. c_str transforma un string en un char array
      Serial.println("Conectado!");
      //Nos suscribimos
      client.subscribe("act3");
      client.subscribe("wiFiManager3");
    }
    else
    {
      Serial.print("fallo : (con error -> )");
      Serial.print(client.state());
      Serial.println(" intentamos de nuevo en 5 segundos");
      delay(5000);
      if (contIntentos==5) {
        configWiFi();
        contIntentos=0;
      }else{
        contIntentos=contIntentos + 1;
      }
    }
  }
}

void sensate(){

  delay(50);
  //humedad = dht.readHumidity();
  humedad = humedad + 1;
  if (humedad ==50) {
    humedad = 0;
  }
  delay(50);
  //temperatura = dht.readTemperature() - 9; //se resta 9 por calibracion
  temperatura = temperatura + 1;
  if (temperatura ==50) {
    temperatura = 0;
  }
  delay(50);
  //vdd = ((ESP.getVcc() / 1024.00) - 0.17);
  vdd = vdd + 1;
  if (vdd==10) {
    vdd=0;
  }
  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Humedad: ");
  Serial.println(humedad);
  Serial.print("Vdd: ");
  Serial.println(vdd);

  /*
sensors_event_t event;

  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println("Error al leer la temperatura");
  }else{
    Serial.print("Temperatura: ");
    Serial.println(temperatura);
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println("Error al leer la humedad");
  }else{
    Serial.print("Humedad: ");
    Serial.println(humedad);
  }

  vdd = ((ESP.getVcc() / 1024.00) - 0.17);
  Serial.print("Vdd: ");
  Serial.println(vdd);
 */
}

void configWiFi(){

  WiFiManager wifiManager;

  String deviceName = "ESP8266_" + String(ESP.getChipId());
  char dName[20];
  deviceName.toCharArray(dName,50);

  Serial.println(deviceName);
  if (!wifiManager.startConfigPortal(dName) && SPIFFSinit) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ESP.reset();
}

void readDataSPIFFS(){
  Serial.println("mounting FS...");

  if (SPIFFSinit) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/configuracion.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/configuracion.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqttServer, json[etParametro1]);
          strcpy(mqttPort, json[etParametro2]);
          strcpy(mqttUser, json[etParametro3]);
          strcpy(mqttPass, json[etParametro4]);
          strcpy(topicoRaizPublicacion, json[etParametro5]);

          Serial.println(mqttServer);
          Serial.println(mqttPort);
          Serial.println(mqttUser);
          Serial.println(mqttPass);
          Serial.println(topicoRaizPublicacion);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void configMQTT(){
  Serial.println("mounting FS...");

  if (SPIFFSinit) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/configuracion.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/configuracion.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqttServer, json[etParametro1]);
          strcpy(mqttPort, json[etParametro2]);
          strcpy(mqttUser, json[etParametro3]);
          strcpy(mqttPass, json[etParametro4]);
          strcpy(topicoRaizPublicacion, json[etParametro5]);

          Serial.println(mqttServer);
          Serial.println(mqttPort);
          Serial.println(mqttUser);
          Serial.println(mqttPass);
          Serial.println(topicoRaizPublicacion);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void SPIFFSbegin(){
  if (SPIFFS.begin()) {
    SPIFFSinit = true;
    Serial.println("SPIFFS iniciado");
    if (!SPIFFS.exists("/configuracion.json")) {
      Serial.println("Archivo de configuaración dañado, grabado parametros de base");

      DynamicJsonBuffer jsonBuffer;
      JsonObject& jsonWrite = jsonBuffer.createObject();

      jsonWrite[etParametro1] = "";
      jsonWrite[etParametro2] = "";
      jsonWrite[etParametro3] = "";
      jsonWrite[etParametro4] = "";
      jsonWrite[etParametro5] = "";

      File configFile = SPIFFS.open("/configuracion.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }
      jsonWrite.printTo(Serial);
      jsonWrite.printTo(configFile);
      configFile.close();
      Serial.println("Archivo grabado correctamente");
    }
  }
}
