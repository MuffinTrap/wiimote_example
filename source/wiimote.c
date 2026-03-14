#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

// This define is needed to test this code on both versions of libogc
#include <ogc/libversion.h>
#if _V_MAJOR_ == 2
#define LIBOGC_2
#elif _V_MAJOR_ == 3
#define LIBOGC_3
#endif

static void *xfb = NULL;
static bool isSearching = false;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	CON_Init(xfb, 20, 20, rmode->fbWidth-20, rmode->xfbHeight-20, rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Clear the framebuffer
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// This function initialises the attached controllers
	WPAD_Init();

	//tell wpad to output the IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);

	//set console starting position to the second row, to make space for tv's offsets
	printf("\x1b[2;0H");

	printf("Hello World!\n");
	printf("Connect a wiimote by pressing a button\n");
	printf("Or press the red sync button on the wii together with the wiimote!\n");
	printf("to toggle searching for guest wiimotes, press +\n");
	printf("to disconnect all wiimotes, press -\n");
	printf("to exit, press the home\n");
	 
#if defined(LIBOGC_2)
    while(true) 
#elif defined(LIBOGC_3)
    while(SYS_MainLoop())
#endif
	
	{
		//reset console location to 8th row
		printf("\x1b[9;0H");

		// Show searching status
		printf("\33[2K\r");
		if (isSearching)
		{
			printf("Searching...\n");
		}
		else
		{
			printf("Not searching\n");
		}

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();
		
		for (int i = 0; i <= 3; i++)
		{
			//clear line and print the wiimote's data
			printf("\33[2K\r");
			WPADData* data = WPAD_Data(i);
			if(data->data_present)
			{
				printf("wiimote %d: x -> %f y-> %f angle -> %f\n", i, data->ir.x, data->ir.y, data->ir.angle);

				// Clear line and print expansion info
				printf("\33[2K\r");
				printf("Expansion: ");
				if (data->exp.type == EXP_NUNCHUK)
				{
					printf("Nunchuck. ");
					printf("Joystick: %.2f deg. ", data->exp.nunchuk.js.ang);
					printf("Buttons: %x \n", data->exp.nunchuk.btns_held);
				}
				else if (data->exp.type == EXP_CLASSIC)
				{
#                   if defined(LIBOGC_2)
                        printf("Classic controller. ");
#                   elif defined(LIBOGC_3)
					if (data->exp.classic.type == CLASSIC_TYPE_ORIG)
					{
						printf("Original Classic controller. ");
					}
					else if (data->exp.classic.type == CLASSIC_TYPE_PRO)
					{
						printf("Pro Classic controller. ");
					}
					else if (data->exp.classic.type == CLASSIC_TYPE_WIIU)
					{
						printf("WiiU Classic controller. ");
					}
#                   endif
					printf("LJ: %.2f deg. ", data->exp.classic.ljs.ang);
					printf("RJ: %.2f deg. ", data->exp.classic.rjs.ang);
					printf("Buttons: %x \n", data->exp.classic.btns_held);
				}
				else if (data->exp.type == EXP_WII_BOARD)
				{
					printf("Wii board. \n");
				}
				else if (data->exp.type == EXP_GUITAR_HERO_3)
				{
					printf("Guitar. \n");
				}
				else
				{
					printf("N/A. \n");
				}
			}
			else
			{
				printf("wiimote %d: Disconnected\n", i);
			}
		}

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) break;

		if ( pressed & WPAD_BUTTON_PLUS )
		{
			// toggle searching for guest wiimotes
			// these stay active and valid on the wii untill the wiimote subsystem is shutdown
			// when searching is started, all wiimotes will disconnect and the system will start searching for new wiimotes
			// the searching lasts for 60 seconds or so.
 #          if defined(LIBOGC_3)         
			if(isSearching)
				WPAD_StopSearch();
			else
				WPAD_Search();

			isSearching = !isSearching;
#           endif
		}
		if ( pressed & WPAD_BUTTON_MINUS )
		{
			for (int i = 0; i <= 3; i++)
			{
				WPAD_Disconnect(i);
			}
		}

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	// loop over all wiimotes and disconnect them
	// this would shutdown the wiimotes and they would only respond after have pressed a button again
	// under normal circumstances, this is not wanted and is why it's disabled here
	// its more user friendly if the wiimote is left in a seeking state so that the launcher can pick it back up again
#if 0
	for (int i = 0;i < WPAD_MAX_WIIMOTES ;i++)
	{
		if(WPAD_Probe(i,0) < 0)
			continue;
		WPAD_Flush(i);
		WPAD_Disconnect(i);
	}
#endif

	// Shutdown the WPAD system
	// Any wiimotes that are connected will be force disconnected
	// this results in any connected wiimotes to be left in a seeking state
	// in a seeking state the wiimotes will automatically reconnect when the subsystem is reinitialized
	WPAD_Shutdown();

	return 0;
}
