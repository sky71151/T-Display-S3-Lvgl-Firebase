
/**
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

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"
#define DATABASE_URL "URL"

void authHandler();

void printResult(AsyncResult &aResult);

void printError(int code, const String &msg);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

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

RealtimeDatabase Database;

AsyncResult aResult_no_callback;

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

    initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);

    authHandler();

    // Binding the FirebaseApp for authentication handler.
    // To unbind, use Database.resetApp();
    app.getApp<RealtimeDatabase>(Database);

    Database.url(DATABASE_URL);

    // In case setting the external async result to the sync task (optional)
    // To unset, use unsetAsyncResult().
    aClient.setAsyncResult(aResult_no_callback);

    Serial.println();

    Serial.println("Set data with priority... ");

    // Library does not provide JSON parser library, the following JSON writer class will be used with
    // object_t for simple demonstration.
    object_t json, obj1, obj2;
    JsonWriter writer;

    for (int i = 0; i < 15; i++)
    {
        // Set priority to object or node that contains children.
        float priority = 15 - i;

        writer.create(obj1, "item_" + String(i + 1), string_t("value_" + String(i + 1))); // -> {"item_x":"value_x"}
        writer.create(obj2, ".priority", string_t(priority));                             // -> {".priority":x}
        writer.join(json, 2, obj1, obj2);

        String path = "/test/items/priority_" + String(15 - i);

        bool status = Database.set<object_t>(aClient, path, json);
        if (status)
            Serial.println(String("ok"));
        else
            printError(aClient.lastError().code(), aClient.lastError().message());
    }

    // Change priority
    Serial.println();
    Serial.print("Set value with priority... ");

    writer.create(obj1, ".value", "item_15");
    // Set priority to primitive value.
    writer.create(obj2, ".priority", 6.0);
    writer.join(json, 2, obj1, obj2);

    bool status = Database.set<object_t>(aClient, "/test/items/priority_1", json);
    if (status)
        Serial.println(String("ok"));
    else
        printError(aClient.lastError().code(), aClient.lastError().message());

    Serial.println();
    Serial.println("Get priority... ");
    double v = Database.get<double>(aClient, "/test/items/priority_1/.priority");

    if (aClient.lastError().code() == 0)
        Serial.println(String(v));
    else
        printError(aClient.lastError().code(), aClient.lastError().message());

    Serial.println();
    Serial.println("Filtering data with priority... ");
    DatabaseOptions options;
    // Now the node priority_1 which its priority changed to 6.0 should include in the filtering result.
    options.filter.orderBy("$priority").startAt(3.0).endAt(8.0);

    String payload = Database.get<String>(aClient, "/test/items", options);
    if (aClient.lastError().code() == 0)
        Serial.println(payload);
    else
        printError(aClient.lastError().code(), aClient.lastError().message());
}

void loop()
{
    authHandler();

    Database.loop();
}

void authHandler()
{
    // Blocking authentication handler with timeout
    unsigned long ms = millis();
    while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
    {
        // The JWT token processor required for ServiceAuth and CustomAuth authentications.
        // JWT is a static object of JWTClass and it's not thread safe.
        // In multi-threaded operations (multi-FirebaseApp), you have to define JWTClass for each FirebaseApp,
        // and set it to the FirebaseApp via FirebaseApp::setJWTProcessor(<JWTClass>), before calling initializeApp.
        JWT.loop(app.getAuth());
        printResult(aResult_no_callback);
    }
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
}

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}