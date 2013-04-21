#include<reg52.h>
#include <intrins.h>
unsigned char led_segment[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};



void  key_scan();
void led_display();

sbit led_selector_o=P3^0;
sbit led_selector_oo=P3^1;

char key_o=0,key_oo=0;




/*
	nrf24 \/
*/
sbit finish_key_input_flag=P3^7;//let the p3.7 key represent finish key input,and is ready to anwser the master

typedef unsigned char uint;//uint is a byte long

sbit 	MISO	=P2^5;
sbit 	MOSI	=P2^1;
sbit	SCK	  =P2^6;
sbit	CE	  =P2^7;
sbit	CSN		=P2^2;
sbit	IRQ		=P3^3;

sbit	led		=P3^6;

#define ADR_WIDTH 5	
#define AW_ADR_WIDTH 0x03
#define PLOAD_WIDTH  20 

uint  TX_ADDRESS[ADR_WIDTH]= {0x34,0x43,0x10,0x10,0x01};	//address that the RF packets contain
uint  RX_ADDRESS[ADR_WIDTH]= {0x34,0x43,0x10,0x10,0x01};	//RF packets with *which address* that the pipes like to receive 

uint 	bdata sta;  
sbit	RX_DR	=sta^6;
sbit	TX_DS	=sta^5;
sbit	MAX_RT	=sta^4;

bit receive_payload_flag=0;

//THE COMMANDS
#define READ_REG        0x00  	 
#define WRITE_REG       0x20 	 
#define RD_RX_PLOAD     0x61  	 
#define WR_TX_PLOAD     0xA0  
#define W_ACK_PAYLOAD_IN_PRX_PIPE0 0xA8	 
#define FLUSH_TX        0xE1 	 
#define FLUSH_RX        0xE2  	 
#define REUSE_TX_PL     0xE3  	 
#define NOP             0xFF  	 

//THE REGISTERS' ADDRESSES
#define CONFIG          0x00   
#define EN_AA           0x01  
#define EN_RXADDR       0x02   
#define SETUP_AW        0x03   
#define SETUP_RETR      0x04  
#define RF_CH           0x05  
#define RF_SETUP        0x06   
#define STATUS          0x07   
#define OBSERVE_TX      0x08   
#define CD              0x09   
#define RX_ADDR_P0      0x0A   
#define RX_ADDR_P1      0x0B  
#define RX_ADDR_P2      0x0C  
#define RX_ADDR_P3      0x0D  
#define RX_ADDR_P4      0x0E  
#define RX_ADDR_P5      0x0F   
#define TX_ADDR         0x10   
#define RX_PW_P0        0x11  
#define RX_PW_P1        0x12   
#define RX_PW_P2        0x13   
#define RX_PW_P3        0x14  
#define RX_PW_P4        0x15   
#define RX_PW_P5        0x16   
#define FIFO_STATUS     0x17  



uint SPI_write_byte(uint);
uint SPI_WRITE_REG_DATA_OR_CMD(uint,uint);
uint SPI_WRITE_REG_ARRAY(uint ,uint * ,uint );
void inerDelay_us(uint);
void SPI_write_addresses();
uint SPI_READ_REG(uint);
uint SPI_READ_REG_ARRAY(uint, uint * , uint);
void init_NRF24L01();
void TX_mode(uint * );
void RX_mode(void);
void TX_RX_mode(uint * );
void nrf24_irq_init();


/*
	nrf24 /\
*/

void main(){
	uint payload_as_ACK[PLOAD_WIDTH]={5,3,2};
	uint payload_to_rec[PLOAD_WIDTH]={0};

	led_selector_o=0;
	led_selector_oo=0;//high voltage selects the diget
	
	while (1){
		key_scan();
		led_display();
		if (~finish_key_input_flag) break;
	};
	
	
	

	payload_as_ACK[0]=key_o+key_oo*10;
	
	nrf24_irq_init();
	init_NRF24L01();
	SPI_write_addresses();
	RX_mode();
	while (1){
		if (receive_payload_flag){
			receive_payload_flag=0;
			SPI_READ_REG_ARRAY(RD_RX_PLOAD,payload_to_rec,PLOAD_WIDTH);
			
			TX_RX_mode(payload_as_ACK);
			//SPI_WRITE_REG_ARRAY(W_ACK_PAYLOAD_IN_PRX_PIPE0,payload_as_ACK,PLOAD_WIDTH);
			if(payload_to_rec[2]==1) {payload_to_rec[2]=0;led=~led;}
		}
	
	}
	
	
	
}


void led_display(){
	char n;
	led_selector_oo=0;//close oo
	P0=led_segment[key_o];//prepare key value
	led_selector_o=1;//open o
	delay1: n=200;while (n--); 
	
	led_selector_o=0;//close o
	P0=led_segment[key_oo];//prepare key value
	led_selector_oo=1;//open oo
	delay2: n=100;while (n--); 

	return;
}


void  key_scan(){
	char temp;
	P1=0xfe;
	temp=P1&0xf0; //read p1.4-->p1.7  to temp
	if (temp!=0xf0){
		switch(temp){
			case 0xe0: key_o=1;key_oo=0;break;
			case 0xd0: key_o=2;key_oo=0;break;
			case 0xb0: key_o=3;key_oo=0;break;
			case 0x70: key_o=4;key_oo=0;break;			
		}
		return;
	}
	
	P1=0xfd;
	temp=P1&0xf0; //read p1.4-->p1.7  to temp
	if (temp!=0xf0){
		switch(temp){
			case 0xe0: key_o=5;key_oo=0;break;
			case 0xd0: key_o=6;key_oo=0;break;
			case 0xb0: key_o=7;key_oo=0;break;
			case 0x70: key_o=8;key_oo=0;break;			
		}
		return;
	}
	
	P1=0xfb;
	temp=P1&0xf0; //read p1.4-->p1.7  to temp
	if (temp!=0xf0){
		switch(temp){
			case 0xe0: key_o=9;key_oo=0;break;
			case 0xd0: key_o=0;key_oo=1;break;
			case 0xb0: key_o=1;key_oo=1;break;
			case 0x70: key_o=2;key_oo=1;break;			
		}
		return;
	}
	
	P1=0xf7;
	temp=P1&0xf0; //read p1.4-->p1.7  to temp
	if (temp!=0xf0){
		switch(temp){
			case 0xe0: key_o=3;key_oo=1;break;
			case 0xd0: key_o=4;key_oo=1;break;
			case 0xb0: key_o=5;key_oo=1;break;
			case 0x70: key_o=6;key_oo=1;break;			
		}
		return;
	}
	
	return;
}



/*
	nrf24 \/
*/

void nrf24_irq() interrupt 2
{
	sta=SPI_READ_REG(STATUS);
	if (RX_DR){
		receive_payload_flag=1;
	}
	if (TX_DS){
	
	}
	if (MAX_RT){
		SPI_WRITE_REG_DATA_OR_CMD(FLUSH_TX,0);
	}
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+STATUS,sta);
}



void  nrf24_irq_init(){
	IT1=1;
	EX1=1;

	PX1=0;
	EA=1;

}

uint SPI_write_byte(uint the_byte){
	uint count;
	SCK=0;
	for (count=0;count<8;count++){
		MOSI=(the_byte&0x80);//get the left most bit of the byte,and put on MOSI
		the_byte=the_byte<<1;//left move one bit
		SCK=1;							//the SCK upside occurs,nrf24 eats the bit on mosi,nrf24 generates the bit on miso
		the_byte = the_byte|MISO;//the right most bit of the byte become MISO
		SCK=0;
	}
	return the_byte;
}


/*
		driver  for nrf24
*/
uint SPI_WRITE_REG_DATA_OR_CMD(uint reg,uint data_cmd){
	uint status;
	CSN =0;
	status =SPI_write_byte(reg);
	SPI_write_byte(data_cmd);
	CSN=1;
	return status;
}

uint SPI_WRITE_REG_ARRAY(uint reg,uint * array,uint length){
	uint status,count;
	CSN=0;
	status =SPI_write_byte(reg);
	for (count=0;count<length;count++){
		SPI_write_byte(array[count]);
	}
	CSN=1;
	return status;
}

void inerDelay_us(unsigned char n){
	for(;n>0;n--) _nop_();
}

void SPI_write_addresses(){
	SPI_WRITE_REG_ARRAY(WRITE_REG+TX_ADDR,TX_ADDRESS,ADR_WIDTH);
	SPI_WRITE_REG_ARRAY(WRITE_REG+RX_ADDR_P0,RX_ADDRESS,ADR_WIDTH);
	
}

uint SPI_READ_REG(uint reg){
	uint status,val;
	CSN=0;
	status=SPI_write_byte(READ_REG+reg);//bcs READ_REG =0,so just a form for uinty
	val=SPI_write_byte(READ_REG+0);
	CSN=1;
	return val;
}

uint SPI_READ_REG_ARRAY(uint reg, uint * array, uint length){
	uint status,count;
	
	CSN = 0;                    		
	status = SPI_write_byte(reg);       		
	
	for(count=0;count<length;count++)
		array[count] = SPI_write_byte(0);    
	
	CSN = 1;                           
	
	return(status);                    
}

void init_NRF24L01(void){
	inerDelay_us(200);//100 us is okay,but 100 us as redundance
	CE=0; 	//let nrf24 stop Trans/Recev packets
	CSN=0;	//let spi starts to work
	SCK=0;	//ready for SCK upside
		
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+EN_RXADDR,0x01);	//allow pipe0 to receive packets
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+EN_AA,0x01);			//allow pipe0 auto ACK,enter the SHOCKBURST MODE
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+RF_CH,0);					//let the channel become 2.4000 GHz
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+RX_PW_P0,PLOAD_WIDTH);//set the payload width of pipe0
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG+RF_SETUP,0x07);		//let the data rate 1 Mbps,the power amplifier 0 dBm
	CSN=1;	
}

void TX_mode(uint * payload){
	CE=0;
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG + CONFIG, 0x0e);//set nrf24   TX mode
	SPI_WRITE_REG_ARRAY(WR_TX_PLOAD,payload,PLOAD_WIDTH);
	CE=1;
	inerDelay_us(17);//10us is okay ,also 7 us is redundance,then it start to transmit packet
	CE=0;
}

void RX_mode(void){
	CE=0;
	SPI_WRITE_REG_DATA_OR_CMD(WRITE_REG + CONFIG, 0x0f);
	CE=1;
	inerDelay_us(200);//130 us is okay,but 70 us as redundance
}

void TX_RX_mode(uint * payload){
	CE=0;
	TX_mode(payload);
	RX_mode();
}

/*
	nrf24 /\
*/
