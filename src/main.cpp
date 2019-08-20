//#include <Adafruit_Sensor.h>
//#include <DHT.h>
//#include <DHT_U.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "8080";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";

//flag for saving data
bool shouldSaveConfig = false;


//#define DHTTYPE DHT11
//#define DHTPIN 2
//DHT dht(DHTPIN, DHTTYPE);
//ADC_MODE(ADC_VCC);

//const char *mac = "11:11:11:11:11:11:11:11";
const char *mac = "33:33:33:33:33:33:33:33";

//const char *ssid = "SCH";
// char *password = "Santiago200204492";

const char *mqttServer = "casachialechascomus.ml";
const int mqttPort = 1883; //este puerto es el que escucha el broker directo sobre tcp. la conexion entre por tcp
const char *mqttUser = "webClient";
const char *mqttPass = "121212";
String topico = "";

WiFiClient espClient;           //se declara una conexion de tipo wifi
PubSubClient client(espClient); //se genera una instancia llamada client de PubSubClient y se le pasa como parametro el medio de conexion que va a usar dicha instancia mqtt, en este caso la conexion wifi del esp

long lastMsg = 0;
char msg[25];

float temperatura = 0;
float humedad = 0;
float vdd = 0;
int beacon = 0;

//******************************
//****DECLARACION FUNCIONES*****
//******************************

void setupWifi();                                               //solo se declara la funcion para luego escribirla debajo del codigo
void callback(char *topic, byte *payload, unsigned int length); //funcion que se llama para el procesamiento de los mensajes que arriban al esp, payload es el mensaje en si
void reconnect();                                               //funcion para reconectar con el broker
void sensate(); //funcion que sensa la temperatura y humedad y la muestra por el puerto serial
void configWiFi();
void saveConfigCallback ();

//*****************************

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  //dht.begin();
  delay(100);
  Serial.begin(115200);
  randomSeed(micros());                   //planta una semilla para la generacion de numeros aleatorios.
  setupWifi();                            //funcion que conecta a wifi
  client.setServer(mqttServer, mqttPort); //seteo del cliente mqtt
  client.setCallback(callback);           //listener de mensajes. cuando llegue un mensaje se ejecutara la funcion que le pasamos entre parentesis, en este caso callback
  digitalWrite(LED_BUILTIN,HIGH);
}

void loop()
{
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

    client.publish("valores/33:33:33:33:33:33:33:33", msg); //a esta funcion publish se le pasa el topico bajo el cual se envian los valores y el char array de los valores. SOLO SE PUEDEN ENVIAR CHAR ARRAY, es por esto que se convierten las variables a string y luego a un char array.
  }
}

//******************************
//*****FUNCIONES****************
//******************************

void setupWifi()
{
  delay(10);/*
  Serial.println();
  Serial.println("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  { //agregar cantidad de intentos y una espera
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP*/

  //WiFiManager


  WiFiManager wifiManager;

  wifiManager.autoConnect("AutoConnectAP");

  //if you get here you have connected to the WiFi
  Serial.println("Conectado. Usando los ultimos parametros guardados)");

}

void callback(char *topic, byte *payload, unsigned int length)
{
  String incoming = "";
  String topico = String(topic);
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i]; //esta funcion convierte a caracter el contenido de payload (uno por uno) y se lo vamos sumando a la varible de tipo string incoming.
  }
  incoming.trim(); //esto limpia de espacios en blanco o caracteres extraños

  /*
  if (incoming == "on")
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("on");
  }
  if (incoming == "off")
  {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("off");
  }
  */
   //reemplazar el codigo siguiente por switch case

  if (topico=="act3" && incoming=="on") {
    Serial.println("Actuador on");
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (topico=="act3" && incoming=="off") {
    Serial.println("Actuador off");
    digitalWrite(LED_BUILTIN, HIGH);

  }
  if (topico=="wiFiManager3" && incoming=="on") {
    Serial.println("Programador on");
    configWiFi();
  }

  if (topico=="wiFiManager3" && incoming=="off") {
    Serial.println("Programador off");
    ESP.reset();
  }
}

void reconnect()
{
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
    }
  }
}

void sensate()
{

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

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configWiFi(){
  /*
  WiFiManager wifiManager;

  if (!wifiManager.startConfigPortal("OnDemandAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  */

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
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

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);

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



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

}
