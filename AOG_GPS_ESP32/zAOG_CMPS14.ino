/*
void CMPS14_handle(void* pvParameters) {
    //cmps14 values
#define CMPS14_ADDRESS 0x60   // Address of CMPS14 shifted right one bit for arduino wire library
    unsigned char high_byte, low_byte, angle8, h2, l2;
    bool roll_neg;
  //  char pitch, cmpsroll;
    unsigned int angle16;
    int roll16;
  //  float roll2;
    float heading;
    //inclinometer variables
//    float rollK = 0, Pc = 0.0, G = 0.0, P = 1.0, Xp = 0.0, Zp = 0.0;
    float XeRoll = 0;
//    float lastRoll = 0, diff = 0;
//    int roll = 0;
    //Kalman control variances
//    const float varRoll = 0.2; // variance, larger is more filtering
//    const float varProcess = 0.01; //process, smaller is more filtering

//    if (WiFi_connect_step != 0) { vTaskDelay(10000); }//waiting for WiFi to start first
    Serial.println("started new task: CMPS14 handle");
    for (;;) { // MAIN LOOP
        if (Set.CMPS14_present == true) {
            Wire.beginTransmission(CMPS14_ADDRESS);  //starts communication with CMPS14
            Wire.write(0x02);                     //Sends the register we wish to start reading from
            Wire.endTransmission(CMPS14_ADDRESS);

            // Request 2 bytes from the CMPS14
            // this will give us the 8 bit bearing,
            // both bytes of the 16 bit bearing, pitch and roll
            Wire.requestFrom(CMPS14_ADDRESS, 2);

            if (Wire.available() <= 2)     // Wait for all bytes to come back
            {
                // Read back the 2 bytes
                high_byte = Wire.read();
                low_byte = Wire.read();


                angle16 = high_byte;                 // Calculate 16 bit angle
                angle16 <<= 8;
                angle16 += low_byte;

                heading = float(angle16) / 10;
                //heading *= 16;
            }

            Wire.beginTransmission(CMPS14_ADDRESS);  //starts communication with CMPS14
            Wire.write(0x1C);                     //Sends the register we wish to start reading from
            Wire.endTransmission(CMPS14_ADDRESS);
            Wire.requestFrom(CMPS14_ADDRESS, 2);

            if (Wire.available() <= 2)
            {

                h2 = Wire.read(); //Roll
                l2 = Wire.read();

                roll16 = h2;
                roll16 <<= 8;
                roll16 += l2;
                if (bitRead(roll16, 15)) { roll_neg = true;}
                else { roll_neg = false; }
                XeRoll = float(roll16) / 10; 
               if (roll_neg) { XeRoll -= 6554; }
               // XeRoll *= 16;
            }
            //endcmps

            Serial.print("Roll: "); Serial.print(XeRoll); Serial.print(" heading: "); Serial.println(heading);

        }
        else {
            delay(1);
            vTaskDelete(NULL);
            delay(1);
        }
        vTaskDelay(80000);
    }
}
*/