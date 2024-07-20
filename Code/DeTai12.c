#include <mega16.h>
#include <delay.h>
#include <alcd.h>
#include <stdio.h>

#define F_CPU 8000000UL

#define ADC_VREF_TYPE ((0 << REFS1) | (1 << REFS0) | (0 << ADLAR))

#define TRIG_PIN 0
#define ECHO_PIN 1

#define BUTTON2 2
#define BUTTON4 5
#define BUTTON3 4
#define BUTTON1 3

#define PUMP_PIN 2

#define AUTO_MODE 1
#define MANUAL_MODE 0

#define PUMP_OFF 1
#define PUMP_ON 0

#define MIN_WATER_LEVEL 15 // Gi? s? m?c nu?c t?i thi?u là 20 cm
#define MAX_WATER_LEVEL 5 // Gi? s? m?c nu?c t?i da là 10 cm

#define MODE_DISTANCE 0
#define MODE_AUTO_MANUAL 1
#define MODE_MIN_LEVEL 2
#define MODE_MAX_LEVEL 3

unsigned char mode = 0;
char buffer[16];

#define TIMEOUT 10000 // Adjust this value as needed

unsigned char mode1 = 0;
unsigned char pump_status = PUMP_OFF;
unsigned int min_water_level = MIN_WATER_LEVEL;
unsigned int max_water_level = MAX_WATER_LEVEL;
unsigned int water_level;
void ADC_init()
{
    // Ch?n AVCC làm di?n áp tham chi?u, không kích ho?t ch?c nang left adjust
    ADMUX = (1 << REFS0);

    // Kích ho?t ADC, ch?n ch? d? chuy?n d?i liên t?c, ch?n ch? d? chuy?n d?i t? d?ng
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE);

    // Ch?n ch? d? chia t? l? 64 (8MHz / 64 = 125kHz)
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
}
unsigned int read_ADC(unsigned char adc_input)
{
    ADMUX = adc_input | (ADC_VREF_TYPE & 0xff);
    // B?t d?u chuy?n d?i
    ADCSRA |= (1 << ADSC);
    // Ch? cho d?n khi chuy?n d?i hoàn t?t
    while ((ADCSRA & (1 << ADIF)) == 0)
        ;
    ADCSRA |= (1 << ADIF);
    return ADCW;
}
unsigned int measure_water_level()
{
    unsigned long echo_time;
    unsigned int distance;
    unsigned long timeout;

    DDRD |= (1 << TRIG_PIN);   // Set TRIG_PIN as output
    DDRD &= ~(1 << ECHO_PIN);  // Set ECHO_PIN as input
    PORTD &= ~(1 << TRIG_PIN); // Set TRIG_PIN to 0

    delay_us(2);
    PORTD |= (1 << TRIG_PIN); // Set TRIG_PIN to 1
    delay_us(10);
    PORTD &= ~(1 << TRIG_PIN); // Set TRIG_PIN to 0
    timeout = 0;
    while (!(PIND & (1 << ECHO_PIN)) && timeout < TIMEOUT)
    {
        timeout++;
    }
    if (timeout >= TIMEOUT)
    {
        // Handle timeout error here
        return 0;
    }
    TCNT1 = 0;
    TCCR1B = (1 << CS10); // Use prescaler 8 with 8MHz crystal
    timeout = 0;
    while ((PIND & (1 << ECHO_PIN)) && timeout < TIMEOUT)
    {
        timeout++;
    }
    if (timeout >= TIMEOUT)
    {
        // Handle timeout error here
        return 0;
    }
    TCCR1B = 0;
    echo_time = TCNT1;
    distance = (unsigned int)((echo_time / 232.8) / 2); // Convert time to distance
    return distance;
}
void display_mode()
{
    lcd_clear();
    lcd_gotoxy(0, 0);
    if (mode1 == AUTO_MODE)
    {
        lcd_putsf("Mode: AUTO");
    }
    else
    {
        lcd_putsf("Mode: MANUAL");
    }
}
void display_min_max()
{
    char buffer[16];
    lcd_gotoxy(0, 1);
    lcd_putsf("Min: ");
    sprintf(buffer, "%d", min_water_level);
    lcd_puts(buffer);
    lcd_putsf("Max: ");
    sprintf(buffer, "%d", max_water_level);
    lcd_puts(buffer);
}
void display_distance(unsigned int distance)
{
    char buffer[16];
    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_putsf("Distance: ");
    sprintf(buffer, "%d cm", distance);
    lcd_puts(buffer);
}

void main(void)
{
    DDRD &= ~(1 << BUTTON2);
    PORTD |= (1 << BUTTON2); // Enable pull-up resistor for BUTTON2

    DDRD &= ~(1 << BUTTON4);
    PORTD |= (1 << BUTTON4); // Enable pull-up resistor for BUTTON4

    DDRD &= ~(1 << BUTTON3);
    PORTD |= (1 << BUTTON3); // Enable pull-up resistor for BUTTON3

    DDRD &= ~(1 << BUTTON1);
    PORTD |= (1 << BUTTON1); // Enable pull-up resistor for BUTTON1

    DDRB |= (1 << PUMP_PIN);
    PORTB |= (1 << PUMP_PIN);

    // Configure Timer1 in Normal mode and use prescaler 1
    TCCR1A = 0;           // Normal mode
    TCCR1B = (1 << CS10); // Use prescaler 1

    ADC_init();
    lcd_init(16);

    while (1)
    {
        if ((PIND & (1 << BUTTON2)) == 0)
        {
            mode++;
            if (mode > 3)
            {
                mode = 0;
            }
            while ((PIND & (1 << BUTTON2)) == 0);
        }
        switch (mode)
        {
        case MODE_DISTANCE:
            water_level = measure_water_level();
            display_distance(water_level);
            display_min_max();
            delay_ms(100);
            break;
        case MODE_AUTO_MANUAL:
            display_mode();
            if ((PIND & (1 << BUTTON4)) == 0)
            {
                mode1 = !mode1;
                display_mode();
                while ((PIND & (1 << BUTTON4)) == 0);
            }
            break;
        case MODE_MIN_LEVEL:
            display_min_max();
            if ((PIND & (1 << BUTTON4)) == 0)
            {
                if (min_water_level > MAX_WATER_LEVEL)
                    min_water_level--;
                lcd_clear();
                lcd_gotoxy(0, 1);
                lcd_putsf("Min: ");
                sprintf(buffer, "%d", min_water_level);
                lcd_puts(buffer);
                while ((PIND & (1 << BUTTON4)) == 0);
            }
            if ((PIND & (1 << BUTTON3)) == 0)
            {
                min_water_level++;
                lcd_clear(); 
                lcd_gotoxy(0, 1);
                lcd_putsf("Min: ");
                sprintf(buffer, "%d", min_water_level);
                lcd_puts(buffer);
                while ((PIND & (1 << BUTTON3)) == 0);
            }
            break;
        case MODE_MAX_LEVEL:
            display_min_max();
            if ((PIND & (1 << BUTTON4)) == 0)
            {
                max_water_level--;
                lcd_clear();
                lcd_gotoxy(7, 1);
                lcd_putsf("Max: ");
                sprintf(buffer, "%d", max_water_level);
                lcd_puts(buffer);
                while ((PIND & (1 << BUTTON4)) == 0);
            }
            if ((PIND & (1 << BUTTON3)) == 0)
            {
                if (max_water_level < MIN_WATER_LEVEL)
                    max_water_level++;
                lcd_clear();      
                lcd_gotoxy(7, 1);
                lcd_putsf("Max: ");
                sprintf(buffer, "%d", max_water_level);
                lcd_puts(buffer);
                while ((PIND & (1 << BUTTON3)) == 0);
            }
            break;
        }
        if (mode1 == AUTO_MODE)
        {
            if (water_level >= min_water_level)
            {
                pump_status = PUMP_ON;
            }
            else if (water_level <= max_water_level)
            {
                pump_status = PUMP_OFF;
            }
        }
        else if (mode1 == MANUAL_MODE)
        {
            if ((PIND & (1 << BUTTON1)) == 0)
            {
                pump_status = !pump_status;
                while ((PIND & (1 << BUTTON1)) == 0);
            }
            if (water_level <= max_water_level && pump_status == PUMP_ON)
            {
                pump_status = PUMP_OFF;
            }
        }
        if (pump_status == PUMP_ON)
        {
            PORTB &= ~(1 << PUMP_PIN); // Turn on the pump
        }
        else
        {
            PORTB |= (1 << PUMP_PIN); // Turn off the pump
        }
        delay_ms(50);
    }
}



