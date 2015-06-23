#define F_CPU 4000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

unsigned char hallOpen, hallClose, manualControl=97, source=97, cnt, releOpen, releClose, res;

void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data);
unsigned char USART_Receive(void);
void ADC_init();
unsigned char ADC_result(unsigned char adc_input);
void cylinderOpen();
void cylinderClose();
void cylinderClose();

int main(void)
{
	DDRB = 0b00000111;
	PORTB = 0b01000000;
	//настраиваем АЦП
	ADC_init();
	//настраиваем USART
	USART_Init(MYUBRR);
	//глобальное разрешение прерываний
	sei();
	
	unsigned char sign;
	
    while(1)
    {		
		_delay_ms(100);
		if(manualControl==97){
			//блок считывания данных
			if(source==97)
				sign = ADC_result(0);
			else if(source==98)
				sign = ADC_result(5);
			//если пришел сигнал 
			if(sign>235){
				cylinderOpen();	
			}
			else if(sign<20){
				cylinderClose();
			}
		}
    }
}

void cylinderOpen(){
	//на выдвижение
	//проверка давления
	if((PINB & (1<<PB6)) == 0){
		_delay_ms(10);
		USART_Transmit(71);
		_delay_ms(10);
		return;
	}
	//выдвигаем
	PORTB = 0b01000010;
	//ждем, пока датчик Холла на открытие сигнализирует о выдвижении цилиндра
	cnt = 0;
	while(ADC_result(1)<140){
		_delay_ms(100);
		cnt++;
		//если за 2 секунды цилиндр не выдвинулся, то ошибка
		if(cnt>20){
			//проверим давление в системе
			if(false){
				USART_Transmit(71);
			}
			//проверим реле
			else if(ADC_result(4)>240)
				//сигнализируем, что реле не сработало
				USART_Transmit(70);
			else
				//сигнализируем, что реле сработало, но цилиндр не выдвинут
				USART_Transmit(69);
			PORTB = 0b01000001;
			return;
		}
	}
	//цилиндр выдвинулся
	PORTB = 0b01000000;
	USART_Transmit(79);	
}

void cylinderClose(){
	//на вжатие
	//проверка давления
	if((PINB & (1<<PB6)) == 0){
		_delay_ms(10);
		USART_Transmit(71);
		_delay_ms(10);
		return;
	}
	//выдвигаем
	PORTB = 0b01000100;
	//ждем, пока датчик Холла на открытие сигнализирует о вжатии цилиндра
	cnt = 0;
	while(ADC_result(2)>0){
		_delay_ms(100);
		cnt++;
		//если за 2 секунды цилиндр не выдвинулся, то ошибка
		if(cnt>20){
			//проверим давление в системе
			if(false){
				USART_Transmit(71);
			}
			//проверим реле
			else if(ADC_result(3)>240)
			//сигнализируем, что реле не сработало
			USART_Transmit(73);
			else
			//сигнализируем, что реле сработало, но цилиндр не задвинут
			USART_Transmit(72);
			PORTB = 0b01000001;
			return;
		}
	}
	//цилиндр выдвинулся
	PORTB = 0b01000000;
	USART_Transmit(75);
}

//функция инициализации USART
void USART_Init(unsigned int ubrr){
	//задаем скорость соединения
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	//включаем приемник и передатчик, разрешаем прерывания по приему байта
	UCSRB = (1<<RXEN) | (1<<TXEN) | (1<<RXCIE);
	//задаем формат кадра: 8 битов данных, 2 стоп-бита
	UCSRC = (1<<URSEL) | (1<<USBS) | (3<<UCSZ0); 
}

//функция передачи байта
void USART_Transmit(unsigned char data){
	//ждем, пока освободиться буфер передачи
	while(!UCSRA & (1<<UDRE));
	//перемещаем данные для отправки в буфер, отправляем
	UDR = data;
}

//обработчик прерывания по приему байта
ISR(USART_RXC_vect){
	switch(UDR){
		case 97:USART_Transmit(65); //отправить A
				break;
		case 109:manualControl = 98;
				 USART_Transmit(65);
				 break;
		case 110:manualControl = 97;
				 USART_Transmit(65);
				 break;
		case 111:USART_Transmit(65);
		         if(manualControl==98)cylinderOpen();
				 break;
		case 107:USART_Transmit(65);
		         if(manualControl==98)cylinderClose();
				 break;
		case 114:USART_Transmit(65);
				 source=97;
				 break;
		case 115:USART_Transmit(65);
				 source=98;
				 break;
		default:break;
	}
}

void ADC_init()
{
	ADMUX = 0b01110000; //если более понятно пишем 0b11110000
	ADCSRA = 0b10000101; //0b10001101
}

unsigned char ADC_result(unsigned char adc_input)
{
	ADMUX=adc_input | (ADMUX & 0xF0); //выставляем канал
	_delay_us(30); //задержка для стабилизации
	ADCSRA |= 0x40;
	while((ADCSRA & 0x10)==0); //Ждём флаг окончания измерения
	ADCSRA|=0x10;
	return ADCH;//Возвращаем старший байт
}
