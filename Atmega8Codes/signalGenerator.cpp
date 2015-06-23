#define F_CPU 4000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data);

unsigned char manualControl = 97;

int main(void)
{
	DDRD = 0b00000000;
	PORTD = 0b00001100;
	DDRB = 0b00000110;
	PORTB = 0b00000100;
	
	//настраиваем USART
	USART_Init(MYUBRR);
	//глобальное разрешение прерываний
	sei();
	
    while(1)
    {
		_delay_ms(100);
		if(manualControl==97){
			if((PIND & (1<<PD2))==0){
				while((PIND & (1<<PD2))==0);
				PORTB = 0b00000000;
				_delay_ms(100);
				PORTB = 0b00000100;
			}
			if((PIND & (1<<PD3))==0){
				while((PIND & (1<<PD3))==0);
				PORTB = 0b00000110;
				_delay_ms(100);
				PORTB = 0b00000100;
			}
		}
    }
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
				 if(manualControl==98){
					 PORTB = 0b00000110;
					 _delay_ms(100);
					 PORTB = 0b00000100;
				 }
				 break;
		case 107:USART_Transmit(65);
				 if(manualControl==98){
					 PORTB = 0b00000000;
					 _delay_ms(100);
					 PORTB = 0b00000100;	 
				 }
				 break;
		default:break;
	}
}
