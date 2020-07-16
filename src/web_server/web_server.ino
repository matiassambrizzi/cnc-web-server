/*
* Connecnting the ESP8622 to local newtwork
* =========================================
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// ESP file system
#include <FS.h>

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
// Private instance of the file to upload
static File fsUploadFile;

static String buffer;



/*
* Prototipos de funciones provadas
* ================================
*/

static void sendCodeOk();
static void notFoundCallback(void);
static void rootCallback(void);
static void xrightCallback(void);
static void fileUploadCallback(void);

/**
* @brief Función para comenzar la transmisión de datos por UART
* @return Nothing
*/
static void sendGcodeCallback(void);

/**
* @brief Funcion para enviar por http un archivo del systema
* de archivos del ESP8266
* @param path es el nombre del archivo
* @return 0 succes, 1 fail
*/
static bool serveFile(String path);

/**
* @brief Funcion para determinar que tipo de archivo se quiere
* transmitir por http
* @param
* @return
*/
static String getContentType(String path);

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
    server.on("/xright", HTTP_GET, xrightCallback);
    server.on("/send", HTTP_GET, sendGcodeCallback);
    // There is a version of server.on(uri, method, fn, _fileUploadHandler);
    server.on("/", HTTP_POST, sendCodeOk, fileUploadCallback);
    server.onNotFound(notFoundCallback);

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

static void sendCodeOk()
{
    server.send(HTTP_OK);
}


static void rootCallback(void)
{
    if(!serveFile("/")) {
        server.send(HTTP_NOT_FOUND, "text/plain", "404: Not Found");
    }
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


static bool serveFile(String path)
{
#ifdef DEBUG
    Serial.println("Serve file:" + path);
#endif
    if(path.endsWith("/")) {
        // If the path ends with / then append index
        path += "index.html";
    }

    String contentType = getContentType(path);

    // Check if the file exist on the file system
    if(SPIFFS.exists(path)) {
        // If true then open the file
        File file = SPIFFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
#ifdef DEBUG
        Serial.println("File sended");
#endif
        return true;
    }

    return false;
}

static String getContentType(String path)
{
    if(path.endsWith(".html")) {
        return "text/html";
    }

    // TODO: Add more file types

    return "text/plain";
}


static void fileUploadCallback()
{
    String filename;
    HTTPUpload &upload = server.upload(); // Get Current Upload

    switch(upload.status) {

        case UPLOAD_FILE_START:
            filename = upload.filename; // nombre del archivo que quiero subir
            if(!filename.startsWith("/")) {
                // on SPIFFS all paths are absolute
                filename = "/" + filename;
            }
#ifdef DEBUG
            Serial.println("handleFIleUpload Name: " + filename);
#endif
            fsUploadFile = SPIFFS.open(filename, "w");
            //Abro el archivo para escribirlo en el SPIFFS
            // calculo que cuando lo abro al no estar en el SPIFFS crea un archivo con ese nombre
            filename = String(); // filenmae = NULL (?
            break;

        case UPLOAD_FILE_WRITE:
            if(fsUploadFile) {
                fsUploadFile.write(upload.buf, upload.currentSize);
            }
            break;

        case UPLOAD_FILE_END:
            if(fsUploadFile) {
                fsUploadFile.close();
                Serial.println("handleFileUpload Size: " + upload.totalSize);
                server.sendHeader("Location", "/");
                server.send(303);
            }
            break;

        default:
            server.send(500, "text/plain", "500: couldn't create file");
    }

}


static void sendGcodeCallback()
{
    char aux;
    // TODO: Mandar al usuario a una página que diga Transmicion en curso
    // Dar las opciones de pausar y continuar.... Ver de poner una lista 
    // con todos los archvios de codigo que esten en el sistema de arhchivo
    // para que pueda elegir cual transmitir...
    server.sendHeader("Location", "/");
    server.send(303);

    // Leer archivo de codigo G del sistema de archivos
    // Comenzar a transmitir
    // Ocupo toda la CPU??
    // Program yield() ??
    File gcodeFile = SPIFFS.open("/program.gcode", "r");

    if(!gcodeFile) {
#ifdef DEBUG
        Serial.println("Error Reading File");
#endif
        return;
    }

    // Send New Program Char
    // Wait for respose
    // And Start reading the file
    while(gcodeFile.available()) {
        // Esto me devuelve un byte
        // Tengo que leer hasta el \n
        if((aux = gcodeFile.read()) == '\n') {
            // Wait for the available space char
            while(Serial.read() != '$') { yield(); }
            Serial.print(buffer + "\n");
            buffer = "";
            yield();
        }else {
            buffer += aux;
        }
    }
    gcodeFile.close();
}







