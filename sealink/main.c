#include<gtk/gtk.h>
#include"callbacks.h"
#include"login_window.h"
#include "common.h"

int g_sockfd; 	// communicator
uint8_t* key; 	// key
volatile int g_LoginReady = 0;
volatile uint32_t g_FileSz = 0;

void initialize() {
	LogInit(LOG_PATH);
	key = srv_DoHandshake(g_sockfd, "dh.param");
	if(!key) {
		logEvent(LOG_ERROR, "Handshake failed\n");
		exit(-1);
	}
	return;
}

int main(int argc,gchar *argv[])
{
    //初始化命令行参数
    gtk_init(&argc,&argv);
    gdk_threads_init();
    g_sockfd = srv_TCPConnect("127.0.0.1", SERVER_PORT);
    if(g_sockfd < 0) {
	    logEvent(LOG_ERROR, "cannot connect to server\n");
	    exit(-1);
    }
    initialize();

    create_login_window();
    //主事件循环
    gtk_main();
    return FALSE;
}

