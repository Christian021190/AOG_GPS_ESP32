//ESP32 programm for UBLOX receivers to send NMEA to AgOpenGPS or other program 

byte vers_nr = 49;
char VersionTXT[120] = " - 15. Februar 2021 by MTZ8302<br>(Ethernet support, NTRIP client for ESP32, multiple WiFi networks)";

//works with 1 or 2 receivers

//1 receiver to send position from UBXPVT message to ESP32
//2 receivers to get position, roll and heading from UBXPVT + UBXRelPosNED via UART to ESP32

//ESP sending $PAOGI or $GGA+VTG+HDT sentence via UDP to IP x.x.x.255 at port 9999 or via USB

//AgOpenGPS sending NTRIP via UDP to port 2233(or USB) -> ESP sends it to UBLOX via UART

//filters roll, heading and on weak GPS signal, position with filter parameters changing dynamic on GPS signal quality

//by Matthias Hammer (MTZ8302) 10.Jan 2021, supported by Franz Husch (Jeep1945), WEder (coffeetrac), NTRIP client by GLAY-AK2 (GitHub)

//change stettings to your need. Afterwards you can change them via webinterface x.x.x.79 (192.168.1.79)
//if connection to your network fails an accesspoint is opened: webinterface 192.168.1.1

//use serial monitor at USB port, to get sebug messages and IP for webinterface at ESP start.

//for easier setup:
//use webinterface, turn debugmodeUBX on and change GPIO pin until you get data from the UBlox receivers on USB serial monitor

//the settings below are written as defalt values and can be reloaded.
//So if changing settings set EEPROM_clear = true; (line ~109) - flash - boot - reset to EEPROM_clear = false - flash again to keep them as defauls





#define HardwarePlatform 0      //0 = runs on ESP32, 1 = runs on Arduino Mega

struct set {
    //connection plan:
    // ESP32--- Right F9P GPS pos --- Left F9P Heading-----Sentences
    //  RX1-27-------TX1--------------------------------UBX-Nav-PVT out   (=position+speed)
    //  TX1-16-------RX1--------------------------------RTCM in           (NTRIP comming from AOG to get absolute/correct postion
    //  RX2-25-----------------------------TX1----------UBX-RelPosNED out (=position relative to other Antenna)
    //  TX2-17-----------------------------RX1----------
    //               TX2-------------------RX2----------RTCM 1077+1087+1097+1127+1230+4072.0+4072.1 (activate in right F9P = NTRIP for relative positioning)

    // IO pins ESP32 side ----------------------------------------------------------------------------
    byte RX1 = 27;                    //right F9P TX1 GPS pos
    byte TX1 = 16;                    //right F9P RX1 GPS pos

    byte RX2 = 25;                    //left F9P TX1 Heading
    byte TX2 = 17;                    //left F9P RX1 Heading

    byte Eth_CS_PIN = 5;              //CS PIN with SPI Ethernet hardware  SPI config: MOSI 23 / MISO 19 / CLK18 / CS5

    byte Button_WiFi_rescan_PIN = 4;  //Button to rescan/reconnect WiFi networks / push to GND

    byte LEDWiFi_PIN = 2;      // WiFi Status LED 0 = off
    byte LEDWiFi_ON_Level = HIGH;    //HIGH = LED on high, LOW = LED on low

    bool CMPS14_present = true;
    float CMPS14HeadingCorrection = 0.0;
    float CMPS14RollCorrection = 0.0;
    int CMPS14_ADDRESS = 0x60;   // Address of CMPS14 shifted right one bit for arduino wire library

    //Network---------------------------------------------------------------------------------------------
#if HardwarePlatform == 0
    //tractors WiFi or mobile hotspots
    char ssid1[24] = "Fendt_209V";           // WiFi network Client name
    char password1[24] = "";                 // WiFi network password//Accesspoint name and password
    char ssid2[24] = "Matthias Cat S62 Pro";// WiFi network Client name
    char password2[24] = "";                 // WiFi network password//Accesspoint name and password
    char ssid3[24] = "";           // WiFi network Client name
    char password3[24] = "";                 // WiFi network password//Accesspoint name and password
    char ssid4[24] = "WLANHammer";       // WiFi network Client name
    char password4[24] = "";                 // WiFi network password//Accesspoint name and password
    char ssid5[24] = "WLAN-TelekomMiniHotspot";    // WiFi network Client name
    char password5[24] = "";                 // WiFi network password//Accesspoint name and password

    char ssid_ap[24] = "GPS_unit_F9P_Net";  // name of Access point, if no WiFi found, NO password!!
    int timeoutRouter = 10;                //time (s) to search for existing WiFi, than starting Accesspoint 

    byte timeoutWebIO = 255;                 //time (min) afterwards webinterface is switched off

    // Ntrip Caster Data
    char NtripHost[40] = "www.sapos-bw-ntrip.de";//  "80.154.101.74";    // Server IP or URL
    int  NtripPort = 2101;                // Server Port
    char NtripMountpoint[40] = "SAPOS-LW-MSM";   // Mountpoint
    char NtripUser[40] = "";     // Username
    char NtripPassword[40] = "";    // Password

    byte NtripSendWhichGGASentence = 2; // 0 = No Sentence will be sended / 1 = send fixed GGA Sentence from belowb/ 2 = GGA from GPS position will be sended

    char NtripFixGGASentence[100] = "$GPGGA,051353.171,4751.637,N,01224.003,E,1,12,1.0,0.0,M,0.0,M,,*6B"; //hc create via www.nmeagen.org

    byte NtripGGASendRate = 10;         // time in seconds between GGA Packets


    //WiFi
    byte WiFi_myip[4] = { 192, 168, 2, 79 };     // Roofcontrol module 
    byte WiFi_gwip[4] = { 192, 168, 1, 1 };      // Gateway IP only used if Accesspoint created
    byte WiFi_ipDest_ending = 255;//ending of IP address to send UDP data to
    byte mask[4] = { 255, 255, 255, 0 };
    byte myDNS[4] = { 8, 8, 8, 8 };         //optional

    //Ethernet
    byte Eth_myip[4] = { 192, 168, 1, 80 };     // Roofcontrol module 
    byte Eth_ipDest_ending = 255;//ending of IP address to send UDP data to
    byte Eth_mac[6] = { 0x90,0xA2,0xDA,0x10,0xB3,0x1B };
    bool Eth_static_IP = false;					// false = use DHPC and set last number to 80 (x.x.x.80) / true = use IP as set above

    unsigned int PortGPSToAOG = 5544;             //this is port of this module: Autosteer = 5577 IMU = 5566 GPS = 
    unsigned int PortFromAOG = 8888;            //port to listen for AOG
    unsigned int AOGNtripPort = 2233;       //port NTRIP data from AOG comes in
    unsigned int PortDestination = 9999;    //Port of AOG that listens
#endif

    //Antennas position
    double AntDist = 74.0;                //cm distance between Antennas
    double AntHight = 228.0;              //cm hight of Antenna
    double virtAntLeft = 42.0;           //cm to move virtual Antenna to the left (was renamed, keep your settings, name of direction was wrong)
    double virtAntForew = 0;// 60.0;            //cm to move virtual Antenna foreward
    double headingAngleCorrection = 90;

    double AntDistDeviationFactor = 1.2;  // factor (>1), of whom lenght vector from both GPS units can max differ from AntDist before stop heading calc
    byte checkUBXFlags = 1;               //UBX sending quality flags, when used with RTK sometimes 
    byte filterGPSposOnWeakSignal = 1;    //filter GPS Position on weak GPS signal

    byte GPSPosCorrByRoll = 1;            // 0 = off, 1 = correction of position by roll (AntHight must be > 0)
    double rollAngleCorrection = 0.0;

    byte MaxHeadChangPerSec = 30;         // degrees that heading is allowed to change per second

    byte DataTransVia =  7;                //transfer data via 0 = USB / 7 = WiFi UDP / 8 = WiFi UDP 2x / 10 = Ethernet UDP

    byte NtripClientBy = 1;               //NTRIP client 0:off /1: listens for AOG NTRIP to UDP (WiFi/Ethernet) or USB serial /2: use ESP32 WiFi NTIRP client

    byte sendOGI = 1;                     //1: send NMEA message 0: off
    byte sendVTG = 0;                     //1: send NMEA message 0: off
    byte sendGGA = 0;                     //1: send NMEA message 0: off
    byte sendHDT = 0;                     //1: send NMEA message 0: off


    bool debugmode = false;
    bool debugmodeUBX = false;
    bool debugmodeHeading = false;
    bool debugmodeVirtAnt = false;
    bool debugmodeFilterPos = false;
    bool debugmodeNTRIP = false;
    bool debugmodeRAW = false;

}; set Set;


bool EEPROM_clear = false;  //set to true when changing settings to write them as default values: true -> flash -> boot -> false -> flash again


// WiFistatus LED 
// blink times: searching WIFI: blinking 4x faster; connected: blinking as times set; data available: light on; no data for 2 seconds: blinking
unsigned int LED_WIFI_time = 0;
unsigned int LED_WIFI_pulse = 2000;   //light on in ms 
unsigned int LED_WIFI_pause = 1500;   //light off in ms
boolean LED_WIFI_ON = false;

//WIFI+Ethernet
unsigned long NtripDataTime = 0, now = 0, WebIOTimeOut = 0, WiFi_connect_timer = 0, WiFi_network_search_timeout = 0;//WiFi_lost_time = 0,
byte Eth_connect_step, WiFi_connect_step = 10, WiFi_STA_connect_call_nr = 1, WiFi_netw_nr = 0, my_WiFi_Mode = 0;   // WIFI_STA = 1 = Workstation  WIFI_AP = 2  = Accesspoint
IPAddress WiFi_ipDestination, Eth_ipDestination; //set in network.ino
bool  WiFi_STA_running = false, task_Eth_NTRIP_running = false, WebIORunning = true, WiFi_UDP_running = false;
char Eth_NTRIP_packetBuffer[512];// buffer for receiving and sending data

/*//Kalman filter roll
double rollK, rollPc, rollG, rollXp, rollZp, rollXe;
double rollP = 1.0;
double rollVar = 0.1; // variance, smaller: faster, less filtering
double rollVarProcess = 0.3;// 0.0005;// 0.0003;  bigger: faster, less filtering// replaced by fast/slow depending on GPS quality
//Kalman filter heading
double headK, headPc, headG, headXp, headZp, headXe;
double headP = 1.0;
double headVar = 0.1; // variance, smaller, more faster filtering
double headVarProcess = 0.1;// 0.001;//  bigger: faster, less filtering// replaced by fast/slow depending on GPS quality*/
//Kalman filter heading
double headVTGK, headVTGPc, headVTGG, headVTGXp, headVTGZp, headVTGXe;
double headVTGP = 1.0;
double headVTGVar = 0.1; // variance, smaller, more faster filtering
double headVTGVarProcess = 0.01;// 0.001;//  bigger: faster, less filtering// replaced by fast/slow depending on GPS quality
/*//Kalman filter heading
double headMixK, headMixPc, headMixG, headMixXp, headMixZp, headMixXe;
double headMixP = 1.0;
double headMixVar = 0.1; // variance, smaller, more faster filtering
double headMixVarProcess = 0.1;// 0.001;//  bigger: faster, less filtering// replaced by fast/slow depending on GPS quality*/
//Kalman filter lat
double latK, latPc, latG, latXp, latZp, latXe;
double latP = 1.0;
double latVar = 0.1; // variance, smaller, more faster filtering
double latVarProcess = 0.3;//  replaced by fast/slow depending on GPS qaulity
//Kalman filter lon
double lonK, lonPc, lonG, lonXp, lonZp, lonXe;
double lonP = 1.0;
double lonVar = 0.1; // variance, smaller, more faster filtering
double lonVarProcess = 0.3;// replaced by fast/slow depending on GPS quality

double VarProcessVeryFast = 0.2;//0,3  used, when GPS signal is weak, no roll, but heading OK
double VarProcessFast = 0.08;//0,15  used, when GPS signal is weak, no roll, but heading OK
double VarProcessMedi = 0.02;//0,08 used, when GPS signal is  weak, no roll no heading
double VarProcessSlow = 0.001;//  0,004used, when GPS signal is  weak, no roll no heading
double VarProcessVerySlow = 0.0001;//0,03  used, when GPS signal is  weak, no roll no heading
bool filterGPSpos = false;

double HeadingQuotaVTG = 0.5, HeadingQuotaCMPS = 0.5;;

#if HardwarePlatform == 0


//NTRIP
unsigned long lifesign, NTRIP_GGA_send_lastTime; 
int cnt;
byte Ntrip_restart = 1; //set 1 to start NTRIP client the first time
bool task_NTRIP_Client_running = false;
String _userAgent = "NTRIP ESP32NTRIPClient";
String _base64Authorization;
String _accept = "*/*";
char RTCM_strm_Buffer[512];         // rtcm Message Buffer


#endif

//UBX
byte UBXRingCount1 = 0, UBXRingCount2 = 0, UBXDigit1 = 0, UBXDigit2 = 0, OGIfromUBX = 0, CMPSRingCount = 255;
short UBXLenght1 = 100, UBXLenght2 = 100;
constexpr unsigned char UBX_HEADER[] = { 0xB5, 0x62 };//all UBX start with this
bool isUBXPVT1 = false,  isUBXRelPosNED = false, existsUBXRelPosNED = false;
unsigned long UBXPVTTime = 0;//debug only

double virtLat = 0.0, virtLon = 0.0;//virtual Antenna Position

//NMEA
byte OGIBuffer[90], HDTBuffer[20], VTGBuffer[50], GGABuffer[80];
bool newOGI = false, newHDT = false, newGGA = false, newVTG = false;
byte OGIdigit = 0, GGAdigit = 0, VTGdigit = 0, HDTdigit = 0;

// ai, 07.10.2020: use the GGA Message to determine Fix-Quality
bool bNMEAstarted = false, bGGAexists = false;
String sNMEA;
int i, iPos;
char cFixQualGGA;
// END ai, 07.10.2020: use the GGA Message to determine Fix-Quality


//heading + roll
double HeadingRelPosNED = 0, cosHeadRelPosNED = 1, HeadingVTG = 0, HeadingVTGOld = 0, cosHeadVTG = 1, HeadingMix = 0, cosHeadMix = 1, HeadingCMPS = 0;
double HeadingDiff = 0, HeadingMax = 0, HeadingMin = 0, HeadingMixBak = 0, HeadingQualFactor = 0.5;
byte noRollCount = 0,  drivDirect = 0;
constexpr double PI180 = PI / 180;
bool dualGPSHeadingPresent = false, rollPresent = false, virtAntPosPresent = false, add360ToRelPosNED = false, add360ToVTG = false;
double rollRelPosNED = 0.0, rollToAOG = 0.0;
byte dualAntNoValueCount = 0, dualAntNoValueMax = 20;// if dual Ant value not valid for xx times, send position without correction/heading/roll

//CMPS14
bool CMPS14GetNewData = true;
double CMPS14CorrFactor = 0.0; 


// Variables ------------------------------
struct NAV_PVT {
    unsigned char cls;
    unsigned char id;
    unsigned short len;
    unsigned long iTOW;  //GPS time ms
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char min;
    unsigned char sec;
    unsigned char valid;
    unsigned long tAcc;
    long nano;
    unsigned char fixType;//0 no fix....
    unsigned char flags;
    unsigned char flags2;
    unsigned char numSV; //number of sats
    long lon;   //deg * 10^-7
    long lat;   //deg * 10^-7
    long height;
    long hMSL;  //heigt above mean sea level mm
    unsigned long hAcc;
    unsigned long vAcc;
    long velN;
    long velE;
    long velD;
    long gSpeed; //Ground Speed mm/s
    long headMot;
    unsigned long sAcc;
    unsigned long headAcc;
    unsigned short pDOP;
    unsigned char flags3;
    unsigned char reserved1;
    long headVeh;
    long magDec;//doesnt fit, checksum was 4 bytes later, so changes from short to long
    unsigned long magAcc;
    unsigned char CK0;
    unsigned char CK1;
};
constexpr byte sizeOfUBXArray = 3;
NAV_PVT UBXPVT1[sizeOfUBXArray];


struct NAV_RELPOSNED {
    unsigned char cls;
    unsigned char id;
    unsigned short len;
    unsigned char ver;
    unsigned char res1;
    unsigned short refStID;
    unsigned long iTOW;
    long relPosN;
    long relPosE;
    long relPosD;
    long relPosLength;
    long relPosHeading;
    long res2;//    unsigned char res2;
    char relPosHPN;
    char relPosHPE;
    char relPosHPD;
    char relPosHPLength;
    unsigned long accN;
    unsigned long accE;
    unsigned long accD;
    unsigned long accLength;
    unsigned long accHeading;
    long res3; // unsigned char res3;
    unsigned long flags;
    unsigned char CK0;
    unsigned char CK1;
};
NAV_RELPOSNED UBXRelPosNED[sizeOfUBXArray];

struct CMPS14DataStruc
{
    float heading;
    float roll;
    unsigned long time;
};
CMPS14DataStruc CMPS14Data[sizeOfUBXArray];


#if HardwarePlatform == 0
#include <AsyncUDP.h>
//#include <HTTP_Method.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
//#include <EthernetWebServer.h>
//#include <EthernetHttpClient.h>
#include <base64.h>
#include "zAOG_ESP32Ping.h"
#include "zAOG_ping.h"

//instances----------------------------------------------------------------------------------------
AsyncUDP WiFi_udpRoof;
AsyncUDP WiFi_udpNtrip;
EthernetUDP Eth_udpRoof;
EthernetUDP Eth_udpNtrip;
WebServer WiFi_Server(80);
//EthernetWebServer Eth_Server(80);
WiFiClient WiFi_Ntrip_cl;


TaskHandle_t taskHandle_WiFi_NTRIP;
TaskHandle_t taskHandle_Eth_connect;
TaskHandle_t taskHandle_Eth_NTRIP;

#endif
TaskHandle_t taskHandle_CMPS14;




#include <Wire.h>

// SETUP ------------------------------------------------------------------------------------------

void setup()
{
    delay(200);//waiting for stable power
    delay(50);//

    //start serials
    Serial.begin(115200);
    delay(50);
#if HardwarePlatform == 0
    Serial1.begin(115200, SERIAL_8N1, Set.RX1, Set.TX1);
    delay(10);
    Serial2.begin(115200, SERIAL_8N1, Set.RX2, Set.TX2);
    delay(10);
#endif
#if HardwarePlatform == 1
    Serial1.begin(115200);
    delay(10);
    Serial2.begin(115200);
    delay(10);
#endif
    Serial.println();//new line

    if (Set.LEDWiFi_PIN != 255) { pinMode(Set.LEDWiFi_PIN, OUTPUT); }
    pinMode(Set.Button_WiFi_rescan_PIN, INPUT_PULLUP);

#if HardwarePlatform == 1
    Set.DataTransVia = 0;//set data via USB
#endif


#if HardwarePlatform == 0
    restoreEEprom();
    delay(60);

    //start Ethernet
    if (Set.DataTransVia == 10) {
        Eth_connect_step = 10;
        xTaskCreatePinnedToCore(Eth_handle_connection, "Core1EthConnectHandle", 3072, NULL, 1, &taskHandle_Eth_connect, 1);
        delay(500);
    }
    else { Eth_connect_step = 255; }
        
    //start WiFi
    WiFi_connect_step = 10;
    WebIORunning = false;
    WiFi_UDP_running = false;
    WiFi_handle_connection();

#endif

    if (Set.CMPS14_present) {
        if (Wire.begin()) {
            CMPSRingCount = 0;
            delay(500);
            xTaskCreate(CMPS14_handle, "CMPS14_handling", 3072, NULL, 1, &taskHandle_CMPS14);
            delay(500);
        }
        else {
            Serial.println("error INIT CMPS14");
            Serial.println("CMPS14 set inactive");
            Set.CMPS14_present = false;
        }
    }

}

// MAIN loop  -------------------------------------------------------------------------------------------

void loop()
{
    getUBX();//read serials    

    if (UBXRingCount1 != OGIfromUBX)//new UXB exists
    {//Serial.println("new UBX to process");
        headingRollCalc();
        if (existsUBXRelPosNED) {
            //virtual Antenna point?
            if ((Set.virtAntForew != 0) || (Set.virtAntLeft != 0) ||
                ((Set.GPSPosCorrByRoll == 1) && (Set.AntHight > 0)))
            {//all data there
                virtualAntennaPoint();
            }
        }
        else //only 1 Antenna
        {
            virtAntPosPresent = false;
            if ((Set.debugmodeHeading) || (Set.debugmodeVirtAnt)) { Serial.println("no dual Antenna values so no virtual Antenna point calc"); }
        }

        //filter position: set kalman variables
        //0: no fix 1: GPS only -> filter slow, else filter fast, but filter due to no roll compensation
        if (UBXPVT1[UBXRingCount1].fixType <= 1) { latVarProcess = VarProcessSlow; lonVarProcess = VarProcessSlow; filterGPSpos = true; }
        else { if (!dualGPSHeadingPresent) { latVarProcess = VarProcessFast; lonVarProcess = VarProcessFast; filterGPSpos = true; } }
        //filterGPSPosition might set false and Kalman variables set, if signal is perfect (in void HeadingRollCalc)

        if (Set.filterGPSposOnWeakSignal == 0) { filterGPSpos = false; }

        filterPosition();//runs allways to fill kalman variables

        if (Set.sendGGA) { buildGGA(); }
        if (Set.sendVTG) { buildVTG(); }
        if (Set.sendHDT) { buildHDT(); }
        buildOGI();//should be build anyway, to decide if new data came in

    }

    //transfer data via 0 = USB
    if (Set.DataTransVia < 5) {//use USB
        if (Set.NtripClientBy == 1) { doSerialNTRIP(); } //gets USB NTRIP and sends to serial 1  
       //send USB
        if ((newOGI) && (Set.sendOGI == 1)) {
            for (byte n = 0; n < (OGIdigit - 1); n++) { Serial.write(OGIBuffer[n]); }
            Serial.println();
            newOGI = false;
        }
        if (newVTG) {
            for (byte n = 0; n < (VTGdigit - 1); n++) { Serial.write(VTGBuffer[n]); }
            Serial.println();
            newVTG = false;
        }
        if (newHDT) {
            for (byte n = 0; n < (HDTdigit - 1); n++) { Serial.write(HDTBuffer[n]); }
            Serial.println();
            newHDT = false;
        }
        if (newGGA) {
            for (byte n = 0; n < (GGAdigit - 1); n++) { Serial.write(GGABuffer[n]); }
            Serial.println();
            newGGA = false;
        }
    }

#if HardwarePlatform == 0
    else {
        //use WiFi?
        if (Set.DataTransVia < 10) {
            if (WiFi_connect_step == 0) {
                //send WiFi UDP // WiFi UDP NTRIP via AsyncUDP: only called once, works with .onPacket
                if ((newOGI) && (Set.sendOGI == 1)) {
                    WiFi_udpRoof.writeTo(OGIBuffer, OGIdigit, WiFi_ipDestination, Set.PortDestination);
                    if (Set.DataTransVia == 8) { delay(5); WiFi_udpRoof.writeTo(OGIBuffer, OGIdigit, WiFi_ipDestination, Set.PortDestination); }
                    if (Set.debugmodeRAW) {
                        Serial.print("millis,"); Serial.print(millis()); Serial.print(",");
                        Serial.print("UBXRingCount1 OGIfromUBX PAOGI,");
                        Serial.print(UBXRingCount1); Serial.print(",");
                        Serial.print(OGIfromUBX); Serial.print(",");
                        Serial.print("DualGPSPres RollPres VirtAntPres DrivDir FilterPos,");
                        Serial.print(dualGPSHeadingPresent); Serial.print(",");
                        Serial.print(rollPresent); Serial.print(",");
                        Serial.print(virtAntPosPresent); Serial.print(",");
                        Serial.print(drivDirect); Serial.print(",");
                        Serial.print(filterGPSpos); Serial.print(",");
                        Serial.print("PVThead RelPosNEDhead,");
                        Serial.print(UBXPVT1[UBXRingCount1].headMot); Serial.print(",");
                        Serial.print(UBXRelPosNED[UBXRingCount2].relPosHeading); Serial.print(",");
                        Serial.print("PVTlat PVTlon,");
                        Serial.print(UBXPVT1[UBXRingCount1].lat); Serial.print(",");
                        Serial.print(UBXPVT1[UBXRingCount1].lon); Serial.print(",");
                        for (byte N = 0; N < OGIdigit; N++) { Serial.write(OGIBuffer[N]); }
                    }
                    newOGI = false;
                }
                if (newGGA) {
                    WiFi_udpRoof.writeTo(GGABuffer, GGAdigit, WiFi_ipDestination, Set.PortDestination);
                    if (Set.DataTransVia == 8) { delay(5); WiFi_udpRoof.writeTo(OGIBuffer, OGIdigit, WiFi_ipDestination, Set.PortDestination); }
                    newGGA = false;
                }
                if (newVTG) {
                    WiFi_udpRoof.writeTo(VTGBuffer, VTGdigit, WiFi_ipDestination, Set.PortDestination);
                    if (Set.DataTransVia == 8) { delay(5); WiFi_udpRoof.writeTo(OGIBuffer, OGIdigit, WiFi_ipDestination, Set.PortDestination); }
                    newVTG = false;
                }
                if (newHDT) {
                    WiFi_udpRoof.writeTo(HDTBuffer, HDTdigit, WiFi_ipDestination, Set.PortDestination);
                    if (Set.DataTransVia == 8) { delay(5); WiFi_udpRoof.writeTo(OGIBuffer, OGIdigit, WiFi_ipDestination, Set.PortDestination); }
                    newHDT = false;
                }
            }
        }
        else {
            //use Ethernet DataTransVia >= 10
            if (Eth_connect_step == 0) {
                if ((Set.NtripClientBy == 1) && (!task_Eth_NTRIP_running)) {
                    xTaskCreatePinnedToCore(Eth_NTRIP_Code, "Core1", 3072, NULL, 1, &taskHandle_Eth_NTRIP, 1);
                    delay(500);
                } //gets Ethernet UDP NTRIP and sends to serial 1 
                if ((newOGI) && (Set.sendOGI == 1)) {
                    Eth_udpRoof.beginPacket(Eth_ipDestination, Set.PortDestination);
                    Eth_udpRoof.write(OGIBuffer,OGIdigit);
                    Eth_udpRoof.endPacket();
                    newOGI = false;
                }
                if (newGGA) {
                    Eth_udpRoof.beginPacket(Eth_ipDestination, Set.PortDestination);
                    Eth_udpRoof.write(GGABuffer, GGAdigit);
                    Eth_udpRoof.endPacket();
                    newGGA = false;
                }
                if (newVTG) {
                    Eth_udpRoof.beginPacket(Eth_ipDestination, Set.PortDestination);
                    Eth_udpRoof.write(VTGBuffer, VTGdigit);
                    Eth_udpRoof.endPacket();
                    newVTG = false;
                }
                if (newHDT) {
                    Eth_udpRoof.beginPacket(Eth_ipDestination, Set.PortDestination);
                    Eth_udpRoof.write(HDTBuffer, HDTdigit);
                    Eth_udpRoof.endPacket();
                    newHDT = false;
                }
            }
        }//else WiFi
    }//else USB

    //WiFi rescann button pressed?
    if (!digitalRead(Set.Button_WiFi_rescan_PIN)) {
        WiFi_connect_timer = 0;
        WiFi_STA_connect_call_nr = 0;//to enable DHCP for WiFi
        WiFi_connect_step = 3;
        my_WiFi_Mode = 0;//set 0 to end Ntrip task
        WiFi_UDP_running = false;
    }

    //search for WiFi networks and connect
    if (WiFi_connect_step > 0) { WiFi_handle_connection(); WiFi_LED_blink(2); }

    now = millis();

    if (WebIORunning) {
        WiFi_Server.handleClient(); //does the Webinterface
        if ((now > WebIOTimeOut) && (Set.timeoutWebIO != 255)) {
            WebIORunning = false;
            WiFi_Server.close();
            delay(100);
            if ((Set.DataTransVia == 0) || (Set.DataTransVia == 10)) { WiFi.mode(WIFI_OFF); WiFi_connect_step = 255; }
            if (Set.debugmode) { Serial.println("switching off Webinterface"); }
        }
    }

    if (Set.NtripClientBy > 0) {
        if (now > (NtripDataTime + 30000))
        {
            NtripDataTime = millis();
            if (Set.NtripClientBy == 2) { Serial.println("no NTRIP data from ESP32 NTRIP client for more than 30s"); }
            else {
                Serial.print("no NTRIP for more than 30s from ");
                if (Set.DataTransVia < 5) { Serial.println("USB"); }
                else
                    if (Set.DataTransVia < 10) { Serial.println("WiFi"); }
                    else
                        if (Set.DataTransVia >= 10) { Serial.println("Ethernet"); }
            }
        }
    }
#endif

}




//-------------------------------------------------------------------------------------------------

void WiFi_LED_blink(byte blkSpeed)   //8. Maerz 2020
{
    unsigned long now = millis();
    if (!LED_WIFI_ON) {
        if (now > (LED_WIFI_time + (LED_WIFI_pause >> blkSpeed))) {
            LED_WIFI_time = now;
            LED_WIFI_ON = true;
#if useLED_BUILTIN
            digitalWrite(LED_BUILTIN, HIGH);
#endif
            digitalWrite(Set.LEDWiFi_PIN, Set.LEDWiFi_ON_Level);
        }
    }
    if (LED_WIFI_ON) {
        if (now > (LED_WIFI_time + (LED_WIFI_pulse >> blkSpeed))) {
            LED_WIFI_time = now;
            LED_WIFI_ON = false;
#if useLED_BUILTIN
            digitalWrite(LED_BUILTIN, LOW);
#endif
            digitalWrite(Set.LEDWiFi_PIN, !Set.LEDWiFi_ON_Level);
        }
    }
}