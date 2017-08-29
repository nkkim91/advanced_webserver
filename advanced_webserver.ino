
/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SimpleDHT.h>


// for DHT22, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT22 = 4;
SimpleDHT22 dht22;

#define SVG_DRAW_MAX_X  (640)
#define SVG_DRAW_MAX_Y  (480)
#define NUM_SVG_TEMP_POINTS (SVG_DRAW_MAX_X/SVG_X_INTERVAL)
#define SVG_X_INTERVAL  (10)

#define SVG_DRAW_TEMP_MIN (10)
#define SVG_DRAW_TEMP_MAX (60)

#define SVG_DRAW_TEMP_NUM_OF_UNIT (6)
#define SCALE_TEXT_LEFT_MARGIN (10)

const char *ssid = "NariNet24";
const char *password = "namnari9";

ESP8266WebServer server ( 80 );

float temperature = 0;
float temperature_old = 0;
float humidity = 0;
float fTemperature[NUM_SVG_TEMP_POINTS];
int nTemperatureIdx = 0;

byte mac[6];
char chostname[32];  /* Nari8226_XXXXXXXX */

const int led = 13;

void handleRoot() {
	digitalWrite ( led, 1 );
	char temp[400];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf ( temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='0'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

		hr, min % 60, sec % 60
	);
	server.send ( 200, "text/html", temp );
	digitalWrite ( led, 0 );
}

void handleNotFound() {
	digitalWrite ( led, 1 );
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", message );
	digitalWrite ( led, 0 );
}

void setup ( void ) {

	pinMode ( led, OUTPUT );
	digitalWrite ( led, 0 );
	Serial.begin ( 115200 );
	WiFi.begin ( ssid, password );
	Serial.println ( "" );

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.println(mac[5],HEX);

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

  sprintf(chostname, "Nari8266_%02x%02x%02x%02x%02x%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  WiFi.hostname(chostname);

	Serial.println ( "" );
#define SVG_DRAW_TEMP_MAX (60)
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	if ( MDNS.begin ( "esp8266" ) ) {
		Serial.println ( "MDNS responder started" );
	}

	server.on ( "/", handleRoot );
	server.on ( "/test.svg", drawGraph );
	server.on ( "/inline", []() {
		server.send ( 200, "text/plain", "this works as well" );
	} );
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );
}

void loop ( void ) {
  int i;
	server.handleClient();

  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT22...");
  
  // read without samples.
  // @remark We use read2 to get a float data, such as 10.1*C
  //    if user doesn't care about the accurate data, use read to get a byte data, such as 10*C.
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
    return;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((float)temperature); Serial.print(" *C, ");
  Serial.print((float)humidity); Serial.println(" RH%");
  fTemperature[nTemperatureIdx++] = temperature;
  nTemperatureIdx %= NUM_SVG_TEMP_POINTS;

#ifdef DEBUG_NK
  for( i = 0; i < NUM_SVG_TEMP_POINTS; i++) {
    Serial.print((float)fTemperature[i]); Serial.print(" ");
  }
  Serial.println("");
#endif  
  delay(2000);
}

void drawGraph() {
	String out = "";
	char temp[100];
	out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"640\" height=\"480\">\n";
 	out += "<rect width=\"640\" height=\"480\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
 	out += "<g stroke=\"black\">\n";

  for(int r = 0, i = 0; r < SVG_DRAW_MAX_Y ; r+= (SVG_DRAW_MAX_Y / SVG_DRAW_TEMP_NUM_OF_UNIT), i+= ((SVG_DRAW_TEMP_MAX - SVG_DRAW_TEMP_MIN) / (SVG_DRAW_TEMP_NUM_OF_UNIT-1)) ) {
    sprintf(temp, "<text x=\"%d\" y=\"%d\" fill=\"red\">%d</text>\n", SCALE_TEXT_LEFT_MARGIN, r + (SVG_DRAW_MAX_Y / SVG_DRAW_TEMP_NUM_OF_UNIT)-1 , SVG_DRAW_TEMP_MAX-i);
    out += temp;
  }
  
  Serial.println("-------------------------------------------------------------");
 	for (int k = nTemperatureIdx, y = (int)fTemperature[k], x = 10, j = 0, l = 0; x < NUM_SVG_TEMP_POINTS * SVG_X_INTERVAL ; x+= SVG_X_INTERVAL) {
    if( k == NUM_SVG_TEMP_POINTS - 1 )
      k = 0;
    else 
      k = k + 1;
    
    int y2 = (int)fTemperature[k];
    int tmp_y, tmp_y2;

    tmp_y = ((y*10 - SVG_DRAW_TEMP_MIN*10) * (SVG_DRAW_MAX_Y / SVG_DRAW_TEMP_NUM_OF_UNIT)) / (((SVG_DRAW_TEMP_MAX - SVG_DRAW_TEMP_MIN) / (SVG_DRAW_TEMP_NUM_OF_UNIT-1)) * 10);
    tmp_y2 = ((y2*10 - SVG_DRAW_TEMP_MIN*10) * (SVG_DRAW_MAX_Y / SVG_DRAW_TEMP_NUM_OF_UNIT)) / (((SVG_DRAW_TEMP_MAX - SVG_DRAW_TEMP_MIN) / (SVG_DRAW_TEMP_NUM_OF_UNIT-1)) * 10);
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 480 - tmp_y, x + SVG_X_INTERVAL, 480 - tmp_y2);
#ifdef DEBUG_NK
    Serial.print("k, y, y2 : "); Serial.print(k); Serial.print(", "); Serial.print(y); Serial.print(", "); Serial.println(y2);
#endif    
 		out += temp;

 		y = y2;
 	}
	out += "</g>\n</svg>\n";
#ifdef DEBUG_NK
  Serial.println("-------------------------------------------------------------");
#endif

	server.send ( 200, "image/svg+xml", out);
}


