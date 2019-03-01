/*



Test Programme for the AW60 micromouse board


This version gives direction control of the two motors but with interrupts to
give PWM speed control. Inputs from the Hall Effect tachometers are used to
adjust the motor speeds so that the machine travels in a straight line.

Three interrupts are used to PWM the motors: the timer overflow for TPM1 timer,
which sets the overflow frequency of 100 Hz with bus clock rate 2MHz, turns both
motors on. TPM1 channel 1 and TPM1 channel 2 turn off the left and right motors.
Variables PWM_LEFT and PWM_RIGHT are the "instantaneous" values of PWM.

Two interrupts are used to measure the speed of the left and right motors.
Consecutive TPM2 channel 0 and channel 1 interrupts are subtracted to give
two variables DIFF_LEFT and DIFF_RIGHT which are used by the procedure "speedcon".


The main programme performs a simple avoidance action, using active low inputs from
the touch bars and active high inputs from the IR sensors.

Variable COUNTER is a 16-bit integer which is decremented every 10 ms.
Variable DISTANCE is a 16-bit integer which is decremented after every
tachometer pulse. One mm of travel is approximately 4 units in DISTANCE.



*/

#include <hidef.h>      // for EnableInterrupts macro
#include "derivative.h" // include peripheral declarations
#include <stdio.h>

#define TBFL PTAD_PTAD1

#define STOP  0b00000000
#define FWD   0b00000101
#define REV   0b00001010
#define ACW   0b00001001
#define CW    0b00000110
#define SFL   0b00000001 //Swerve forward and left
#define SFR   0b00000100 //Swerve forward and right

#define bitClear 0
#define SCALE 1


void revleft (void);
void revright (void);
void stop (void);
void reverse (void);
void turnleft (void);
void turnright (void);
void lineleft (void);
void lineright (void);
void lineturnleft (void);
void iravoidl (void);
void iravoidr (void);
void speedcon (void);

void lcd_init(void);
void writecmd(int);
void writedata(char);
void delay(int);

//The motors are connected to Port F bits 0 to 3 in pairs.
//Sense of switching is 01 = forward, 10 = reverse,
//00 = brake, 11 = freewheel.

byte DRIVE;
byte MODCNTH = 0x4E, MODCNTL = 0x20;            // values for the modulo registers (0x4E20)

byte INIT_CHNCNTH = 0x40, INIT_CHNCNTL = 0x00;  // set the IOC register to 0x3000 for output compare function

byte lcdadc1[2], lcdadc2[2], lcdadc3[2], lcdadc4[2], lcdadc5[2];

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

int adc1, adc2, adc3, adc4, adc5, intcounter, select, preselect;

void main(void)
{
	
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

	//	PTBDD = 0b11111111;
	//	PTBD = 0;
	
        select = 0;
        preselect = 2;
        intcounter = 5;
        
        PTADD = 0xFF;     //LCD pins
        PTGDD = 0x07;     //LCD read/enable
        ADC1CFG = 0x70;   //white line sensor
       
        lcd_init();
        
        delay(50000);
        delay(50000);
        delay(50000);
        
        writecmd(0x81); //moves cursor
        writedata('I');
        writedata('N');
        writedata('D');
        writedata('I');
        writedata('A');
        
        while((PTDD & 0b00000100) != 0){
        
        if ((PTDD & 0b00001000) == 0){select++;}
        if (preselect != select){
        if (select > 2){select = 0;}
        preselect = select;        
        switch (select){
                
                case 0:
                writecmd(0xC1);
                writedata('A');
                writedata('V');
                writedata('O');
                writedata('I');
                writedata('D');
                writedata(' ');
                        break;
                case 1:
                writecmd(0xC1);
                writedata('L');
                writedata('I');
                writedata('N');
                writedata('E');
                writedata(' ');
                        break;
                case 2:
                writecmd(0xC1);
                writedata('B');
                writedata('A');
                writedata('T');
                writedata('T');
                writedata('L');
                writedata('E');
                        break;
        }
        while((PTDD & 0b00001000) == 0){}
        }
        }
        
    EnableInterrupts;   // enable interrupts
  
    switch (select){
    
        case 0:
        
        case 1:
    for(;;) {
 
	DRIVE = FWD;

        if (adc2 > 50){lineright();}
        if (adc4 > 50){lineleft();}
                
        if ((adc1 & adc2 & adc3 & adc4 & adc5) > 50){lineturnleft();}
        
	if ((PTDD & 0b00001100) != 0b00001100){
		if ((PTDD & 0b00001000) == 0){revleft();}
		if ((PTDD & 0b00000100) == 0){revright();}
	}
	if ((PTDD & 0b11000000) != 0){
		if ((PTDD_PTDD7== 0)&(PTDD_PTDD6==1)){iravoidl();}
		if ((PTDD_PTDD7== 1)&(PTDD_PTDD6==0)){iravoidr();}

	}

    }   // loop forever
                break;
        case 2:
        for(;;){
	DRIVE = FWD;
	
	if(adc1 < 50){iravoidr();}
	if(adc5 < 50){iravoidl();}
	//if((adc1 & adc2 & adc3 & adc4 & adc5) < 50){
	//        if (adc1>adc5){revleft();}else{revright();}
	//}
        }
                break;
    }

}

interrupt 11 void TPM1SC_overflow()
{   // interrupt vector: Vtpm1

         TPM1SC_TOF = bitClear;

         PTFD = DRIVE;       // turn on motors as configured by DRIVE (port A switches).
         
         speedcon();

         if (COUNTER != 0){COUNTER--;}
         
        intcounter--;
         
        TPM1SC_TOF = 0;       
        ADC1SC1 = 0;
        while (ADC1SC1_COCO == 0){  
        }
        adc1 = ADC1RL*100/257;
        
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
         
        if(intcounter == 1){
        intcounter = 50;       
 
        sprintf(lcdadc1,"%1d",adc1);
        sprintf(lcdadc2,"%1d",adc2);
        sprintf(lcdadc3,"%1d",adc3);
        sprintf(lcdadc4,"%1d",adc4);
        sprintf(lcdadc5,"%1d",adc5);
        
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
        writedata('%');
        
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
        DISTANCE = 200;
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
