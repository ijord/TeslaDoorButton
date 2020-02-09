#include <fauxmoESP.h>

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <U8g2lib.h>

/* INSERT CREDENTIALS BELOW */
const char *ssid = "";  //ENTER YOUR WIFI SSID
const char *password = "";  //WIFI Password
String authorization_code = ""; //Enter token if you don't want code to have your user/pass. However tokens expire every 45 days.

const char *TeslaUsername = "";  //Tesla Username / Password. Will be used to get new token if the old one is expired. 
const char *TeslaPassword = "";
const int CarNumber = 0; //0 first car, 1 second. Only two supported. Increase DynamicJsonDocument IN(1600) to handle more
#define AlexaIntegration true


/* Set these to your desired credentials. */

const char *host = "owner-api.teslamotors.com";
const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80

#define vehicle_url   1
#define wake_url      2
#define lock_url      3
#define unlock_url    4


String vehicle_name = "TESLA"; 
String vehicle_id = "";

//SHA1 finger print of certificate use web browser to view and copy
const char fingerprint[] PROGMEM = "11 AB EF 76 55 1F EC DA 31 51 61 26 97 68 FD 13 9A 04 D6 B6";

int HueState=0;


//HUE
fauxmoESP fauxmo;
#define ID_REDCAR "Tesla Lock"

//U8G2
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);


//=======================================================================
//                    Power on setup
//=======================================================================

void setup() {
  u8g2.begin();
  u8g2.setFont(  u8g2_font_VCR_OSD_mf); // set the target font to calculate the pixel width
  u8g2.setFontMode(0);    // enable transparent mode, which is faster
  WriteOLED("BOOTING","");
  
  delay(1000);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
  WriteOLED("Connecting","WiFi");
  Serial.print("Connecting");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  WriteOLED("Connected",ssid); 
  delay(3000); 
  Serial.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);  //IP address assigned to your ESP
  WriteOLED("IP",String(ip[3]));
  delay(3000);
  pinMode(0, INPUT_PULLUP);

  fauxmo.addDevice(ID_REDCAR);
  fauxmo.setPort(80); // required for gen3 devices
  fauxmo.enable(AlexaIntegration);
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    // Callback when a command from Alexa is received. 
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

    // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
    // Otherwise comparing the device_name is safer.

     if (strcmp(device_name, ID_REDCAR)==0) 
     {
        if (state)
        {
          HueState = 1;
        }
        else
        {
          HueState = 2;
        }
    }
  });
if (authorization_code == "")
{
  Serial.println("Getting token");
   WriteOLED("Getting","Token"); 

  authorization_code = getauth();
} 
  Serial.println("Getting Car Name");
   WriteOLED("Getting","Car Name"); 
  GetCars(CarNumber);
  
  Serial.println("Car Name: " + vehicle_name);
  vehicle_name.toUpperCase();
  Serial.println("Exiting Setup");
 
 
}

//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {

  while (true) // remain here until told to break
  {
    fauxmo.handle();

    
    WriteOLED(vehicle_name,""); 

    switch (HueState)
    {
      case 1:
        WriteOLED("Alexa","UnLocking");
        unlock();
        HueState=0; 
        break;
      case 2:
        WriteOLED("Alexa","Locking");
        lock();
        HueState=0; 
        break;
      default:
        break; 
    }
    if (Serial.available() > 0)
    { // did something come in?
      switch (Serial.read())
      {
        case 'a':
          authorization_code = getauth();
          break;
         case 'b':
          GetAwake(CarNumber);
          break;
        case 'u':
          unlock();
          break;
        case 'v':
          //post(vehicle_url);
          break;
        case 'l':
          lock();
          break;

        default:
          Serial.println("Unknown");
      }
      //break;
    }
    if (!digitalRead(0))
    {
      int HoldTime = 0;
      while (!digitalRead(0))
      {
       HoldTime++;
       if (HoldTime > 100)
        break;
       delay(10);
      }
      if (HoldTime < 100)
      {
        Serial.println("Button Clicked");
           WriteOLED("CLICKED",""); 
          unlock();
      }
      else
      {
        Serial.println("Button Held");
           WriteOLED("LOCKING",""); 
          lock();
      }
    }
  }
}

bool unlock()
{
  if (post(unlock_url).indexOf("error") > 0)
  {
    Serial.println("Waking");
    WriteOLED("WAKING",""); 

    Serial.println(post(wake_url));
      int x = 0;

    while (post(unlock_url).indexOf("error") > 0)
    {
      
      Serial.println("Retrying");
         WriteOLED("TRY " + String(x),""); 
         x=x+1;
    }
    Serial.println("Done, Unlocked");
  }
  else
  {
    Serial.println("STATE: AWAKE, UNLOCKED");
  }
  WriteOLED("UNLOCKED",""); 
  delay (3000);
  return true;
}

bool lock()
{
  if (post(lock_url).indexOf("error") > 0)
  {
    Serial.println("Waking");
    Serial.println(post(wake_url));
    while (post(lock_url).indexOf("error") > 0)
    {
      Serial.println("Retrying");
    }
    Serial.println("Done, Locked");
  }
  else
  {
    Serial.println("STATE: AWAKE, LOCKED");
  }
         WriteOLED("LOCKED","");
delay (3000);
  return true;
}


String post1(int action)
{
  String url = "";
  switch (action)
  {
    case unlock_url:
      url = String("POST /api/1/vehicles/" + String(vehicle_id) + "/command/door_unlock");
      break;
    case lock_url:
      url = String("POST /api/1/vehicles/" + String(vehicle_id) + "/command/door_lock");
      break;
    case wake_url:
      url = String("POST /api/1/vehicles/" + String(vehicle_id) + "/wake_up");
      break;
    case vehicle_url:
      url = String("GET /api/1/vehicles/");
      break;
  }
  Serial.println(url);
  String line;
  bool expired = false;
  WiFiClientSecure client;
  client.setFingerprint(fingerprint);
  client.connect(host, httpsPort);

  client.println(String(url + " HTTP/1.1"));
  client.println(String("Host: ") + host);
  client.println("User-Agent: curl/7.58.0");
  //client.println("Accept: */*");
  client.println(String("Authorization: Bearer ") + authorization_code);
  client.println("Connection: close\r\n");

  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line.indexOf("401") > 0)
    {
      Serial.println("EXPIRED");
      expired = true;
    }

    Serial.println(line);
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  client.setTimeout(100);
  line = client.readString();  
  Serial.println(line);
  if (expired == false)
    return line;
  else
    return "BadAuth";
}

String post(int action)
{
  String line = post1(action);
  if (line == "BadAuth"){
      Serial.println("getting Auth");
      authorization_code = getauth();
      Serial.println("got auth");
      line = post1(action);
  }
  else
    return line;
}
  
String getauth()
{
  String line;
  String url = "/oauth/token";

  WiFiClientSecure client;
  client.setFingerprint(fingerprint);
  client.connect(host, httpsPort);

  DynamicJsonDocument OUT(220);
  
  OUT["grant_type"] = "password";
  OUT["client_id"] = "81527cff06843c8634fdc09e8ac0abefb46ac849f38fe1e431c2ef2106796384";
  OUT["client_secret"] = "c7257eb71a564034f9419ee651c7d0e5f7aa6bfbd18bafb5c5c033b093bb2fa3";
  OUT["email"] = TeslaUsername;
  OUT["password"] = TeslaPassword;
  
  serializeJson(OUT, line);

  client.println(String("POST ") + url + " HTTP/1.1");
  client.println(String("Host: ") + host);
  client.println("User-Agent: curl/7.58.0");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(line.length());
 
  client.println("");
  client.println(line);

  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  
  line = client.readStringUntil('{');  //Read Line by Line
  //Serial.println("1: " + line);
  line = ("{" +  client.readStringUntil('}') + "}");
  Serial.println(line);

  //const size_t capacity = 277 + 220;
  DynamicJsonDocument IN(277+220);

  deserializeJson(IN, line);

  const char* access_token = IN["access_token"]; //
  const char* token_type = IN["token_type"]; // "bearer"
  long expires_in = IN["expires_in"]; // 3888000
  const char* refresh_token = IN["refresh_token"]; // 
  long created_at = IN["created_at"]; // 
  
  Serial.println(access_token);
  Serial.println(token_type);
  Serial.println(expires_in);
  Serial.println(refresh_token);
  Serial.println(created_at);

  return access_token;
}

void GetCars(int Number)
{
  String Data = post(vehicle_url);
  Serial.println("GetCarName Data:");
  Serial.println(Data);
  DynamicJsonDocument IN(1600);
  deserializeJson(IN, Data);
  JsonObject response = IN["response"][Number];
  const char* response_display_name = response["display_name"];
  const char *id_s = response["id_s"];

  vehicle_id = id_s;
  Serial.println(response_display_name);
  Serial.println(id_s);
  vehicle_name = response_display_name;
}

String GetAwake(int Number)
{
  String Data = post(vehicle_url);
  Serial.println("GetCarName Data:");
  Serial.println(Data);
  DynamicJsonDocument IN(1600);
  deserializeJson(IN, Data);
  JsonObject response = IN["response"][Number];
  const char* response_state = response["state"];
  Serial.println(response_state);
  return response_state;
}

void WriteOLED(String A, String B)
{
    u8g2_uint_t x;
    u8g2.clearBuffer();
    //A.toUpperCase();
    //B.toUpperCase();
 
  if (B.length() > 0)
  {
    u8g2.setFont(  u8g2_font_VCR_OSD_mf); // set the target font to calculate the pixel width
    u8g2.setCursor(x, 15);
    u8g2.print(A);
    u8g2.setCursor(x, 32);
    u8g2.print(B);
  }
  else
  {
    if (A.length() == 7)
    {
      u8g2.setFont(u8g2_font_logisoso30_tf);
      u8g2.setCursor(0, 31);
    } 
    else if (A.length() == 8)
    {
      u8g2.setFont(u8g2_font_logisoso24_tf);
      u8g2.setCursor(0, 28);
    } 
    else
    {
      if (A.length() == 4)
        A = (" " + A);
      u8g2.setFont(u8g2_font_logisoso32_tf);
      u8g2.setCursor(0, 32);
    } 

    u8g2.print(A);
  }
  u8g2.sendBuffer();
}
