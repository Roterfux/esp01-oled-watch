#include "ESP8266WiFi.h"
#include <WiFiUdp.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>

char buffer[20];
char *password         = "JuV30062013";
char *ssid             = "Fuxbau";
String MyNetworkSSID   = "Fuxbau"; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork      = false;       // Used to flag specific network has been found
bool Fl_NetworkUP      = false; // Used to flag network connected and operational.
unsigned int localPort = 2390; // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
// IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
//pool.ntp.org
const char *ntpServerName = "0.at.pool.ntp.org"; //"time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing
                                    // packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
int reconnect_interval  = 30;
int count           = 0;
unsigned long epoch = 0;

extern "C" {
    #include "user_interface.h"
}


void setup() {
    Serial.begin(115200);
    delay(2500); // wait for uart to settle and print Espressif blurb..
    // Wire.pins(int sda, int scl), etc
    Wire.pins(0, 2); // on ESP-01.
    Wire.begin();
    StartUp_OLED(); // Init Oled and fire up!
    clear_display();
    setXY(6, 1);
    sendStr("Start up!");
    udp.begin(localPort);
    Scan_Wifi_Networks();
    Do_Connect();
    epoch = ntpRequest();
    clear_display();
}

void loop() {
  if (!Fl_NetworkUP) {
    Scan_Wifi_Networks();
    if (Fl_MyNetwork) {
        Do_Connect();
        if (Fl_NetworkUP) { }
        else{
            // Connection failure
            clear_display();                   // Clear OLED
            sendStrXY("Not connected!", 4, 1); // YELLOW LINE DISPLAY
        }
    }
    else {
        // Nope my network not identified in Scan
        clear_display(); // Clear OLED
        sendStrXY("Not connected!", 4, 1);
    }
  }

  if (count > reconnect_interval) {
        unsigned long time_update = ntpRequest();
        if(time_update > 0) {
            reconnect_interval = 30;
            epoch = time_update;
            sendStrXY("?", 0, 0);
        } else {
            epoch = epoch + 6;
            // try more often
            reconnect_interval = 300;
            sendStrXY("!", 0, 0);
        }
    count = 0;
    }
    else {
        epoch++;
        count++;
    }
    printTime(1);
    printTime(2);
    printTime(3);
    delay(1050); // Wait a little before trying again
}

unsigned long ntpRequest() {
    sendStrXY("Request Time...", 0, 0);
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(2500);
    clear_display(); // Clear OLED
    int cb = udp.parsePacket();
    if (!cb) {
        sendStrXY("No Wifi connect.", 0, 0);
        return 0;
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
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        epoch = secsSince1900 - seventyYears;
    }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress &address) {
    Serial.println("sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); // NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

void printTime(int kind) {
    char pos_y = 4;

    if (kind == 1) {
        // Hours
        int calc = (((epoch % 86400L) / 3600) + 2);
        int hours;
        if (calc >= 24) hours = calc - 24; else hours = calc;

        setXY(pos_y, 4);
        if (hours < 10)
            sendStr("0");
        drawConvertedText(hours);
        sendStr(":");
    }
    if (kind == 2) {
        int calc = (epoch % 3600) / 60;
        // Minutes
        setXY(pos_y, 7);
        if (calc < 10)
            sendStr("0");
        drawConvertedText(calc);
        sendStr(":");
    }
    if (kind == 3) {
        // Seconds
        int calc = epoch % 60;
        setXY(pos_y, 10);
        if (calc < 10)
            sendStr("0");
        drawConvertedText(calc);
    }
    setXY(0, 0);
}

void drawConvertedText(unsigned long value)
{
    char display_buffer[4];
    String display_text = String(value);
    display_text.toCharArray(display_buffer, 4);
    sendStr(display_buffer);
}
