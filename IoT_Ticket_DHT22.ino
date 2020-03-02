/* 
Author: Giang Le
Board: WEMOS D1 R1 ESP8266
Sensor: DHT22
*/

// Libraries
#include <ESP8266WiFi.h> //to connect to AP
#include <WiFiClientSecure.h> //to connect to server
#include <DHTesp.h> // to use DHT22

// WiFi login parameters
const char *ssid = "yourSSID";
const char *password = "yourWiFiPassWord";

// Server login parameters
const char *server = "my.iot-ticket.com";
/* Generate authorization on https://www.base64encode.org/
 * with your username and password in format:
 * username:password */
const char *auth = "yourAuthorization"; 
/* Find fingerprint of the server on 
 * https://www.grc.com/fingerprints.htm */
const char fingerprint[] PROGMEM = "97 5D F5 D6 EF 7B 8A 25 0C 07 8C F5 28 2F 7B F2 01 74 06 17"; 
const int httpsPort = 443; // for http, use 80
const char *readInfoURL = "/api/v1/devices";
const char *newDeviceURL = "/api/v1/devices";
const char *deviceURL = "/api/v1/process/write/yourDeviceID/";

// Device info
char *name = "YourDeviceName";
char *manufacturer = "YourDeviceManufacture";
char *type = "TypeOfYourDevice";
char *enterpriseID = "YourEnterpriseID";
char *description = "YourDescription";
char *appVer = "YourAppVersion";
char *chip = "YourChip";
WiFiClientSecure wifiClient;

// DHT22 sensor setup
DHTesp sensor;
float humidity = 0;
float temperature = 0;

// Board info
const char BOARD_NAME[] = "YourBoardName";

char connect_wifi(const char *id, const char *pw){
	Serial.println("--- --- --- --- --- --- --- --- ---");
	Serial.print(BOARD_NAME);
	Serial.print(" connecting to ");
	Serial.println(ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(id, pw);

	while (WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.println("WiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

char connect_to_server(WiFiClientSecure *client,const char *sv, const char fp[], const int port){
	Serial.println("--- --- --- --- --- --- --- --- ---");
	Serial.print("Connecting to ");
	Serial.println(sv);
	Serial.print("Using fingerprint:");
	Serial.println(fp);
	client->setFingerprint(fp);
	if(!wifiClient.connect(sv, port)){
		Serial.println("Connection Failed.");
		return 0; // failed
	}
	Serial.print("Connected to ");
	Serial.println(sv);
	return 1; //connected
}

char rest_get(WiFiClientSecure *client, const char *url, const char *sv, const char *au ){
	Serial.print("Resquest URL: ");
	Serial.println(url);
	client->print(
		String("GET ") + url + " HTTP/1.1\r\n" +
		"Host: " + sv + "\r\n" + 
		"Authorization: Basic " + au + "\r\n" + 
		"Content-Type: application/json \r\n" + 
		"Connection: close \r\n\r\n"	
	);
	Serial.println("Reques sent. Reading response...");
	while(client->connected()){
		String header = client->readStringUntil('\n');
		if(header == "\r"){
			Serial.print(header);
			break;
		}
	}

	String code = client->readStringUntil('\n');
	Serial.print("Response code: ");
	Serial.println(code);

	String data = client->readStringUntil('\n');
	Serial.println("Response message: ");
	Serial.println(data);
	Serial.println("End of response message.");
}

char rest_post(WiFiClientSecure *client, const char *url, const char *sv, const char *au, String *content){
	Serial.print("Resquest URL: ");
	Serial.println(url);
	client->print(
		String("POST ") + url + " HTTP/1.1\r\n" +
		"Host: " + sv + "\r\n" + 
		"Authorization: Basic " + au + "\r\n" + 
		"Content-Type: application/json \r\n" + 
		"Connection: close \r\n" +
		"Content-Length: " + content->length() + "\r\n\r\n" +
		*content + "\r\n"
	);
	Serial.println("Reques sent. Reading response...");
	while(client->connected()){
		String header = client->readStringUntil('\n');
		if(header == "\r"){
			Serial.print(header);
			break;
		}
	}

	String code = client->readStringUntil('\n');
	Serial.print("Response code: ");
	Serial.println(code);

	String response_msg = client->readStringUntil('\n');
	Serial.println("Response message: ");
	Serial.println(response_msg);
	Serial.println("End of response message.");
}

void get_data(DHTesp *ss, float *hum, float *temp){
	delay(ss->getMinimumSamplingPeriod());
	*hum = ss->getHumidity();
	*temp = ss->getTemperature();
	Serial.print("Humidity: ");
	Serial.print(*hum);
	Serial.print(". Temperature: ");
	Serial.println(*temp);
}

void newID(char *name, char *manufacturer, char *type, char *enterpriseID, char *description, char *appVer, char *chip, String *device_info){
  *device_info = "{\"name\": \"" + String(name) + "\", " + 
  "\"manufacturer\": \"" + String(manufacturer) + "\", " + 
  "\"type\": \"" + String(type) +"\", " + 
  "\"enterpriseId\":\"" + String(enterpriseID) + "\", " + 
  "\"description\": \"" + String(description) + "\", " + 
  "\"attributes\":[{\"key\":\"Application Version\", \"value\": \"" + String(appVer) + "\"}, " + 
  "{\"key\":\"Chip\",\"value\":\"" + String(chip) + "\"}]}";
  Serial.print(*device_info);
}



void setup(){
	Serial.begin(115200);
	connect_wifi(ssid, password);
	sensor.setup(D2, DHTesp::DHT22);
	if(connect_to_server(&wifiClient, server, fingerprint, httpsPort) == 1){
		rest_get(&wifiClient, readInfoURL, server, auth);

	}
  if(connect_to_server(&wifiClient, server, fingerprint, httpsPort) == 1){
    String device_info;
    delay(5000);
    newID(name, manufacturer, type, enterpriseID, description, appVer, chip, &device_info);
    rest_post(&wifiClient, readInfoURL, server, auth, &device_info);
  }


}
void loop(){
	Serial.println("\n--- --- Reading sensor --- ---");
	get_data(&sensor, &humidity, &temperature);
	String content = "[{\"name\": \"Temperature\", \"v\": \"" + String(temperature) + "\"}" +
	",{\"name\": \"Humidity\", \"v\": \"" + String(humidity) + "\"}]";
	if(connect_to_server(&wifiClient, server, fingerprint, httpsPort) == 1){
		rest_post(&wifiClient, deviceURL, server, auth, &content);
	}
	delay(2000);
}
