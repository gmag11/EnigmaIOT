
#ifndef _ENIGMAIOTNODE_WEB_PORTAL_CONFIGURER_h
#define _ENIGMAIOTNODE_WEB_PORTAL_CONFIGURER_h

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#include <queue>

#include "rtc_data.h"

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void(bool status)> onWiFiManagerExit_t;
typedef std::function<void(void)> simpleEventHandler_t;
#else
typedef void (*onWiFiManagerExit_t)(bool status);
typedef void (*onWiFiManagerStarted_t)(void);
#endif

/**
 * @brief wifi manager node configurer, start AP and ask user to enter configuration
 */
class EnigmaIOTNodeWifiConfigurerClass {
   protected:
    AsyncWiFiManager* wifiManager;                  ///< @brief Wifi configuration portal
    onWiFiManagerExit_t notifyWiFiManagerExit;      ///< @brief Function called when configuration portal exits
    simpleEventHandler_t notifyWiFiManagerStarted;  ///< @brief Function called when configuration portal is started

   public:
    /**
     * @brief initialize wifi manager and start configuration portal,
     * populate data with user provided data
     * @param data pointer to TRC data structure
     */
    bool configure(rtcmem_data_t* data);

    /**
     * @brief Register callback to be called on wifi manager exit
     * @param handle Callback function pointer
     */
    void onWiFiManagerExit(onWiFiManagerExit_t handle) {
        notifyWiFiManagerExit = handle;
    }

    /**
     * @brief Register callback to be called on wifi manager start
     * @param handle Callback function pointer
     */
    void onWiFiManagerStarted(simpleEventHandler_t handle) {
        notifyWiFiManagerStarted = handle;
    }

    /**
     * @brief Adds a parameter to configuration portal
     * @param p Configuration parameter
     */
    void addWiFiManagerParameter(AsyncWiFiManagerParameter* p) {
        if (wifiManager) {
            wifiManager->addParameter(p);
        }
    }
};

extern EnigmaIOTNodeWifiConfigurerClass EnigmaIOTNodeWifiConfigurer;

#endif  // _ENIGMAIOTNODE_WEB_PORTAL_CONFIGURER_h
