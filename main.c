/*
MicroMouse Team:India 2018/19
*/


//Include Statements
#include <hidef.h>      // for EnableInterrupts macro
#include "derivative.h" // include peripheral declarations
#include <stdio.h>


//Port Definitions
#define TBFL PTAD_PTAD1

//Bit Definitions
#define STOP  0b00000000
#define FWD   0b00000101
#define REV   0b00001010
#define ACW   0b00001001
#define CW    0b00000110
#define SFL   0b00000001 //Swerve forward and left
#define SFR   0b00000100 //Swerve forward and right

//Constants
#define bitClear 0
#define SCALE 1


//Procedure Declarations
void revleft (void);
void revright (void);
void stop (void);
void reverse (void);
void turnleft (void);
void turnright (void);
void lineleft (void);
void lineright (void);
void lineturnleft (void);
void lineturnright (void);
void iravoidl (void);
void iravoidr (void);
void speedcon (void);
void lcd_init(void);
void writecmd(int);
void writedata(char);
void delay(int);
void distdelay(int);


//The motors are connected to Port F bits 0 to 3 in pairs.
//Sense of switching is 01 = forward, 10 = reverse,
//00 = brake, 11 = freewheel.

byte DRIVE;
byte MODCNTH = 0x4E, MODCNTL = 0x20;            // values for the modulo registers (0x4E20)

byte INIT_CHNCNTH = 0x40, INIT_CHNCNTL = 0x00;  // set the IOC register to 0x3000 for output compare function

//Declare White Line Sensor Variables
byte lcdadc1[2], lcdadc2[2], lcdadc3[2], lcdadc4[2], lcdadc5[2], lcderr[1];


//Word Declarations
word REPEAT = 0x4E20;
word NOM_SPEED = 0x2000;
word PW_LEFT = 0x2000;
word PW_RIGHT = 0x2000;
word PW_MAX = 0x3000;
word PW_MIN = 0x1000;
word CORR = 0;

word COUNTER;
word DISTANCE;

word NEW_LEFT;
word OLD_LEFT;
word DIFF_LEFT;

word NEW_RIGHT;
word OLD_RIGHT;
word DIFF_RIGHT;


//Declare Integers
int adc1, adc2, adc3, adc4, adc5, intcounter, select, preselect, error, errlcd, alladc, adc1hl, adc2hl, adc3hl, adc4hl, adc5hl, mode;
long alladcext ;

//PID Loop Variables
int Kp = 2048;
int speedChange = 0;


// *************************************************************************************************************************************
// *************************************************************************************************************************************

//Start Here! Void Main

// *************************************************************************************************************************************
// *************************************************************************************************************************************

void main(void)
{
	
	//Initial Setup
    SOPT   = 0x00;      // disable COP (watchtimer)
	ICGC1 = 0x74;	// select external quartz crystal

    // Init_GPIO init code 
    PTFDD = 0x0F;   // set port F as outputs for motor drive on bits PTFD bits 0-3.
    PTDPE = 0b00001100; // use PTDD bits 2, 3 as input ports for touch bars

    // configure TPM module 1                                                                                                                                                                                                                                   
    TPM1SC   = 0x48;  // format: TOF(0) TOIE(1) CPWMS(0) CLKSB(0) CLKSA(1) PS2(0) PS1(0) PS0(0)
    TPM1MOD = REPEAT; // set the counter modulo registers to 0x4E20 (= 20000 decimal).

    // configure TPM1 channel 1
    TPM1C1SC = 0x50;     // TPM1 Channel 1
    TPM1C1V = NOM_SPEED;   // set the channel 1 registers to 0x1000
    TPM1C2SC = 0x50;     // TPM1 Channel 2
    TPM1C2V = NOM_SPEED;   // set the channel 2 registers to 0x1000
	TPM2SC =  0x08;			//select bus clock for TPM2, no TOV
	TPM2C0SC = 0x44;		//turn on edge interrupt for TPM2 C0
	TPM2C1SC = 0x44;		//turn on edge interrupt for TPM2 C1


	//Initialise Variables
    select = 0;
    preselect = 2;
    intcounter = 5;
    
    //Name Ports
    PTADD = 0xFF;     //LCD pins
    PTGDD = 0x07;     //LCD read/enable
    ADC1CFG = 0x70;   //white line sensor
   
   	//Start LCD Init Procedure
    lcd_init();
    
    writecmd(0x81); //moves cursor
    writedata('I');
    writedata('N');
    writedata('D');
    writedata('I');
    writedata('A');
             
                 
    //***********         
    //MENU SYSTEM
    //***********
    while((PTDD & 0b00000100) != 0){
    
    if ((PTDD & 0b00001000) == 0){select++;}
    if (preselect != select){
    if (select > 2){select = 0;}
    preselect = select;        
    
    //Display the current mode on the screen
    switch (select){        
            case 0: //Obstical Avoidance Mode
            writecmd(0xC1);
            writedata('A');
            writedata('V');
            writedata('O');
            writedata('I');
            writedata('D');
            writedata(' ');
                    break;
            case 1: //Line Following Mode
            writecmd(0xC1);
            writedata(' ');
            writedata('L');
            writedata('I');
            writedata('N');
            writedata('E');
                    break;
            case 2: //Combat Mode
            writecmd(0xC1);
            writedata('B');
            writedata('A');
            writedata('T');
            writedata('T');
            writedata('L');
            writedata('E');
                    break;
    }
    while((PTDD & 0b00001000) == 0){}}}
    
    //Start the interupts
    EnableInterrupts;
          
    //Main Loop Here
    switch (select){
        case 0: //Obstical Avoidance Mode
    for(;;) {
    	//Drive foreward by default
        DRIVE = FWD;
        if ((PTDD & 0b00001100) != 0b00001100){
        if ((PTDD & 0b00001000) == 0){revleft();}
        if ((PTDD & 0b00000100) == 0){revright();}        }
        if ((PTDD & 0b11000000) != 0){
        if ((PTDD_PTDD7== 0)&(PTDD_PTDD6==1)){iravoidl();}
        if ((PTDD_PTDD7== 1)&(PTDD_PTDD6==0)){iravoidr();}        }
    }    
    
        case 1: //Line Following Mode
    for(;;) {
    
    //  White Line Sensor Array
    //
    //  adc1	adc2	adc3	adc4	adc5
    //		||||||||||||||||||||||||||||
    
        if ((adc2 & adc4)<50) //If already on the line
        {DRIVE = FWD;}else{
        if (adc1 > 50){lineright();} //Too Far Left
        elseif (adc5 > 50){lineleft();}}}//Too Far Right
  
        if ((adc1 & adc2 & adc3 & adc4 & adc5) > 50){lineturnleft();}}}//Reached a corner, turn left
    
        case 2: //Combat Mode
        for(;;){
	      DRIVE = FWD; //Drives foreward by default
		
        if(adc1 > 50){iravoidr();mode = 0;}
	    if(adc5 > 50){iravoidl();mode = 0;}

        if ((PTDD & 0b11000000) != 0){
        if ((PTDD_PTDD7== 0)&(PTDD_PTDD6==1)){lineright();mode = 1;}
        if ((PTDD_PTDD7== 1)&(PTDD_PTDD6==0)){lineleft();mode = 1;}
        }
        
        if ((PTDD & 0b00001100) != 0b00001100){
        if ((PTDD & 0b00001000) == 0){distdelay(50);reverse();mode = 2;}
        if ((PTDD & 0b00000100) == 0){distdelay(50);reverse();mode = 2;}
        }}
    }
}

interrupt 11 void TPM1SC_overflow() //100Hz Interupt
{   // interrupt vector: Vtpm1

         TPM1SC_TOF = bitClear; //Clears itself
         PTFD = DRIVE;       // turn on motors as configured by DRIVE (port A switches).
         speedcon(); //Straight driving algorithmx

         if (COUNTER != 0){COUNTER--;} //Decrement Counter
         
        intcounter--; //Decrement a different counter
         
        //Read white line sensor values
        TPM1SC_TOF = 0;       
        ADC1SC1 = 0;
        while (ADC1SC1_COCO == 0){  
        }
        adc1 = ADC1RL*100/257; //Convert into a percentage
        
        ADC1SC1 = 1;
        while (ADC1SC1_COCO == 0){  
        }
        adc2 = ADC1RL*100/257;
        
        ADC1SC1 = 2;
        while (ADC1SC1_COCO == 0){  
        }
        adc3 = ADC1RL*100/257;
        
        ADC1SC1 = 3;
        while (ADC1SC1_COCO == 0){  
        }
        adc4 = ADC1RL*100/257;
        
        ADC1SC1 = 4;
        while (ADC1SC1_COCO == 0){  
        }
        adc5 = ADC1RL*100/257;
        
        switch (select){
        
        case 0:
        break;
                
        case 1:
        
        if (adc1>60){
        adc1hl = 0;}else{adc1hl = 10000;}
        
        if (adc2>60){
        adc2hl = 0;}else{adc2hl = 1000;}
        
        if (adc3>60){
        adc3hl = 0;}else{adc3hl = 100;}
        
        if (adc4>60){
        adc4hl = 0;}else{adc4hl = 10;}
        
        if (adc5>60){
        adc5hl = 0;}else{adc5hl = 1;}
        
        alladc = adc1hl+adc2hl+adc3hl+adc4hl+adc5hl; //Format a string that can be written to the display
        switch (alladc){
        
        default:
                error = 0;
                break;
        case 1 :
                error = 4;
                break;
        case 11 :
                error = 3;
                break;
        case 111 :
                error = 2;
                break;
        case 110 :
                error = 1;
                break;
        case 1110 :
                error = 0;
                break;
        case 1100 :
                error = -1;
                break;
        case 11100 :
                error = -2;
                break;
        case 11000 :
                error = -3;
                break;
        case 10000 :
                error = -4;
                break;
        }
        
        if(intcounter == 1){
        intcounter = 50;       
        
        sprintf(lcdadc1,"%1d",adc1);
        sprintf(lcdadc2,"%1d",adc2);
        sprintf(lcdadc3,"%1d",adc3);
        sprintf(lcdadc4,"%1d",adc4);
        sprintf(lcdadc5,"%1d",adc5);
        
        errlcd = error + 5;
        sprintf(lcderr,"%1d",errlcd);
        
        if(adc1<10){
        lcdadc1[1]=lcdadc1[0];
        lcdadc1[0]='0';
        }
        if(adc2<10){
        lcdadc2[1]=lcdadc2[0];
        lcdadc2[0]='0';
        }        
        if(adc3<10){
        lcdadc3[1]=lcdadc3[0];
        lcdadc3[0]='0';
        }
        if(adc4<10){
        lcdadc4[1]=lcdadc4[0];
        lcdadc4[0]='0';
        }
        if(adc5<10){
        lcdadc5[1]=lcdadc5[0];
        lcdadc5[0]='0';
        }
 
        writecmd(0xC0); //moves cursor
        writedata('.');
        writedata(lcdadc5[0]); //print binary ascii
        writedata(lcdadc4[0]);
        writedata(lcdadc3[0]);
        writedata(lcdadc2[0]);
        writedata(lcdadc1[0]);
        writedata('.');
        writedata(lcderr[0]);
        }
        break;
        
        case 2:
        if(intcounter == 1){
        intcounter = 50;
        writecmd(0xC0);
        switch (mode){
        
        case 0:
        writedata(' ');
        writedata('D');
        writedata('O');
        writedata('D');
        writedata('G');
        writedata('E');
        writedata('D');
        writedata(' ');
        break;
                
        case 1:
        writedata('S');
        writedata('P');
        writedata('O');
        writedata('T');
        writedata('T');
        writedata('E');
        writedata('D');
        writedata(' ');
        break;
        
        case 2:
        writedata('!');
        writedata('B');
        writedata('A');
        writedata('A');
        writedata('A');
        writedata('M');
        writedata('M');
        writedata('!');
        break;
        
        }
        }
        }
}

interrupt 6 void TPM1C1SC_int()
{   // interrupt vector: Vtpm1ch1
    
        TPM1C1SC_CH1F = bitClear;
 
         PTFD = PTFD | 0x03;     // set free-wheel mode for one motor instead of turn off

}

interrupt 7 void TPM1C2SC_int()
{   // interrupt vector: Vtpm1ch1

        TPM1C2SC_CH2F = bitClear;
    
        PTFD = PTFD | 0x0C;     // set free-wheel mode for other motor instead of turn off

}

interrupt 12 void TPM2C0SC_int()
{
	TPM2C0SC_CH0F = bitClear;

	NEW_LEFT = TPM2C0V;
	DIFF_LEFT = NEW_LEFT-OLD_LEFT;
	OLD_LEFT = NEW_LEFT;
	if (DISTANCE != 0){DISTANCE--;}

	//	PTBD = PTBD ^ 0B00000001;
}

interrupt 13 void TPM2C1SC_int()
{
	TPM2C1SC_CH1F = bitClear;

	NEW_RIGHT = TPM2C1V;
	DIFF_RIGHT = NEW_RIGHT-OLD_RIGHT;
	OLD_RIGHT = NEW_RIGHT;
	if (DISTANCE != 0){DISTANCE--;}

	//	PTBD = PTBD ^ 0B00000010;
}

void revleft (void)
{
        stop();
        reverse();
        stop();
        turnleft();
        stop();
        //forward
}

void revright (void)
{
        stop();
        reverse();
        stop();
        turnright();
        stop();
        //forward
}

void stop (void)
{
        DRIVE = 0;
}

void reverse (void)
{
        DISTANCE = 10;
        DRIVE = REV;
        while (DISTANCE != 0){}
}

void turnleft (void)
{
        DISTANCE = 100;
        DRIVE = ACW;
        while (DISTANCE != 0){}
}

void turnright (void)
{
        DISTANCE = 100;
        DRIVE = CW;
        while (DISTANCE != 0){}
}

void lineleft (void)
{
        DISTANCE = 1;
        DRIVE = SFL;
        while (DISTANCE != 0){}
}

void lineright (void)
{
        DISTANCE = 1;
        DRIVE = SFR;
        while (DISTANCE != 0){}
}

void lineturnleft (void)
{
        DISTANCE = 40;
        DRIVE = ACW;
        while (DISTANCE != 0){}
}

void lineturnright (void)
{
        DISTANCE = 40;
        DRIVE = ACW;
        while (DISTANCE != 0){}
}


void iravoidl (void)
{

        stop();
        turnleft();
        stop();
        //forward
}

void iravoidr (void)
{

        stop();
        turnright();
        stop();
        //forward
}

void speedcon (void){
        
        if (DIFF_RIGHT != DIFF_LEFT) {
                
                if(DIFF_RIGHT>DIFF_LEFT) {
                
                CORR= (DIFF_RIGHT - DIFF_LEFT)/SCALE;
                PW_LEFT = PW_LEFT+CORR;
                if (PW_LEFT>PW_MAX){PW_LEFT=PW_MAX; PW_RIGHT= PW_MIN;} 
                else{PW_RIGHT = PW_RIGHT - CORR;}
                 
                } 
                
                else {
                
                CORR= (DIFF_LEFT - DIFF_RIGHT)/SCALE;
                 
                PW_RIGHT = PW_RIGHT + CORR;
                if (PW_RIGHT>PW_MAX){PW_RIGHT=PW_MAX; PW_LEFT= PW_MIN;} 
                else{PW_LEFT = PW_LEFT - CORR;}
                
                }
	//The next two lines may need to be swapped over
            TPM1C2V=PW_LEFT;
            TPM1C1V=PW_RIGHT;    
        }
        
}        
 
void lcd_init(void){
         
        writecmd(0x38);    //for 8 bit mode
         
        writecmd(0x0C);    //display on, cursor off
         
        writecmd(0x01);    //clear display
         
        writecmd(0x80);    //force cursor to beginning of 1st line
         
        }
 
void writedata(char t){
 
        PTGD_PTGD0 = 1;            
        PTAD = t;                       //Data transfer
        PTGD_PTGD1  = 1;            
        delay(150);
        PTGD_PTGD1  = 0;           
        delay(150);
 
        }
 
void writecmd(int z){
 
        PTGD_PTGD0 = 0;            
        PTAD = z;
        PTGD_PTGD1  = 1;            
        delay(150);
        PTGD_PTGD1  = 0;            
        delay(150);
 
        }
        
void delay(int a){
 
        int i;      
        for(i=0;i<a;i++){   
                }                
        }
 
void distdelay(int a){

        DISTANCE = a;
        while (DISTANCE != 0){}
                        
       }
