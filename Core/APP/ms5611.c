#include "ms5611.h"
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "math.h"

float Current_altitude=0;

extern SPI_HandleTypeDef hspi3;

static void MS5611_SpiLock(void)
{
  // SPI3 当前仅供 MS5611 使用时无需互斥；如后续多任务/多设备共享 SPI3，可在此加入 spi3Mutex。
}

static void MS5611_SpiUnlock(void)
{
}

const double T1 = 15.0 + 273.15;	/* temperature at base height in Kelvin */
const double A  = -6.5 / 1000;		/* temperature gradient in degrees per metre */
const double g  = 9.80665;				/* gravity constant in m/s/s */
const double RR  = 287.05;				/* ideal gas constant in J/kg/K */
const double p1 = 1013.15;
uint16_t C[8];  							// calibration coefficients

__IO ITStatus Bar_Conv_Flag = RESET;
__IO uint32_t Bar_RunCount=0;
__IO uint32_t D1; // ADC value of the pressure conversion
__IO uint32_t D2; // ADC value of the temperature conversion
//��ѹ�ƶ�ȡ����ѹֵ
MS56XX_Data 	  m_Ms56xx;

/*******************************��������****************************************
* ��������: void Baro_Reset(void)
* �������:
* ���ز���:
* ��    ��: ��λms56xx������
* ��    ��: 
*******************************************************************************/
void Baro_Reset(void)
{
	uint8_t txdata = CMD_RESET;
	MS5611_SpiLock();
	BARO_CS_L;
	HAL_SPI_Transmit(&hspi3, &txdata, 1, 50);
	HAL_Delay(10);
	BARO_CS_H;
	MS5611_SpiUnlock();
}

/*******************************��������****************************************
* ��������: unsigned int Baro_Cmd_Prom(char coef_num)
* �������: coef_num��ϵ��number
* ���ز���:
* ��    ��: ��ȡ�����������ϵ��
* ��    ��: 
*******************************************************************************/
unsigned int Baro_Cmd_Prom(char coef_num)
{
	unsigned int rc = 0;
	uint8_t txdata[3] = {0}, rxdata[3] = {0};
	MS5611_SpiLock();
	BARO_CS_L;
	txdata[0] = CMD_PROM_RD + coef_num * 2;
	HAL_SPI_TransmitReceive(&hspi3, txdata, rxdata, 3, 50);
	BARO_CS_H;
	MS5611_SpiUnlock();
	
//	HAL_Delay(1);//��������
	
	rc = ((unsigned int)rxdata[1] << 8) | (unsigned int)rxdata[2];

	return rc;
}

/*******************************��������****************************************
* ��������: void Baro_Read_Coe(void)
* �������:
* ���ز���:
* ��    ��: ��ȡ����
* ��    ��: 
*******************************************************************************/
void Baro_Read_Coe(void)
{
//	printf("Raw PROM Data:\r\n");
	for(int i = 0; i < 8; i++) 
	{
		uint16_t raw_data = Baro_Cmd_Prom(i);
//		printf("PROM[%d] = 0x%04X (%u)\r\n", i, raw_data, raw_data);
		C[i] = raw_data; // ֱ�Ӹ�ֵ������CRC
	}
		
//	for (uint8_t i = 0; i < 8; i++)
//	{
//		C[i] = Baro_Cmd_Prom(i);
//	}
}

/*******************************��������****************************************
* ��������: uint8_t Baro_Crc4_Check(uint32_t n_prom[])
* �������: n_prom[]����ȡ��ϵ����������
* ���ز���: uint8_t�����ؼ����CRCУ��ֵ����ȷУ�鷵��ֵӦ����0X0B��
* ��    ��: CRC ���㣬������֤��ȡ����8λ����ϵ���Ƿ���ȷ
* ��    ��: 
*******************************************************************************/
uint8_t Baro_Crc4_Check(uint16_t n_prom[])
{
	uint32_t cnt;
	uint32_t n_rem = 0U, crc_read;
	uint8_t n_bit;
	
	crc_read = n_prom[7];
	n_prom[7] = (0xff00 & (n_prom[7]));
	
	//���
//	crc_read = n_prom[7] & 0x000F; 
//	n_prom[7] &= 0xFFF0;

	for (cnt = 0; cnt < 16; cnt++)
	{
		if (cnt%2==1) n_rem ^= (unsigned short) ((n_prom[cnt>>1]) & 0x00FF);
		else n_rem ^= (unsigned short) (n_prom[cnt>>1]>>8);
		for (n_bit = 8; n_bit > 0; n_bit--)
		{
			if (n_rem & (0x8000))
				n_rem = (n_rem << 1) ^ 0x3000;
			else
				n_rem = (n_rem << 1);
		}
	}

	n_rem = ((n_rem >> 12) & 0x000F); // // final 4-bit reminder is CRC code
	n_prom[7] = crc_read;
	
	return (n_rem ^ 0x00);
}



/*******************************��������****************************************
* ��������: void Baro_En_Conv(char cmd)
* �������:
* ���ز���:
* ��    ��: enable pressure adc cov
* ��    ��: 
*******************************************************************************/
void Baro_En_Conv(char cmd)
{
	uint8_t txdata;
	MS5611_SpiLock();
	BARO_CS_L;
	txdata = CMD_ADC_CONV + cmd;
	HAL_SPI_Transmit(&hspi3, &txdata, 1, 50);
	BARO_CS_H;
	MS5611_SpiUnlock();
}

/*******************************��������****************************************
* ��������: void Baro_Enable_Temp_Conv(void)
* �������:
* ���ز���:
* ��    ��: ʹ���¶�ת������
* ��    ��: 
*******************************************************************************/
void Baro_Enable_Temp_Conv(void)
{
	Baro_En_Conv(CMD_ADC_D2+CMD_ADC_4096);
}

/*******************************��������****************************************
* ��������: void Baro_Enable_Press_Conv(void)
* �������:
* ���ز���:
* ��    ��: ʹ��ѹ��ת������
* ��    ��: 
*******************************************************************************/
void Baro_Enable_Press_Conv(void)
{
	Baro_En_Conv(CMD_ADC_D1+CMD_ADC_4096);
}

/*******************************��������****************************************
* ��������: unsigned long Baro_Read_Data(void)
* �������:
* ���ز���:  ���ض�ȡ����ֵ
* ��    ��: read adc result, after calling Baro_En_Conv()
* ��    ��: 
*******************************************************************************/
unsigned long Baro_Read_Data(void)
{
	uint8_t txdata[4];
	uint8_t rxdata[4];
	uint32_t temp;
	MS5611_SpiLock();
	BARO_CS_L;
	txdata[0] = CMD_ADC_READ;
	HAL_SPI_TransmitReceive(&hspi3, txdata, rxdata, 4, 50);
	BARO_CS_H;
	MS5611_SpiUnlock();

	// ADC 24bit，rxdata[1..3] 为有效数据
	temp = ((uint32_t)rxdata[1] << 16) | ((uint32_t)rxdata[2] << 8) | (uint32_t)rxdata[3];

	return temp;
}

/*******************************��������****************************************
* ��������: float Baro_Cal_Alt(unsigned long D1, unsigned long D2)
* �������: D1:��ѹԭʼ��ֵ �� D2�¶�ԭʼ��ֵ
* ���ز���:
* ��    ��: �������ѹֵ����λmba��
* ��    ��: 
*******************************************************************************/
float Baro_Cal_Alt(unsigned long D1, unsigned long D2)
{
	int32_t P; //�������ѹ��ֵ
	int32_t TEMP; //��������¶�ֵ����λ��0.01��C��
	int32_t dT; //ʵ���¶���ο��¶ȵĲ�ֵ
	int64_t OFF,OFF2; //ƫ������һ�׺Ͷ��ײ������֣�
	int64_t SENS,SENS2; //�����ȣ�һ�׺Ͷ��ײ������֣�
	
	// ����ʵ���¶���ο��¶ȵĲ�ֵ
	dT = D2 - (uint32_t)C[5] * 256;

  //һ���¶Ȳ�������
#ifdef MS5611
	OFF = (int64_t)C[2] * 65536 + (int64_t)C[4] * dT / 128;
	SENS = (int64_t)C[1] * 32768 + (int64_t)C[3] * dT / 256;
#endif

#ifdef MS5607
	OFF = (int64_t)C[2] * 131072 + (int64_t)C[4] * dT / 64;
	SENS = (int64_t)C[1] * 65536 + (int64_t)C[3] * dT / 128;
#endif
  // ����ʵ���¶�ֵ����λ��0.01��C��
	TEMP = 2000 + ((int64_t) dT * C[6]) / 8388608;
	//printf("TEMP is ��%ld  ",TEMP);

	OFF2 = 0;
	SENS2 = 0;
	
	//�����¶Ȳ�������
	if (TEMP < 2000)
	{
		OFF2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 2;
		SENS2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 4;
	}

	if (TEMP < -1500)
	{
		OFF2 = OFF2 + 7 * ((TEMP + 1500) * (TEMP + 1500));
		SENS2 = SENS2 + 11 * ((TEMP + 1500) * (TEMP + 1500)) / 2;
	}
	
	//Ӧ�ö��ײ���
	OFF = OFF - OFF2;
	SENS = SENS - SENS2;
	
	//����������ѹֵ
	P = (D1 * SENS / 2097152 - OFF) / 32768;
//	printf("P is :%d\r\n",P);
	return P;
}

/*******************************��������****************************************
* ��������: void Baro_Init(void)
* �������:
* ���ز���:
* ��    ��: ��ʼ��ms56xx
* ��    ��:
*******************************************************************************/
void Baro_Init(void)
{
	Baro_Reset();//��λ
	Baro_Read_Coe();//��ȡУ׼ϵ��
	
	//����һ���¶�ת��
	Baro_Enable_Temp_Conv();
}

/*******************************��������****************************************
* ��������: void Baro_Read_Pressure_Test(void)
* �������:
* ���ز���:
* ��    ��: ���Դ��������ݶ�ȡ�Ƿ���ȷ��
* ��    ��: 
*******************************************************************************/
void Baro_Read_Pressure_Test(void)
{
	/* 1.����һ����ѹת�� */
	Baro_Enable_Press_Conv();
	/* 2.��ʱ10ms�����ȡת������¶���ֵ*/
	HAL_Delay(10);
	D1 = Baro_Read_Data();
	/* 3.����һ���¶�ת��*/
	Baro_Enable_Temp_Conv();
	/*4.��ʱ10ms����ȡת�����ѹ����ֵ*/
	HAL_Delay(10);
	D2 = Baro_Read_Data();
	/* 5.���ù�ʽ�������յ���ѹ��ֵ��*/
	m_Ms56xx.pressure = Baro_Cal_Alt(D1,D2);
	printf("pressure is %lf mbar \r\n",m_Ms56xx.pressure/100.0);
}


/**
 * @brief �����׼��ѹ�߶ȣ�ʹ��PaΪ��λ��
 * @param pressure_pa ��ǰ��ѹֵ����λ��Pa��
 * @param reference_pressure_pa �ο���ƽ����ѹ����λ��Pa��Ĭ��101325��
 * @return �߶�ֵ����λ���ף�
 */
float calculate_pressure_altitude_pa(int32_t pressure_pa, int32_t reference_pressure_pa)
{
    // ������ȫ���
    if (pressure_pa <= 0) {
        return 0.0f;
    }
    
    // ����Ĭ�ϲο���ѹ����׼��ƽ����ѹ101325 Pa��
    if (reference_pressure_pa <= 0) {
        reference_pressure_pa = 101325; // ��׼��ƽ����ѹ
    }
    
    // ��int32_tת��Ϊfloat���м���
    float pressure_float = (float)pressure_pa;
    float reference_float = (float)reference_pressure_pa;
    
    // ���ʱ�׼����ģ�͹�ʽ��ʹ��PaΪ��λ��
    return 44330.76f * (1.0f - powf(pressure_float / reference_float, 0.190263f));
}



/*******************************��������****************************************
* ��������: void Loop_Read_Bar(void)
* �������:
* ���ز���:
* ��    ��:1000hz��Ƶ�ʵ��øú��������������ѹ�����ݻ���100hzƵ�ʸ��¡�
            ע�⣺�ٵ�һ�����øú���ǰ����������һ���¶�ת����
* ��    ��:
*******************************************************************************/
void Loop_Read_Bar(void)
{
	//����ִ��һ�θñ���+1
	Bar_RunCount++;

	//Bar_RunCountÿ�仯10�Σ�if���ִ��һ�Σ�����if���100hzִ��
	if(Bar_RunCount % 10 ==0)
	{
		if(Bar_Conv_Flag==RESET)
		{
			Bar_Conv_Flag = SET;
			//��ȡ�¶�rawֵ
			D2 = Baro_Read_Data();
			//������һ����ѹת��
			Baro_Enable_Press_Conv();
		}
		else
		{
			Bar_Conv_Flag=RESET;
			//��ȡ��ѹrawֵ
			D1 = Baro_Read_Data();
			m_Ms56xx.pressure = Baro_Cal_Alt(D1,D2);				
			//������һ���¶�ת��
			Baro_Enable_Temp_Conv();
			
			Current_altitude=calculate_pressure_altitude_pa(m_Ms56xx.pressure,STANDARD_ATMOSPHERIC_PRESSURE);
			// printf("pressure is %lf hPa \r\n",m_Ms56xx.pressure/100.0);
			// printf("Current_altitude is %lf m \r\n",Current_altitude);
			
//      printf("%ld\r\n",m_Ms56xx.pressure);		
			
		}

	}
}






