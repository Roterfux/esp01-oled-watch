#include "ESP8266WiFi.h"
#include <Wire.h>
#include <WiFiUdp.h>


char buffer[20];
char* password = "JuV30062013";
char* ssid     = "Fuxbau";
String MyNetworkSSID = "Fuxbau"; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork = false; // Used to flag specific network has been found
bool Fl_NetworkUP = false; // Used to flag network connected and operational.


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
  // print out all system information
  Serial.print("Heap: "); Serial.println(system_get_free_heap_size());
  Serial.print("Boot Vers: "); Serial.println(system_get_boot_version());
  Serial.print("CPU: "); Serial.println(system_get_cpu_freq());
  Serial.println();
  Serial.println("OLED network Acquire Application Started....");
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
  if (!Fl_NetworkUP)
  {
    Serial.println("Starting Process Scanning...");
    sendStrXY("SCANNING ...", 0, 1);
    Scan_Wifi_Networks();
    //Draw_WAVES();
    delay(2000);

    if (Fl_MyNetwork)
    {
      // Yep we have our network lets try and connect to it..
      Serial.println("MyNetwork has been Found... attempting Connection to Network...");
      Do_Connect();

      if (Fl_NetworkUP)
      {
        // Connection success
        Serial.println("Connected OK");
        //Draw_WIFI();
        delay(4000);
        clear_display(); // Clear OLED
        IPAddress ip = WiFi.localIP(); // // Convert IP Here
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        ipStr.toCharArray(buffer, 20);
        sendStrXY("NETWORK DETAILS", 0, 1);
        sendStrXY("NET: ", 2, 1);
        sendStrXY((ssid), 2, 6);
        sendStrXY((buffer), 4, 1); // Print IP to yellow part OLED
      }
      else
      {
        // Connection failure
        Serial.println("Not Connected");
        clear_display(); // Clear OLED
        sendStrXY("CHECK NETWORK", 0, 1);
        sendStrXY("DETAILS", 2, 1);
        sendStrXY("NO CONNECTION...", 6, 1); // YELLOW LINE DISPLAY
        delay(3000);
      }
    }
    else
    {
      // Nope my network not identified in Scan
      Serial.println("Not Connected");
      clear_display(); // Clear OLED
      sendStrXY("MY NETWORK", 0, 1);
      sendStrXY("NOT FOUND IN", 2, 1);
      sendStrXY("SCAN", 4, 1);
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
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord  = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    sendStrXY("Time:", 6, 1);
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    // --- HOURS ---
    printTime(epoch, 1);
    // --- MINUTES ---
    char display_buffer_minutes[4];
    String display_time_minutes = String((epoch  % 3600) / 60);
    display_time_minutes.toCharArray(display_buffer_minutes, 4);
    sendStrXY((":"), 6, 9);
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      Serial.print('0');
      sendStrXY(("0"), 6, 10);
      sendStrXY((display_buffer_minutes), 6, 11);
    }
    else{ sendStrXY((display_buffer_minutes), 6, 10); }
    sendStrXY((":"), 6, 12);
    // --- SECONDS ---
    char display_buffer_seconds[4];
    String display_time_seconds = String(epoch % 60);
    display_time_seconds.toCharArray(display_buffer_seconds, 4);
    Serial.println(epoch % 60); // print the second
    if ( (epoch) < 10 ) {
      Serial.print('0');
      sendStrXY(("0"), 6, 13);
      sendStrXY((display_buffer_seconds), 6, 15);
    }
    else{ sendStrXY((display_buffer_seconds), 6, 13); }
    sendStrXY((":"), 6, 12);
  }
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void printTime(unsigned long epoch, int kind){
  char display_buffer_hours[4];

  
  if (kind == 1){
    // --- HOURS ---
    String display_time_hours = String(((epoch  % 86400L) / 3600) + 2);
    display_time_hours.toCharArray(display_buffer_hours, 4);
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      Serial.print('00');
      sendStrXY(("0"), 6, 7);
      sendStrXY((display_buffer_hours), 6, 8);
    }
    else{ 
      sendStrXY((display_buffer_hours), 6, 7);
    }
    
  } else if (kind == 2){
    
    // Minutes
    
  } else {
    
    // Seconds
    
  }
}

