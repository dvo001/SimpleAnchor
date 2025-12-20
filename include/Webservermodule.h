
#ifndef WEBSERVER_MODULE_H
#define WEBSERVER_MODULE_H

#include <ESPAsyncWebServer.h>
#include <atomic>

class WebServerModule {
public:
    static WebServerModule& getInstance(); // Singleton-Methode
    void initializeAnchorServer();

private:
    WebServerModule(); // Privater Konstruktor für Singleton
    WebServerModule(const WebServerModule&) = delete;
    WebServerModule& operator=(const WebServerModule&) = delete;
    AsyncWebServer anchorServer;
};

#endif // WEBSERVER_MODULE_H
