#include "ESP8266WiFi.h"
#include <Wire.h>
#include <WiFiUdp.h>

char buffer[20];
char* password         = "...";
char* ssid             = "...";
String MyNetworkSSID   = "..."; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork      = false; // Used to flag specific network has been found
bool Fl_NetworkUP      = false; // Used to flag network connected and operational.

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

extern "C" {
#include "user_interface.h"
}

void setup() {
    Serial.begin(115200);
    delay(2000); // wait for uart to settle and print Espressif blurb..

    //Wire.pins(int sda, int scl), etc
    Wire.pins(0, 2); //on ESP-01.
    Wire.begin();
    StartUp_OLED(); // Init Oled and fire up!
    Serial.println("OLED Init...");
    clear_display();
    sendStrXY("START-UP ...  ", 6, 1);
    delay(3500);
    Serial.println("Setup done");
    clear_display(); // Clear OLED

    udp.begin(localPort);
}

void loop()
{

    if (!Fl_NetworkUP) {
        Serial.println("Starting Process Scanning...");
        sendStrXY("SCANNING ...", 0, 1);
        Scan_Wifi_Networks();
        //Draw_WAVES();
        delay(2000);

        if (Fl_MyNetwork) {
            // Yep we have our network lets try and connect to it..
            Serial.println("MyNetwork has been Found... attempting Connection to Network...");
            Do_Connect();

            if (Fl_NetworkUP) {
                // Connection success
                Serial.println("Connected OK");
                //delay(4000);
                clear_display(); // Clear OLED
                IPAddress ip = WiFi.localIP(); // // Convert IP Here
                String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
                ipStr.toCharArray(buffer, 20);
                sendStrXY("NET: ", 1, 1);
                sendStrXY((ssid), 1, 6);
                sendStrXY((buffer), 3, 1); // Print IP to yellow part OLED
            }
            else {
                // Connection failure
                Serial.println("Not Connected");
                clear_display(); // Clear OLED
                sendStrXY("Not connected!", 4, 1); // YELLOW LINE DISPLAY
                delay(3000);
            }
        }
        else {
            // Nope my network not identified in Scan
            Serial.println("Not Connected");
            clear_display(); // Clear OLED
            sendStrXY("Not connected!", 4, 1);
            delay(3000);
        }
    }
    ntpRequest();
    delay(10000);    // Wait a little before trying again
}

void ntpRequest(){
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);

    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);

    int cb = udp.parsePacket();
    if (!cb) {
        Serial.println("no packet yet");
    }
    else {
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        // the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord  = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        Serial.print("Seconds since Jan 1 1900 = " );
        Serial.println(secsSince1900);

        // now convert NTP time into everyday time
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;

        printTime(epoch, 1);
        printTime(epoch, 2);
        printTime(epoch, 3);
    }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{   // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

void printTime(unsigned long epoch, int kind){
    char display_buffer_hours[4];
    char display_buffer_minutes[4];
    char display_buffer_seconds[4];
    char pos_y = 5;

    Serial.print("UTC time: ");
    sendStrXY("Time:", pos_y, 1);

    if (kind == 1){
        // --- HOURS ---
        String display_time_hours = String(((epoch  % 86400L) / 3600) + 2);
        display_time_hours.toCharArray(display_buffer_hours, 4);
        Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
        Serial.print(':');

        if ( ((epoch  % 86400L) / 3600) < 10 ) {
            Serial.print('00');
            sendStrXY(("0"), pos_y, 7);
            sendStrXY((display_buffer_hours), pos_y, 8);
        }
        else {
            sendStrXY((display_buffer_hours), pos_y, 7);
        }
    } else if (kind == 2){
        // Minutes
        String display_time_minutes = String((epoch  % 3600) / 60);
        display_time_minutes.toCharArray(display_buffer_minutes, 4);
        sendStrXY((":"), pos_y, 9);
        Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
        Serial.print(':');

        if ( (epoch % 60) < 10 ) {
            Serial.print('0');
            sendStrXY(("0"), pos_y, 10);
            sendStrXY((display_buffer_minutes), pos_y, 11);
        }
        else{
            sendStrXY((display_buffer_minutes), pos_y, 10);
        }
        sendStrXY((":"), pos_y, 12);
    } else {
        // Seconds
        String display_time_seconds = String(epoch % 60);
        display_time_seconds.toCharArray(display_buffer_seconds, 4);
        Serial.println(epoch % 60); // print the second
        if ( (epoch) < 10 ) {
            Serial.print('0');
            sendStrXY(("0"), pos_y, 13);
            sendStrXY((display_buffer_seconds), pos_y, 15);
        }
        else {
            sendStrXY((display_buffer_seconds), pos_y, 13);
        }
        sendStrXY((":"), pos_y, 12);
    }
}
// end of file ;)
