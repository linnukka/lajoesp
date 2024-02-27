#ifndef LAJOMQTTDEFS
#define LAJOMQTTDEFS


// #define host            "mosquitto"
#define wifissidTo    		""
#define wifipassTo    	""
#define hostname    	"lajo"

#define mqttserverip    "192.168.1." 
#define mqttusername	""
#define mqttpasswd		""
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