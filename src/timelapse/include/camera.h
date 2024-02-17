#ifndef __CAMERA_H__
#define __CAMERA_H__
#include <stdio.h>
#include <string>
#include "comm.h"
class Camera 
{
    public:
        static Camera* shared(){
            if(mInstance==NULL)
            {
                mInstance = new Camera();
            }   
            return mInstance;
        }
        Camera(){
            s32chnlId = 0;
            s32Ret = RK_FAILURE;
            u32Width = 1920;
            u32Height = 1080;
            enCodecType = RK_VIDEO_ID_Unused;
            current_fps = 0;
            video_file = NULL;
            image_file = NULL;
            RK_MPI_SYS_Init();
            vi_dev_init();
            vi_chn_init(s32chnlId, u32Width, u32Height);
        };
        ~Camera()
        {
            deinit();
            RK_MPI_VI_DisableChn(0, s32chnlId);
            RK_MPI_VI_DisableDev(0);
            RK_MPI_SYS_Exit();
        }
        void deinit()
        {
            if (video_file)
            {
                fclose(video_file);
                video_file = NULL;
            }
            if (image_file)
            {
                fclose(image_file);
                image_file = NULL;
            }
            quitMode();
            RK_MPI_VI_DisableChn(0, s32chnlId);
            RK_MPI_VI_DisableDev(0);
            RK_MPI_SYS_Exit();
        }
        void startRecord(int32_t time)
        {
            initVideo();
            quit_thread = false;
            total_frames = time*25;
            char * tmp_file = (char*)malloc(128);
            int n = 1;
            sprintf(tmp_file,"/mnt/sdcard/www/videos/%.4d.avi",n);
            while(true)
            {
                if(isFileExist(tmp_file))
                {
                    n++;
                    sprintf(tmp_file,"/mnt/sdcard/www/videos/%.4d.avi",n);
                }
                else
                {
                    break;
                }
            }
            video_file = fopen(tmp_file, "w");
            free(tmp_file);

            VENC_RECV_PIC_PARAM_S stRecvParam;
            memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
            stRecvParam.s32RecvPicNum = -1;
            RK_MPI_VENC_StartRecvFrame(s32chnlId, &stRecvParam);
            usleep(30000); // sleep 30 ms.
            pthread_create(&main_thread, NULL, GetMediaBuffer0, NULL);
        }
        void stopRecord()
        {
            quit_thread = true;
            quitMode();
            // pthread_join(main_thread, NULL);
        }
        void initVideo()
        {
            if(enCodecType==RK_VIDEO_ID_JPEG)
            {
                quitMode();
            }
            if(enCodecType==RK_VIDEO_ID_AVC)
            {
                return;
            }
            enCodecType = RK_VIDEO_ID_AVC;
            venc_init(0, u32Width, u32Height, enCodecType);

            stSrcChn.enModId = RK_ID_VI;
            stSrcChn.s32DevId = 0;
            stSrcChn.s32ChnId = s32chnlId;

            stDestChn.enModId = RK_ID_VENC;
            stDestChn.s32DevId = 0;
            stDestChn.s32ChnId = 0;
            RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        }

        void initSnap()
        {
            if(enCodecType==RK_VIDEO_ID_AVC)
            {
                quitMode();
            }
            if(enCodecType==RK_VIDEO_ID_JPEG)
            {
                return;
            }
            enCodecType = RK_VIDEO_ID_JPEG;
            venc_init(0, u32Width, u32Height, enCodecType);

            stSrcChn.enModId = RK_ID_VI;
            stSrcChn.s32DevId = 0;
            stSrcChn.s32ChnId = s32chnlId;

            stDestChn.enModId = RK_ID_VENC;
            stDestChn.s32DevId = 0;
            stDestChn.s32ChnId = 0;
            RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        }
        int snap(int id = 1)
        {
            initSnap();
            char * tmp_file = (char*)malloc(128);
            int n = id;
            sprintf(tmp_file,"/mnt/sdcard/www/images/%.5d.jpeg",n);
            if(id>0)
            {
                while(true)
                {
                    if(isFileExist(tmp_file))
                    {
                        n++;
                        sprintf(tmp_file,"/mnt/sdcard/www/images/%.5d.jpeg",n);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            image_file = fopen(tmp_file, "w");
            free(tmp_file);

            VENC_RECV_PIC_PARAM_S stRecvParam;
            memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
            stRecvParam.s32RecvPicNum = 1;
            s32Ret = RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);
            usleep(10000);

	        void *pData = RK_NULL;
            VENC_STREAM_S stFrame;
            stFrame.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S));
            s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
            if (s32Ret == RK_SUCCESS) {
                if (image_file) 
                {
                    pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
                    
                    fwrite(pData, 1, stFrame.pstPack->u32Len, image_file);
                    fflush(image_file);
                }
                s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
            }
            fclose(image_file);
            image_file = NULL;
            return n;
        }
        void quitMode()
        {
            if(enCodecType!=RK_VIDEO_ID_Unused)
            {
                enCodecType = RK_VIDEO_ID_Unused;
                RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
                RK_MPI_VENC_StopRecvFrame(0);
                RK_MPI_VENC_DestroyChn(0);
            }
        }
        bool isFileExist(char*file)
        {
            bool isExist = false;
            FILE *fp = fopen(file , "r");
            if ( fp == NULL )
            {
                isExist = false;
            }
            else
            {
                isExist = true;
                fclose(fp);
            }
            return isExist;
        }
        RK_U64 getNowUs() {
            struct timespec time = {0, 0};
            clock_gettime(CLOCK_MONOTONIC, &time);
            return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
        }
        double current_fps;
        FILE *video_file;
        FILE *image_file;
    private:
        static Camera* mInstance;
        pthread_t main_thread;
        bool quit_thread;
        RK_CODEC_ID_E enCodecType;
        MPP_CHN_S stSrcChn;
        MPP_CHN_S stDestChn;
        RK_S32 s32chnlId;
        RK_S32 s32Ret;
        RK_U32 u32Width;
        RK_U32 u32Height;
        int64_t total_frames;
        RK_S32 venc_init(int chnId, int width, int height, RK_CODEC_ID_E enType) {
            VENC_CHN_ATTR_S stAttr;
            VENC_CHN_PARAM_S stParam;
            memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
            memset(&stParam, 0, sizeof(VENC_CHN_PARAM_S));

            stAttr.stRcAttr.enRcMode = enType==RK_VIDEO_ID_AVC?VENC_RC_MODE_H264VBR:VENC_RC_MODE_MJPEGFIXQP; // jpeg need VENC_RC_MODE_MJPEGFIXQP
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
            if(enType==RK_VIDEO_ID_JPEG)
            {
                stAttr.stVencAttr.stAttrJpege.bSupportDCF = RK_FALSE;
                stAttr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
                stAttr.stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

                VENC_JPEG_PARAM_S stJpegParam;
                stJpegParam.u32Qfactor = 80;
                RK_MPI_VENC_SetJpegParam(0, &stJpegParam);
            }

            RK_MPI_VENC_CreateChn(chnId, &stAttr);

            VENC_RECV_PIC_PARAM_S stRecvParam;
            memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
            stRecvParam.s32RecvPicNum = enType==RK_VIDEO_ID_JPEG?1:-1;
            RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

            return 0;
        }
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

        static void *GetMediaBuffer0(void *arg) {
            (void)arg;
            void *pData = RK_NULL;
            int s32Ret;
            RK_U64 now = Camera::shared()->getNowUs();
            VENC_STREAM_S stFrame;
            stFrame.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S));

            while(!Camera::shared()->quit_thread)
            {
                
                // RK_MPI_VI_ResumeChn(0,s32chnlId);
                // VENC_RECV_PIC_PARAM_S stRecvParam;
                // memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
                // stRecvParam.s32RecvPicNum = 1;
                // RK_MPI_VENC_StartRecvFrame(s32chnlId, &stRecvParam);
                s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
                // RK_MPI_VENC_StopRecvFrame(s32chnlId);
                if (s32Ret == RK_SUCCESS) {
                    if (Camera::shared()->video_file) 
                    {
                        pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
                        
                        fwrite(pData, 1, stFrame.pstPack->u32Len, Camera::shared()->video_file);
                        fflush(Camera::shared()->video_file);
                    }
                    else
                    {
                        printf("video_file err\n");
                    }
                    s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
                    if (s32Ret != RK_SUCCESS) {
                        RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
                    }
                }
                else
                {
                    printf("err: %x\n", s32Ret);
                }
                RK_U64 t = Camera::shared()->getNowUs();
                if(stFrame.pstPack->u32Len>1)
                {
                    Camera::shared()->current_fps = 1000000.0/(t - now);
                    printf("fps:%.1f\n",Camera::shared()->current_fps);
                }
                else
                {
                    printf("no frame:%d\n",stFrame.pstPack->u32Len);
                }
                // RK_MPI_VI_PauseChn(0,s32chnlId);
                now = t;
                usleep(10000);
                if(Camera::shared()->total_frames==-1)
                {
                    continue;
                }
                if(Camera::shared()->total_frames>0)
                {
                    Camera::shared()->total_frames--;
                }
                if(Camera::shared()->total_frames==0)
                {
                    break;
                }
            }
            Camera::shared()->quit_thread = true;
            free(stFrame.pstPack);
            if (Camera::shared()->video_file)
            {
                fclose(Camera::shared()->video_file);
                Camera::shared()->video_file = NULL;
            }
        }
};

Camera* Camera::mInstance = NULL;
#endif