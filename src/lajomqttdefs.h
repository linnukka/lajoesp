#ifndef LAJOMQTTDEFS
#define LAJOMQTTDEFS


// #define host            "mosquitto"
#define wifissidTo    		"Torstila"
#define wifipassTo    	"TORSTILA"
#define hostname    	"lajo"

#define mqttserverip    "192.168.1.3" 
#define mqttusername	"torstila"
#define mqttpasswd		"K0skenh0v1"
#define mqttenabled		"1"
#define mqttserver		"mosquitto"
#define mqttport		"1883"
#define mqtttimeout		"60"
#define mqttretain		"true"
#define mqttqos			"1"
#define mqttclientname  "lajoesp"
#define mqttreconnectdelay  5000

#define mqttstatustopic     "kuivaamo/lajittelija/ohjaus/status"
#define mqttdebugtopic      "kuivaamo/lajittelija/ohjaus/debug"
#define mqttcommandtopic    "kuivaamo/lajittelija/ohjaus/cmd"

#define serialterminator    "\r"

#endif