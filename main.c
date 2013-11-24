#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#define BAUD 115200
#include <util/setbaud.h>

#define sbi(REG,BIT) (REG |= (1 << BIT))
#define cbi(REG,BIT) (REG &= ~(1 << BIT))

#define setOutput(ddr, pin) ((ddr) |= (1 << (pin)))
#define setLow(port, pin) ((port) &= ~(1 << (pin)))
#define setHigh(port, pin) ((port) |= (1 << (pin)))
#define pulse(port, pin) do { setHigh((port), (pin)); setLow((port), (pin)); } while (0)
#define outputState(port, pin) ((port) & (1 << (pin)))

#define ADC_BUFFER_SIZE 1024

volatile uint16_t adc_counter;
volatile uint8_t adc_data[ADC_BUFFER_SIZE];

volatile int16_t stop_index = -1;
volatile bool freeze = false;
volatile uint8_t comp = 0xFF;

void init_adc();
void init_comparator();
void start_adc();
void stop_adc();

void init_adc()
{
    cbi(ADCSRA, ADEN); // disable adc
    cbi(ADCSRA, ADSC); // stop conversion
    
    cbi(ADMUX, REFS1); // choose AVcc with external cap
    sbi(ADMUX, REFS0); // for adc voltage reference
    sbi(ADMUX, ADLAR); // left adjust adc readings for 8 bit
    ADMUX |= ( 0 & 0x07 ); // choose analog 0 pin
    
    sbi(ADCSRA, ADATE); // enable auto trigger of adc
    sbi(ADCSRA, ADIE); // enable adc interrupt
    
    cbi(ADCSRA, ADPS2); // set prescaler to 4
    sbi(ADCSRA, ADPS1); //
    cbi(ADCSRA, ADPS0); //
    
    cbi(ADCSRB, ACME); // choose AIN1 for comparator
    
    cbi(ADCSRB, ADTS2); // choose free running mode
    cbi(ADCSRB, ADTS1); // for auto trigger adc
    cbi(ADCSRB, ADTS0);
}

void start_adc()
{
    sbi( ADCSRA, ADEN );
    sbi( ADCSRA, ADSC );
}

void stop_adc()
{
    cbi( ADCSRA, ADEN );
    cbi( ADCSRA, ADSC );
}

void init_comparator(void)
{
    cbi( ACSR, ACD ); // turn on comparator
    cbi( ACSR, ACBG ); // choose digital pin 7 for comparator
    cbi( ACSR, ACIE ); // disable interrupt
    
    cbi( ACSR, ACIC ); // disable input capture interrupt
}

ISR(ADC_vect)
{
    adc_data[adc_counter] = ADCH; // read adc
    if (freeze) {
        freeze = false;
        stop_index = -1;
    } else if (adc_counter == stop_index) {
        freeze = true;
    } else if ( comp == 0 && (ACSR & (1 << ACO)) && stop_index < 0) {
        stop_index = ((adc_counter + (ADC_BUFFER_SIZE >> 1) ) & 0x03FF);
    }
    adc_counter = (( adc_counter + 1 ) & 0x03FF); // increment adc counter
    comp = (comp << 1); // push next comparator value
    comp += (ACSR & (1 << ACO));
}

void uart_init() {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    
#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif
    
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8 bit
    UCSR0B = _BV(RXEN0) | _BV(TXEN0); // enable rx and tx
}

void uart_putchar(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0); // wait until reg empty
    UDR0 = c;
}

char uart_getchar() {
    loop_until_bit_is_set(UCSR0A, RXC0); // wait for byte
    return UDR0;
}

void init_pwm()
{
    setOutput(DDRD, PORTD3);
    sbi(TCCR2A, COM2B1);
    sbi(TCCR2A, WGM20);
    sbi(TCCR2B, CS22);
    OCR2B = 128;
}

void read_cmd()
{
    while (!freeze); // wait for the adc to freeze
    stop_adc();
    
    int i;
    for (i = 0; i < ADC_BUFFER_SIZE - adc_counter; i++)
        uart_putchar(((char *)adc_data)[adc_counter + i]);
    for (i = 0; i < adc_counter; i++)
        uart_putchar(((char *)adc_data)[i]);
    
    start_adc();
}

void trigger_val_cmd()
{
    uint8_t val = 0;
    val += ((uart_getchar() - 48) * 100);
    val += ((uart_getchar() - 48) * 10);
    val += ((uart_getchar() - 48) * 1);
    OCR2B = val;
}

int main() {
    
    uart_init();

    sei(); // enable interrupts

    init_adc();
    init_comparator();
    init_pwm();
    
    start_adc();
    
    while (true) {
       
        char command = uart_getchar();
        
        switch (command) {
            case 'r':
                read_cmd();
                break;
            case 't':
                trigger_val_cmd();
                break;
        }
        
    }
    
    return 0;
    
}
