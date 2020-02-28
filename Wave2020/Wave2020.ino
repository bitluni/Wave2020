//Attribution 4.0 International (CC BY 4.0)
//bitluni 2020

#include <driver/dac.h>
#include <math.h>
#include "wave.h"

#include <soc/rtc.h>
#include "driver/i2s.h"

static const i2s_port_t i2s_num = (i2s_port_t)I2S_NUM_0; // i2s port number

//static i2s_config_t i2s_config;
static const i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
     .sample_rate = 1000000,  //not really used
     .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT, 
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = I2S_COMM_FORMAT_I2S_MSB,
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
     .dma_buf_count = 2,
     .dma_buf_len = 1024  //big buffers to avoid noises
};

int sinTab[256];
int smoothTab[256];

void setup() 
{
  for(int i = 0; i < 256; i++)
  {
    sinTab[i] = (int)(sin(M_PI / 128 * i) * 127);
    float x = i / 128.f;
    if(i < 128)
      smoothTab[i] = int(256 * (x * x * (3 - 2 * x)));
    else
      smoothTab[i] = smoothTab[255 - i];
  }
  
  Serial.begin(115200);
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);    //start i2s driver
  i2s_set_pin(i2s_num, NULL);                           //use internal DAC

  i2s_set_sample_rates(i2s_num, 1000000);               //dummy sample rate, since the function fails at high values

  //this is the hack that enables the highest sampling rate possible ~13MHz, have fun
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S); 
  SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 15, I2S_TX_BCK_DIV_NUM_S);
}

int bpos = 0;
short buff[1024];
void pixel(int x, int y)
{
  buff[bpos++ ^ 1] = x << 8;
  buff[bpos++ ^ 1] = y << 8;
  if(bpos == 1024)
  {
    bpos = 0;
    i2s_write_bytes(i2s_num, (char*)buff, sizeof(buff), portMAX_DELAY);
  }
}

void loop() 
{
  static int t = 0;
  t++;
  for(int j = 0; j < 10; j++)
  {
    for(int i = 0; i < 256; i+=(10 - j))
    {
      pixel(((i * (256 - (10 - j) * 4)) >> 8) + (10 - j) * 2, ((sinTab[(i * 2 + t + j * 10) & 255] * smoothTab[i] * (1 + j)) >> 12) + 160 - j * 4);
    }
  }
  int p = 0;
  int sx = 128 - wave::xres / 2;
  int sy = 255 - (128 - wave::yres / 2);
  for(int y = 0; y < wave::yres; y++)
    for(int x = 0; x < wave::xres; x++)
      if(wave::pixels[p++] < 64)
        pixel(sx + x, sy - y);
}
