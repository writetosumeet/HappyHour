#include <avr/interrupt.h>
#include <util/twi.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include "LCD_4bit.h" // copied from LCD project, enhanced to include printRegister & reduced delay in writeToLCD (3s to 1s)
#include "I2C.h"

# define RTC_ADDRESS 0b01101000 // deviceAddr has 0 padded in front to make it 8 bits
# define READ 1
# define WRITE 0

char rtc_buffer[33] = "DD Mon YYYY DAY HH:MM:SS XX.YYC";
// DD Mon YYYY DAY
// HH:MM:SS XX.YYC

char date_str[12] = __DATE__; // Mon DD YYYY
char time_str[9] = __TIME__; // HH:MM:SS

char updatedBuffer[7] = {20, 6, 28, 1, 16, 51};   // random numbers just to initialize the buffers
char snapshotBuffer[7] = {20, 6, 28, 1, 16, 51};


// conversion: chars from __DATE__ to BCD to put into RTC
// takes 3 characters as input and returns a binary number
char getCurrentMonth(char M, char o, char n)
{
  char mon[4] = {M, o, n};
  
  if(strcmp(mon,"Jan")==0) return 0b00000001;
  if(strcmp(mon,"Feb")==0) return 0b00000010;
  if(strcmp(mon,"Mar")==0) return 0b00000011;
  if(strcmp(mon,"Apr")==0) return 0b00000100;
  if(strcmp(mon,"May")==0) return 0b00000101;
  if(strcmp(mon,"Jun")==0) return 0b00000110;
  if(strcmp(mon,"Jul")==0) return 0b00000111;
  if(strcmp(mon,"Aug")==0) return 0b00001000;
  if(strcmp(mon,"Sep")==0) return 0b00001001;
  if(strcmp(mon,"Oct")==0) return 0b00010000;
  if(strcmp(mon,"Nov")==0) return 0b00010001;
  if(strcmp(mon,"Dec")==0) return 0b00010010;
  
  return 0b00000001; // if nothing matches return Jan
}

// reads current month from RTC in BCD format, copies into rtc_buffer (for lcd display) in 3 char format
void setCurrentMonth()
{
  char mon = TWDR;
  rtc_buffer[3]='J';   rtc_buffer[4]='a';    rtc_buffer[5]='n'; // set Jan as default
  
  if(mon==0b00000001) { rtc_buffer[3]='J';   rtc_buffer[4]='a';    rtc_buffer[5]='n';  }
  if(mon==0b00000010) { rtc_buffer[3]='F';   rtc_buffer[4]='e';    rtc_buffer[5]='b';  }
  if(mon==0b00000011) { rtc_buffer[3]='M';   rtc_buffer[4]='a';    rtc_buffer[5]='r';  }
  if(mon==0b00000100) { rtc_buffer[3]='A';   rtc_buffer[4]='p';    rtc_buffer[5]='r';  }
  if(mon==0b00000101) { rtc_buffer[3]='M';   rtc_buffer[4]='a';    rtc_buffer[5]='r';  }
  if(mon==0b00000110) { rtc_buffer[3]='J';   rtc_buffer[4]='u';    rtc_buffer[5]='n';  }
  if(mon==0b00000111) { rtc_buffer[3]='J';   rtc_buffer[4]='u';    rtc_buffer[5]='l';  }
  if(mon==0b00001000) { rtc_buffer[3]='A';   rtc_buffer[4]='u';    rtc_buffer[5]='g';  }
  if(mon==0b00001001) { rtc_buffer[3]='S';   rtc_buffer[4]='e';    rtc_buffer[5]='p';  }
  if(mon==0b00010000) { rtc_buffer[3]='O';   rtc_buffer[4]='c';    rtc_buffer[5]='t';  }
  if(mon==0b00010001) { rtc_buffer[3]='N';   rtc_buffer[4]='o';    rtc_buffer[5]='v';  }
  if(mon==0b00010010) { rtc_buffer[3]='D';   rtc_buffer[4]='e';    rtc_buffer[5]='c';  }

}

// calculate day from date
char getCurrentDay() 
{
  int y,m,d,tempMon;
  y = ((date_str[7]-48)*1000) + ((date_str[8]-48)*100) + ((date_str[9]-48)*10) + (date_str[10]-48); // conversion: char to dec
  d = ((date_str[4]-48)*10) + (date_str[5]-48); // conversion: char to dec
  
  tempMon = getCurrentMonth(date_str[0], date_str[1], date_str[2]);
  m = (((tempMon & 0b00010000)>>4)*10) + (tempMon & 0b00001111); // convert BCD to dec
  
  // copied from internet
  int weekday  = (d += m < 3 ? y-- : y - 2 , 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
  return weekday+1; // adding 1 since RTC considers 1 as sunday
  
}

// reads from RTC in BCD format, copies into rtc_buffer (for lcd display) in 3 char format
void setCurrentDay()
{
  char d = TWDR;
  
  if(d==0b00000001) { rtc_buffer[12]='S';   rtc_buffer[13]='u';    rtc_buffer[14]='n';  }
  if(d==0b00000010) { rtc_buffer[12]='M';   rtc_buffer[13]='o';    rtc_buffer[14]='n';  }
  if(d==0b00000011) { rtc_buffer[12]='T';   rtc_buffer[13]='u';    rtc_buffer[14]='e';  }
  if(d==0b00000100) { rtc_buffer[12]='W';   rtc_buffer[13]='e';    rtc_buffer[14]='d';  }
  if(d==0b00000101) { rtc_buffer[12]='T';   rtc_buffer[13]='h';    rtc_buffer[14]='u';  }
  if(d==0b00000110) { rtc_buffer[12]='F';   rtc_buffer[13]='r';    rtc_buffer[14]='i';  }
  if(d==0b00000111) { rtc_buffer[12]='S';   rtc_buffer[13]='a';    rtc_buffer[14]='t';  }
  
}


// reads 3 char day from rtc_buffer and returns the corresponding decimal number
char getNumericDay()
{
  
  if ((rtc_buffer[12]=='S') && (rtc_buffer[13]=='u') && (rtc_buffer[14]=='n')) return 1;
  if ((rtc_buffer[12]=='M') && (rtc_buffer[13]=='o') && (rtc_buffer[14]=='n')) return 2;
  if ((rtc_buffer[12]=='T') && (rtc_buffer[13]=='u') && (rtc_buffer[14]=='e')) return 3;
  if ((rtc_buffer[12]=='W') && (rtc_buffer[13]=='e') && (rtc_buffer[14]=='d')) return 4;
  if ((rtc_buffer[12]=='T') && (rtc_buffer[13]=='h') && (rtc_buffer[14]=='u')) return 5;
  if ((rtc_buffer[12]=='F') && (rtc_buffer[13]=='r') && (rtc_buffer[14]=='i')) return 6;
  if ((rtc_buffer[12]=='S') && (rtc_buffer[13]=='a') && (rtc_buffer[14]=='t')) return 7;
  
  return 1; // default return to avoid compiler warning
}



// check Year to see if RTC needs to be initialised with system date-time
unsigned char checkYear()
{
  unsigned char flag = 1;
  
  sendStart();
  
  // Read Year in RTC to check if first time power up and values need to be setup
  if (TW_STATUS==TW_START)
  {
    // write slave device address and Write=0 in TWDR | SLA+W
    sendDeviceAddress((RTC_ADDRESS<<1)|WRITE);
    
    if(TW_STATUS==TW_MT_SLA_ACK)
    {
      // send rtc memory location
      writeData(0b00000110); // RTC memory location - Year (0x06h)
      
      if(TW_STATUS==TW_MT_DATA_ACK)
      {
        
        // reading from RTC memory location 06h
        // send repeat start
        sendStart();
        
        if (TW_STATUS==TW_REP_START)
        {
          // write slave device address and Read=1 in TWDR | SLA+R
          sendDeviceAddress((RTC_ADDRESS<<1)|READ);
          
          if (TW_STATUS==TW_MR_SLA_ACK)
          {
            
              // Read Year set in RTC - send NACK as only 1 byte to be read
              readData('N');
              
              if (TWDR==0)
                  flag=0;
    
              // send stop bit
              sendStop();
            
          } // end of sending device address SLA+R
        } // end of sending repeat start 
        
      } // end of sending memory location to read
    } // end of sending device address SLA+W               
  } // end of sending start
  
  return flag;
}


// set values in RTC from system date time (first time setup)
void setRTCValues()
{
  char i, temp;
  
  sendStart(); 
  
  if(TW_STATUS==TW_START) 
  {
    // write slave device address and Write=0 in TWDR | SLA+W
    sendDeviceAddress((RTC_ADDRESS<<1)|WRITE);
    
    if(TW_STATUS==TW_MT_SLA_ACK)
    {
      
      // send rtc memory location
      writeData(0b00000000); // first RTC memory location - Seconds (0x00h)
      
      if(TW_STATUS==TW_MT_DATA_ACK)
      {
        // setup all RTC values required
        for (i=0;i<7;i++)
        {
          if(i==0)
          {
            temp = 0;
            temp |= (time_str[7] - 48);
            temp = (temp & 0b10001111) | ((time_str[6] - 48)<<4);
            writeData(temp); // setup seconds from __TIME__ HH:MM:SS
          }
          if(i==1)
          {
            temp = 0;
            temp |= (time_str[4] - 48);
            temp = (temp & 0b10001111) | ((time_str[3] - 48)<<4);
            writeData(temp); // setup minutes from __TIME__ HH:MM:SS
          }
          if(i==2)
          {
            temp = 0;
            temp |= (time_str[1] - 48);
            temp = (temp & 0b11001111) | ((time_str[0] - 48)<<4);
            writeData(temp); // setup hour from __TIME__ HH:MM:SS
          }
          if(i==3)
          {
            writeData(getCurrentDay()); // setup day
          }
          if(i==4)
          {
            temp = 0;
            temp |= (date_str[5] - 48);
            if (date_str[4] > 48)
              temp = (temp & 0b11001111) | ((date_str[4] - 48)<<4);
            writeData(temp); // setup date from __DATE__ Mon DD YYYY
          }
          if(i==5)
          {
            //century is hardcoded as 0 (bit7)
            writeData(getCurrentMonth(date_str[0], date_str[1], date_str[2])); // setup month from __DATE__ Mon DD YYYY
          }
          if(i==6)
          {
            temp = 0;
            temp |= (date_str[10] - 48);
            temp = (temp & 0b00001111) | ((date_str[9] - 48)<<4);
            writeData(temp); // setup year from __DATE__ Mon DD YYYY
          }
        }
        
        // send stop bit
        sendStop();
        
      } // end of sending memory location to write
    } // end of sending device address SLA+W
  } // end of sending start     
}


// Update values in RTC based on user input. Actual update only if user has changed the value
void updateRTCValues()
{
  char i, temp, rtcAddressWrite;
  
  rtcAddressWrite = (RTC_ADDRESS<<1) | WRITE;
  
    // setup all RTC values required
    //updatedBuffer / snapshotBuffer; // Year / Month / Date / Day / Hour / Minute
    // updating only if changed by user i.e., updatedBuffer[i] ! = snapshotBuffer[i]
    // converting decimal to BCD
    for (i=0;i<6;i++)
    {
          // set Minutes if updated 
          if((i==0) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
          {
            writeOneByte(rtcAddressWrite, i, 0); // set seconds to 0
            temp = updatedBuffer[5-i] / 10; 
            temp = (temp << 4) | (updatedBuffer[5-i] % 10);
            writeOneByte(rtcAddressWrite, i+1, temp);
          }
          
          // set Hour if updated
          if((i==1) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
          {
            temp = updatedBuffer[5-i] / 10; 
            temp = (temp << 4) | (updatedBuffer[5-i] % 10);
            writeOneByte(rtcAddressWrite, i+1, temp);
          }
          
          // set Day if updated
          if((i==2) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
            writeOneByte(rtcAddressWrite, i+1, updatedBuffer[5-i]); 
          
          // set Date if updated
          if((i==3) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
          {
            temp = updatedBuffer[5-i] / 10; 
            temp = (temp << 4) | (updatedBuffer[5-i] % 10);
            writeOneByte(rtcAddressWrite, i+1, temp);
          }
          
          // set Month if updated - century is hardcoded as 0 (bit7)
          if((i==4) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
          {
            temp = updatedBuffer[5-i] / 10; 
            temp = (temp << 4) | (updatedBuffer[5-i] % 10);
            writeOneByte(rtcAddressWrite, i+1, temp);
          }
          
          // set Year if updated
          if((i==5) && (updatedBuffer[5-i] != snapshotBuffer[5-i]))
          {
            temp = updatedBuffer[5-i] / 10; 
            temp = (temp << 4) | (updatedBuffer[5-i] % 10);
            writeOneByte(rtcAddressWrite, i+1, temp);
          }
    }
        
}

// reads values from RTC, keeps storing in rtc_buffer and finally displays all on lcd
void displayRTCValues()
{
  char i, temp;
  
  // write starting memory location to read from
  // send start
  sendStart();
  
  if (TW_STATUS==TW_START)
  {
    // write slave device address and Write=0 in TWDR | SLA+W
    sendDeviceAddress((RTC_ADDRESS<<1)|WRITE);
    
    if(TW_STATUS==TW_MT_SLA_ACK)
    {
      // send rtc memory location
      writeData(0b00000000); // first RTC memory location - (0x00h)
      
      if(TW_STATUS==TW_MT_DATA_ACK)
      {
        
        // reading from RTC memory location 00h
        // send repeat start
        sendStart();
        
        if (TW_STATUS==TW_REP_START)
        {
          // write slave device address and Read=1 in TWDR | SLA+R
          sendDeviceAddress((RTC_ADDRESS<<1)|READ);
          
          if (TW_STATUS==TW_MR_SLA_ACK)
          {
            // request data from RTC 
            for(i=0;i<19;i++) // ideally check TWSR after every memory location read
            {
              // NACK sent if last byte to be read
              if(i==18)
                readData('N');
              else
                readData('A');
              
              //printRegister("TWDR", TWDR);
              if(i==0)
              {
                // format seconds and update rtc_buffer[22][23]
                rtc_buffer[22]= ((TWDR & 0b01110000)>>4) + 48;
                rtc_buffer[23]= (TWDR & 0b00001111) + 48;
              }
              if(i==1)
              {
                // format minutes and update rtc_buffer[19][20]
                rtc_buffer[19] = ((TWDR & 0b01110000)>>4) + 48;
                rtc_buffer[20] = (TWDR & 0b00001111) + 48;
              }
              if(i==2)
              {
                // format 24-hour and update rtc_buffer[16][17]
                rtc_buffer[16] = ((TWDR & 0b00110000)>>4) + 48;
                rtc_buffer[17] = (TWDR & 0b00001111) + 48;
              }
              if(i==3)
              {
                // format day and update rtc_buffer[12][13][14]
                setCurrentDay();
              }
              if(i==4)
              {
                // format date and update rtc_buffer[0][1]
                rtc_buffer[0] = ((TWDR & 0b00110000)>>4) + 48;
                rtc_buffer[1] = (TWDR & 0b00001111) + 48;
              }
              if(i==5)
              {
                // format century & month and update rtc_buffer[3][4][5][7][8]
                rtc_buffer[7] = 2 + 48; // hardcoded century
                rtc_buffer[8] = 0 + 48;
                setCurrentMonth();
              }
              if(i==6)
              {
                // format year and update rtc_buffer[9][10]
                rtc_buffer[9] = ((TWDR & 0b11110000)>>4) + 48;
                rtc_buffer[10] = (TWDR & 0b00001111) + 48;
              }
              if(i==17)
              {
                // read temperature 2's complement integer
                rtc_buffer[25] = (TWDR / 10) + 48;
                rtc_buffer[26] = (TWDR % 10) + 48;
              }
              if(i==18)
              {
                // read temperature - fractional part
                rtc_buffer[28]=0; rtc_buffer[29]=0; // default value
                temp = TWDR >> 6;
                if (temp==0) { rtc_buffer[28]='0'; rtc_buffer[29]='0'; }
                if (temp==1) { rtc_buffer[28]='2'; rtc_buffer[29]='5'; }
                if (temp==2) { rtc_buffer[28]='5'; rtc_buffer[29]='0'; }
                if (temp==3) { rtc_buffer[28]='7'; rtc_buffer[29]='5'; }
              }
              
              if (TW_STATUS==TW_MR_DATA_NACK)
              {
                sendStop();
              } // end of reading data                    
            }
            writeToLCD(rtc_buffer);
            
          } // end of sending device address SLA+R
        } // end of sending repeat start 
        
      } // end of sending memory location to read
    } // end of sending device address SLA+W               
  } // end of sending start
  
}

// displays 'Set ...' on lcd based on mode selected by user
void displayMode(unsigned char mode)
{
  char *displayString = "Setting date and time values";
  
  if (mode==0)
  {
    snprintf(displayString, 15, "Set Year: %d", updatedBuffer[mode]); // integer takes 4 bytes
    writeToLCD(displayString);
  }
  if (mode==1)
  {
    snprintf(displayString, 16, "Set Month: %d", updatedBuffer[mode]);
    writeToLCD(displayString);
  }
  if (mode==2)
  {
    snprintf(displayString, 15, "Set Date: %d", updatedBuffer[mode]);
    writeToLCD(displayString);
  } 
  if (mode==3)
  {
    snprintf(displayString, 14, "Set Day: %d", updatedBuffer[mode]);  
    writeToLCD(displayString);
  }
  if (mode==4)
  {
    snprintf(displayString, 15, "Set Hour: %d", updatedBuffer[mode]);
    writeToLCD(displayString);
  }
  if (mode==5)
  {
    snprintf(displayString, 18, "Set Minutes: %d", updatedBuffer[mode]);
    writeToLCD(displayString);
  }
}

// increment value in updatedBuffer based on mode
void Increment(unsigned char mode)
{
  // add loop in Year / Month / Date / Day / Hour / Minute
  char lowerLimit[7] = {0, 1, 1, 1, 0, 0};
  char upperLimit[7] = {99, 12, 31, 7, 23, 59};
  
  if(updatedBuffer[mode]==upperLimit[mode])
    updatedBuffer[mode] = lowerLimit[mode];
  else
    updatedBuffer[mode]++;

}

// decrement value in updatedBuffer based on mode
void Decrement(unsigned char mode)
{
  // add loop in Year / Month / Date / Day / Hour / Minute
  char lowerLimit[7] = {0, 1, 1, 1, 0, 0};
  char upperLimit[7] = {99, 12, 31, 7, 23, 59};
  
  if(updatedBuffer[mode]==lowerLimit[mode])
    updatedBuffer[mode] = upperLimit[mode];
  else
    updatedBuffer[mode]--;
  
}


// store snapshot of current date time in snapshot/updatedBuffers
void initializeBuffers()
{
  unsigned char tempMon, i;
  //rtc_buffer[33] = "DD Mon YYYY DAY HH:MM:SS XX.YYC";
  //updatedBuffer = snapshotBuffer; // Year / Month / Date / Day / Hour / Minute

  // setup year
  snapshotBuffer[0] = ((rtc_buffer[9]-48)*10) + (rtc_buffer[10]-48);
    
  // setup month 
  tempMon = getCurrentMonth(rtc_buffer[3], rtc_buffer[4], rtc_buffer[5]);
  snapshotBuffer[1] = (((tempMon & 0b00010000)>>4)*10) + (tempMon & 0b00001111); // convert BCD to dec
  
  // setup date
  snapshotBuffer[2] = ((rtc_buffer[0]-48)*10) + (rtc_buffer[1]-48); // conversion: char to dec
  
  // setup day
  snapshotBuffer[3] = getNumericDay();
    
  //setup Hour
  snapshotBuffer[4] = ((rtc_buffer[16]-48)*10) + (rtc_buffer[17]-48);
  
  // setup Minutes
  snapshotBuffer[5] = ((rtc_buffer[19]-48)*10) + (rtc_buffer[20]-48);
  
  // copy snapshot buffer into updatedBuffer
  for (i=0;i<6;i++)
    updatedBuffer[i] = snapshotBuffer[i];
}


#define NUM_OF_EVENTS 24

int main (void)
{
  unsigned char mode=0, setupPressed=1, tempMon, i, j;
  char currentDate, currentMonth, currentHour, currentMinute;
  char eventDate[NUM_OF_EVENTS] =  {7, 11, 30, 11, 23, 14, 20, 16, 14, 13, 17, 30, 21, 17, 12,  7, 4, 12, 21,  5, 29, 8, 24, 3};
  char eventMonth[NUM_OF_EVENTS] = {3,  4,  6, 12,  5, 11,  4,  6,  2, 12, 10,  7, 11,  7,  2, 12, 3, 11,  9, 12, 12, 4,  2, 8};
  // Maximum string length should be 26
  char* eventPerson[NUM_OF_EVENTS] = {
                                        "Birthday Poonam",    
                                        "Birthday Lok Veer",    
                                        "Anniversary Poonam&LokVeer",      
                                        "Birthday Praveen", 
                                        "Birthday Curie", 
                                        "Birthday Sumeet", 
                                        "Anniversary Curie & Sumeet", 
                                        "Birthday Nikita",    
                                        "Birthday Yuri", 
                                        "Anniversary Nikita & Yuri",      
                                        "Birthday Monika",    
                                        "Birthday Vineet",    
                                        "Anniversary Monika&Vineet",      
                                        "Birthday Gungun",    
                                        "Birthday Anjali",    
                                        "Birthday Pooja",    
                                        "Birthday Akshat",    
                                        "Anniversary Pooja & Akshat",      
                                        "Birthday Shaurya",    
                                        "Birthday Akhil",    
                                        "Birthday Geetika",    
                                        "Birthday Anup",    
                                        "Anniversary Geetika & Anup",      
                                        "Birthday Anshi"   
                                      };
  char* lcd_buffer = {"Happy "};
  
  // Buttons - Setup (PD5) / Mode (PD6) / + (PD7) / - (PB0)
  // setup Pins as Input
  DDRD = (DDRD & 0b00011111); 
  DDRB = (DDRB & 0b11111110);
  
  // Initialize LCD
  initializeLCD();
  
  // Initialize I2C
  // enable global interrupts
  sei();
  
  // set TWI bit rate
  // set SCL frequency = f_cpu / (16 + (2*TWBR*Prescalar)); 
  // TWBR = 32; Prescalar = 1 (default); SCL freq = 100kHz; f_cpu = 8MHz
  // TWBR = 12; Prescalar = 1 (default); SCL freq = 200kHz; f_cpu = 8MHz
  // TWBR = 5; Prescalar = 1 (default); SCL freq = 307.7kHz; f_cpu = 8MHz
  // TWBR = 2; Prescalar = 1 (default); SCL freq = 400kHz; f_cpu = 8MHz
  TWBR = 12;
  // error handling missing in every if - what to do if TW_STATUS is not what it should be 
  
  if (checkYear()==0)
  {
    // first time power up - set RTC Values
    setRTCValues();
  }
  
 
  
 // get and display RTC Values *******************************
        while(1) // continuous display loop
        {
            displayRTCValues(); 
          
            setupPressed = 1;
          
            // Buttons - Setup (PD5) / Mode (PD6) / + (PD7) / - (PB0)
            // check if setup button is pressed
            while (((PIND & 0b00100000)>>5)==0)
            {
                  if (setupPressed==1)
                  {
                    // Initialize updatedBuffer and snapshotBuffer
                    initializeBuffers();
                    
                    // Initialize mode
                    mode = 0;
                    setupPressed = 0;
                  }
                  
                  displayMode(mode);
              
                  // if Mode button pressed
                  if (((PIND & 0b01000000)>>6)==0)
                  {
                    mode++;
                    if (mode==6) // loop mode in valid values 0 to 5
                        mode = 0;
                  }
                  
                  // if + button pressed
                  if (((PIND & 0b10000000)>>7)==0)
                        Increment(mode);
                  
                  // if - button pressed
                  if ((PINB & 0b00000001)==0)
                        Decrement(mode);
                  
            } // end of while (setup button is pressed)
            
            if (setupPressed == 0)
            {
                // update RTC with updatedBuffer
                updateRTCValues();
                writeToLCD("Saved successfully");
            }
            
            
            // at every midnight - check event and display message on LCD
            //get current time
            currentHour = ((rtc_buffer[16]-48)*10) + (rtc_buffer[17]-48);
            currentMinute = ((rtc_buffer[19]-48)*10) + (rtc_buffer[20]-48);
            
            if ((currentHour==0) && (currentMinute==0))
            {
                  // get current date and month
                  tempMon = getCurrentMonth(rtc_buffer[3], rtc_buffer[4], rtc_buffer[5]);
                  currentMonth = (((tempMon & 0b00010000)>>4)*10) + (tempMon & 0b00001111); // convert BCD to dec
                  currentDate = ((rtc_buffer[0]-48)*10) + (rtc_buffer[1]-48); // conversion: char to dec
                  
                  //if date DD/MM matches any date in event array 
                 for (i=0;i<NUM_OF_EVENTS;i++)
                 {
                       if ((eventDate[i]==currentDate) && (eventMonth[i]==currentMonth))
                       {
                             //display happy <event person array>[i]
                             j = 0;
                             while(eventPerson[i][j] != '\0')
                             {
                               lcd_buffer[j+6] = eventPerson[i][j];
                               j++;
                             }
                             lcd_buffer[j+6] = '\0';
                             
                             writeToLCD(lcd_buffer);
                             // while mode button not pressed
                             while (((PIND & 0b01000000)>>6)!=0);
                       }
                  } // end of looping through event array
              
             } // end of if midnight
            

        } // end of while(1)
 
  cli();
  
  return(0);
}



