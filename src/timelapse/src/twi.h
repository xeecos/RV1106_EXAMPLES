#ifndef __TWI_H__
#define __TWI_H__
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include "rk_sysfs.h"
#include "rk_gpio.h"
unsigned char twi_dcount = 10;

#define GPIO_SYSFS_PATH	"/sys/class/gpio"
#define GPIO(bank, pin)	((((bank) * 32) + (pin)))
#define SCL   GPIO(RK_GPIO3, RK_PC7)
#define SDA   GPIO(RK_GPIO3, RK_PD0)
int gpio_export(uint32_t gpio)
{
    int ret;
    char file[64];

    ret = sprintf(file, "%s/gpio%d", GPIO_SYSFS_PATH, gpio);
    if (ret < 0)
        return ret;
    if (!access(file, F_OK)) {
        // printf("warning: gpio %d, file (%s) has exist\n", gpio, file);
        return 0;
    }
    return write_sysfs_int("export", GPIO_SYSFS_PATH, gpio);
}

int gpio_unexport(uint32_t gpio)
{
    return write_sysfs_int("unexport", GPIO_SYSFS_PATH, gpio);
}

int gpio_set_direction(uint32_t gpio, enum gpio_direction input)
{
    int ret;
    char path[64];
    const char *direction = input ? "in" : "out";

    ret = sprintf(path, "%s/gpio%d", GPIO_SYSFS_PATH, gpio);
    if (ret < 0)
        return ret;

    return write_sysfs_string("direction", path, direction);
}

int gpio_get_direction(uint32_t gpio)
{
    int ret;
    char path[64];
    char string[15];

    ret = sprintf(path, "%s/gpio%d", GPIO_SYSFS_PATH, gpio);
    if (ret < 0)
        return ret;

    ret = read_sysfs_string("direction", path, string);
    if (ret < 0)
        return ret;

    return (memcmp(string, "out", sizeof("out"))) ? GPIO_DIRECTION_INPUT : GPIO_DIRECTION_OUTPUT;
}

int gpio_export_direction(uint32_t gpio, enum gpio_direction input)
{
    int ret;

    ret = gpio_export(gpio);
    if (ret < 0)
        goto out;

    ret = gpio_set_direction(gpio, input);
    if (ret < 0)
        gpio_unexport(gpio);
out:
    return ret;
}

int gpio_set_value(uint32_t gpio, int value)
{
    int ret;
    char path[64] = {0};
    char direction[8] = {0};

    ret = sprintf(path, "%s/gpio%d", GPIO_SYSFS_PATH, gpio);
    if (ret < 0)
        return ret;

    ret = read_sysfs_string("direction", path, direction);
    if (ret < 0)
        return ret;

    if (strcmp(direction, "out") != 0)
        return -EINVAL;

    return write_sysfs_int("value", path, value);
}

int gpio_get_value(uint32_t gpio)
{
    int ret;
    char path[64];

    ret = sprintf(path, "%s/gpio%d", GPIO_SYSFS_PATH, gpio);
    if (ret < 0)
        return ret;

    return read_sysfs_posint("value", path);
}


static void SDA_OUTPUT()
{
    gpio_export_direction(SDA, GPIO_DIRECTION_OUTPUT);
}
static inline void SDA_LOW() {
    gpio_set_value(SDA, 0);
}

static inline void SDA_HIGH() {
    gpio_set_value(SDA, 1);
}
static void SDA_INPUT()
{
    gpio_export_direction(SDA, GPIO_DIRECTION_INPUT);
}
static inline uint32_t SDA_READ() { 
    return gpio_get_value(SDA);
}

static void SCL_OUTPUT()
{
    gpio_export_direction(SCL, GPIO_DIRECTION_OUTPUT);
}
static void SCL_LOW() {
    gpio_set_value(SCL, 0);
}

static void SCL_HIGH() {
    gpio_set_value(SCL, 1);
}

static void SCL_INPUT()
{
    gpio_export_direction(SCL, GPIO_DIRECTION_INPUT);
}
static uint32_t SCL_READ() {
    return gpio_get_value(SCL);
}

#define TWI_CLOCK_STRETCH 10

void twi_setClock(unsigned int freq){
    twi_dcount = 10;
}

void twi_init(){
    SDA_OUTPUT();
    SCL_OUTPUT();
    SDA_LOW();
    SCL_LOW();
    SDA_HIGH();
    SCL_HIGH();
    SDA_INPUT();
    SCL_INPUT();
    twi_setClock(100000);
    SCL_OUTPUT();
    SDA_OUTPUT();
}

void twi_stop(void){
    SDA_INPUT();
    SCL_INPUT();
}

static void twi_delay(uint32_t v){
    usleep(v);
}

static bool twi_write_start(void) {

    SCL_HIGH();
    SDA_HIGH();
    SDA_INPUT();
    if (SDA_READ() == 0) 
    {

        SDA_OUTPUT();
        return false;
    }
    SDA_OUTPUT();
    twi_delay(twi_dcount);
    SDA_LOW();
    twi_delay(twi_dcount);
    return true;
}

static bool twi_write_stop(void){
  unsigned int i = 0;
  SCL_LOW();
  SDA_LOW();
  twi_delay(twi_dcount);
  SCL_HIGH();
  SCL_INPUT();
  while (SCL_READ() == 0 && (i++) < TWI_CLOCK_STRETCH)
  {
    // Clock stretching (up to 100us)
    twi_delay(1);
  }
  SCL_OUTPUT();
  twi_delay(twi_dcount);
  SDA_HIGH();
  twi_delay(twi_dcount);

  return true;
}

bool do_log = false;
static bool twi_write_bit(bool bit) {
  unsigned int i = 0;
  SCL_LOW();
  if (bit) {SDA_HIGH();}
  else {SDA_LOW();}
  twi_delay(twi_dcount+1);
  SCL_HIGH();
  SCL_INPUT();
  while (SCL_READ() == 0 && (i++) < TWI_CLOCK_STRETCH)
  {
    // Clock stretching (up to 100us)
    twi_delay(1);
  }
  twi_delay(twi_dcount);
  SCL_OUTPUT();
  return true;
}

static bool twi_read_bit(void) {
  unsigned int i = 0;
  SCL_LOW();
  SDA_HIGH();
  twi_delay(twi_dcount+2);
  SCL_HIGH();
  SCL_INPUT();
  while (SCL_READ() == 0 && (i++) < TWI_CLOCK_STRETCH)
  {
    // Clock stretching (up to 100us)
    twi_delay(1);
  }
  SDA_INPUT();
  bool bit = SDA_READ();
  twi_delay(twi_dcount);
  SDA_OUTPUT();
  SCL_OUTPUT();
  return bit;
}

static bool twi_write_byte(unsigned char byte) {
  
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) {
    twi_write_bit((byte & 0x80) != 0);
    byte <<= 1;
  }
  return !twi_read_bit();//NACK/ACK
}

static unsigned char twi_read_byte(bool nack) {
  unsigned char byte = 0;
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) byte = (byte << 1) | twi_read_bit();
  twi_write_bit(nack);
  return byte;
}

unsigned char twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop){
    unsigned int i;
    if(!twi_write_start()) return 4;//line busy
    if(!twi_write_byte(((address << 1) | 0) & 0xFF)) {
        if (sendStop) twi_write_stop();
        return 2; //received NACK on transmit of address
    }
    for(i=0; i<len; i++) {
        if(!twi_write_byte(buf[i])) {
        if (sendStop) twi_write_stop();
        return 3;//received NACK on transmit of data
        }
    }
    if(sendStop) twi_write_stop();
    i = 0;
    SDA_INPUT();
    while(SDA_READ() == 0 && (i++) < 10){
        SCL_LOW();
        twi_delay(twi_dcount);
        SCL_HIGH();
        twi_delay(twi_dcount);
    }
    SDA_OUTPUT();
    return 0;
}

unsigned char twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop){
    unsigned int i;
    if(!twi_write_start()) return 4;//line busy
    if(!twi_write_byte(((address << 1) | 1) & 0xFF)) {
        if (sendStop) twi_write_stop();
        return 2;//received NACK on transmit of address
    }
    for(i=0; i<(len-1); i++) buf[i] = twi_read_byte(false);
    buf[len-1] = twi_read_byte(true);
    if(sendStop) twi_write_stop();
    i = 0;
    SDA_INPUT();
    while(SDA_READ() == 0 && (i++) < 10){
        SCL_LOW();
        twi_delay(twi_dcount);
        SCL_HIGH();
        twi_delay(twi_dcount);
    }
    SDA_OUTPUT();
    return 0;
}
#endif