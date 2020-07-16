/*
* Connecnting the ESP8622 to local newtwork
* =========================================
*/
#include "HardwareSerial.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// ESP file system
#include <FS.h>;

#define DEBUG


/*
* HTTP STATUS CODES
* =================
*/

typedef enum {
    HTTP_OK = 200,
    HTTP_SEE_OTHERS = 303,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_ERROR = 500
} http_status_codes_t;

/*
* Variables locales privadas
* ==========================
*/

// Hacer esto configurable en config.h
static const char *ssid = "Fibertel WiFi679 2.4GHz";
static const char *pswrd = "01418509537";
// Hacer esto configurable en config.h
const unsigned long baudrate = 115200;
// Nombre para el DNS
// Hacer esto configurable en config.h
static const char *host = "cnc";
// Instance of web server
ESP8266WebServer server(80);


/*
* Prototipos de funciones provadas
* ================================
*/

static void notFoundCallback(void);
static void rootCallback(void);
static void xrightCallback(void);

/*
* SetUp Function
* ==============
*/
void setup()
{
    // TODO: Ver que la configuración machee a la edu-ciaa
    Serial.begin(115200);
    // Solo iniciliazr wifi, dejar procesar al stack TCP/IP del esp
    WiFi.begin(ssid, pswrd);

    delay(500);
    if(WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println(".");
    }

#ifdef DEBUG
    Serial.println("Connected!");
    // Obtengo la SSID de la red
    Serial.println("SSID: " + WiFi.SSID());
    Serial.println("IP:");
    // Imprimo la direccion IP
    Serial.println(WiFi.localIP());
#endif

    // Le indico al server que función tiene que ejecutar cuando alguien hacer
    // un http get en "/"
    server.on("/", HTTP_GET, rootCallback);
    server.onNotFound(notFoundCallback);
    server.on("/xright", HTTP_GET, xrightCallback);

    delay(500);
    delay(500);
    //Start the SPI flash file system
    SPIFFS.begin();

    // Inicio el server DNS
    // En teoria ahora se puede abrir un navegador y teclear host.local
    if (MDNS.begin(host)) {
      MDNS.addService("http", "tcp", 80);
    }
    delay(500);

    // Inicio el servidor web
    server.begin();
    Serial.println("HTTP server started");
}

/*
* Infinite Loop
* =============
*/
void loop()
{
    server.handleClient();
    MDNS.update();
}


/*
* Implementación de funciones privadas
* ====================================
*/

static void rootCallback(void)
{
    server.send(HTTP_OK, "text/plain", "Hello to the CNC!!!");
}

static void notFoundCallback(void)
{
    String msg = "404: Not Found\n\n";
    msg += "URI: ";
    msg += server.uri();
    msg += "\nMethod: ";
    msg += (server.method() == HTTP_GET) ? "GET" : "POST";
    msg += "\nArguments: ";
    msg += server.args();
    for (uint8_t i = 0; i < server.args(); i++) {
        msg += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(HTTP_NOT_FOUND, "text/plain", msg);
}


static void xrightCallback()
{

    Serial.print("G1X1\n");
    // Si estoy aca es porque alguien apreto xright
    // Redirect the client to the success page
    server.sendHeader("Location","/");
    server.send(HTTP_SEE_OTHERS);
}














