
void waitForTwint()
{
  // wait till TWINT - twi interrupt bit is set
  while((TWCR & (1<<TWINT))==0);
}

void sendStart()
{
  // send start bit - for Start / Repeat Start
  // TWINT | TWEA | TWSTA | TWSTO | TWWC | TWEN | - | TWIE
  TWCR = (1<<TWSTA)|(1<<TWEN)|(1<<TWINT);
  
  waitForTwint();
  //printRegister("TWSR 00001000", TWSR); //- for start
  //printRegister("TWSR 00010000", TWSR); //- for repeat start
}

void sendStop()
{
  // send stop bit
  // TWINT | TWEA | TWSTA | TWSTO | TWWC | TWEN | - | TWIE
  TWCR = (1<<TWSTO)|(1<<TWEN)|(1<<TWINT);
  //printRegister("TWSR 11111000", TWSR);
}

void sendDeviceAddress(char deviceAddr)
{
  // deviceAddr is sent with read or write bit set
  TWDR = deviceAddr; 
  
  // TWINT | TWEA | TWSTA | TWSTO | TWWC | TWEN | - | TWIE
  TWCR = (1<<TWINT)|(1<<TWEN);
  
  waitForTwint();
  //printRegister("TWSR 00011000", TWSR); - SLA+W
  //printRegister("TWSR 01000000", TWSR); - SLA+R
}

void writeData(char value)
{
  TWDR = value; 
  TWCR = (1<<TWINT)|(1<<TWEN);
  
  waitForTwint();
  //printRegister("TWSR 00101000", TWSR);
}

void readData(char flag)
{
  // request data from RTC - TWEA=1 if more than 1 byte to be read | ack sent after byte received
  // NACK sent when requesting last byte
  
  if (flag=='A')
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
  else
    TWCR = (1<<TWINT)|(1<<TWEN)|(0<<TWEA);
  
  waitForTwint();
  
  //printRegister("TWSR 01011000", TWSR); - data recieved and Nack sent
  //printRegister("TWSR 01010000", TWSR); - data recieved and Ack sent
  
  // This will place data in TWDR
}

// write value to RTC in given address (one memory location update)
// deviceAddress is sent with read or write bit set
void writeOneByte(char deviceAddress, char memoryAddress, char value)
{
  sendStart(); 
  
  if(TW_STATUS==TW_START) 
  {
    // write slave device address and Write=0 in TWDR | SLA+W
    sendDeviceAddress(deviceAddress);
    
    if(TW_STATUS==TW_MT_SLA_ACK)
    {
      // send rtc memory location
      writeData(memoryAddress); 
      
      if(TW_STATUS==TW_MT_DATA_ACK)
        writeData(value);
      
      // send stop bit
      sendStop();
      
    } // end of sending device address SLA+W
  } // end of sending start 
  
}
