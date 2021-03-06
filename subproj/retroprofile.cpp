#if 0
<<COMMENT1
//retroprofile - a headless libretro frontend, for performance testing and PGO compilation
//usage: retroprofile corepath rompath frames
//example: ./retroprofile roms/gambatte_libretro.so roms/zeldaseasons.gbc 10000
COMMENT1

cd subproj || true
cd ..
g++ -I. subproj/retroprofile.cpp \
	libretro.cpp dylib.cpp memory.cpp video.cpp \
	-ldl -lrt -DDYLIB_POSIX -DWINDOW_MINIMAL window-none.cpp -Os -s -o retroprofile
mv retroprofile ~/bin

<<COMMENT2
#endif

#include "minir.h"
#include <stdlib.h>
#include <stdio.h>

void no_audio(struct audio * This, unsigned int numframes, const int16_t * samples) {}
int16_t no_input(struct libretroinput * This, unsigned port, unsigned device, unsigned index, unsigned id) { return 0; }

int main(int argc, char * argv[])
{
	//window_init(&argc, &argv);
	unsigned int frames=atoi(argv[3]);
	
	uint64_t t_init=window_get_time();
	struct libretro * core=libretro_create(argv[1], NULL, NULL);
	if (!core)
	{
		puts("Couldn't load core.");
		return 1;
	}
	
	video* v=list_video[0]->create2d(0);
	
	static struct audio noaudio;
	noaudio.render=no_audio;
	
	static struct libretroinput noinput;
	noinput.query=no_input;
	
	core->attach_interfaces(core, v, &noaudio, &noinput);
	
	if (!core->load_rom(core, NULL,0, argv[2]))
	{
		puts("Couldn't load ROM.");
		return 1;
	}
	
	uint64_t t_run=window_get_time();
	
	unsigned int fps=0;
	while (window_get_time() < t_run+1000000 && fps < frames)
	{
		core->run(core);
		fps++;
	}
	for (unsigned int i=fps;i<frames;i++)
	{
		if (i%fps==0) printf("%i/%i\r", i, frames),fflush(stdout);
		core->run(core);
	}
	uint64_t t_run_done=window_get_time();
	
	core->free(core);
	uint64_t t_end=window_get_time();
	
	printf("Time to load ROM: %luus\n", t_run-t_init);
	if (frames)
	{
		printf("Time per frame: %luus / ", (t_run_done-t_run)/frames);
		printf("Frames per second: %f\n", (float)frames*1000000/(t_run_done-t_run));
	}
	printf("Time to exit: %luus\n", t_end-t_run_done);
	printf("Total time: %luus\n", t_end-t_init);
}
#if 0
COMMENT2
#endif
