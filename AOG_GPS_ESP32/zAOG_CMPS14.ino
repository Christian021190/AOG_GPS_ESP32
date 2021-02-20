
void readCMPS() {

    unsigned char high_byte, low_byte, angle8, h2, l2;
    bool roll_neg;
    unsigned int angle16;
    int roll16;

    HeadingCMPS = 0.0;
    rollCMPS = 0.0;
    Wire.beginTransmission(Set.CMPS14_ADDRESS);  //starts communication with CMPS14
    Wire.write(0x02);                     //Sends the register we wish to start reading from
    Wire.endTransmission(Set.CMPS14_ADDRESS);

    // Request 2 bytes from the CMPS14
    // this will give us the 8 bit bearing,
    // both bytes of the 16 bit bearing, pitch and roll
    Wire.requestFrom(Set.CMPS14_ADDRESS, 2);

    if (Wire.available() <= 2)     // Wait for all bytes to come back
    {
        // Read back the 2 bytes
        high_byte = Wire.read();
        low_byte = Wire.read();


        angle16 = high_byte;                 // Calculate 16 bit angle
        angle16 <<= 8;
        angle16 += low_byte;

        HeadingCMPS = float(angle16) / 10;
    }

    Wire.beginTransmission(Set.CMPS14_ADDRESS);  //starts communication with CMPS14
    Wire.write(0x1C);                     //Sends the register we wish to start reading from
    Wire.endTransmission(Set.CMPS14_ADDRESS);
    Wire.requestFrom(Set.CMPS14_ADDRESS, 2);

    if (Wire.available() <= 2)
    {

        h2 = Wire.read(); //Roll
        l2 = Wire.read();

        roll16 = h2;
        roll16 <<= 8;
        roll16 += l2;
        if (bitRead(roll16, 15)) { roll_neg = true; }
        else { roll_neg = false; }
        rollCMPS = float(roll16) / 10;
        if (roll_neg) { rollCMPS -= 6554; }
    }
}

