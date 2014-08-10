/*
 * Copyright (C) 2014 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * 请阅读xf86-input-evdev源代码了解mtdev如何使用
 */

#include <fstream>
#include <vector>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtdev.h>
#include <X11/extensions/XTest.h>

#include "gesture_recognition.h"

#define DEVICE "/dev/input/event13"
#define SAMPLE_FILE "sample.txt"

// 可以使用xev来显示keycode
#define CTRL_KEYCODE 37                                                         
#define ALTR_KEYCODE 64                                                         
#define UP_KEYCODE 111
#define DOWN_KEYCODE 116
#define LEFT_KEYCODE 113                                                        
#define RIGHT_KEYCODE 114                                                       
#define WIN_KEYCODE 133                                                         
#define MENU_KEYCODE 135                                                        
#define PLUS_KEYCODE 21                                                  
#define MINUS_KEYCODE 20                                                        
#define D_KEYCODE 40                                                            
#define W_KEYCODE 25

#define ZOOM_TIMES 26

#define CHECK(dev, name)            \
    if (mtdev_has_mt_event(dev, name))  \
        printf("support %s\n", #name);  \
    else    \
        printf("unsupport %s\n", #name);

typedef struct {
	int slot;
	int x;
	int y;
} point_t;

static Display* m_dpy = NULL;
static struct mtdev* m_dev = NULL;
static int m_fd = -1;
static std::vector<MTGR::sample_t> m_samples;
static std::ofstream m_sample_file;
static int m_slot = 0;
static int m_x = -1;
static int m_y = -1;
static int m_num_fingers = 0;
static int m_zoom_times = ZOOM_TIMES;
static bool m_two_fingers = false;
static bool m_four_fingers = false;

static void m_process_absolute_motion_event(struct input_event* ev)
{
	int value = ev->value;

	if (ev->code > ABS_MAX)
		return;

	if (ev->code == ABS_MT_SLOT) {
		m_slot = value;
		m_x = -1;
		m_y = -1;
	}

	if (ev->code == ABS_MT_POSITION_X) 
		m_x = value;

	if (ev->code == ABS_MT_POSITION_Y) 
		m_y = value;

	if (ev->code == ABS_MT_TRACKING_ID) {
        if (value >= 0) 
            m_num_fingers++;
        else {
            m_num_fingers--;
		    m_x = -1;
		    m_y = -1;
        }
	}

	if (m_x == -1 || m_y == -1)
		return;

    if (m_num_fingers == 2) 
        m_two_fingers = true;

    if (m_num_fingers == 4) { 
        m_four_fingers = true;
        m_two_fingers = false;
    }

    MTGR::point_t point;
    point.x = m_x;
    point.y = m_y;
    MTGR::sample_t sample;
    sample.slot = m_slot;
    sample.point = point;
    m_samples.push_back(sample);
    // FIXME: 手指头还没有离开触摸板的时候，就可以实时判断手势咯
    // 但是，会触发太多次zoom in/out事件，在类似TouchEgg应用的时候，
    // 就会一下子就放大到太大，或一下子就缩小到太小，在实际应用的时候，
    // 还有很多细节要处理。
    MTGR::GestureRecognition gr;
    switch (gr.predict(m_samples)) {
    case 5:
        if (m_two_fingers && !m_zoom_times && m_dpy) {
            XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);            
            XTestFakeKeyEvent(m_dpy, MINUS_KEYCODE, True, 0);           
            XTestFakeKeyEvent(m_dpy, MINUS_KEYCODE, False, 0);          
            XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);           
            XFlush(m_dpy);
            m_zoom_times = ZOOM_TIMES;
        } else {
            m_zoom_times--;
        }
        break;
    case 6:
        if (m_two_fingers && !m_zoom_times && m_dpy) {                          
            XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);                    
            XTestFakeKeyEvent(m_dpy, PLUS_KEYCODE, True, 0);                   
            XTestFakeKeyEvent(m_dpy, PLUS_KEYCODE, False, 0);                  
            XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);                   
            XFlush(m_dpy);                                                      
            m_zoom_times = ZOOM_TIMES;                                          
        } else {                                                                
            m_zoom_times--;                                                     
        }
        break;
    }

    printf("DEBUG: slot[%d] (x, y) = (%d, %d) num_fingers %d\n", 
           m_slot, m_x, m_y, m_num_fingers);
	m_sample_file << m_slot << " " << m_x << " " << m_y << "\n";
    m_sample_file.flush();
}

static void m_process_event(struct input_event *ev)
{
    MTGR::GestureRecognition gr;

    switch (ev->type) {
    case EV_SYN:
        switch (ev->code) {
        case SYN_REPORT:
            if (m_num_fingers == 0) { 
                switch (gr.predict(m_samples)) {
                case 0:
                    printf("DEBUG: predict NULL\n");
                    break;
                case 1:
                    printf("DEBUG: predict rightward\n");
                    if (m_four_fingers && m_dpy) {                                   
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);    
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, True, 0);    
                        XTestFakeKeyEvent(m_dpy, RIGHT_KEYCODE, True, 0);   
                        XTestFakeKeyEvent(m_dpy, RIGHT_KEYCODE, False, 0);  
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, False, 0);   
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);   
                        XFlush(m_dpy);                                      
                    }
                    break;
                case 2:
                    printf("DEBUG: predict leftward\n");
                    if (m_four_fingers && m_dpy) {                                   
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);    
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, True, 0);    
                        XTestFakeKeyEvent(m_dpy, LEFT_KEYCODE, True, 0);    
                        XTestFakeKeyEvent(m_dpy, LEFT_KEYCODE, False, 0);   
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, False, 0);   
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);   
                        XFlush(m_dpy);                                      
                    }
                    break;
                case 3:
                    printf("DEBUG: predict upward\n");
                    if (m_four_fingers && m_dpy) {
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);       
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, True, 0);       
                        XTestFakeKeyEvent(m_dpy, UP_KEYCODE, True, 0);   
                        XTestFakeKeyEvent(m_dpy, UP_KEYCODE, False, 0);  
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, False, 0);   
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);   
                        XFlush(m_dpy);
                    }
                    break;
                case 4:
                    printf("DEBUG: predict downward\n");
                    if (m_four_fingers && m_dpy) {                              
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);        
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, True, 0);        
                        XTestFakeKeyEvent(m_dpy, DOWN_KEYCODE, True, 0);          
                        XTestFakeKeyEvent(m_dpy, DOWN_KEYCODE, False, 0);         
                        XTestFakeKeyEvent(m_dpy, ALTR_KEYCODE, False, 0);          
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);          
                        XFlush(m_dpy);                                          
                    }
                    break;
                case 5:
                    printf("DEBUG: predict zoom in\n");
                    if (m_four_fingers && m_dpy) {                                   
                        XTestFakeKeyEvent(m_dpy, WIN_KEYCODE, True, 0);        
                        XTestFakeKeyEvent(m_dpy, WIN_KEYCODE, False, 0);       
                        XFlush(m_dpy);                                      
                    }
                    break;
                case 6:
                    printf("DEBUG: predict zoom out\n");
                    if (m_two_fingers && m_dpy) {
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, True, 0);
                        XTestFakeKeyEvent(m_dpy, PLUS_KEYCODE, True, 0);
                        XTestFakeKeyEvent(m_dpy, PLUS_KEYCODE, False, 0);
                        XTestFakeKeyEvent(m_dpy, CTRL_KEYCODE, False, 0);
                        XFlush(m_dpy);
                    }
                    break;
                case 7:
                    printf("DEBUG: predict rotate clockwise\n");
                    break;
                case 8:
                    printf("DEBUG: predict rotate anti-clockwise\n");
                    break; 
                }
                m_samples.clear();
                m_four_fingers = false;
            }
            printf("DEBUG: [SYN_REPORT]\n");
            printf("DEBUG: num_fingers %d\n", m_num_fingers);
            break;
        }
        break;
    case EV_ABS:
		m_process_absolute_motion_event(ev);
		break;
	}
}

static void m_cleanup()
{
	XCloseDisplay(m_dpy);
    m_dpy = NULL;
    
    m_sample_file.close();

	if (m_dev) {
		mtdev_close_delete(m_dev);
		m_dev = NULL;
	}

	if (m_fd) {
		close(m_fd);
		m_fd = -1;
	}
}

static void m_signal_callback_handler(int signum)
{
	printf("Bye :)\n");
	m_cleanup();
	exit(signum);
}

int main(int argc, char *argv[])
{
	struct input_event ev;

    m_dpy = XOpenDisplay(NULL);
    if (!m_dpy) 
        printf("fail top open x display\n");

	m_fd = open(argv[1] ? argv[1] : DEVICE, O_RDONLY);
	if (m_fd == -1) {
		printf("fail to open device\n");
		return -1;
	}

	if (m_dev = mtdev_new_open(m_fd)) {
		unlink(SAMPLE_FILE);
		m_sample_file.open(SAMPLE_FILE);

		CHECK(m_dev, ABS_MT_SLOT);
		CHECK(m_dev, ABS_MT_TOUCH_MAJOR);
		CHECK(m_dev, ABS_MT_TOUCH_MINOR);
		CHECK(m_dev, ABS_MT_WIDTH_MAJOR);
		CHECK(m_dev, ABS_MT_WIDTH_MINOR);
		CHECK(m_dev, ABS_MT_ORIENTATION);
		CHECK(m_dev, ABS_MT_POSITION_X);
		CHECK(m_dev, ABS_MT_POSITION_Y);
		CHECK(m_dev, ABS_MT_TOOL_TYPE);
		CHECK(m_dev, ABS_MT_BLOB_ID);
		CHECK(m_dev, ABS_MT_TRACKING_ID);
		CHECK(m_dev, ABS_MT_PRESSURE);
		CHECK(m_dev, ABS_MT_DISTANCE);
		CHECK(m_dev, ABS_MT_TOOL_X);
		CHECK(m_dev, ABS_MT_TOOL_Y);

		printf("ctrl+c to quit\n");
		signal(SIGINT, m_signal_callback_handler);

	    while (mtdev_get(m_dev, m_fd, &ev, 1) * sizeof(struct input_event))
			m_process_event(&ev);
	}

	return 0;
}
