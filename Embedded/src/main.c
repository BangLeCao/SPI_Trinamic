#include "stm32f4xx.h"

/*DUNG SPI2 KET NOI VOI TMC4361*/
/*
PB12(SPI2 - NSS)  <----> 30 - TMC 4361(SPI1_CNS)
PB13(SPI2 - SCK)  <----> 31 - TMC 4361(SPI1_SCK)
PB14(SPI2 - MISO) <----> 33 - TMC 4361(SPI1_MISO)
PB15(SPI2 - MOSI) <----> 32 - TMC 4361(SPI1_MOSI)
PC9 (PWM)         <----> 23 - TMC 4361(CLK16)
3V  (STM32F4)     <----> 5  - TMC 4361(+5V_USB)
GND (STM32F4)     <----> 2  - TMC 4361(GND)

PA0 : TX
PA1 : RX
*/

SPI_InitTypeDef SPIinit;
GPIO_InitTypeDef GPIOinit;

uint32_t data;
uint8_t address;

uint8_t receive_uart[9], send_uart[7];
uint8_t rxbuff[9];
uint32_t pos;
uint32_t pos1;
uint32_t pos2;
uint8_t flag_setup = 0;
uint8_t flag_ready = 0;

uint32_t receive_data_ac;
uint8_t receive_add_ac;

uint32_t receive_data_ta;
uint8_t receive_add_ta;
uint16_t k;
double i;
uint32_t ii;
uint8_t mode_flag = '0';
uint8_t read_flag, send_flag;

void delay_us(uint16_t period){
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
  	TIM6->PSC = 83;
  	TIM6->ARR = period-1;
  	TIM6->CNT = 0;
  	TIM6->EGR = 1;

  	TIM6->SR  = 0;
  	TIM6->CR1 = 1;

  	while (!TIM6->SR);
    
  	TIM6->CR1 = 0;
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
}

void delay_01ms(uint16_t period){

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
  	TIM6->PSC = 8399;
  	TIM6->ARR = period-1;
  	TIM6->CNT = 0;
  	TIM6->EGR = 1;

  	TIM6->SR  = 0;
  	TIM6->CR1 = 1;

  	while (!TIM6->SR);
    
  	TIM6->CR1 = 0;
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, DISABLE);
}
/*CAU HINH SPI2 PB12 PB13 PB14 PB15*/
void SPI2_Config()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	SPIinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
	SPIinit.SPI_CPHA = SPI_CPHA_1Edge;
	SPIinit.SPI_CPOL = SPI_CPOL_Low;
	SPIinit.SPI_DataSize = SPI_DataSize_8b;
	SPIinit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPIinit.SPI_FirstBit = SPI_FirstBit_MSB;
	SPIinit.SPI_Mode = SPI_Mode_Master;
	SPIinit.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI2, &SPIinit);
	SPI_Cmd(SPI2, ENABLE);

}

void GPIOConfig()
{
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
	
	GPIOinit.GPIO_Mode = GPIO_Mode_AF;
	GPIOinit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIOinit.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	
	GPIO_Init(GPIOB, &GPIOinit);

	GPIOinit.GPIO_Mode = GPIO_Mode_OUT;
	GPIOinit.GPIO_OType = GPIO_OType_PP;
	GPIOinit.GPIO_Pin = GPIO_Pin_12;
	GPIOinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIOinit.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOB, &GPIOinit);
}
/*CAU HINH CHAN PWM TAO XUNG CLOCK CHO TMC4361*/
void CLK16_Config()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef timInit;
	
	timInit.TIM_ClockDivision = 0;
	timInit.TIM_CounterMode = TIM_CounterMode_Up;
	timInit.TIM_Period = 6-1;
	timInit.TIM_Prescaler = 0;
	timInit.TIM_RepetitionCounter = 0;
	
	TIM_TimeBaseInit(TIM3, &timInit);
	TIM_Cmd(TIM3, ENABLE);
	
	TIM_OCInitTypeDef pwmInit;
	
	pwmInit.TIM_OCMode = TIM_OCMode_PWM1;
	pwmInit.TIM_OCPolarity = TIM_OCPolarity_High;
	pwmInit.TIM_OutputState = TIM_OutputState_Enable;
	pwmInit.TIM_Pulse = 3;
	
	TIM_OC4Init( TIM3, &pwmInit);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	
	GPIO_InitTypeDef gpioInit;
	
	gpioInit.GPIO_Mode = GPIO_Mode_AF;
	gpioInit.GPIO_OType = GPIO_OType_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_9;
	gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpioInit.GPIO_Speed = GPIO_High_Speed;
	
	GPIO_Init(GPIOC, &gpioInit);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);
}



void sendData(unsigned char address, unsigned long datagram)
{
	send_flag = 0;
	unsigned char i_address;
	unsigned long i_datagram;
	delay_01ms(1000);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	delay_us(10);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, address);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_address = SPI_I2S_ReceiveData(SPI2);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 24)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 24);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 16)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 16);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 8)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 8);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 0)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 0);
	 
	delay_01ms(1000);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	send_flag = 1;
}


void readData(unsigned char address, unsigned long datagram)
{
	read_flag = 0;
	unsigned char i_address;
	unsigned long i_datagram;
	delay_01ms(1000);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	delay_us(10);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, address);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_address = SPI_I2S_ReceiveData(SPI2);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 24)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 24);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 16)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 16);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 8)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 8);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE) != 1);
	SPI_I2S_SendData(SPI2, (datagram >> 0)&0xFF);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE) != 1);
	i_datagram |= (SPI_I2S_ReceiveData(SPI2) << 0);
	
	
	address = i_address;
	data = i_datagram;        
	delay_01ms(1000);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	read_flag = 1;
}


void UART_CONFIG(void)
{
  GPIO_InitTypeDef 	GPIO_InitStructure; 
	USART_InitTypeDef USART_InitStructure;  
	DMA_InitTypeDef   DMA_InitStructure;
  NVIC_InitTypeDef  NVIC_InitStructure;	
   
  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  /* Enable UART clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	/* Enable DMA1 clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  /* Connect UART4 pins to AF2 */  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4); 

  /* GPIO Configuration for UART4 Tx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* GPIO Configuration for USART Rx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
       
  /* USARTx configured as follow:
		- BaudRate = 115200 baud  
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(UART4, &USART_InitStructure);

  /* Enable USART */
  USART_Cmd(UART4, ENABLE);
	/* Enable UART4 DMA */
  USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);
	
	/* DMA1 Stream2 Channel4 for USART4 Rx configuration */			
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rxbuff;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 9;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream2, &DMA_InitStructure);
  DMA_Cmd(DMA1_Stream2, ENABLE);
		
	/* Enable DMA Interrupt to the highest priority */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Transfer complete interrupt mask */
  DMA_ITConfig(DMA1_Stream2, DMA_IT_TC, ENABLE);
}

void send_uart_pc()
{
	uint16_t k;
	for(k=0; k<7; k++)
		{
			USART_SendData(UART4, send_uart[k]);
			while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
		}
}

void control_pos(unsigned long pos)
{
		uint16_t i;
		sendData(0xB7, pos);
		delay_01ms(5000);
		readData(0x21,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		receive_data_ta = receive_data_ta & 0x0000FFFF;
		i = (double)receive_data_ta;
		i = (i*360)/51200;
		ii = (uint32_t)i;
		send_uart[2] = ((ii/100)%10 + 48);
		send_uart[3] = ((ii/10)%10 + 48);
		send_uart[4] = (ii%10 + 48);
		send_uart_pc();
}

void auto_pos(unsigned long pos1, unsigned long pos2)
{
		uint16_t i;
		sendData(0xB7, pos1);
		delay_01ms(5000);
		readData(0x21,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		receive_data_ta = receive_data_ta & 0x0000FFFF;
		i = (double)receive_data_ta;
		i = (i*360)/51200;
		ii = i;
		i = (uint16_t)i;
		send_uart[2] = ((i/100)%10 + 48);
		send_uart[3] = ((i/10)%10 + 48);
		send_uart[4] = (i%10 + 48);
		send_uart_pc();
		
		sendData(0xB7, pos2);
		delay_01ms(5000);
		readData(0x21,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		receive_data_ta = receive_data_ta & 0x0000FFFF;
		i = (double)receive_data_ta;
		i = (i*360)/51200;
		ii = i;
		i = (uint16_t)i;
		send_uart[2] = ((i/100)%10 + 48);
		send_uart[3] = ((i/10)%10 + 48);
		send_uart[4] = (i%10 + 48);
		send_uart_pc();
}



void DMA1_Stream2_IRQHandler(void)
{
	uint16_t i;
	DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_TCIF2);
	for(i=0; i<9; i++)
	{
		receive_uart[i] = rxbuff[i];
	}
	DMA_Cmd(DMA1_Stream2, ENABLE);
	if(receive_uart[0] == 'A' && receive_uart[8] == 'Z')
	{
		mode_flag = receive_uart[1];
		if(mode_flag == 48)
		{
			pos = (receive_uart[5]-48)*100 + (receive_uart[6]-48)*10 + (receive_uart[7]-48);
			pos = (pos*51200)/360;
		}
		else if(mode_flag == 49)
		{
			pos1 = (receive_uart[2]-48)*100 + (receive_uart[3]-48)*10 + (receive_uart[4]-48);
			pos1 = (pos1*51200)/360;
			
			pos2 = (receive_uart[5]-48)*100 + (receive_uart[6]-48)*10 + (receive_uart[7]-48);
			pos2 = (pos2*51200)/360;
		}
		
	if(mode_flag == 48)
		{
			control_pos(pos);
		}
	}
}

void TIMER_CONFIG()
{
	TIM_TimeBaseInitTypeDef timInit;
	NVIC_InitTypeDef nvicInit;
	
	nvicInit.NVIC_IRQChannel = TIM2_IRQn;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 0;
	nvicInit.NVIC_IRQChannelSubPriority = 1;
	
	NVIC_Init(&nvicInit);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	timInit.TIM_ClockDivision = 0;
	timInit.TIM_CounterMode = TIM_CounterMode_Up;
	timInit.TIM_Period = 84000-1;
	timInit.TIM_Prescaler = 100 -1;
	
	TIM_TimeBaseInit(TIM2, &timInit);
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM2, DISABLE);
}

void TIM2_IRQHandler() 
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != 0)
	{
		if(read_flag == 1 && send_flag == 1)
		{
			uint16_t i;
			TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
			readData(0x21,0x00000000);
			receive_add_ta = address;
			receive_data_ta = data;
			receive_data_ta = receive_data_ta & 0x0000FFFF;
			//receive_data_ta = receive_data_ta*scale;
			i = (double)receive_data_ta;
			i = (i*360)/51200;
			ii = i;
			i = (uint16_t)i;
			send_uart[2] = ((i/100)%10 + 48);
			send_uart[3] = ((i/10)%10 + 48);
			send_uart[4] = (i%10 + 48);
			send_uart_pc();
		}
		else
		{
			
		}
	}
}

void setup_TMC4361()
{
	sendData(0xCF,0x52535400);
  sendData(0x83,0x00540022); //Inputfilter : START and encoderpins
  sendData(0x84,0x4440006B);
	// bit(3:0) = 11 --> TMC26x driver
	// bit (31 - 20) --> SPI_OUT_LOW_TIME = SPI_OUT_HIGH_TIME = SPI_OUT_BLOCK_TIME = 4 clock cycles
  sendData(0x80,0x00006026);
	// bit (0) = 0 --> not use S-shaped ramp
	// bit (1) = 1 --> ACC values are set directly as step per clock cycle
	// bit (2) = 1 --> Bow values are set directly as steps per clock cycle
	// bit (3) = 0 
  sendData(0x90,0x00010001);
	// STP_LENGTH_ADD = 1
	// DIR_SETUP_TIME = 1
  sendData(0xEC,0x00000000);
	// bit (9) = 0 --> Disable STEP pulse interpolation
	// bit (10) = 0 --> single edge STEP pulses
	// bit (3..0) = 0 --> 256 microsteps per 90 degrees
  sendData(0xEC,0x00090585);
	
	sendData(0xEC,0x000A0000); // disable SMARTEN
	
  sendData(0xEC,0x000C0018);
	// SGT = 0
	// Cs = 24
  sendData(0xEC,0x000EF000);
  sendData(0xA4,0x00000000); // v = 0
  sendData(0xA1,0x00000000); // X_ACTUAL = 0
  sendData(0xB7,0x00000000); // X_TARGET = 0
	
	sendData(0xD4,0x00009C40); //encoder resolution = 200p/rev
	sendData(0x9C,0x00FF00FF); //CL_BETA = CL_GAMMA = 255
	// ENC_POS_DEV : displays the deviation between XACTUAL and ENC_POS
	sendData(0xA0,0x00000004); //position mode
	//sendData(0xA4,0x00100000);
 
	sendData(0xDC,0x00010000);
	//CL_P = 1 --> CL_DELTA_P = 2^16 = 65536 --> Real value = CL_DELTA_P/2^16 = 1 --> gain = 1
	//CL_DELTA_P is a proportional controller that compensates a detected position deviation between position setpoint and position actual
	sendData(0x87,0x00400000); // Closed-loop operation is enabled
	sendData(0xB7,0x00000080); // Move to FS position
	sendData(0x87,0x01400000); // Closed-loop calibration is active
	delay_01ms(10);						 // Wait for actual position to FS position
	sendData(0x87,0x00400000); // Closed-loop calibration is deactived
	sendData(0xA4,0x00000000); // Set velocity = 0
	
	sendData(0xA0,0x00000001); // T-ramp + velocity mode
	sendData(0xA8,0x00004E20); // AMAX
	sendData(0xA9,0x00004E20); // DMAX
	sendData(0xE0,0x00030D40); // dcStep velocity = 0x30D40 = 200Kpps
	sendData(0xE1,0x003D0900); //
	sendData(0xE3,0x02000B64);
	// Encoder velocity update time = 0x0200 = 512 clock cycles
	// Filter encoder to caculate mean encoder velocity = 11
	// Delay period between 2 consecutive actual encoder velocity values = 100 clock cycles
	sendData(0xDF,0x00000014); //cl_tolerance = 0x14 = 20
	//The tolerance range for position deviation
	sendData(0xDC,0x00010000); //cl_p = 1
	//Gain parameter that compensates a detected position deviation between internal and external position
	sendData(0xDA,0x000000C8); //cl_vlimit_p= 0xC8 = 200
	//P parameter of the PI regulator, which controls the maximum velocity
	sendData(0xDB,0x00000032); //cl_vlimit_i= 0x32 = 50
	//I parameter of the PI regulator, which controls the maximum velocity
	sendData(0xDD,0x000003E8); //cl_vlimit_diclip= 0x3E8 = 1000
	// to limit the velocity for error compensation
	sendData(0xDE,0x00030D40); //cl_vlimit= 0x30D40 = 200000 pps
	//to limit the maximum velocity deviation above the maximum velocity VMAX
	sendData(0x86,0x00321F05);//cl scaling values
	sendData(0x98,0x000003E8);// cl_upscale=1000
	sendData(0x99,0x00002710);// cl_dnscale=10000
	sendData(0x87,0x1A400000);// cl with gamma correction, vlimit and turned on velocity mode
	sendData(0x85,0x00000080);// cl scaling on
	
	sendData(0xA0,0x00000005); // T-ramp + position mode
}

		uint8_t p;
void run()
{
	if(mode_flag == 49)
		{
			p = 1;
			auto_pos(pos1, pos2);
		}
		else if(mode_flag == 48)
		{
			p = 0;
			control_pos(pos);
		}
}

int main(void)
{
 	send_uart[0] = 65; // STX
	send_uart[1] = send_uart[5] = 44; // ","
	send_uart[6] = 90; // ETX

 	SystemInit();
	SPI2_Config();
	CLK16_Config();
	GPIOConfig();
	UART_CONFIG();
	setup_TMC4361();
	//TIMER_CONFIG();
	//TIM_Cmd(TIM2, ENABLE);
	//TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	sendData(0xA0,0x00000005);
	sendData(0xA4,0x030D4000);
	sendData(0xA1,0x0);
	
  while (1)
  {
		/*
		sendData(0xA0,0x00000005);
		sendData(0xA4,0x030D4000);
		sendData(0xA1,0x0);
		
		sendData(0xB7,0x00000000);
		delay_01ms(5000);
		readData(0x37,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		k = 0;
		delay_01ms(20000);
		
		
		sendData(0xB7,0x00003200);
		delay_01ms(5000);
		readData(0x37,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		k = 90;
		delay_01ms(20000);

		
		sendData(0xB7,0x00006400);
		delay_01ms(5000);
		readData(0x37,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		k = 180;
		delay_01ms(20000);

		
		sendData(0xB7,0x00009600);
		delay_01ms(5000);
		readData(0x37,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		k = 270;
		delay_01ms(20000);

		
		sendData(0xB7,0x0000C800);
		delay_01ms(5000);
		readData(0x37,0x00000000);
		receive_add_ta = address;
		receive_data_ta = data;
		k = 360;
		delay_01ms(20000);
		*/
		//run();
  }
}
