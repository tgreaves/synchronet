#include <stdarg.h>

#include "ciowrap.h"
#include "x_cio.h"
#include "console.h"
WORD	x_curr_attr=0x0700;

void x_scrollup(void)
{
	char *buf;
	int i,j;

	buf=(char *)malloc(DpyCols*DpyRows*2);
	x_gettext(1,2,DpyCols,DpyRows+1,buf);
	x_puttext(1,1,DpyCols,DpyRows,buf);
	j=0;
	for(i=0;i<DpyCols;i++) {
		buf[j++]=' ';
		buf[j++]=x_curr_attr>>8;
	}
	x_puttext(1,DpyRows+1,DpyCols,DpyRows+1,buf);
	free(buf);
}

int x_puttext(int sx, int sy, int ex, int ey, unsigned char *fill)
{
	int x,y;
	unsigned char *out;
	WORD	sch;
	struct text_info	ti;

	curs_gettextinfo(&ti);

	if(		   sx < 1
			|| sy < 1
			|| ex < 1
			|| ey < 1
			|| sx > ti.screenwidth
			|| sy > ti.screenheight
			|| sx > ex
			|| sy > ey
			|| ex > ti.screenwidth
			|| ey > ti.screenheight
			|| fill==NULL)
		return(0);

	out=fill;
	for(y=sy-1;y<ey;y++) {
		for(x=sx-1;x<ex;x++) {
			sch=*(out++);
			sch |= (*(out++))<<8;
			vmem[y*DpyCols+x]=sch;
		}
	}
}

int x_gettext(int sx, int sy, int ex, int ey, unsigned char *fill)
{
	int x,y;
	unsigned char *out;
	WORD	sch;
	struct text_info	ti;

	curs_gettextinfo(&ti);

	if(		   sx < 1
			|| sy < 1
			|| ex < 1
			|| ey < 1
			|| sx > ti.screenwidth
			|| sy > ti.screenheight
			|| sx > ex
			|| sy > ey
			|| ex > ti.screenwidth
			|| ey > ti.screenheight
			|| fill==NULL)
		return(0);

	out=fill;
	for(y=sy-1;y<ey;y++) {
		for(x=sx-1;x<ex;x++) {
			sch=vmem[y*DpyCols+x];
			*(out++)=sch & 0xff;
			*(out++)=sch >> 8;
		}
	}
}

void x_textattr(unsigned char attr)
{
	x_curr_attr=attr<<8;
}

int x_kbhit(void)
{
	return(tty_kbhit());
}

void x_delay(long msec)
{
	usleep(msec*1000);
}

int x_wherey(void)
{
	return(CursRow0)+1;
}

int x_wherex(void)
{
	return(CursCol0)+1;
}

/* Put the character _c on the screen at the current cursor position. 
 * The special characters return, linefeed, bell, and backspace are handled
 * properly, as is line wrap and scrolling. The cursor position is updated. 
 */
int x_putch(unsigned char ch)
{
	WORD sch;

	sch=x_curr_attr|ch;

	switch(ch) {
		case '\r':
			CursCol0=0;
			break;
		case '\n':
			CursRow0++;
			while(CursRow0>DpyRows) {
				x_scrollup();
				CursRow0--;
			}
			break;
		case '\b':
			sch=0x0700;
			if(CursCol0>0)
				CursCol0--;
			vmem[CursCol0+CursRow0*DpyCols]=sch;
			break;
		case 7:		/* Bell */
			tty_beep();
			break;
		default:
			vmem[CursCol0+CursRow0*DpyCols]=sch;
			CursCol0++;
			if(CursCol0>=DpyCols) {
				CursCol0=0;
				CursRow0++;
				while(CursRow0>DpyRows) {
					x_scrollup();
					CursRow0--;
				}
			}
	}

	return(ch);
}

int x_cprintf(char *fmat, ...)
{
	va_list argptr;
	unsigned char   str[4097];
	int             pos;
	int				ret;

	va_start(argptr,fmat);
	ret=vsprintf(str,fmat,argptr);
	va_end(argptr);
	if(ret>=0)
		x_cputs(str);
	else
		ret=EOF;
	return(ret);
}

int x_cputs(unsigned char *str)
{
	int             pos;
	int				ret=0;

	for(pos=0;str[pos];pos++) {
		ret=str[pos];
		putch(str[pos]);
	}
	return(ret);
}

void x_gotoxy(int x, int y)
{
	CursRow0=y-1;
	CursCol0=x-1;
}

void x_clrscr(void)
{
	int x,y;
	struct text_info info;
	x_gettextinfo(&info);
	for(y=0;y<info.screenheight;y++) {
		for(x=0;x<info.screenwidth;x++) {
			vmem[x+y*DpyCols]=0x0700;
		}
	}
}

void x_gettextinfo(struct text_info *info)
{
	info->currmode=VideoMode;
	info->screenheight=DpyRows+1;
	info->screenwidth=DpyCols;
}

void x_setcursortype(int type)
{
	switch(type) {
		case _NOCURSOR:
			CursStart=0xff;
			CursEnd=0;
			break;
		case _SOLIDCURSOR:
			CursStart=0;
			CursEnd=FH;
			break;
		default:
		    CursStart = VGA_CRTC[CRTC_CursStart];
		    CursEnd = VGA_CRTC[CRTC_CursEnd];
			break;
	}
}

void x_textbackground(int colour)
{
	colour &= 0x0f;
	x_curr_attr &= 0x0f00;
	x_curr_attr |= (colour<<12);
}

void x_textcolor(int colour)
{
	colour &= 0x0f;
	x_curr_attr &= 0xf000;
	x_curr_attr |= (colour<<8);
}

void x_clreol(void)
{
	int x;

	for(x=CursCol0;x<DpyCols;x++)
		vmem[CursRow0*DpyCols+x]=x_curr_attr;
}

int x_getch(void)
{
	return(tty_read(TTYF_BLOCK));
}

int x_getche(void)
{
	int ch;

	if(x_nextchar)
		return(x_getch());
	ch=x_getch();
	if(ch)
		x_putch(ch);
	return(ch);
}

int x_beep(void)
{
	tty_beep();
	return(0);
}

void x_highvideo(void)
{
	int attr;

	attr=x_curr_attr>>8;
	attr |= 8;
	x_textattr(attr);
}

void x_lowvideo(void)
{
	int attr;

	attr=x_curr_attr>>8;
	attr &= 0xf7;
	x_textattr(attr);
}

void x_normvideo(void)
{
	x_textattr(0x07);
}
