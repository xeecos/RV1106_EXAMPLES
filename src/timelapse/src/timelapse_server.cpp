#include <stdio.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "httplib.h"
#include "comm.h"
// #define MINIMP4_IMPLEMENTATION
// #include "minimp4.h"

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

static bool quit = false;
pthread_t main_thread;
FILE *file = NULL;
MPP_CHN_S stSrcChn, stDestChn;
RK_S32 s32Ret = RK_FAILURE;
RK_U32 u32Width = 1920;
RK_U32 u32Height = 1080;
RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
RK_CHAR *pCodecName = "H264";
RK_S32 s32chnlId = 1;

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

RK_U64 getNowUs() {
	struct timespec time = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
}
static RK_S32 venc_init(int chnId, int width, int height, RK_CODEC_ID_E enType) {
	printf("========%s========\n", __func__);
	VENC_RECV_PIC_PARAM_S stRecvParam;
	VENC_CHN_ATTR_S stAttr;
	VENC_CHN_PARAM_S stParam;
	memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
	memset(&stParam, 0, sizeof(VENC_CHN_PARAM_S));

	stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR; // jpeg need VENC_RC_MODE_MJPEGFIXQP
	stAttr.stRcAttr.stH264Cbr.u32BitRate = 10 * 1024;
	stAttr.stRcAttr.stH264Cbr.u32Gop = 60;

	stAttr.stVencAttr.enType = enType;
	stAttr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
	stAttr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
	stAttr.stVencAttr.u32PicWidth = width;
	stAttr.stVencAttr.u32PicHeight = height;
	stAttr.stVencAttr.u32VirWidth = width;
	stAttr.stVencAttr.u32VirHeight = height;
	stAttr.stVencAttr.u32StreamBufCnt = 2;
	stAttr.stVencAttr.u32BufSize = width * height * 3 / 2;
	stAttr.stVencAttr.enMirror = MIRROR_NONE;

	// stAttr.stVencAttr.stAttrJpege.bSupportDCF = RK_FALSE;
	// stAttr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
	// stAttr.stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

	RK_MPI_VENC_CreateChn(chnId, &stAttr);

	stParam.stFrameRate.bEnable = RK_TRUE;
	stParam.stFrameRate.s32SrcFrmRateNum = 2;
	stParam.stFrameRate.s32SrcFrmRateDen = 1;
	stParam.stFrameRate.s32DstFrmRateNum = 25;
	stParam.stFrameRate.s32DstFrmRateDen = 1;
	RK_MPI_VENC_SetChnParam(chnId, &stParam);

	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = 1;
	RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

	return 0;
}

// demo板dev默认都是0，根据不同的channel 来选择不同的vi节点
int vi_dev_init() {
	printf("%s\n", __func__);
	int ret = 0;
	int devId = 0;
	int pipeId = devId;

	VI_DEV_ATTR_S stDevAttr;
	VI_DEV_BIND_PIPE_S stBindPipe;
	memset(&stDevAttr, 0, sizeof(stDevAttr));
	memset(&stBindPipe, 0, sizeof(stBindPipe));
	// 0. get dev config status
	ret = RK_MPI_VI_GetDevAttr(devId, &stDevAttr);
	if (ret == RK_ERR_VI_NOT_CONFIG) {
		// 0-1.config dev
		ret = RK_MPI_VI_SetDevAttr(devId, &stDevAttr);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_SetDevAttr %x\n", ret);
			return -1;
		}
	} else {
		printf("RK_MPI_VI_SetDevAttr already\n");
	}
	// 1.get dev enable status
	ret = RK_MPI_VI_GetDevIsEnable(devId);
	if (ret != RK_SUCCESS) {
		// 1-2.enable dev
		ret = RK_MPI_VI_EnableDev(devId);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_EnableDev %x\n", ret);
			return -1;
		}
		// 1-3.bind dev/pipe
		stBindPipe.u32Num = pipeId;
		stBindPipe.PipeId[0] = pipeId;
		ret = RK_MPI_VI_SetDevBindPipe(devId, &stBindPipe);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_SetDevBindPipe %x\n", ret);
			return -1;
		}
	} else {
		printf("RK_MPI_VI_EnableDev already\n");
	}

	return 0;
}

int vi_chn_init(int channelId, int width, int height) {
	int ret;
	// VI init
	VI_CHN_ATTR_S vi_chn_attr;
	memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
	vi_chn_attr.stIspOpt.u32BufCount = 4;
	vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF; // VI_V4L2_MEMORY_TYPE_MMAP;
	vi_chn_attr.stSize.u32Width = width;
	vi_chn_attr.stSize.u32Height = height;
	FRAME_RATE_CTRL_S sfr;
	sfr.s32DstFrameRate = 25;
	sfr.s32SrcFrameRate = 2;
	vi_chn_attr.stFrameRate = sfr;
	vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
	vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE; // COMPRESS_AFBC_16x16;
	vi_chn_attr.u32Depth = 2;
	ret = RK_MPI_VI_SetChnAttr(0, channelId, &vi_chn_attr);
	ret |= RK_MPI_VI_EnableChn(0, channelId);
	if (ret) {
		printf("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	return ret;
}

static FILE *venc0_file;
static void *GetMediaBuffer0(void *arg) {
	(void)arg;
	printf("========%s========\n", __func__);
	void *pData = RK_NULL;
	int s32Ret;

	RK_U64 now = getNowUs();

	VENC_STREAM_S stFrame;
	stFrame.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S));

    while(!quit)
	{
		
		RK_MPI_VI_ResumeChn(0,s32chnlId);
        VENC_RECV_PIC_PARAM_S stRecvParam;
		memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
		stRecvParam.s32RecvPicNum = 1;
		RK_MPI_VENC_StartRecvFrame(s32chnlId, &stRecvParam);
		s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
		RK_MPI_VENC_StopRecvFrame(s32chnlId);
		if (s32Ret == RK_SUCCESS) {
			if (venc0_file) 
			{
				pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
				
				fwrite(pData, 1, stFrame.pstPack->u32Len, venc0_file);
				fflush(venc0_file);
			}
			s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
			if (s32Ret != RK_SUCCESS) {
				RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
			}
		}
		else
		{
			printf("no frame\n");
		}
		RK_U64 t = getNowUs();
		if(stFrame.pstPack->u32Len>1000)
		{
			printf("fps:%.1f\n",1000000.0/(t - now));
		}
		else
		{
			printf("no frame\n");
		}
		RK_MPI_VI_PauseChn(0,s32chnlId);
		now = t;
		usleep(500000);
		// sleep(1);
	}
	free(stFrame.pstPack);
	if (venc0_file)
	{
		fclose(venc0_file);
		venc0_file = NULL;
	}
}

static void sigterm_handler(int sig) {
	quit = true;
	if (venc0_file)
		fclose(venc0_file);
	printf("close file\n");
	exit(0);
}

void deinitCamera()
{
    pthread_join(main_thread, NULL);
	RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    RK_MPI_VI_DisableChn(0, s32chnlId);
    RK_MPI_VENC_StopRecvFrame(0);
    RK_MPI_VENC_DestroyChn(0);
	RK_MPI_VI_DisableDev(0);
	RK_MPI_SYS_Exit();
}
void initCamera()
{

	venc0_file = fopen("/mnt/sdcard/tmp.avi", "w");

	if (RK_MPI_SYS_Init() != RK_SUCCESS) {
        
	}

	vi_dev_init();
	vi_chn_init(s32chnlId, u32Width, u32Height);
	venc_init(0, u32Width, u32Height, enCodecType);

	// bind vi to venc
	stSrcChn.enModId = RK_ID_VI;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = s32chnlId;

	stDestChn.enModId = RK_ID_VENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = 0;
	printf("====RK_MPI_SYS_Bind vi0 to venc0====\n");
	s32Ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
	if (s32Ret != RK_SUCCESS) {
		RK_LOGE("bind 0 ch venc failed");
	}



	// VENC_JPEG_PARAM_S stJpegParam;
	// stJpegParam.u32Qfactor = 77;
	// RK_MPI_VENC_SetJpegParam(0, &stJpegParam);
}
int fd = 0;
// uint8_t SCCB_Probe()
// {
//     uint8_t reg = 0x00;
//     uint8_t slv_addr = 0x00;

//     for (uint8_t i=0; i<127; i++) {
// 		int res = twi_writeTo(slv_addr, &reg, 1, true);
//         if (res == 0) {
//             slv_addr = i;
//             break;
//         }
// 		printf("addr:%x res:%d\n",i,res);  
//         twi_delay(1000); 
//     } 
//     return slv_addr;
// }
int main()
{
    printf("timelapse server init\n");
    signal(SIGINT, sigterm_handler);

	// gpio_export_direction(GPIO(RK_GPIO3,RK_PC5), GPIO_DIRECTION_OUTPUT);
	// gpio_set_value(GPIO(RK_GPIO3,RK_PC5), 1);
	// usleep(100);
	// gpio_set_value(GPIO(RK_GPIO3,RK_PC5), 0);
	// usleep(100);
    initCamera(); 
	// fd = i2c_init("/dev/i2c-4");
	// twi_init();
	// printf("i2cdetect addr : %x\n",SCCB_Probe());
	// while(1)
	// { 
    // 	gpio_export_direction(SCL, GPIO_DIRECTION_OUTPUT);
	// 	gpio_set_value(SCL, 1);
	// 	usleep(1000);
    // 	gpio_export_direction(SCL, GPIO_DIRECTION_OUTPUT);
	// 	gpio_set_value(SCL, 0);
	// 	usleep(1000);
	// }

    // HTTP
    httplib::Server svr;

    // svr.Get("/i2c/set", [](const httplib::Request &req, httplib::Response &res) {
	// 	if (req.has_param("addr")) {
    //         int addr = stoi(req.get_param_value("addr"));
	// 		printf("i2cdetect addr : ");
	// 		if (i2c_write(fd, 0x30, 0, 0) == 0)
	// 		{
	// 			printf("0x%x,", addr);
	// 		}
	// 		printf("\r\n");
    //     }
    //     res.set_content("i2c", "text/plain");
    // });
    svr.Get("/exposure/get", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content("get exp", "text/plain");
    });
    svr.Get("/exposure/set", [](const httplib::Request &req, httplib::Response &res) {
        if (req.has_param("v")) {
            int v = stoi(req.get_param_value("v"));
            char * out = (char*)malloc(128);
            sprintf(out,"{\"exp\":%d}", v);
            res.set_content(out, "application/json");
            free(out);

            char * expv = (char*)malloc(128);
            sprintf(expv,  "exposure=%d", v);
			char* args[] = {"","-d","/dev/v4l-subdev2","--set-ctrl",expv, NULL};
			if(fork()==0){
				execv("/usr/bin/v4l2-ctl", args);
			}
			free(expv);
            return;
        }
        res.set_content("{\"res\":\"no exp\"}", "application/json");
    });
    svr.Get("/gain/set", [](const httplib::Request &req, httplib::Response &res) {
        if (req.has_param("v")) {
            int v = stoi(req.get_param_value("v"));
            char * out = (char*)malloc(128);
            sprintf(out,"{\"gain\":%d}", v);
            res.set_content(out, "application/json");
            free(out);

            char * expv = (char*)malloc(128);
            sprintf(expv,  "analogue_gain=%d", v);
			char* args[] = {"","-d","/dev/v4l-subdev2","--set-ctrl",expv, NULL};
			if(fork()==0){
				execv("/usr/bin/v4l2-ctl", args);
			}
			free(expv);
            return;
        }
        res.set_content("{\"res\":\"no gain\"}", "application/json");
    });
    svr.Get("/vbk/set", [](const httplib::Request &req, httplib::Response &res) {
        if (req.has_param("v")) {
            int v = stoi(req.get_param_value("v"));
            char * out = (char*)malloc(128);
            sprintf(out,"{\"vertical_blanking\":%d}", v);
            res.set_content(out, "application/json");
            free(out);

            char * expv = (char*)malloc(128);
            sprintf(expv,  "vertical_blanking=%d", v);
			char* args[] = {"","-d","/dev/v4l-subdev2","--set-ctrl",expv, NULL};
			if(fork()==0){
				execv("/usr/bin/v4l2-ctl", args);
			}
			free(expv);
            return;
        }
        res.set_content("{\"res\":\"no vertical_blanking\"}", "application/json");
    });
    svr.Get("/hbk/set", [](const httplib::Request &req, httplib::Response &res) {
        if (req.has_param("v")) {
            int v = stoi(req.get_param_value("v"));
            char * out = (char*)malloc(128);
            sprintf(out,"{\"horizontal_blanking\":%d}", v);
            res.set_content(out, "application/json");
            free(out);

            char * expv = (char*)malloc(128);
            sprintf(expv,  "horizontal_blanking=%d", v);
			char* args[] = {"","-d","/dev/v4l-subdev2","--set-ctrl",expv, NULL};
			if(fork()==0){
				execv("/usr/bin/v4l2-ctl", args);
			}
			free(expv);
            return;
        }
        res.set_content("{\"res\":\"no horizontal_blanking\"}", "application/json");
    });
    svr.Get("/snap", [](const httplib::Request &req, httplib::Response &res) {

        quit = false;
        usleep(30000); // sleep 30 ms.

		pthread_create(&main_thread, NULL, GetMediaBuffer0, NULL);
        res.set_content("{\"res\":\"snap\"}", "application/json");
    });
    svr.Get("/stop", [](const httplib::Request &req, httplib::Response &res) {
        quit = true;
		usleep(30000);
    	pthread_join(main_thread, NULL);
        res.set_content("{\"res\":\"quit\"}", "application/json");

        // deinitCamera();
    });
    svr.set_mount_point("/", "./");
    svr.listen("0.0.0.0", 8080);
    return 0;
}