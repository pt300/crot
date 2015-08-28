/*
 * crot.c
 *
 *  Created on: 27-08-2015
 *      Author: patryk
 */

#include "crot.h"

static struct termios stored_settings;

int main(int argc, char** argv) {
	FILE* oy;
	uint8_t flags = 1;//1 - EOF, 2 - reported error, 3 - img type
	uint8_t* imgsrc;
	if((oy = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Error opening file %s.\n", argv[1]);
		return EXIT_FAILURE;
	}
	int byte;
	int cnt = 4;
	const uint8_t magic_rev[4] = "EMEM";	//it's backwards.
	while((byte = fgetc(oy)) != EOF) {	//magic
		if(byte!=magic_rev[--cnt]) {
			fprintf(stderr, "Invalid magic.\n");
			flags = 2;
			break;
		}
		if(!cnt) {
			flags = 0;
			break;
		}
	}
	if(!(flags&3)) {	//type
		byte = fgetc(oy);
		if(byte == EOF) {
			flags |= 1;
		}
		else if(byte>1) {
			fprintf(stderr, "Invalid image type \"%i\".\nExpected 0 - B/W or 1-RGB.\n", byte);
			flags |= 2;
		}
		else {
			flags |= byte<<2;
		}
	}
	if(!(flags&3)) {
		cnt = 450;
		if(flags&4)
			cnt = 10800;
		flags |= 1;
		imgsrc = calloc(cnt, sizeof(uint8_t));
		int fcnt = 0;
		while((byte = fgetc(oy)) != EOF) { //data
			imgsrc[fcnt++] = (uint8_t)byte;
			if(fcnt==cnt) {
				flags &= ~1;
				if(fgetc(oy) != EOF) {
					fprintf(stderr, "Expected EOF, got data.\n");
					free(imgsrc);
					flags |= 2;
				}
				break;
			}
		}
	}
	if(flags&1) {
		fprintf(stderr, "Unexpected EOF.\n");
	}
	fclose(oy);
	if(flags&3) {
		return EXIT_FAILURE;
	}
	setup_screen();
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int s = w.ws_col>w.ws_row*2?w.ws_row*2:w.ws_col;
	int sx = (w.ws_col-s)/2;
	int sy = (w.ws_row-s/2)/2;
	int rt = 360;
	if(flags&4)
		cnt = s*s*3;
	else
		cnt = (s*s)/8+!!((s*s)%8);
	uint8_t* rotout = calloc(cnt, sizeof(uint8_t));
	if(flags&4) {
		resample8(s, s, &imgsrc);
		while(!kbhit()) {
			puts("\e[f");
			rot8(--rt, s, s, imgsrc, rotout);
			rep8(sx, sy, s, s, rotout);
			usleep(50000);
			if(!rt) rt=360;
		}
	}
	else {
		resample1(s, s, &imgsrc);
		while(!kbhit()) {
			puts("\e[f");
			rot1(--rt, s, s, imgsrc, rotout);
			rep1(sx, sy, s, s, rotout);
			usleep(50000);
			if(!rt) rt=360;
		}
	}
	free(rotout);
	free(imgsrc);
	getchar();
	clean_screen(0);
	return EXIT_SUCCESS;
}

void setup_screen(void) {
	struct termios new_settings;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(0, TCSANOW, &new_settings);
	signal(SIGINT, clean_screen);
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	puts("\e[?25l");
	while(--w.ws_row) {
		putchar('\n');
	}
}

void clean_screen(int fug) {
	puts("\e[0m\e[?25h\e[2J\e[f");
	tcsetattr(0, TCSANOW, &stored_settings);
	exit(EXIT_SUCCESS);
}

int kbhit(void) //http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

int rep1(int startx, int starty, int x, int y, uint8_t* img) {
	int cx = 0;
	int cy = 0;
	while(cy<y) {
		printf("\e[%i;%iH", cy/2+starty, startx);
		while(cx<x) {
			printf("\e[%im ", get(cx, cy, x, img)?47:0);
			cx++;
		}
		//puts("\e[0m");
		cx = 0;
		cy+=2; //height ~x2 width
	}
	fflush(stdout);
	return 0;
}

int rot1(int rd, int x, int y, uint8_t* mem, uint8_t* dest) {
	rd %= 360;
	int cox = (int)(cosf(2*PI*rd/360-1)*x);
	int six = (int)(sinf(2*PI*rd/360-1)*y);
	int coy = (int)(cosf(2*PI*rd/360-1)*x);
	int siy = (int)(sinf(2*PI*rd/360-1)*y);
	//uint8_t* tmp = calloc(x*y, sizeof(uint8_t));
	int cx;
	int cy;
	int chx;
	int chy;
	int ichx;
	int ichy;
	cx = x;
	cy = y;
	while(cy--) {
		while(cx--) {
			ichx = cx - x/2;  //0 in center of image
			ichy = cy - y/2;
			chx = ichx*cox/x-ichy*siy/y;
			chy = ichx*six/x+ichy*coy/y;
			chy += y/2;  //0 back on 0
			chx += x/2;
			if(chx>=0 && chx<x && chy>=0 && chy<y) {
				set(get(cx, cy, x, mem), chx, chy, x, dest);
			}
		}
		cx = x;
	}
	return 0;
}

int get(int x, int y, int width, uint8_t* mem) {
	return (*(mem+(x+y*width)/8)>>(x+y*width)%8)&1;
}

void set(int data, int x, int y, int width, uint8_t* mem) {
	if(data) {
		*(mem+(x+y*width)/8)|=1<<((x+y*width)%8);
	}
	else {
		*(mem+(x+y*width)/8)&=~(1<<((x+y*width)%8));
	}
}

int resample1(int newWidth, int newHeight, uint8_t** mem) {
	uint8_t* newData = calloc((newWidth*newHeight)/8+!!((newWidth*newHeight)%8), sizeof(uint8_t));
	uint8_t* mp = *mem;
	int cx = newWidth;
	int cy = newHeight;
	while(cy--) {
		while(cx--) {
			set(get(cx*60/newWidth, cy*60/newHeight, 60, mp), cx, cy, newWidth, newData);
		}
		cx = newWidth;
	}
	free(mp);
	*mem = newData;

	return 0;
}

int resample8(int newWidth, int newHeight, uint8_t** mem) {
	uint8_t* newData = malloc(newWidth * newHeight * 3);
	uint8_t* mp = *mem;
	int cx = newWidth;
	int cy = newHeight;
	while(cy--) {
		while(cx--) {
			*(newData+cx*3+cy*newWidth*3) = *(mp+cx*60/newWidth*3+cy*60/newHeight*60*3);
			*(newData+cx*3+cy*newWidth*3+1) = *(mp+cx*60/newWidth*3+cy*60/newHeight*60*3+1);
			*(newData+cx*3+cy*newWidth*3+2) = *(mp+cx*60/newWidth*3+cy*60/newHeight*60*3+2);
		}
		cx = newWidth;
	}
	free(mp);
	*mem = newData;
	return 0;
}

int rot8(int rd, int x, int y, uint8_t* mem, uint8_t* dest) {
	rd %= 360;
	int cox = (int)(cosf(2*PI*rd/360-1)*x);
	int six = (int)(sinf(2*PI*rd/360-1)*y);
	int coy = (int)(cosf(2*PI*rd/360-1)*x);
	int siy = (int)(sinf(2*PI*rd/360-1)*y);
	//uint8_t* tmp = calloc(x*y, sizeof(uint8_t));
	int cx;
	int cy;
	int chx;
	int chy;
	int ichx;
	int ichy;
	cx = x;
	cy = y;
	while(cy--) {
		while(cx--) {
			ichx = cx - x/2;  //0 in center of image
			ichy = cy - y/2;
			chx = ichx*cox/x-ichy*siy/y;
			chy = ichx*six/x+ichy*coy/y;
			chy += y/2;  //0 back on 0
			chx += x/2;
			if(chx>=0 && chx<x && chy>=0 && chy<y) {
				*(dest+cx*3+cy*x*3) = *(mem+chx*3+chy*x*3);
				*(dest+cx*3+cy*x*3+1) = *(mem+chx*3+chy*x*3+1);
				*(dest+cx*3+cy*x*3+2) = *(mem+chx*3+chy*x*3+2);
			}
		}
		cx = x;
	}
	return 0;
}

int rgb8ToConsole(int* c) {
	mapgs(c+r);
	mapgs(c+g);
	mapgs(c+b);
	if(c[r]==c[g]&&c[g]==c[b]&&c[r]&&c[r]!=31) {
		return 231 + c[r];
	}
	mapc(c+r);
	mapc(c+g);
	mapc(c+b);
	return 16+c[b]+c[g]*6+c[r]*36;
};

inline void mapgs(int* x)
{
	*x = *x * 31 / 255;
}

inline void mapc(int* x)
{
	*x = *x * 5 / 31;
}

int rep8(int startx, int starty, int x, int y, uint8_t* img) {
	int cx = 0;
	int cy = 0;
	int a[3];
	while(cy<y) {
		printf("\e[%i;%iH", cy/2+starty, startx);
		while(cx<x) {
			//printf("\e[%im ", get(cx, cy, x, img)?47:0);
			a[r] = *(img+cx*3+cy*x*3+r);
			a[g] = *(img+cx*3+cy*x*3+g);
			a[b] = *(img+cx*3+cy*x*3+b);
			//if(!(a[r]+a[g]+a[b]))
			//	printf("\e[0m ");
			//else
			printf("\e[48;5;%im ", rgb8ToConsole(a));
			cx++;
		}
		//puts("\e[0m");
		cx = 0;
		//cy++;
		cy+=2; //height ~x2 width
	}
	fflush(stdout);
	return 0;
}
