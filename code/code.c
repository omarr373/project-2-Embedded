#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile int seconds = 0;
volatile int minutes = 0;
volatile int hours = 0;
volatile int paused = 0;
volatile int mode = 0;

void INT0_Init(void) {
    DDRD &= ~(1 << PD2);
    MCUCR |= (1 << ISC01);
    GICR |= (1 << INT0);
}

void INT1_Init(void) {
    DDRD &= ~(1 << PD3);
    MCUCR |= (1 << ISC11) | (1 << ISC10);
    GICR |= (1 << INT1);
}

void INT2_Init(void) {
    DDRB &= ~(1 << PB2);
    MCUCSR &= ~(1 << ISC2);
    GICR |= (1 << INT2);
}

ISR(INT0_vect) {
    seconds = 0;
    minutes = 0;
    hours = 0;
}

ISR(INT1_vect) {
    paused = 1;
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

ISR(INT2_vect) {
    paused = 0;
    TCCR1B |= (1 << CS12) | (1 << CS10);
}

void Timer1_Init(void) {
    TCCR1A |= (1 << FOC1A);
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
    OCR1A = 15624;
    TIMSK |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
    if (!paused) {
        if (mode == 0) {
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                    if (hours >= 24) {
                        hours = 0;
                    }
                }
            }
        } else {
            if (seconds == 0 && minutes == 0 && hours == 0) {
                PORTD |= (1 << PD0);
            } else {
                seconds--;
                if (seconds < 0) {
                    seconds = 59;
                    minutes--;
                    if (minutes < 0) {
                        minutes = 59;
                        hours--;
                        if (hours < 0) {
                            hours = 0;
                        }
                    }
                }
            }
        }
    }
}

void Adjust_Hours(void) {
    static uint8_t last_state_PB1 = 1;
    static uint8_t last_state_PB0 = 1;
    uint8_t current_state_PB1 = PINB & (1 << PB1);
    uint8_t current_state_PB0 = PINB & (1 << PB0);

    if (paused) {
        if (!current_state_PB1 && last_state_PB1) {
            _delay_ms(20);
            if (!(PINB & (1 << PB1))) {
                hours = (hours + 1) % 24;
                PORTD &= ~(1 << PD0);
            }
        }
        if (!current_state_PB0 && last_state_PB0) {
            _delay_ms(20);
            if (!(PINB & (1 << PB0))) {
                hours = (hours == 0) ? 23 : hours - 1;
                PORTD &= ~(1 << PD0);
            }
        }
    }
    last_state_PB1 = current_state_PB1;
    last_state_PB0 = current_state_PB0;
}

void Adjust_Minutes(void) {
    static uint8_t last_state_PB4 = 1;
    static uint8_t last_state_PB3 = 1;
    uint8_t current_state_PB4 = PINB & (1 << PB4);
    uint8_t current_state_PB3 = PINB & (1 << PB3);

    if (paused) {
        if (!current_state_PB4 && last_state_PB4) {
            _delay_ms(20);
            if (!(PINB & (1 << PB4))) {
                minutes = (minutes + 1) % 60;
                PORTD &= ~(1 << PD0);
            }
        }
        if (!current_state_PB3 && last_state_PB3) {
            _delay_ms(20);
            if (!(PINB & (1 << PB3))) {
                minutes = (minutes == 0) ? 59 : minutes - 1;
                PORTD &= ~(1 << PD0);
            }
        }
    }
    last_state_PB4 = current_state_PB4;
    last_state_PB3 = current_state_PB3;
}

void Adjust_Seconds(void) {
    static uint8_t last_state_PB6 = 1;
    static uint8_t last_state_PB5 = 1;
    uint8_t current_state_PB6 = PINB & (1 << PB6);
    uint8_t current_state_PB5 = PINB & (1 << PB5);

    if (paused) {
        if (!current_state_PB6 && last_state_PB6) {
            _delay_ms(20);
            if (!(PINB & (1 << PB6))) {
                seconds = (seconds + 1) % 60;
                PORTD &= ~(1 << PD0);
            }
        }
        if (!current_state_PB5 && last_state_PB5) {
            _delay_ms(20);
            if (!(PINB & (1 << PB5))) {
                seconds = (seconds == 0) ? 59 : seconds - 1;
                PORTD &= ~(1 << PD0);
            }
        }
    }
    last_state_PB6 = current_state_PB6;
    last_state_PB5 = current_state_PB5;
}

void Change_Mode(void) {
    static uint8_t last_state_PB7 = 1;
    uint8_t current_state_PB7 = PINB & (1 << PB7);

    if (paused) {
        if (!current_state_PB7 && last_state_PB7) {
            _delay_ms(20);
            if (!(PINB & (1 << PB7))) {
                mode = !mode;
                if (mode == 0) {
                    PORTD |= (1 << PD4);
                    PORTD &= ~(1 << PD5);
                } else {
                    PORTD |= (1 << PD5);
                    PORTD &= ~(1 << PD4);
                }
            }
        }
    }
    last_state_PB7 = current_state_PB7;
}

void Display_Time(void) {
    int h1 = hours / 10;
    int h2 = hours % 10;
    int m1 = minutes / 10;
    int m2 = minutes % 10;
    int s1 = seconds / 10;
    int s2 = seconds % 10;

    int bcd_values[10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

    #define DISPLAY_DELAY 1

    PORTA = 0x01;
    PORTC = bcd_values[h1];
    _delay_ms(DISPLAY_DELAY);

    PORTA = 0x02;
    PORTC = bcd_values[h2];
    _delay_ms(DISPLAY_DELAY);

    PORTA = 0x04;
    PORTC = bcd_values[m1];
    _delay_ms(DISPLAY_DELAY);

    PORTA = 0x08;
    PORTC = bcd_values[m2];
    _delay_ms(DISPLAY_DELAY);

    PORTA = 0x10;
    PORTC = bcd_values[s1];
    _delay_ms(DISPLAY_DELAY);

    PORTA = 0x20;
    PORTC = bcd_values[s2];
    _delay_ms(DISPLAY_DELAY);
}

int main(void) {
    DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD0);
    DDRA |= 0x3F;
    DDRC |= 0x0F;
    DDRD &= ~((1 << PD2) | (1 << PD3));
    DDRB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5) | (1 << PB6) | (1 << PB7));
    PORTB |= ((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5) | (1 << PB6) | (1 << PB7));
    PORTD |= (1 << PD2);
    PORTD |= (1 << PD4);
    PORTD &= ~(1 << PD5);

    INT0_Init();
    INT1_Init();
    INT2_Init();
    Timer1_Init();
    sei();

    while (1) {
        Adjust_Hours();
        Adjust_Minutes();
        Adjust_Seconds();
        Change_Mode();
        Display_Time();
    }
}
