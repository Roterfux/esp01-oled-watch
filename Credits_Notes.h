/*
    =============================== OLED Connections =============================
    Connections://Wire.pins(int sda, int scl)  Wire.pins(0, 2); //on ESP-01. 
    ==== ESP8266 Pin====    ==== OLED Pin====
            Vcc  1                Vcc  1
            Rxd  2                
            Rst  3                
           GPIO0 4                SDA  4             use 1.8K Ohm pullup
           CH_PD 5                
           CPIO2 6                SCL  3             " ditto "
             Txd 7                
             Gnd 8                Gnd  2
            
        
/*
/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 *  Comes with Arduino ESP8266 build under examples..

 http://www.esp8266.com/viewtopic.php?f=29&t=3256
 
*/
