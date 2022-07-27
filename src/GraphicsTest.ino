#include "ESP8266WiFi.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFiUdp.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/*
  U8glib Example Overview:
    Frame Buffer Examples: clearBuffer/sendBuffer. Fast, but may not work with
  all Arduino boards because of RAM consumption Page Buffer Examples:
  firstPage/nextPage. Less RAM usage, should work with all Arduino boards. U8x8
  Text Only Example: No RAM usage, direct communication with display controller.
  No graphics, 8x8 Text only.

*/
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/2, /* data=*/0, /* reset=*/U8X8_PIN_NONE);

char buffer[20];
char *password = "password";
char *ssid           = "ssid";
String MyNetworkSSID = "ssid"; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork = false;       // Used to flag specific network has been found
bool Fl_NetworkUP = false; // Used to flag network connected and operational.
unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServerIP;        // time.nist.gov NTP server address
const char *ntpServerName = "0.at.pool.ntp.org"; //"time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing
                                    // packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
int reconnect_interval = 10;
int count              = 0;
unsigned long epoch    = 0;

extern "C" {
#include "user_interface.h"
}

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_u8glib_4_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void drawConvertedText(int x, int y, unsigned long value)
{
    char display_buffer[4];
    String display_text = String(value);
    display_text.toCharArray(display_buffer, 4);
    //sendStr(display_buffer);
    u8g2.drawStr(x, y, display_buffer);
}

void printTime(int kind) {
    u8g2.setFont(u8g2_font_inr21_mr);
    u8g2.setFontPosCenter();
    int font_width = 19;
    int y          = 32;
    int xh         = 5;
    int xm         = 46;
    int xs         = 86;

    if (kind == 1) {
        // Hours
        int calc = (((epoch % 86400L) / 3600) + 2);
        int hours;
        if (calc >= 24) hours = calc - 24; else hours = calc;
        if (hours < 10) {
            u8g2.drawStr(xh, y, "0");
            xh = xh + font_width;
        }
        drawConvertedText(xh, y, hours);
    }
    if (kind == 2) {

        int calc = (epoch % 3600) / 60;
        // Minutes
        if (calc < 10) {
            u8g2.drawStr(xm, y, "0");
            xm = xm + font_width;
        }
        drawConvertedText(xm, y, calc);
    }
    if (kind == 3) {
        // Seconds

        int calc = epoch % 60;
        if (calc < 10) {
            u8g2.drawStr(xs, y, "0");
            xs = xs + font_width;
        }
        drawConvertedText(xs, y, calc);
    }
}

void draw(void) {
  u8g2_prepare();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawFrame(2, 2, 124, 60);

  //u8g2.setFont(u8g2_font_u8glib_4_tf);
  //u8g2.drawStr(1, 1, "Time");

  printTime(1);
  printTime(2);
  printTime(3);

}

void setup(void) {
  u8g2.begin();

  Wire.pins(0, 2); // on ESP-01.
  Wire.begin();
  udp.begin(localPort);
  //Scan_Wifi_Networks();
  //Do_Connect();
  //epoch = ntpRequest();
}

void loop(void) {
  if (!Fl_NetworkUP) {
    Scan_Wifi_Networks();
    if (Fl_MyNetwork) {
      Do_Connect();
      //u8g2.clearBuffer();
      if (Fl_NetworkUP) {
      } else {
        u8g2.drawStr(40, 0, "Not connected!");
      }
      u8g2.sendBuffer();
    } else {
      u8g2.drawStr(40, 0, "Not connected!");
    }
  }

  if (count > reconnect_interval) {
    unsigned long time_update = ntpRequest();
    if (time_update > 0) {
      reconnect_interval = 30;
      epoch = time_update;
      u8g2.setFont(u8g2_font_u8glib_4_tf);
      u8g2.drawStr(1, 1, "Time?");
    } else {
      epoch = epoch + 6;
      // try more often
      reconnect_interval = 300;
      u8g2.setFont(u8g2_font_u8glib_4_tf);
      u8g2.drawStr(1, 1, "Time!");
    }
    //WiFi.disconnect();
    count = 0;
  } else {
    epoch++;
    count++;
  }

  // picture loop
  //u8g2.firstPage();
  u8g2.clearBuffer();
  draw();
  //u8g2.nextPage();
  u8g2.sendBuffer();
  // deley between each page
  delay(950);
}

unsigned long ntpRequest() {
  // sendStrXY("Request Time...", 0, 0);
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(3000);
  // clear_display(); // Clear OLED
  int cb = udp.parsePacket();
  if (!cb) {
    // sendStrXY("No Wifi connect.", 0, 0);
    return 0;
  } else {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
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
