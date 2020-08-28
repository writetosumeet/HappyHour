
// mapping of RS /EN / D7-D0 pins
// RS - PB1 / EN - PB2 / D7 - PC3 / D6 - PC2 / D5 - PC1 / D4 - PC0 
// In case any other pins are used - update byteToLCD and initialize functions

void byteToLCD(unsigned char flag, unsigned char byteToWrite)
{
  // ANDing with 1 does not change value / ORing with 0 does not change value
  // write high bits on selected Ports
  PORTC = (PORTC & 0b11110000) | ((byteToWrite & 0b11110000)>>4); // mask not to be changed bits / shift and align byteToWrite with PORTC pins
  
  // RS = 0 for command / 1 for data
  if (flag == 0)
    PORTB = (PORTB & 0b11111101) | (0b00000000);
  else
    PORTB = (PORTB & 0b11111101) | (0b00000010);
  
  // Enable = 1
  PORTB = (PORTB & 0b11111011) | (0b00000100);
  
  // wait 100 micro second
  _delay_us(100);
  
  // Enable = 0
  PORTB = (PORTB & 0b11111011) | (0b00000000);
  
  // wait 100 micro second
  _delay_us(100);
  
  // write low bits on selected Ports
  PORTC = (PORTC & 0b11110000) | (byteToWrite & 0b00001111); // mask not to be changed bits / shift and align byteToWrite with PORTC pins
  
  // Enable = 1
  PORTB = (PORTB & 0b11111011) | (0b00000100);
  
  // wait 100 micro second
  _delay_us(100);
  
  // Enable = 0
  PORTB = (PORTB & 0b11111011) | (0b00000000);
}


void writeToLCD(char* str)
{
  unsigned char strLength = strlen(str);
  unsigned char i;
  
  // initial delay for stabilization
  _delay_ms(10);
  
  // Clear display
  byteToLCD(0, 0x01);
  
  // delay for clear display
  _delay_ms(10);
  
  for (i=0; i<strLength; i++)
  {
    // if more than 16 char - move cursor to next line on screen
    if(i==16)
    {
      byteToLCD(0, 0xC0);
      // if next character is blank - avoid printing it
      if(str[i]==' ')
        i++;
    }
    byteToLCD(1, str[i]);
    
    /*
    // to enable rolling display - upto 40 char per line
    // if more than 16 char - shift display to left
    if(i==16)
    byteToLCD(0, 0x07);  // 0 0 0 0 0 1 I/D S   I/D- Increment: 1;  S - Shift cursor left(1) after writing character
    
    if(i>=16)
    _delay_ms(400);*/
    // return cursor and display to home
    //byteToLCD(0, 0x02);
  }
  _delay_ms(1000);
}




void initializeLCD()
{
  _delay_ms(100); // delay for power stabilization
  
  // set data direction of avr pins as output
  //1 = output, 0 = input
  // RS - PB1 / EN - PB2 / D7 - PC3 / D6 - PC2 / D5 - PC1 / D4 - PC0 
  // ANDing with 1 does not change value / ORing with 0 does not change value
  DDRC = (DDRC & 0b11110000) | (0b00001111); // mask others and make Pins 3-0 as 1
  DDRB = (DDRB & 0b11111001) | (0b00000110); // mask others and make Pins 1 and 2 as 1
  
  // set LCD in 8-bit, 1 line, 5*8 mode (default) - this is to ensure the LCD always start in same (default) state
  // without this, the LCD displays garbage characters especially at reset of microcontroller (without lcd power reset)
  byteToLCD(0, 0x30);
  //byteToLCD(0, 0x33); uncomment in case LCD displays garbage on reset
  
  // Set LCD as 4-bit 1 line device - send twice as required by datasheet (initially machine assumes 8 bit)
  byteToLCD(0, 0x02); // removing this makes LCD display garbage
  byteToLCD(0, 0x28); // 0 0 1 DL N F x x  DL- 1:8-bit 0:4-bit ; N- 1:2 lines 0:1 line display ; F- Font 0:5x8 char 1:5x10 char
  
  // Switch on display
  byteToLCD(0, 0x0F); // 0 0 0 0 1 D C B    D- display; C- cursor; B- Blinking
  
  // Set cursor increment after each write
  byteToLCD(0, 0x06);  // 0 0 0 0 0 1 I/D S   I/D- Increment: 1;  S - Shift cursor right(0) after writing character
  
  // Write to LCD
  writeToLCD("LCD Initialization done");
}


// function to print an 8 bit register in binary format
void printRegister(char*name, char value)
{
  unsigned char i;
  unsigned char len = strlen(name);
  char lcd_buffer[33]; // Buffer to represent the 16*2 LCD. One extra space needed to denote the end of string
  
  // Initialize LCD buffer string with spaces
  for (i=0;i<32;i++)
    lcd_buffer[i]=' ';
  
  // display register name in Line 1 of LCD
  for (i=0; i<len; i++)
    lcd_buffer[i] = name[i];
  
  // display register value in Line 2 of LCD
  for (i=16;i<24;i++)
  {
    if((value & (1<<(23-i)))==0)
      lcd_buffer[i]='0';
    else
      lcd_buffer[i]='1';
  }
  
  writeToLCD(lcd_buffer);
  _delay_ms(3000);
}





