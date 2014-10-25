#include <msp430f5529.h>
#include "ws2812.h"
#include "driverlib.h"

#define NUMBEROFLEDS	60
#define ENCODING 		3		// possible values 3 and 4

void configClock(void);
void configSPI(void);
void ws_set_colors_blocking(uint8_t* buffer, ledcount_t ledCount);
void ws_set_colors_dma(uint8_t* buffer, ledcount_t ledCount);
void ws_rotate(ledcolor_t* leds, ledcount_t ledCount);

volatile uint16_t band_readings[7] = {0};
volatile uint16_t band_strengths[7] = {0};
volatile uint16_t max_reading = 10;
volatile uint8_t  cur_band = 0;
volatile uint8_t  f_new_full_reading = 0;

volatile uint16_t signal_min = 4096;
volatile uint16_t signal_max = 0;
volatile uint16_t signal_amp = 0;

int main(void) {
	// buffer to store encoded transport data
	uint8_t frameBuffer[(ENCODING * sizeof(ledcolor_t) * NUMBEROFLEDS)] = { 0, };
	WDT_A_hold(WDT_A_BASE);

	ledcolor_t leds[NUMBEROFLEDS] = {
			// rainbow colors
			{ 0x19, 0x2f, 0x3 },
			{ 0x21, 0x2a, 0x0 },
			{ 0x27, 0x24, 0x0 },
			{ 0x2d, 0x1d, 0x1 },
			{ 0x31, 0x15, 0x5 },
			{ 0x32, 0xe, 0xb },
			{ 0x32, 0x8, 0x12 },
			{ 0x2f, 0x3, 0x19 },
			{ 0x2a, 0x0, 0x21 },
			{ 0x24, 0x0, 0x28 },
			{ 0x1d, 0x2, 0x2d },
			{ 0x15, 0x5, 0x31 },
			{ 0xe, 0xb, 0x32 },
			{ 0x8, 0x12, 0x32 },
			{ 0x3, 0x19, 0x2f },
			{ 0x0, 0x21, 0x2a },
	};

	uint8_t update;
	ledcolor_t blankLed = {0x00, 0x00, 0x00};
	uint8_t colorIdx;
	ledcolor_t led;

	configClock();
	configSPI();

	// Setup the timer ////////////////////////////////////////////////////////

	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

	TIMER_A_clearTimerInterruptFlag(TIMER_A0_BASE);

	TIMER_A_configureUpMode(TIMER_A0_BASE, TIMER_A_CLOCKSOURCE_ACLK,
			TIMER_A_CLOCKSOURCE_DIVIDER_1, 500, TIMER_A_TAIE_INTERRUPT_ENABLE,
			TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE, TIMER_A_DO_CLEAR);

	TIMER_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

	// Setup the ADC //////////////////////////////////////////////////////////
	//Enable A/D channel A0
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6,
			GPIO_PIN0
	);

	//Initialize the ADC12_A Module
	/*
	 * Base address of ADC12_A Module
	 * Use internal ADC12_A bit as sample/hold signal to start conversion
	 * USE MODOSC 5MHZ Digital Oscillator as clock source
	 * Use default clock divider of 1
	 */
	ADC12_A_init(ADC12_A_BASE,
			ADC12_A_SAMPLEHOLDSOURCE_SC,
			ADC12_A_CLOCKSOURCE_ADC12OSC,
			ADC12_A_CLOCKDIVIDER_1);

	ADC12_A_enable(ADC12_A_BASE);

	/*
	 * Base address of ADC12_A Module
	 * For memory buffers 0-7 sample/hold for 256 clock cycles
	 * For memory buffers 8-15 sample/hold for 4 clock cycles (default)
	 * Enable Multiple Sampling
	 */
	ADC12_A_setupSamplingTimer(ADC12_A_BASE,
			ADC12_A_CYCLEHOLD_256_CYCLES,
			ADC12_A_CYCLEHOLD_4_CYCLES,
			ADC12_A_MULTIPLESAMPLESENABLE);

	//Configure Memory Buffer
	/*
	 * Base address of the ADC12_A Module
	 * Configure memory buffer 0
	 * Map input A0 to memory buffer 0
	 * Vref+ = AVcc
	 * Vref- = AVss
	 * Memory buffer 0 is not the end of a sequence
	 */
	ADC12_A_memoryConfigure(ADC12_A_BASE,
			ADC12_A_MEMORY_0,
			ADC12_A_INPUT_A0,
			ADC12_A_VREFPOS_AVCC,
			ADC12_A_VREFNEG_AVSS,
			ADC12_A_NOTENDOFSEQUENCE);

	//Enable memory buffer 0 interrupt
	ADC12_A_clearInterrupt(ADC12_A_BASE,
			ADC12IFG0);
	ADC12_A_enableInterrupt(ADC12_A_BASE,
			ADC12IE0);

	//Enable/Start first sampling and conversion cycle
	/*
	 * Base address of ADC12_A Module
	 * Start the conversion into memory buffer 0
	 * Use the repeated single-channel
	 */
	ADC12_A_startConversion(ADC12_A_BASE,
			ADC12_A_MEMORY_0,
			ADC12_A_REPEATED_SINGLECHANNEL);

	uint16_t max_amp = 0;

	ledcolor_t blue_led = {0, 0, 63};

	__bis_SR_register(GIE);

	while (1) {

//		led = blankLed;
//		// set all LEDs with the same color and simulate a sunrise
//		for(colorIdx=0; colorIdx < 0xFF; colorIdx++) {
//			led.red = colorIdx + 1;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x1FFFF);
//		}
//		for(colorIdx=0; colorIdx < 0xD0; colorIdx++) {
//			led.green = colorIdx;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x2FFFF);
//		}
//		for(colorIdx=0; colorIdx < 0x50; colorIdx++) {
//			led.blue = colorIdx;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x3FFFF);
//		}
//		__delay_cycles(0xFFFFF);

		if (f_new_full_reading) {
			// Got a new amplitude
			f_new_full_reading = 0;
			if (signal_amp > max_amp)
				max_amp = signal_amp;

			uint8_t light_count = ((NUMBEROFLEDS/2) * signal_amp) / max_amp;
			fillSymmetricalBufferSingleColor(&blue_led, light_count, NUMBEROFLEDS/2, frameBuffer, ENCODING);
			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
			// do stuff...
		}
	}

	/////////////// PWM ///////////////////////////////////////////////////////
	//P2.0 as PWM output
//	GPIO_setAsPeripheralModuleFunctionOutputPin(
//			GPIO_PORT_P2,
//			GPIO_PIN0
//	);
//
//	//Generate PWM - Timer runs in Up-Down mode
//	TIMER_A_generatePWM(TIMER_A1_BASE,
//				TIMER_A_CLOCKSOURCE_SMCLK,
//				TIMER_A_CLOCKSOURCE_DIVIDER_1,
//				max_reading,
//				TIMER_A_CAPTURECOMPARE_REGISTER_1,
//				TIMER_A_OUTPUTMODE_RESET_SET,
//				0
//		);

//    __bis_SR_register(GIE);


//
//	while(1) {
//		// blank all LEDs
//		fillFrameBufferSingleColor(&blankLed, NUMBEROFLEDS, frameBuffer, ENCODING);
//		ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//		__delay_cycles(0x100000);
//
//		// Animation - Part1
//		// set one LED after an other (one more with each round) with the colors from the LEDs array
//		fillFrameBuffer(leds, NUMBEROFLEDS, frameBuffer, ENCODING);
//		for(update=1; update <= NUMBEROFLEDS; update++) {
//			ws_set_colors_dma(frameBuffer, update);
//			__delay_cycles(0xFFFFF);
//		}
//		__delay_cycles(0xFFFFFF);
//
//		// Animation - Part2
//		// shift previous LED pattern
//		for(update=0; update < 15*8; update++) {
//			ws_rotate(leds, NUMBEROFLEDS);
//			fillFrameBuffer(leds, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x7FFFF);
//		}
//
//		// Animation - Part3
//		led = blankLed;
//		// set all LEDs with the same color and simulate a sunrise
//		for(colorIdx=0; colorIdx < 0xFF; colorIdx++) {
//			led.red = colorIdx + 1;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x1FFFF);
//		}
//		for(colorIdx=0; colorIdx < 0xD0; colorIdx++) {
//			led.green = colorIdx;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x2FFFF);
//		}
//		for(colorIdx=0; colorIdx < 0x50; colorIdx++) {
//			led.blue = colorIdx;
//			fillFrameBufferSingleColor(&led, NUMBEROFLEDS, frameBuffer, ENCODING);
//			ws_set_colors_dma(frameBuffer, NUMBEROFLEDS);
//			__delay_cycles(0x3FFFF);
//		}
//		__delay_cycles(0xFFFFF);
//	}
//	return 0;
}

void ws_rotate(ledcolor_t* leds, ledcount_t ledCount) {
	ledcolor_t tmpLed;
	ledcount_t ledIdx;

	tmpLed = leds[ledCount-1];
	for(ledIdx=(ledCount-1); ledIdx > 0; ledIdx--) {
		leds[ledIdx] = leds[ledIdx-1];
	}
	leds[0] = tmpLed;
}

// copy bytes from the buffer to SPI transmit register
// should be reworked to use DMA
void ws_set_colors_blocking(uint8_t* buffer, ledcount_t ledCount) {
	uint16_t bufferIdx;
	for (bufferIdx=0; bufferIdx < (ENCODING * sizeof(ledcolor_t) * ledCount); bufferIdx++) {
		while (!(UCB0IFG & UCTXIFG));		// wait for TX buffer to be ready
		UCB0TXBUF = buffer[bufferIdx];
	}
	__delay_cycles(300);
}

void ws_set_colors_dma(uint8_t* buffer, ledcount_t ledCount) {
	DMA0SA = (__SFR_FARPTR) buffer;		// source address
	DMA0DA = (__SFR_FARPTR) &UCB0TXBUF;	// destination address
	// single transfer, source increment, source and destination byte access, interrupt enable
	DMACTL0 = DMA0TSEL__USCIB0TX; // Trigger DMA on DMA0TSEL__UCB0TXIFG
	DMA0SZ = (ENCODING * sizeof(ledcolor_t) * ledCount);

	DMA0CTL = DMADT_0 | DMADSTINCR_0 | DMASRCINCR_3 | DMADSTBYTE | DMASRCBYTE | DMAEN; //| DMAIE;

	//start DMA by toggling the interrupt flag.
	UCB0IFG ^= UCTXIFG;
	UCB0IFG ^= UCTXIFG;
}

void ws_set_colors_dma_dl(uint8_t* buffer, ledcount_t ledCount) {
	DMA_setSrcAddress(DMA_CHANNEL_0, buffer, DMA_DIRECTION_INCREMENT);
	DMA_setDstAddress(DMA_CHANNEL_0, &UCB0TXBUF, DMA_DIRECTION_UNCHANGED);
	DMA0DA = (__SFR_FARPTR) &UCA0TXBUF;	// destination address
	// single transfer, source increment, source and destination byte access, interrupt enable
	DMACTL0 = DMA0TSEL__USCIB0TX; // Trigger DMA on DMA0TSEL__UCB0TXIFG
	DMA0SZ = (ENCODING * sizeof(ledcolor_t) * ledCount);

	DMA0CTL = DMADT_0 | DMADSTINCR_0 | DMASRCINCR_3 | DMADSTBYTE | DMASRCBYTE | DMAEN; //| DMAIE;

	//start DMA by toggling the interrupt flag.
	UCB0IFG ^= UCTXIFG;
	UCB0IFG ^= UCTXIFG;
}

// configure MCLK and SMCLK to be sourced by DCO with a frequency of
//   8Mhz (3-bit encoding)
// 6.7MHz (4-bit encoding)
void configClock(void) {
	// DCO frequency setting = 8MHz
	UCS_clockSignalInit(UCS_FLLREF, UCS_REFOCLK_SELECT,
		UCS_CLOCK_DIVIDER_1);

	UCS_initFLLSettle(
			8000, // 8000
			244   // 8 MHz / 32KHz
	);

	UCS_clockSignalInit(UCS_ACLK, UCLKSEL__XT1CLK, UCS_CLOCK_DIVIDER_1);

	WDT_A_hold(WDT_A_BASE);

}

void configSPI(void) {

	USCI_A_SPI_masterInit(
			USCI_B0_BASE,
			USCI_A_SPI_CLOCKSOURCE_SMCLK,
			8000000,
			2666666,
			USCI_A_SPI_MSB_FIRST,
			USCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,
			USCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW
	);

	USCI_A_SPI_enable(USCI_B0_BASE);

	GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P3, GPIO_PIN0);

}

uint16_t reading = 0;

#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
	switch (__even_in_range(TA0IV, 0x0E)) {
	case 0x0E: // Flip the light on and off.
		GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
		f_new_full_reading = 1;
		signal_amp = signal_max - signal_min;
		signal_min = 4096;
		signal_max = 0;
		break;
	default: break;
	}
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR(void)
{
        static uint8_t index = 0;

        switch (__even_in_range(ADC12IV, 34)) {
        case  0: break;         //Vector  0:  No interrupt
        case  2: break;         //Vector  2:  ADC overflow
        case  4: break;         //Vector  4:  ADC timing overflow
        case  6:                //Vector  6:  ADC12IFG0
        	reading = ADC12_A_getResults(ADC12_A_BASE,
        		ADC12_A_MEMORY_0);
        	if (reading < signal_min)
        		signal_min = reading;
        	if (reading > signal_max)
        		signal_max = reading;
        case  8: break;         //Vector  8:  ADC12IFG1
        case 10: break;         //Vector 10:  ADC12IFG2
        case 12: break;         //Vector 12:  ADC12IFG3
        case 14: break;         //Vector 14:  ADC12IFG4
        case 16: break;         //Vector 16:  ADC12IFG5
        case 18: break;         //Vector 18:  ADC12IFG6
        case 20: break;         //Vector 20:  ADC12IFG7
        case 22: break;         //Vector 22:  ADC12IFG8
        case 24: break;         //Vector 24:  ADC12IFG9
        case 26: break;         //Vector 26:  ADC12IFG10
        case 28: break;         //Vector 28:  ADC12IFG11
        case 30: break;         //Vector 30:  ADC12IFG12
        case 32: break;         //Vector 32:  ADC12IFG13
        case 34: break;         //Vector 34:  ADC12IFG14
        default: break;
        }
}
