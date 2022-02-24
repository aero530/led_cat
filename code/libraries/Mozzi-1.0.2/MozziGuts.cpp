/*
 * MozziGuts.cpp
 *
 * Copyright 2012 Tim Barrass.
 *
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */

 #if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <util/atomic.h>
#include "MozziGuts.h"
#include "mozzi_config.h" // at the top of all MozziGuts and analog files
#include "mozzi_analog.h"
#include "CircularBuffer.h"
//#include "mozzi_utils.h"

#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
#include "IntervalTimer.h"
#else
#include "TimerZero.h"
#include "TimerOne.h"
#include "FrequencyTimer2.h"
#endif


#if !(F_CPU == 16000000 || F_CPU == 48000000)
#warning "Mozzi has been tested with a cpu clock speed of 16MHz on Arduino and 48MHz on Teensy 3!  Results may vary with other speeds."
#endif

// this seems to get included before mozzi_analog.cpp
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
	ADC *adc; // adc object
	uint8_t teensy_pin;
#endif

/*
ATmega328 technical manual, Section 12.7.4:
The dual-slope operation [of phase correct pwm] has lower maximum operation
frequency than single slope operation. However, due to the symmetric feature
of the dual-slope PWM modes, these modes are preferred for motor control
applications.
Due to the single-slope operation, the operating frequency of the
fast PWM mode can be twice as high as the phase correct PWM mode that use
dual-slope operation. This high frequency makes the fast PWM mode well suited
for power regulation, rectification, and DAC applications. High frequency allows
physically small sized external components (coils, capacitors)..

DAC, that's us!  Fast PWM.

PWM frequency tests
62500Hz, single 8 or dual 16 bits, bad aliasing
125000Hz dual 14 bits, sweet
250000Hz dual 12 bits, gritty, if you're gonna have 2 pins, have 14 bits
500000Hz dual 10 bits, grittier
16384Hz single nearly 9 bits (original mode) not bad for a single pin, but carrier freq noise can be an issue
*/


//-----------------------------------------------------------------------------------------------------------------
// ring buffer for audio output
CircularBuffer <unsigned int> output_buffer; // fixed size 256

//-----------------------------------------------------------------------------------------------------------------

#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
// not storing backups, just turning timer on and off for pause
#else

// to store backups of timer registers so Mozzi can be stopped and pre_mozzi timer values can be restored
static uint8_t pre_mozzi_TCCR0A, pre_mozzi_TCCR0B, pre_mozzi_OCR0A, pre_mozzi_TIMSK0;
static uint8_t pre_mozzi_TCCR1A, pre_mozzi_TCCR1B, pre_mozzi_OCR1A, pre_mozzi_TIMSK1;

#if (AUDIO_MODE == HIFI)
#if defined(TCCR2A)
static uint8_t pre_mozzi_TCCR2A, pre_mozzi_TCCR2B, pre_mozzi_OCR2A, pre_mozzi_TIMSK2;
#elif defined(TCCR2)
static uint8_t pre_mozzi_TCCR2, pre_mozzi_OCR2, pre_mozzi_TIMSK;
#elif defined(TCCR4A)
static uint8_t pre_mozzi_TCCR4A, pre_mozzi_TCCR4B, pre_mozzi_TCCR4C, pre_mozzi_TCCR4D, pre_mozzi_TCCR4E, pre_mozzi_OCR4C, pre_mozzi_TIMSK4;
#endif
#endif


static void backupPreMozziTimer1()
{
	// backup pre-mozzi register values for pausing later
	pre_mozzi_TCCR1A = TCCR1A;
	pre_mozzi_TCCR1B = TCCR1B;
	pre_mozzi_OCR1A = OCR1A;
	pre_mozzi_TIMSK1 = TIMSK1;
}


//-----------------------------------------------------------------------------------------------------------------


// to store backups of mozzi's changes to timer registers so Mozzi can be paused and unPaused
static uint8_t mozzi_TCCR0A, mozzi_TCCR0B, mozzi_OCR0A, mozzi_TIMSK0;
static uint8_t mozzi_TCCR1A, mozzi_TCCR1B, mozzi_OCR1A, mozzi_TIMSK1;

#if (AUDIO_MODE == HIFI)
#if defined(TCCR2A)
static uint8_t mozzi_TCCR2A, mozzi_TCCR2B, mozzi_OCR2A, mozzi_TIMSK2;
#elif defined(TCCR2)
static uint8_t mozzi_TCCR2, mozzi_OCR2, mozzi_TIMSK;
#elif defined(TCCR4A)
static uint8_t mozzi_TCCR4A, mozzi_TCCR4B, mozzi_TCCR4C, mozzi_TCCR4D, mozzi_TCCR4E, mozzi_OCR4C, mozzi_TIMSK4;
#endif
#endif


static void backupMozziTimer1()
{
	// backup mozzi register values for unpausing later
	mozzi_TCCR1A = TCCR1A;
	mozzi_TCCR1B = TCCR1B;
	mozzi_OCR1A = OCR1A;
	mozzi_TIMSK1 = TIMSK1;
}

#endif // end of timer backups for non-Teensy 3 boards
//-----------------------------------------------------------------------------------------------------------------

#if (USE_AUDIO_INPUT==true)

// ring buffer for audio input
CircularBuffer <unsigned int>input_buffer; // fixed size 256

static boolean audio_input_is_available;
static int audio_input; // holds the latest audio from input_buffer
uint8_t adc_count = 0;


int getAudioInput()
{
	return audio_input;
}


static void startFirstAudioADC()
{
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
	adc->startSingleRead(AUDIO_INPUT_PIN); // ADC lib converts pin/channel in startSingleRead
#else
	adcStartConversion(adcPinToChannelNum(AUDIO_INPUT_PIN));
#endif
}

/*
static void receiveFirstAudioADC() 
{
	// nothing
}
*/

static void startSecondAudioADC()
{
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
	adc->startSingleRead(AUDIO_INPUT_PIN);
#else
	ADCSRA |= (1 << ADSC); // start a second conversion on the current channel
#endif
}



static void receiveSecondAudioADC()
{
	if (!input_buffer.isFull()) 
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
		input_buffer.write(adc->readSingle());
#else
		input_buffer.write(ADC);
#endif
}


#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
void adc0_isr(void) 
#else
ISR(ADC_vect, ISR_BLOCK)
#endif
{
	switch (adc_count){
	case 0:
		// 6us
		receiveSecondAudioADC();
		adcReadSelectedChannels();
		break;

	case 1:
		// <2us, <1us w/o receive
		//receiveFirstControlADC();
		startSecondControlADC();
		break;

	case 2:
		// 3us
		receiveSecondControlADC();
		startFirstAudioADC();
		break;


		//      	case 3:
		// invisible
		//      	receiveFirstAudioADC();
		//      	break;

	}
	adc_count++;
}
#endif // end main audio input section


void audioHook() // 2us excluding updateAudio()
{
//setPin13High();
#if (USE_AUDIO_INPUT==true)
		if (!input_buffer.isEmpty()) 
			audio_input = input_buffer.read();
#endif

	if (!output_buffer.isFull()) {
		output_buffer.write((unsigned int) (updateAudio() + AUDIO_BIAS));

	}
//setPin13Low();
}



//-----------------------------------------------------------------------------------------------------------------
#if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS)

#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
//teensy 3 architecture

  IntervalTimer timer1;


static void teensyAudioOutput()
{
	
#if (USE_AUDIO_INPUT==true)
	adc_count = 0;
	startSecondAudioADC();
#endif

	analogWrite(AUDIO_CHANNEL_1_PIN, (int)output_buffer.read());
}


static void startAudioStandard()
{
	//backupPreMozziTimer1(); // not for Teensy 3.1
	
	analogWriteResolution(12);
	adc->setAveraging(0);
	adc->setConversionSpeed(ADC_MED_SPEED); // could be ADC_HIGH_SPEED, noisier
	timer1.begin(teensyAudioOutput, 1000000UL/AUDIO_RATE); 

	//backupMozziTimer1(); // // not for Teensy 3.1
}
	
#else

// avr architecture
static void startAudioStandard()
{
	backupPreMozziTimer1();

	pinMode(AUDIO_CHANNEL_1_PIN, OUTPUT);	// set pin to output for audio
	//	pinMode(AUDIO_CHANNEL_2_PIN, OUTPUT);	// set pin to output for audio
#if (AUDIO_MODE == STANDARD)
	Timer1.initializeCPUCycles(16000000UL/AUDIO_RATE, PHASE_FREQ_CORRECT);		// set period, phase and frequency correct
#else // (AUDIO_MODE == STANDARD_PLUS)
	Timer1.initializeCPUCycles(16000000UL/PWM_RATE, FAST);	// fast mode enables higher PWM rate
#endif
	Timer1.pwm(AUDIO_CHANNEL_1_PIN, AUDIO_BIAS);		// pwm pin, 50% of Mozzi's duty cycle, ie. 0 signal
	//Timer1.pwm(AUDIO_CHANNEL_2_PIN, AUDIO_BIAS);
	TIMSK1 = _BV(TOIE1); 	// Overflow Interrupt Enable (when not using Timer1.attachInterrupt())
	//TIMSK1 |= _BV(TOIE1) | _BV(OCIE1A); // Overflow Interrupt Enable and Output Compare A Match Interrupt Enable
	backupMozziTimer1();
}

/*
// trying to get ac output for piezo
unsigned int output;

//extern unsigned long interruptcounter;
ISR(TIMER1_COMPA_vect){
	//Serial.print(9);
	//interruptcounter++;
	// change polarity of pwm output pins 9 and 10 - one becomes ground, the other becomes max-output
	AUDIO_CHANNEL_1_OUTPUT_REGISTER = 0;//STANDARD_PWM_RESOLUTION;
	//AUDIO_CHANNEL_2_OUTPUT_REGISTER = STANDARD_PWM_RESOLUTION-output;
}
*/

/* Interrupt service routine moves sound data from the output buffer to the
Arduino output register, running at AUDIO_RATE. */

ISR(TIMER1_OVF_vect, ISR_BLOCK)
{

#if (AUDIO_MODE == STANDARD_PLUS) && (AUDIO_RATE == 16384) // only update every second ISR, if lower audio rate
	static boolean alternate;
	alternate = !alternate;
	if(alternate)
	{
#endif

#if (USE_AUDIO_INPUT==true)
		adc_count = 0;
		startSecondAudioADC();
#endif

//if (!output_buffer.isEmpty()) {
/*
output =  output_buffer.read();
AUDIO_CHANNEL_1_OUTPUT_REGISTER = output;
AUDIO_CHANNEL_2_OUTPUT_REGISTER = 0;
*/
	AUDIO_CHANNEL_1_OUTPUT_REGISTER = output_buffer.read();
//}

	// flip signal polarity - instead of signal going to 0 (both pins 0), it goes to pseudo-negative of its current value.
	// this would set non-inverted when setting sample value, and then inverted when top is reached (in an interrupt)
	// non-invert
	//TCCR1A |= _BV(COM1A1);
	// invert
	//TCCR1A |= ~_BV(COM1A1)
	
	
#if (AUDIO_MODE == STANDARD_PLUS) && (AUDIO_RATE==16384) // all this conditional compilation is so clutsy!
	}
#endif

}
// end avr
#endif
// end STANDARD

//-----------------------------------------------------------------------------------------------------------------
#elif (AUDIO_MODE == HIFI)

static void startAudioHiFi()
{
	backupPreMozziTimer1();
	// pwm on timer 1
	pinMode(AUDIO_CHANNEL_1_highByte_PIN, OUTPUT);	// set pin to output for audio, use 3.9k resistor
	pinMode(AUDIO_CHANNEL_1_lowByte_PIN, OUTPUT);	// set pin to output for audio, use 499k resistor
	Timer1.initializeCPUCycles(16000000UL/125000, FAST);		// set period for 125000 Hz fast pwm carrier frequency = 14 bits
	Timer1.pwm(AUDIO_CHANNEL_1_highByte_PIN, 0);		// pwm pin, 0% duty cycle, ie. 0 signal
	Timer1.pwm(AUDIO_CHANNEL_1_lowByte_PIN, 0);		// pwm pin, 0% duty cycle, ie. 0 signal
	backupMozziTimer1();
	// audio output interrupt on timer 2, sets the pwm levels of timer 1
	setupTimer2();
}

/* set up Timer 2 using modified FrequencyTimer2 library */
void dummy(){}


static void backupPreMozziTimer2()
{
	//backup Timer2 register values
#if defined(TCCR2A)
	pre_mozzi_TCCR2A = TCCR2A;
	pre_mozzi_TCCR2B = TCCR2B;
	pre_mozzi_OCR2A = OCR2A;
	pre_mozzi_TIMSK2 = TIMSK2;
#elif defined(TCCR2)
	pre_mozzi_TCCR2 = TCCR2;
	pre_mozzi_OCR2 = OCR2;
	pre_mozzi_TIMSK = TIMSK;
#elif defined(TCCR4A)
	pre_mozzi_TCCR4B = TCCR4A;
	pre_mozzi_TCCR4B = TCCR4B;
	pre_mozzi_TCCR4B = TCCR4C;
	pre_mozzi_TCCR4B = TCCR4D;
	pre_mozzi_TCCR4B = TCCR4E;
	pre_mozzi_OCR4C = OCR4C;
	pre_mozzi_TIMSK4 = TIMSK4;
#endif
}



static void backupMozziTimer2()
{
#if defined(TCCR2A)
	mozzi_TCCR2A = TCCR2A;
	mozzi_TCCR2B = TCCR2B;
	mozzi_OCR2A = OCR2A;
	mozzi_TIMSK2 = TIMSK2;
#elif defined(TCCR2)
	mozzi_TCCR2 = TCCR2;
	mozzi_OCR2 = OCR2;
	mozzi_TIMSK = TIMSK;
#elif defined(TCCR4A)
	mozzi_TCCR4B = TCCR4A;
	mozzi_TCCR4B = TCCR4B;
	mozzi_TCCR4B = TCCR4C;
	mozzi_TCCR4B = TCCR4D;
	mozzi_TCCR4B = TCCR4E;
	mozzi_OCR4C = OCR4C;
	mozzi_TIMSK4 = TIMSK4;
#endif
}


// audio output interrupt on timer 2 (or 4 on ATMEGA32U4 cpu), sets the pwm levels of timer 1
static void setupTimer2()
{
	backupPreMozziTimer2(); // to reset while pausing
	
	FrequencyTimer2::setPeriodCPUCycles(16000000UL/AUDIO_RATE);
	FrequencyTimer2::setOnOverflow(dummy);
	FrequencyTimer2::enable();

	// backup mozzi register values for unpausing later
	backupMozziTimer2();
}



#if defined(TIMER2_COMPA_vect)
ISR(TIMER2_COMPA_vect)
#elif defined(TIMER2_COMP_vect)
ISR(TIMER2_COMP_vect)
#elif defined(TIMER4_COMPA_vect)
ISR(TIMER4_COMPA_vect)
#else
#error "This board does not have a hardware timer which is compatible with FrequencyTimer2"
void dummy_function(void)
#endif
{
#if (USE_AUDIO_INPUT==true)
	adc_count = 0;
	startSecondAudioADC();
#endif

		// read about dual pwm at http://www.openmusiclabs.com/learning/digital/pwm-dac/dual-pwm-circuits/
		// sketches at http://wiki.openmusiclabs.com/wiki/PWMDAC,  http://wiki.openmusiclabs.com/wiki/MiniArDSP
		//if (!output_buffer.isEmpty()){
		unsigned int out = output_buffer.read();
		// 14 bit, 7 bits on each pin
		//AUDIO_CHANNEL_1_highByte_REGISTER = out >> 7; // B00111111 10000000 becomes B1111111
		// try to avoid looping over 7 shifts - need to check timing or disassemble to see what really happens
		unsigned int out_high = out<<1; // B00111111 10000000 becomes B01111111 00000000
		AUDIO_CHANNEL_1_highByte_REGISTER = out_high >> 8; // B01111111 00000000 produces B01111111
		//
		AUDIO_CHANNEL_1_lowByte_REGISTER = out & 127; 
		//}
}

//  end of HIFI

#endif


//-----------------------------------------------------------------------------------------------------------------

static void updateControlWithAutoADC()
{
	updateControl();
	/*
	#if (USE_AUDIO_INPUT==true)
		adc_count = 0;
		startSecondAudioADC();
#endif
*/
	adcStartReadCycle();
}


/* Sets up Timer 0 for control interrupts. This is the same for all output
options Using Timer0 for control disables Arduino's time functions but also
saves on the interrupts and blocking action of those functions. May add a config
option for Using Timer2 instead if needed. (MozziTimer2 can be re-introduced for
that). */
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
IntervalTimer timer0;
#endif


static void startControl(unsigned int control_rate_hz)
{
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
	timer0.begin(updateControlWithAutoADC, 1000000/control_rate_hz);
#else
	// backup pre-mozzi register values
	pre_mozzi_TCCR0A = TCCR0A;
	pre_mozzi_TCCR0B = TCCR0B;
	pre_mozzi_OCR0A = OCR0A;
	pre_mozzi_TIMSK0 = TIMSK0;

	TimerZero::init(1000000/control_rate_hz,updateControlWithAutoADC); // set period, attach updateControlWithAutoADC()
	TimerZero::start();

	// backup mozzi register values for unpausing later
	mozzi_TCCR0A = TCCR0A;
	mozzi_TCCR0B = TCCR0B;
	mozzi_OCR0A = OCR0A;
	mozzi_TIMSK0 = TIMSK0;
#endif
}


void startMozzi(int control_rate_hz)
{
	setupMozziADC(); // you can use setupFastAnalogRead() with FASTER or FASTEST in setup() if desired (not for Teensy 3.1)
	// delay(200); // so AutoRange doesn't read 0 to start with
	startControl(control_rate_hz);
#if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS)
	startAudioStandard();
#elif (AUDIO_MODE == HIFI)
	startAudioHiFi();
#endif
}


void pauseMozzi(){
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
	timer1.end();
#else
	// restore backed up register values
	TCCR0A = pre_mozzi_TCCR0A;
	TCCR0B = pre_mozzi_TCCR0B;
	OCR0A = pre_mozzi_OCR0A;
	TIMSK0 = pre_mozzi_TIMSK0;

	TCCR1A = pre_mozzi_TCCR1A;
	TCCR1B = pre_mozzi_TCCR1B;
	OCR1A = pre_mozzi_OCR1A;
	TIMSK1 = pre_mozzi_TIMSK1;

#if (AUDIO_MODE == HIFI)
#if defined(TCCR2A)
	TCCR2A = pre_mozzi_TCCR2A;
	TCCR2B = pre_mozzi_TCCR2B;
	OCR2A = pre_mozzi_OCR2A;
	TIMSK2 = pre_mozzi_TIMSK2;
#elif defined(TCCR2)
	TCCR2 = pre_mozzi_TCCR2;
	OCR2 = pre_mozzi_OCR2;
	TIMSK = pre_mozzi_TIMSK;
#elif defined(TCCR4A)
	TCCR4B = pre_mozzi_TCCR4A;
	TCCR4B = pre_mozzi_TCCR4B;
	TCCR4B = pre_mozzi_TCCR4C;
	TCCR4B = pre_mozzi_TCCR4D;
	TCCR4B = pre_mozzi_TCCR4E;
	OCR4C = pre_mozzi_OCR4C;
	TIMSK4 = pre_mozzi_TIMSK4;
#endif
#endif	
#endif	
}


void unPauseMozzi()
{
#if defined(__MK20DX128__) || defined(__MK20DX256__) // teensy 3, 3.1
  timer1.begin(teensyAudioOutput, 1000000UL/AUDIO_RATE); 
#else
	// restore backed up register values
	TCCR0A = mozzi_TCCR0A;
	TCCR0B = mozzi_TCCR0B;
	OCR0A = mozzi_OCR0A;
	TIMSK0 = mozzi_TIMSK0;

	TCCR1A = mozzi_TCCR1A;
	TCCR1B = mozzi_TCCR1B;
	OCR1A = mozzi_OCR1A;
	TIMSK1 = mozzi_TIMSK1;

#if (AUDIO_MODE == HIFI)
#if defined(TCCR2A)
	TCCR2A = mozzi_TCCR2A;
	TCCR2B = mozzi_TCCR2B;
	OCR2A = mozzi_OCR2A;
	TIMSK2 = mozzi_TIMSK2;
#elif defined(TCCR2)
	TCCR2 = mozzi_TCCR2;
	OCR2 = mozzi_OCR2;
	TIMSK = mozzi_TIMSK;
#elif defined(TCCR4A)
	TCCR4B = mozzi_TCCR4A;
	TCCR4B = mozzi_TCCR4B;
	TCCR4B = mozzi_TCCR4C;
	TCCR4B = mozzi_TCCR4D;
	TCCR4B = mozzi_TCCR4E;
	OCR4C = mozzi_OCR4C;
	TIMSK4 = mozzi_TIMSK4;
#endif
#endif
#endif
}


unsigned long audioTicks()
{
	return output_buffer.count();
}


unsigned long mozziMicros()
{
	return audioTicks() * MICROS_PER_AUDIO_TICK;
}



// Unmodified TimerOne.cpp has TIMER3_OVF_vect.
// Watch out if you update the library file.
// The symptom will be no sound.
// ISR(TIMER1_OVF_vect)
// {
// 	Timer1.isrCallback();
// }
