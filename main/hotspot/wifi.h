/*
 * Captive portal hotspot
 */

#ifndef WIFI_H
#define WIFI_H


/* SSID (network name) to set up the softAP with */
#define WIFI_SSID "bojler eladó"
/* WiFi password (WPA or WPA2) to use for the softAP
 * password not work if:
 *  - less then 8 character
 *  - contains not ASCII characters */
#define WIFI_PASSWORD "ezerocca"
/* Max number of the STA connects to AP */
#define WIFI_MAX_STA_CONN 3
/* this IP is needed for captive portal to work on android */
#define WIFI_IP ESP_IP4TOADDR(192,9,200,1)


void wifi_ini();


#endif /* WIFI_H */
