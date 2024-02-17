
// #define SC3336_CHIP_ID_HI_ADDR		0x3107
// #define SC3336_CHIP_ID_LO_ADDR		0x3108
// #define SC3336_CHIP_ID			0xcc41

// int i2c_init(char *dev)
// {
//     int fd = open(dev, O_RDWR);
//     if (fd < 0)
//     {
//         printf("fail to open %s \r\n", dev);
//         exit(1);
//     }
//     return fd;
// }
// int i2c_read(int fd, uint8_t addr, uint8_t reg, uint8_t *val)
// {
//     // int retries;
//     ioctl(fd, I2C_TENBIT, 0);
//     if (ioctl(fd, I2C_SLAVE, addr) < 0)
//     {
//         printf("fail to set i2c device slave address!\n");
//         close(fd);
//         return -1;
//     }
//     ioctl(fd, I2C_RETRIES, 5);

//     if (write(fd, &reg, 1) == 1)
//     {
//         if (read(fd, val, 1) == 1)
//         {
//             return 0;
//         }
//     }
//     else
//     {
//         return -1;
//     }
// }

// int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t val)
// {
//     // int retries;
//     uint8_t data[2];

//     data[0] = reg;
//     data[1] = val;

//     // ioctl(fd, I2C_TENBIT, 0);

//     if (ioctl(fd, I2C_SLAVE, addr) < 0)
//     {
//         printf("fail to set i2c device slave address!\n");
//         close(fd);
//         return -1;
//     }

//     // ioctl(fd, I2C_RETRIES, 5);
// 	int res = write(fd, data, 2);
// 	printf("res:%d\n",res);
// 	return res;
// }