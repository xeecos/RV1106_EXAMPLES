#include <stdio.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "httplib.h"
#include "camera.h"
static Camera *cam;
static void sigterm_handler(int sig) {
	cam->deinit();
	exit(0);
}

int main()
{
    printf("timelapse server init\n");
    signal(SIGINT, sigterm_handler);

    cam = Camera::shared(); 

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
        int n = cam->snap();
		char * tmp_file = (char*)malloc(128);
		sprintf(tmp_file,"{\"res\":\"/images/%.5d.jpeg\"}",n);
        res.set_content(tmp_file, "application/json");
		free(tmp_file);
    });
	svr.Get("/preview", [](const httplib::Request &req, httplib::Response &res) {
        int n = cam->snap(0);
		char * tmp_file = (char*)malloc(128);
		sprintf(tmp_file,"/mnt/sdcard/www/images/%.5d.jpeg\0",n);
		std::ifstream in(tmp_file, std::ios::in | std::ios::binary);
		if(in){
			std::ostringstream contents;
			contents << in.rdbuf();
			in.close();
			res.set_content(contents.str(), "image/jpeg");
		}
		free(tmp_file);
    });
    svr.Get("/record/start", [](const httplib::Request &req, httplib::Response &res) {

		cam->startRecord(10);
        res.set_content("{\"res\":\"start record\"}", "application/json");
    });
    svr.Get("/record/stop", [](const httplib::Request &req, httplib::Response &res) {
		cam->stopRecord();
        res.set_content("{\"res\":\"stop record\"}", "application/json");

        // deinitCamera();
    });
    svr.Get("/fps", [](const httplib::Request &req, httplib::Response &res) {
		char * out = (char*)malloc(128);
		sprintf(out,  "{\"fps\":\"%.1f\"}", cam->current_fps);
        res.set_content(out, "application/json");
		free(out);
    });
    svr.set_mount_point("/", "/mnt/sdcard/www");
    svr.listen("0.0.0.0", 8080);
    return 0;
}