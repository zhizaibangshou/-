#include "delay.h"
#include "usart3.h"
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"	  
#include "usart.h"


//串口发送缓存区
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];
u8 USART3_TX_BUF[USART3_MAX_SEND_LEN];

#ifdef USART3_RX_EN   								//如果使能了接收   	 
 
//串口接收缓存区
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//接收缓冲,最大USART3_MAX_RECV_LEN个字节. 	
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 				//接收缓冲,最大USART3_MAX_RECV_LEN个字节.


//通过判断接收连续2个字符之间的时间差不大于100ms来决定是不是一次连续的数据.
//如果2个字符接收间隔超过100ms,则认为不是1次连续数据.也就是超过100ms没有接收到
//任何数据,则表示此次接收完毕.
//接收到的数据状态
//[15]:0,没有接收到数据;1,接收到了一批数据.
//[14:0]:接收到的数据长度	  
u16 USART3_RX_STA = 0;
void USART3_IRQHandler(void)//串口3中断服务程序
{
    u8 Res;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.我在这里就不去掉了
    OSIntEnter();
#endif
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
    {
        Res =USART_ReceiveData(USART3);//(USART1->DR);	//读取接收到的数据

        if((USART3_RX_STA&0x8000)==0)//接收未完成
        {
            if(USART3_RX_STA&0x4000)//接收到了0x0d
            {
                if(Res!=0x0a)USART3_RX_STA=0;//接收错误,重新开始
                else USART3_RX_STA|=0x8000;	//接收完成了
            }
            else //还没收到0X0D
            {
                if(Res==0x0d)USART3_RX_STA|=0x4000;
                else
                {
                    USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res ;
                    USART3_RX_STA++;
                    if(USART3_RX_STA>(USART_REC_LEN-1))USART3_RX_STA=0;//接收数据错误,重新开始接收
                }
            }
        }
    }
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
    OSIntExit();
#endif
}


  
#endif	

#ifdef USART2_RX_EN   								//如果使能了接收   	
u16 USART2_RX_STA = 0;					
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//接收缓冲,最大USART2_MAX_RECV_LEN个字节.
//ucos定义都删了
void USART2_IRQHandler(void)//串口2中断服务程序
{
	u8 Res;
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
    {
        Res =USART_ReceiveData(USART2);//(USART1->DR);	//读取接收到的数据

        if((USART2_RX_STA&0x8000)==0)//接收未完成
        {
            if(USART2_RX_STA&0x4000)//接收到了0x0d
            {
                if(Res!=0x0a)USART2_RX_STA=0;//接收错误,重新开始
                else USART2_RX_STA|=0x8000;	//接收完成了
            }
            else //还没收到0X0D
            {
                if(Res==0x0d)USART2_RX_STA|=0x4000;
                else
                {
                    USART2_RX_BUF[USART2_RX_STA&0X3FFF]=Res ;
                    USART2_RX_STA++;
                    if(USART2_RX_STA>(USART_REC_LEN-1))USART2_RX_STA=0;//接收数据错误,重新开始接收
                }
            }
        }
    }
}

#endif
void usart2_init(u32 bound)
{  
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//使能USART2时钟

	USART_DeInit(USART2);  //复位串口2

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2); //GPIOA2复用为USART2
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2); //GPIOA2复用为USART2

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3; 	//GPIOA2和GPIOA3初始化
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;				//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 				//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 				//上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); 						//初始化GPIOA2，和GPIOA3

	USART_InitStructure.USART_BaudRate = bound;										//波特率 
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//收发模式

	USART_Init(USART2, &USART_InitStructure); 			//初始化串口2
	USART_Cmd(USART2, ENABLE);               			//使能串口2 
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);		//开启中断2   

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级1 ，按自己逻辑编辑即可
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							//根据指定的参数初始化VIC寄存器
}

//初始化IO 串口3
//bound:波特率	  
void usart3_init(u32 bound)
{  
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟

	USART_DeInit(USART3);  //复位串口3

	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOB11复用为USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOB10复用为USART3	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; 	//GPIOB11和GPIOB10初始化
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;				//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 				//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 				//上拉
	GPIO_Init(GPIOB,&GPIO_InitStructure); 						//初始化GPIOB11，和GPIOB10

	USART_InitStructure.USART_BaudRate = bound;										//波特率 
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//收发模式

	USART_Init(USART3, &USART_InitStructure); 			//初始化串口3
	USART_Cmd(USART3, ENABLE);               			//使能串口 
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);		//开启中断   

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							//根据指定的参数初始化VIC寄存器
	USART3_RX_STA=0;					//清零
}


//串口3,printf 函数
//确保一次发送数据不超过USART3_MAX_SEND_LEN字节
void u2_printf(char* fmt,...)  
{  
	u16 i,j;
	va_list ap;//定义了一个空指针
	memset(USART2_TX_BUF,'\0',USART2_MAX_SEND_LEN);
	va_start(ap,fmt);//
	vsprintf((char*)USART2_TX_BUF,fmt,ap);//格式化传进来的参数
	va_end(ap);
	i=strlen((const char*)USART2_TX_BUF);//此次发送数据的长度
	
	for(j=0;j<i;j++)//循环发送数据
	{
		//等待上次传输完成 
		while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
		//发送数据到串口3 
		USART_SendData(USART2,(uint8_t)USART2_TX_BUF[j]);
	}
	
}

//串口3,printf 函数
//确保一次发送数据不超过USART3_MAX_SEND_LEN字节
void u3_printf(char* fmt,...)  
{  
	u16 i,j;
	va_list ap;//定义了一个空指针
	memset(USART3_TX_BUF,'\0',USART3_MAX_SEND_LEN);
	va_start(ap,fmt);//
	vsprintf((char*)USART3_TX_BUF,fmt,ap);//格式化传进来的参数
	va_end(ap);
	i=strlen((const char*)USART3_TX_BUF);//此次发送数据的长度
	
	for(j=0;j<i;j++)//循环发送数据
	{
		//等待上次传输完成 
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET);
		//发送数据到串口3 
		USART_SendData(USART3,(uint8_t)USART3_TX_BUF[j]);
	}
	
}

