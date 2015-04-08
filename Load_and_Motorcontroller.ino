/*
Version 0.5
merge of Loadcontroller and MotorSwitchAndCOntroll Program

*/



#include <Wire.h>
#include <Adafruit_MCP4725.h>
//#include <math.h>

String version = "0.5";


//
#define FASTADC 1
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif



boolean debug = false;
boolean MotorMode = false;


int readA0;
int readA1;
int readA2;



long Number;
int readAnalog;
int resistance = 200;
int I_set;

String msg = "HH";


const long buad = 9600;

Adafruit_MCP4725 dac;
float V_ref;


//pin setup
int BRK = 8;
int DIR = 7;
int PWM_out = 9;
int VPROPI = A3;		// current sense pin 500mV/1A
int motorSwitch = 5;	// Set Pin HIGH for Motormode; LOW for Generatormode
int Sleep=4;


/*
* Functions for the Loadcontroller part
*/

float Vref()
{ //Vin=3.001; Vref=Vin*1024/Vbit
	return analogRead(A3);
}



void setCurrent_R(int res)
{
	//Version 1
	//readAnalog = map(analogRead(A0),0,1023,0,4095);
	//res=constrain(res,0,200);
	//int set = constrain(round(readAnalog/res),0,4095);
	//dac.setVoltage(set,false);
	//if (debug)
	//{
	//Serial.print(readAnalog);
	//Serial.print(", ");
	//Serial.print(res);
	//Serial.print(", ");
	//Serial.println(set);
	
	//correct version
	
	int set;
	float Voltage =analogRead(A0);
	
	
	if (res)
	{
		set = constrain((int)round(4096*Voltage/(Vref()*(float)res)),0,4095);
	}
	else
	{
		set = 0;
	}
	if (debug)
	{
		Serial.print("I_set_R: ");
		Serial.println(set);
	}
	dac.setVoltage(set,false);
	
	//}
}


/*
* Functions for the Motorcontroller part
*/

void Brake_active()
{
	digitalWrite(BRK,HIGH);
	
}

void Brake_deactive()
{
	digitalWrite(BRK,LOW);
}


void forward(int speed)
{
	Brake_active();					//Set Brake for better Performance according to DRV8801 Truth Table on page: https://www.pololu.com/product/2136
	digitalWrite(DIR,HIGH);
	analogWrite(PWM_out,speed);
}

void backward(int speed)
{
	Brake_active();					//Set Brake for better Performance according to DRV8801 Truth Table on page: https://www.pololu.com/product/2136
	digitalWrite(DIR,LOW);
	analogWrite(PWM_out,speed);
}

void Brake()
{
	
	digitalWrite(PWM_out,LOW);
	//backward(Brakespeed);
	Brake_active();
}

void Brake_release()
{
	digitalWrite(PWM_out,LOW);
	Brake_deactive();
}

void SleepMode(boolean Mode)
{
	if (Mode)
	{
		digitalWrite(Sleep,LOW);
	}
	else
	{
		digitalWrite(Sleep,HIGH);
		delay(2);
	}
}

void switchOff()
{
	backward(0);
	Brake_release();
	SleepMode(true);
	digitalWrite(motorSwitch,LOW);
}


/*
* Arduino Setup
*/


void setup()
{
	#if FASTADC
	// set prescale to 16
	sbi(ADCSRA,ADPS2);
	cbi(ADCSRA,ADPS1);
	cbi(ADCSRA,ADPS0);
	#endif
	
	// Motorcontroller
	TCCR1B = TCCR1B & 0b11111000 | 0x01; // sets Timer1 (Pin 9 and 10) to 31300Hz; source: http://playground.arduino.cc/Main/TimerPWMCheatsheet
	pinMode(BRK,OUTPUT);
	pinMode(DIR,OUTPUT);
	pinMode(PWM_out,OUTPUT);
	pinMode(motorSwitch,OUTPUT);
	pinMode(Sleep,OUTPUT);
	SleepMode(true);
	
	
	//Loadcontroller

	TWBR = 12; // set I2C Speed to 400 kHz

	dac.begin(0x60);
	
	
	


	Serial.begin(buad);
	Serial.println("Loadcontroller, Motorswitch and Control. Ready");
	//Serial.println("Input Speed only from 0 to 128");
	Serial.print("Version: ");
	Serial.println(version);	
	Serial.println("Ready");


}



/*
* Main Function
*/



void loop()
{

	if(Serial.available()>0)
	{
		msg="";
		while(Serial.available()>0)
		{
			msg+=char(Serial.read());
			delay(10);
		}
		
		
	}
	Number = msg.substring(1).toInt();
	if(msg.substring(1).length()>1&&(Number<0))
	{
		Number=4095;				//error handling of to large or low numbers from input
	}
	
	msg = msg.substring(0,1);
	msg.toUpperCase();
	
	if (msg.equals("D"))
	{		
		debug=!debug;
		Serial.println(debug);
		msg="";
		if (debug)
		{
			Serial.print("Number: ");
			Serial.print(Number);
			Serial.print(", msg: ");
			Serial.println(msg);			
		}
	}
	else if (msg.equals("I"))
	{
		float current = Number;
		I_set = constrain(round(current*1365/1000),0,4095);
		if (debug)
		{
			Serial.print("I_Set_I: ");
			Serial.println(I_set);
			
		}
		Serial.println(".");
		//dac.setVoltage(I_set,false);
		//msg="";
	}
	//else if (msg.equals("R")) //maybe O for ohm
	//{
		//resistance = Number;
		//
		//Serial.println(".");
		////msg="";
		//
	//}
	else if (msg.equals("M"))
	{
		SleepMode(false);
		MotorMode=true;
		digitalWrite(motorSwitch,HIGH);
		int Motorspeed=constrain(Number,0,255);
		
		if (Motorspeed!=0)
		{
			Brake_active();
			forward(Motorspeed);
			//last_MotorSpeed=Motorspeed;
		}
		else
		{
			//Brake(last_MotorSpeed);
			//last_MotorSpeed=0;
			Brake();
		}
		
		if(debug&&(MotorMode==true))
		{
			Serial.print("Motorspeed: ");
			Serial.println(Motorspeed);
			Serial.println(("Motormode On."));
			
		}
		else if (debug&&(MotorMode==false))
		{
			Serial.println(("Motormode Off."));
		}
		msg="";
	}
	else if(msg.equals("B"))
	{
		SleepMode(false);
		MotorMode=true;
		digitalWrite(motorSwitch,HIGH);
		int Motorspeed=constrain(Number,0,255);
		
		if (Motorspeed!=0)
		{
			Brake_active();
			backward(Motorspeed);
			//last_MotorSpeed=Motorspeed;
		}
		else
		{
			//Brake(last_MotorSpeed);
			//last_MotorSpeed=0;
			Brake();
		}
		msg="";
	}
	else if (msg.equals("R"))
	{
		Brake_release();
	}
	else if (msg.equals("G"))
	{
		switchOff();
		MotorMode=false;
	}
	else if(msg.equals("H"))
	{
		Serial.println("");
		Serial.println("+++++++++++++++++++++++++++++++++++++");
		Serial.println("+ Load- and Motorcontroller/-switch +");
		Serial.print("+ Firmeware Version: ");
		Serial.print(version);
		Serial.println("            +");
		Serial.println("+++++++++++++++++++++++++++++++++++++");
		Serial.println("");		
		Serial.println("HH    : This Help");
		Serial.println("");
		Serial.println("   Commands for Loadcontroller:");
		Serial.println("Ixxxx : Sets the load to the current xxxx in mA (0 < xxxx < 3000)");
		Serial.println("");
		Serial.println("   Commands for Motorcontroller:");
		Serial.println("Mxxx  : changes to Motormode and sets a Motorspeed Value of xxx [(0 < xxx < 255) = 0% to 100% Motorspeed]");
		Serial.println("Bxxx  : reverses the Motor and sets it to a speed of xxx (Values and remarks the same like Mxxx)");
		Serial.println("R     : releases the brake, stays in Motormode");
		Serial.println("G     : change state to Generatormode; sets the Motorcontroller into sleep mode");
		
		Serial.println("EOH");
		
		
		
	}

	if (debug&&(msg.length()>0))
	{
		Serial.print("Resistance: ");
		Serial.println(resistance);
		Serial.print("Current: ");
		Serial.println(I_set);
		msg="";
		
	}
	
	
	//setCurrent_R(resistance);
	dac.setVoltage(I_set,false);
	//Serial.println("");
	
	msg="";
}





