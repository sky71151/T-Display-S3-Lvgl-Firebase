/**
 * SYNTAX:
 *
 * CloudStorage::getMetadata(<AsyncClient>, <FirebaseStorage::Parent>, <GoogleCloudStorage::GetOptions>, <AsyncResultCallback>, <uid>);
 *
 * <AsyncClient> - The async client.
 * <GoogleCloudStorage::Parent> - The GoogleCloudStorage::Parent object included Storage bucket Id and object in its constructor.
 * <GoogleCloudStorage::GetOptions> - The GoogleCloudStorage::GetOptions that holds the get options.
 * For the get options, see https://cloud.google.com/storage/docs/json_api/v1/objects/get#optional-parameters
 * <AsyncResultCallback> - The async result callback (AsyncResultCallback).
 * <uid> - The user specified UID of async result (optional).
 *
 * The bucketid is the Storage bucket Id of object to get metadata.
 * The object is the object in Storage bucket to get metadata.
 *
 * The complete usage guidelines, please visit https://github.com/mobizt/FirebaseClient
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_GIGA)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>) || defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#elif __has_include(<WiFiC3.h>) || defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

#include <FirebaseClient.h>

#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "Web_API_KEY"

/**
 * This information can be taken from the service account JSON file.
 *
 * To download service account file, from the Firebase console, goto project settings,
 * select "Service accounts" tab and click at "Generate new private key" button
 */
#define FIREBASE_PROJECT_ID "PROJECT_ID"
#define FIREBASE_CLIENT_EMAIL "CLIENT_EMAIL"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

// Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "BUCKET-NAME.appspot.com"

void timeStatusCB(uint32_t &ts);

void asyncCB(AsyncResult &aResult);

void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

// ServiceAuth is required for Google Cloud Storage functions.
ServiceAuth sa_auth(timeStatusCB, FIREBASE_CLIENT_EMAIL, FIREBASE_PROJECT_ID, PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

FirebaseApp app;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

CloudStorage cstorage;

bool taskCompleted = false;

void setup()
{

    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
    ssl_client.setInsecure();
#if defined(ESP8266)
    ssl_client.setBufferSizes(4096, 1024);
#endif
#endif

    initializeApp(aClient, app, getAuth(sa_auth), asyncCB, "authTask");

    // Binding the FirebaseApp for authentication handler.
    // To unbind, use cstorage.resetApp();
    app.getApp<CloudStorage>(cstorage);
}

void loop()
{
    // The async task handler should run inside the main loop
    // without blocking delay or bypassing with millis code blocks.

    // The JWT token processor required for ServiceAuth and CustomAuth authentications.
    // JWT is a static object of JWTClass and it's not thread safe.
    // In multi-threaded operations (multi-FirebaseApp), you have to define JWTClass for each FirebaseApp,
    // and set it to the FirebaseApp via FirebaseApp::setJWTProcessor(<JWTClass>), before calling initializeApp.
    JWT.loop(app.getAuth());

    app.loop();

    cstorage.loop();

    if (app.ready() && !taskCompleted)
    {
        taskCompleted = true;

        Serial.println("Get object metadata...");

        GoogleCloudStorage::GetOptions options;

        cstorage.getMetadata(aClient, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), options, asyncCB, "getMetadataTask");
    }
}

void timeStatusCB(uint32_t &ts)
{
#if defined(ESP8266) || defined(ESP32) || defined(CORE_ARDUINO_PICO)
    if (time(nullptr) < FIREBASE_DEFAULT_TS)
    {

        configTime(3 * 3600, 0, "pool.ntp.org");
        while (time(nullptr) < FIREBASE_DEFAULT_TS)
        {
            delay(100);
        }
    }
    ts = time(nullptr);
#elif __has_include(<WiFiNINA.h>) || __has_include(<WiFi101.h>)
    ts = WiFi.getTime();
#endif
}

void asyncCB(AsyncResult &aResult)
{
    // WARNING!
    // Do not put your codes inside the callback and printResult.

    printResult(aResult);
}

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
}