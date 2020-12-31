/* Synchronet file database-related exported functions */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include "filedat.h"
#include "dat_rec.h"
#include "datewrap.h"	// time32()
#include "str_util.h"
#include "nopen.h"
#include "smblib.h"
#include "load_cfg.h"	// smb_open_dir()

/****************************************************************************/
/* Gets filedata from dircode.DAT file										*/
/* Need fields .name ,.dir and .offset to get other info    				*/
/* Does not fill .dateuled or .datedled fields.                             */
/****************************************************************************/
BOOL DLLCALL getfiledat(scfg_t* cfg, file_t* f)
{
	char buf[F_LEN+1],str[MAX_PATH+1];
	int file;
	long length;

	SAFEPRINTF2(str,"%s%s.dat",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDONLY|O_BINARY,SH_DENYWR))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(f->datoffset>length) {
		close(file);
		return(FALSE); 
	}
	if(length%F_LEN) {
		close(file);
		return(FALSE); 
	}
	lseek(file,f->datoffset,SEEK_SET);
	if(read(file,buf,F_LEN)!=F_LEN) {
		close(file);
		return(FALSE); 
	}
	close(file);
	getrec(buf,F_ALTPATH,2,str);
	f->altpath=hptoi(str);
	getrec(buf,F_CDT,LEN_FCDT,str);
	f->cdt=atol(str);

	if(f->size == 0) {					// only read disk if f->size == 0
		struct stat st;
		getfilepath(cfg,f,str);
		if(stat(str, &st) == 0) {
			f->size = st.st_size;
			f->date = (time32_t)st.st_mtime;
		} else
			f->size = -1;	// indicates file does not exist
	}
#if 0
	if((f->size>0L) && cur_cps)
		f->timetodl=(ushort)(f->size/(ulong)cur_cps);
	else
#endif
		f->timetodl=0;

	getrec(buf,F_DESC,LEN_FDESC,f->desc);
	getrec(buf,F_ULER,LEN_ALIAS,f->uler);
	getrec(buf,F_TIMESDLED,5,str);
	f->timesdled=atoi(str);
	getrec(buf,F_OPENCOUNT,3,str);
	f->opencount=atoi(str);
	if(buf[F_MISC]!=ETX)
		f->misc=buf[F_MISC]-' ';
	else
		f->misc=0;
	return(TRUE);
}

/****************************************************************************/
/* Puts filedata into DIR_code.DAT file                                     */
/* Called from removefiles                                                  */
/****************************************************************************/
BOOL DLLCALL putfiledat(scfg_t* cfg, file_t* f)
{
    char buf[F_LEN+1],str[MAX_PATH+1],tmp[128];
    int file;
    long length;

	putrec(buf,F_CDT,LEN_FCDT,ultoa(f->cdt,tmp,10));
	putrec(buf,F_DESC,LEN_FDESC,f->desc);
	putrec(buf,F_DESC+LEN_FDESC,2, "\r\n");
	putrec(buf,F_ULER,LEN_ALIAS+5,f->uler);
	putrec(buf,F_ULER+LEN_ALIAS+5,2, "\r\n");
	putrec(buf,F_TIMESDLED,5,ultoa(f->timesdled,tmp,10));
	putrec(buf,F_TIMESDLED+5,2, "\r\n");
	putrec(buf,F_OPENCOUNT,3,ultoa(f->opencount,tmp,10));
	putrec(buf,F_OPENCOUNT+3,2, "\r\n");
	buf[F_MISC]=(char)f->misc+' ';
	putrec(buf,F_ALTPATH,2,hexplus(f->altpath,tmp));
	putrec(buf,F_ALTPATH+2,2, "\r\n");
	SAFEPRINTF2(str,"%s%s.dat",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_WRONLY|O_BINARY,SH_DENYRW))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(length%F_LEN) {
		close(file);
		return(FALSE); 
	}
	if(f->datoffset>length) {
		close(file);
		return(FALSE); 
	}
	lseek(file,f->datoffset,SEEK_SET);
	if(write(file,buf,F_LEN)!=F_LEN) {
		close(file);
		return(FALSE); 
	}
	length=(long)filelength(file);
	close(file);
	if(length%F_LEN) {
		return(FALSE);
	}
	return(TRUE);
}

/****************************************************************************/
/* Adds the data for struct filedat to the directory's data base.           */
/* changes the .datoffset field only                                        */
/* returns 1 if added successfully, 0 if not.								*/
/****************************************************************************/
BOOL DLLCALL addfiledat(scfg_t* cfg, file_t* f)
{
	char	str[MAX_PATH+1],fname[13],c,fdat[F_LEN+1];
	char	tmp[128];
	uchar	*ixbbuf,idx[3];
    int		i,file;
	long	l,length;
	time_t	uldate;

	/************************/
	/* Add data to DAT File */
	/************************/
	SAFEPRINTF2(str,"%s%s.dat",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDWR|O_BINARY|O_CREAT,SH_DENYRW,DEFFILEMODE))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(length==0L)
		l=0L;
	else {
		if(length%F_LEN) {
			close(file);
			return(FALSE); 
		}
		for(l=0;l<length;l+=F_LEN) {    /* Find empty slot */
			lseek(file,l,SEEK_SET);
			read(file,&c,1);
			if(c==ETX) break; 
		}
		if(l/F_LEN>=MAX_FILES) {
			close(file);
			return(FALSE); 
		} 
	}
	putrec(fdat,F_CDT,LEN_FCDT,ultoa(f->cdt,tmp,10));
	putrec(fdat,F_DESC,LEN_FDESC,f->desc);
	putrec(fdat,F_DESC+LEN_FDESC,2, "\r\n");
	putrec(fdat,F_ULER,LEN_ALIAS+5,f->uler);
	putrec(fdat,F_ULER+LEN_ALIAS+5,2, "\r\n");
	putrec(fdat,F_TIMESDLED,5,ultoa(f->timesdled,tmp,10));
	putrec(fdat,F_TIMESDLED+5,2, "\r\n");
	putrec(fdat,F_OPENCOUNT,3,ultoa(f->opencount,tmp,10));
	putrec(fdat,F_OPENCOUNT+3,2, "\r\n");
	fdat[F_MISC]=(char)f->misc+' ';
	putrec(fdat,F_ALTPATH,2,hexplus(f->altpath,tmp));
	putrec(fdat,F_ALTPATH+2,2, "\r\n");
	f->datoffset=l;
	idx[0]=(uchar)(l&0xff);          /* Get offset within DAT file for IXB file */
	idx[1]=(uchar)((l>>8)&0xff);
	idx[2]=(uchar)((l>>16)&0xff);
	lseek(file,l,SEEK_SET);
	if(write(file,fdat,F_LEN)!=F_LEN) {
		close(file);
		return(FALSE); 
	}
	length=(long)filelength(file);
	close(file);
	if(length%F_LEN) {
		return(FALSE);
	}

	/*******************************************/
	/* Update last upload date/time stamp file */
	/*******************************************/
	SAFEPRINTF2(str,"%s%s.dab",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_WRONLY|O_CREAT|O_BINARY,SH_DENYRW,DEFFILEMODE))!=-1) {
		time32_t now=time32(NULL);
		/* TODO: LE required */
		write(file,&now,sizeof(time32_t));
		close(file); 
	}

	/************************/
	/* Add data to IXB File */
	/************************/
	SAFECOPY(fname,f->name);
	for(i=8;i<12;i++)   /* Turn FILENAME.EXT into FILENAMEEXT */
		fname[i]=fname[i+1];
	SAFEPRINTF2(str,"%s%s.ixb",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDWR|O_CREAT|O_BINARY,SH_DENYRW,DEFFILEMODE))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(length) {    /* IXB file isn't empty */
		if(length%F_IXBSIZE) {
			close(file);
			return(FALSE); 
		}
		if((ixbbuf=(uchar *)malloc(length))==NULL) {
			close(file);
			return(FALSE); 
		}
		if(read(file,ixbbuf,length)!=length) {
			close(file);
			free(ixbbuf);
			return(FALSE); 
		}
	/************************************************/
	/* Sort by Name or Date, Assending or Decending */
	/************************************************/
		if(cfg->dir[f->dir]->sort==SORT_NAME_A || cfg->dir[f->dir]->sort==SORT_NAME_D) {
			for(l=0;l<length;l+=F_IXBSIZE) {
				for(i=0;i<12 && toupper(fname[i])==toupper(ixbbuf[l+i]);i++);
				if(i==12) {     /* file already in directory index */
					close(file);
					free(ixbbuf);
					return(FALSE); 
				}
				if(cfg->dir[f->dir]->sort==SORT_NAME_A 
					&& toupper(fname[i])<toupper(ixbbuf[l+i]))
					break;
				if(cfg->dir[f->dir]->sort==SORT_NAME_D 
					&& toupper(fname[i])>toupper(ixbbuf[l+i]))
					break; 
			} 
		}
		else {  /* sort by date */
			for(l=0;l<length;l+=F_IXBSIZE) {
				uldate=(ixbbuf[l+14]|((long)ixbbuf[l+15]<<8)
					|((long)ixbbuf[l+16]<<16)|((long)ixbbuf[l+17]<<24));
				if(cfg->dir[f->dir]->sort==SORT_DATE_A && f->dateuled<uldate)
					break;
				if(cfg->dir[f->dir]->sort==SORT_DATE_D && f->dateuled>uldate)
					break; 
			} 
		}
		lseek(file,l,SEEK_SET);
		if(write(file,fname,11)!=11) {  /* Write filename to IXB file */
			close(file);
			free(ixbbuf);
			return(FALSE); 
		}
		if(write(file,idx,3)!=3) {  /* Write DAT offset into IXB file */
			close(file);
			free(ixbbuf);
			return(FALSE); 
		}
		write(file,&f->dateuled,4);
		write(file,&f->datedled,4);              /* Write 0 for datedled */
		if(write(file,&ixbbuf[l],length-l)!=length-l) { /* Write rest of IXB */
			close(file);
			free(ixbbuf);
			return(FALSE); 
		}
		free(ixbbuf); 
	}
	else {              /* IXB file is empty... No files */
		if(write(file,fname,11)!=11) {  /* Write filename it IXB file */
			close(file);
			return(FALSE); 
		}
		if(write(file,idx,3)!=3) {  /* Write DAT offset into IXB file */
			close(file);
			return(FALSE); 
		}
		write(file,&f->dateuled,4);
		write(file,&f->datedled,4); 
	}
	length=(long)filelength(file);
	close(file);
	return(TRUE);
}

/****************************************************************************/
/* Gets file data from dircode.ixb file										*/
/* Need fields .name and .dir filled.                                       */
/* only fills .offset, .dateuled, and .datedled                             */
/****************************************************************************/
BOOL DLLCALL getfileixb(scfg_t* cfg, file_t* f)
{
	char			str[MAX_PATH+1],fname[13];
	uchar *	ixbbuf;
	int				file;
	long			l,length;

	SAFEPRINTF2(str,"%s%s.ixb",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDONLY|O_BINARY,SH_DENYWR))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(length%F_IXBSIZE) {
		close(file);
		return(FALSE); 
	}
	if((ixbbuf=(uchar *)malloc(length))==NULL) {
		close(file);
		return(FALSE); 
	}
	if(read(file,ixbbuf,length)!=length) {
		close(file);
		free(ixbbuf);
		return(FALSE); 
	}
	close(file);
	SAFECOPY(fname,f->name);
	for(l=8;l<12;l++)	/* Turn FILENAME.EXT into FILENAMEEXT */
		fname[l]=fname[l+1];
	for(l=0;l<length;l+=F_IXBSIZE) {
		SAFEPRINTF(str,"%11.11s",ixbbuf+l);
		if(!stricmp(str,fname))
			break; 
	}
	if(l>=length) {
		free(ixbbuf);
		return(FALSE); 
	}
	l+=11;
	f->datoffset=ixbbuf[l]|((long)ixbbuf[l+1]<<8)|((long)ixbbuf[l+2]<<16);
	f->dateuled=ixbbuf[l+3]|((long)ixbbuf[l+4]<<8)
		|((long)ixbbuf[l+5]<<16)|((long)ixbbuf[l+6]<<24);
	f->datedled=ixbbuf[l+7]|((long)ixbbuf[l+8]<<8)
		|((long)ixbbuf[l+9]<<16)|((long)ixbbuf[l+10]<<24);
	free(ixbbuf);
	return(TRUE);
}

/****************************************************************************/
/* Updates the datedled and dateuled index record fields for a file			*/
/****************************************************************************/
BOOL DLLCALL putfileixb(scfg_t* cfg, file_t* f)
{
	char	str[MAX_PATH+1],fname[13];
	uchar*	ixbbuf;
	int		file;
	long	l,length;

	SAFEPRINTF2(str,"%s%s.ixb",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDWR|O_BINARY,SH_DENYRW))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(length%F_IXBSIZE) {
		close(file);
		return(FALSE); 
	}
	if((ixbbuf=(uchar *)malloc(length))==NULL) {
		close(file);
		return(FALSE); 
	}
	if(read(file,ixbbuf,length)!=length) {
		close(file);
		free(ixbbuf);
		return(FALSE); 
	}
	SAFECOPY(fname,f->name);
	for(l=8;l<12;l++)	/* Turn FILENAME.EXT into FILENAMEEXT */
		fname[l]=fname[l+1];
	for(l=0;l<length;l+=F_IXBSIZE) {
		SAFEPRINTF(str,"%11.11s",ixbbuf+l);
		if(!stricmp(str,fname))
			break; 
	}
	free(ixbbuf);

	if(l>=length) {
		close(file);
		return(FALSE); 
	}
	
	lseek(file,l+11+3,SEEK_SET);

	write(file,&f->dateuled,4);
	write(file,&f->datedled,4);

	close(file);

	return(TRUE);
}


/****************************************************************************/
/* Removes DAT and IXB entries for the file in the struct 'f'               */
/****************************************************************************/
BOOL DLLCALL removefiledat(scfg_t* cfg, file_t* f)
{
	char	c,str[MAX_PATH+1],ixbname[12],*ixbbuf,fname[13];
    int		i,file;
	long	l,length;

	SAFECOPY(fname,f->name);
	for(i=8;i<12;i++)   /* Turn FILENAME.EXT into FILENAMEEXT */
		fname[i]=fname[i+1];
	SAFEPRINTF2(str,"%s%s.ixb",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_RDONLY|O_BINARY,SH_DENYWR))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if(!length) {
		close(file);
		return(FALSE); 
	}
	if((ixbbuf=(char *)malloc(length))==0) {
		close(file);
		return(FALSE); 
	}
	if(read(file,ixbbuf,length)!=length) {
		close(file);
		free(ixbbuf);
		return(FALSE); 
	}
	close(file);
	if((file=sopen(str,O_WRONLY|O_TRUNC|O_BINARY,SH_DENYRW))==-1) {
		free(ixbbuf);
		return(FALSE); 
	}
	for(l=0;l<length;l+=F_IXBSIZE) {
		for(i=0;i<11;i++)
			ixbname[i]=ixbbuf[l+i];
		ixbname[i]=0;
		if(stricmp(ixbname,fname))
			if(write(file,&ixbbuf[l],F_IXBSIZE)!=F_IXBSIZE) {
				close(file);
				free(ixbbuf);
				return(FALSE); 
		} 
	}
	free(ixbbuf);
	close(file);
	SAFEPRINTF2(str,"%s%s.dat",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=sopen(str,O_WRONLY|O_BINARY,SH_DENYRW))==-1) {
		return(FALSE); 
	}
	lseek(file,f->datoffset,SEEK_SET);
	c=ETX;          /* If first char of record is ETX, record is unused */
	if(write(file,&c,1)!=1) { /* So write a D_T on the first byte of the record */
		close(file);
		return(FALSE); 
	}
	close(file);
	if(f->dir==cfg->user_dir)  /* remove file from index */
		rmuserxfers(cfg,0,0,f->name);
	return(TRUE);
}

/****************************************************************************/
/****************************************************************************/
BOOL DLLCALL findfile(scfg_t* cfg, uint dirnum, const char *filename)
{
	smb_t smb;

	if(cfg == NULL || filename == NULL)
		return FALSE;

	if(smb_open_dir(cfg, &smb, dirnum) != SMB_SUCCESS)
		return FALSE;

	int result = smb_findfile(&smb, filename, /* idx: */NULL);
	smb_close(&smb);
	return result == SMB_SUCCESS;
}

/****************************************************************************/
/* Turns FILE.EXT into FILE    .EXT                                         */
/****************************************************************************/
char* DLLCALL padfname(const char *filename, char *str)
{
    int c,d;

	for(c=0;c<8;c++)
		if(filename[c]=='.' || !filename[c]) break;
		else str[c]=filename[c];
	d=c;
	if(filename[c]=='.') c++;
	while(d<8)
		str[d++]=' ';
	if(filename[c]>' ')	/* Change "FILE" to "FILE        " */
		str[d++]='.';	/* (don't add a dot if there's no extension) */
	else
		str[d++]=' ';
	while(d<12)
		if(!filename[c]) break;
		else str[d++]=filename[c++];
	while(d<12)
		str[d++]=' ';
	str[d]=0;
	return(str);
}

/****************************************************************************/
/* Turns FILE    .EXT into FILE.EXT                                         */
/****************************************************************************/
char* DLLCALL unpadfname(const char *filename, char *str)
{
    int c,d;

	for(c=0,d=0;filename[c];c++)
		if(filename[c]!=' ') str[d++]=filename[c];
	str[d]=0;
	return(str);
}

/****************************************************************************/
/* Removes any files in the user transfer index (XFER.IXT) that match the   */
/* specifications of dest, or source user, or filename or any combination.  */
/****************************************************************************/
BOOL DLLCALL rmuserxfers(scfg_t* cfg, int fromuser, int destuser, char *fname)
{
    char str[MAX_PATH+1],*ixtbuf;
    int file;
    long l,length;

	SAFEPRINTF(str,"%sxfer.ixt", cfg->data_dir);
	if(!fexist(str))
		return(FALSE);
	if(!flength(str)) {
		remove(str);
		return(FALSE); 
	}
	if((file=sopen(str,O_RDONLY|O_BINARY,SH_DENYWR))==-1) {
		return(FALSE); 
	}
	length=(long)filelength(file);
	if((ixtbuf=(char *)malloc(length))==NULL) {
		close(file);
		return(FALSE); 
	}
	if(read(file,ixtbuf,length)!=length) {
		close(file);
		free(ixtbuf);
		return(FALSE); 
	}
	close(file);
	if((file=sopen(str,O_WRONLY|O_TRUNC|O_BINARY,SH_DENYRW))==-1) {
		free(ixtbuf);
		return(FALSE); 
	}
	for(l=0;l<length;l+=24) {
		if(fname!=NULL && fname[0]) {               /* fname specified */
			if(!strncmp(ixtbuf+l+5,fname,12)) {     /* this is the file */
				if(destuser && fromuser) {          /* both dest and from user */
					if(atoi(ixtbuf+l)==destuser && atoi(ixtbuf+l+18)==fromuser)
						continue;                   /* both match */
				}
				else if(fromuser) {                 /* from user */
					if(atoi(ixtbuf+l+18)==fromuser) /* matches */
						continue; 
				}
				else if(destuser) {                 /* dest user */
					if(atoi(ixtbuf+l)==destuser)    /* matches */
						continue; 
				}
				else continue;		                /* no users, so match */
			}
		}
		else if(destuser && fromuser) {
			if(atoi(ixtbuf+l+18)==fromuser && atoi(ixtbuf+l)==destuser)
				continue; 
		}
		else if(destuser && atoi(ixtbuf+l)==destuser)
			continue;
		else if(fromuser && atoi(ixtbuf+l+18)==fromuser)
			continue;
		write(file,ixtbuf+l,24); 
	}
	close(file);
	free(ixtbuf);

	return(TRUE);
}

int DLLCALL openextdesc(scfg_t* cfg, uint dirnum)
{
	char str[MAX_PATH+1];
	SAFEPRINTF2(str,"%s%s.exb",cfg->dir[dirnum]->data_dir,cfg->dir[dirnum]->code);
	return nopen(str,O_RDONLY);
}

void DLLCALL closeextdesc(int file)
{
	if(file >= 0)
		close(file);
}

void DLLCALL getextdesc(scfg_t* cfg, uint dirnum, ulong datoffset, char *ext)
{
	int file;

	memset(ext,0,F_EXBSIZE+1);
	if((file=openextdesc(cfg, dirnum))==-1)
		return;
	lseek(file,(datoffset/F_LEN)*F_EXBSIZE,SEEK_SET);
	read(file,ext,F_EXBSIZE);
	close(file);
}

// fast (operates on open .exb file)
void DLLCALL fgetextdesc(scfg_t* cfg, uint dirnum, ulong datoffset, char *ext, int file)
{
	lseek(file,(datoffset/F_LEN)*F_EXBSIZE,SEEK_SET);
	read(file,ext,F_EXBSIZE);
}

void DLLCALL putextdesc(scfg_t* cfg, uint dirnum, ulong datoffset, char *ext)
{
	char str[MAX_PATH+1],nulbuf[F_EXBSIZE];
	int file;

	strip_ansi(ext);
	strip_invalid_attr(ext);	/* eliminate bogus ctrl-a codes */
	memset(nulbuf,0,sizeof(nulbuf));
	SAFEPRINTF2(str,"%s%s.exb",cfg->dir[dirnum]->data_dir,cfg->dir[dirnum]->code);
	if((file=nopen(str,O_WRONLY|O_CREAT))==-1)
		return;
	lseek(file,0L,SEEK_END);
	while(filelength(file)<(long)(datoffset/F_LEN)*F_EXBSIZE)
		write(file,nulbuf,sizeof(nulbuf));
	lseek(file,(datoffset/F_LEN)*F_EXBSIZE,SEEK_SET);
	write(file,ext,F_EXBSIZE);
	close(file);
}

/****************************************************************************/
/* Update the upload date for the file 'f'                                  */
/****************************************************************************/
int DLLCALL update_uldate(scfg_t* cfg, file_t* f)
{
	char str[MAX_PATH+1],fname[13];
	int i,file;
	long l,length;

	/*******************/
	/* Update IXB File */
	/*******************/
	SAFEPRINTF2(str,"%s%s.ixb",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=nopen(str,O_RDWR))==-1)
		return(errno); 
	length=(long)filelength(file);
	if(length%F_IXBSIZE) {
		close(file);
		return(-1); 
	}
	SAFECOPY(fname,f->name);
	for(i=8;i<12;i++)   /* Turn FILENAME.EXT into FILENAMEEXT */
		fname[i]=fname[i+1];
	for(l=0;l<length;l+=F_IXBSIZE) {
		read(file,str,F_IXBSIZE);      /* Look for the filename in the IXB file */
		str[11]=0;
		if(!stricmp(fname,str)) break; 
	}
	if(l>=length) {
		close(file);
		return(-2); 
	}
	lseek(file,l+14,SEEK_SET);
	write(file,&f->dateuled,4);
	close(file);

	/*******************************************/
	/* Update last upload date/time stamp file */
	/*******************************************/
	SAFEPRINTF2(str,"%s%s.dab",cfg->dir[f->dir]->data_dir,cfg->dir[f->dir]->code);
	if((file=nopen(str,O_WRONLY|O_CREAT))==-1)
		return(errno);

	write(file,&f->dateuled,4);
	close(file); 
	return(0);
}

/****************************************************************************/
/* Returns full (case-corrected) path to specified file						*/
/****************************************************************************/
char* DLLCALL getfilepath(scfg_t* cfg, file_t* f, char* path)
{
	char	fname[MAX_PATH+1];

	unpadfname(f->name,fname);
	if(f->dir>=cfg->total_dirs)
		safe_snprintf(path,MAX_PATH,"%s%s",cfg->temp_dir,fname);
	else
		safe_snprintf(path,MAX_PATH,"%s%s",f->altpath>0 && f->altpath<=cfg->altpaths 
			? cfg->altpath[f->altpath-1] : cfg->dir[f->dir]->path
			,fname);
	fexistcase(path);
	return(path);
}

// This function called without opening the filebase (for fast new-scans)
BOOL DLLCALL newfiles(smb_t* smb, time_t t)
{
	char str[MAX_PATH + 1];
	SAFEPRINTF(str, "%s.sid", smb->file);
	return fdate(str) > t;
}

smbfile_t* DLLCALL loadfiles(smb_t* smb, const char* filespec, time_t t, size_t* count)
{
	*count = 0;

	long start = 0;	
	if(t) {
		idxrec_t idx;
		start = smb_getmsgidx_by_time(smb, &idx, t);
		if(start < 0)
			return NULL;
	}

	smbfile_t* file_list = malloc(smb->status.total_files * sizeof(smbfile_t));
	if(file_list == NULL)
		return NULL;
	memset(file_list, 0, smb->status.total_files * sizeof(smbfile_t));

	fseek(smb->sid_fp, start * sizeof(smbfileidxrec_t), SEEK_SET);
	while(!feof(smb->sid_fp)) {
		smbfileidxrec_t fidx;

		if(smb_fread(smb, &fidx, sizeof(fidx), smb->sid_fp) != sizeof(fidx))
			break;

		if(fidx.idx.number==0)	/* invalid message number, ignore */
			continue;

		if(filespec != NULL && *filespec != '\0') {
//			SAFECOPY(filename, fidx.filename);
			if(!wildmatchi(fidx.filename, filespec, /* path: */FALSE))
				continue;
		}
		smbfile_t file = { .idx = fidx.idx };
		int result = smb_getmsghdr(smb, &file);
		if(result != SMB_SUCCESS)
			break;
		file.extdesc = smb_getmsgtxt(smb, &file, GETMSGTXT_ALL);
		file.dir = smb->dirnum;
		file_list[*count] = file;
		(*count)++;
	}

	return file_list;
}

static int filename_compare_a(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return stricmp(f1->filename, f2->filename);
}

static int filename_compare_ac(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return strcmp(f1->filename, f2->filename);
}

static int filename_compare_d(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return stricmp(f2->filename, f1->filename);
}

static int filename_compare_dc(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return strcmp(f2->filename, f1->filename);
}

static int filedate_compare_a(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return f1->hdr.when_imported.time - f2->hdr.when_imported.time;
}

static int filedate_compare_d(const void* v1, const void* v2)
{
	smbfile_t* f1 = (smbfile_t*)v1;
	smbfile_t* f2 = (smbfile_t*)v2;

	return f2->hdr.when_imported.time - f1->hdr.when_imported.time;
}

void DLLCALL sortfiles(smbfile_t* filelist, size_t count, enum file_sort order)
{
	switch(order) {
		case SORT_NAME_A:
			qsort(filelist, count, sizeof(*filelist), filename_compare_a);
			break;
		case SORT_NAME_D:
			qsort(filelist, count, sizeof(*filelist), filename_compare_d);
			break;
		case SORT_NAME_AC:
			qsort(filelist, count, sizeof(*filelist), filename_compare_ac);
			break;
		case SORT_NAME_DC:
			qsort(filelist, count, sizeof(*filelist), filename_compare_dc);
			break;
		case SORT_DATE_A:
			qsort(filelist, count, sizeof(*filelist), filedate_compare_a);
			break;
		case SORT_DATE_D:
			qsort(filelist, count, sizeof(*filelist), filedate_compare_d);
			break; 
	}
}

void DLLCALL freefiles(smbfile_t* filelist, size_t count)
{
	for(size_t i = 0; i < count; i++)
		smb_freefilemem(&filelist[i]);
	free(filelist);
}

BOOL DLLCALL loadfile(scfg_t* cfg, uint dirnum, const char* filename, smbfile_t* file)
{
	smb_t smb;

	if(smb_open_dir(cfg, &smb, dirnum) != SMB_SUCCESS)
		return FALSE;

	int result = smb_loadfile(&smb, filename, file);
	smb_close(&smb);
	return result == SMB_SUCCESS;
}

BOOL DLLCALL updatefile(scfg_t* cfg, smbfile_t* file)
{
	smb_t smb;

	if(smb_open_dir(cfg, &smb, file->dir) != SMB_SUCCESS)
		return FALSE;

	BOOL result = smb_updatemsg(&smb, file) == SMB_SUCCESS;
	smb_close(&smb);
	return result;
}

BOOL DLLCALL removefile(scfg_t* cfg, uint dirnum, const char* filename)
{
	smb_t smb;

	if(smb_open_dir(cfg, &smb, dirnum) != SMB_SUCCESS)
		return FALSE;

	int result;
	smbfile_t file;
	if((result = smb_loadfile(&smb, filename, &file)) == SMB_SUCCESS)
		result = smb_removefile(&smb, &file);
	smb_freefilemem(&file);
	smb_close(&smb);
	return result == SMB_SUCCESS;
}

/****************************************************************************/
/* Returns full (case-corrected) path to specified file						*/
/* 'path' should be MAX_PATH + 1 chars in size								*/
/****************************************************************************/
char* DLLCALL getfullfilepath(scfg_t* cfg, smbfile_t* f, char* path)
{
	if(f->dir >= cfg->total_dirs)
		safe_snprintf(path, MAX_PATH, "%s%s", cfg->temp_dir, f->filename);
	else
		safe_snprintf(path, MAX_PATH, "%s%s", f->hdr.altpath > 0 && f->hdr.altpath <= cfg->altpaths 
			? cfg->altpath[f->hdr.altpath-1] : cfg->dir[f->dir]->path
			,f->filename);
	fexistcase(path);
	return path;
}

off_t DLLCALL getfilesize(scfg_t* cfg, smbfile_t* f)
{
	char fpath[MAX_PATH + 1];
	if(f->size > 0)
		return f->size;
	f->size = flength(getfullfilepath(cfg, f, fpath));
	return f->size;
}

time_t DLLCALL getfiledate(scfg_t* cfg, smbfile_t* f)
{
	char fpath[MAX_PATH + 1];
	if(f->date > 0)
		return f->date;
	f->date = fdate(getfullfilepath(cfg, f, fpath));
	return f->date;
}

ulong DLLCALL gettimetodl(scfg_t* cfg, smbfile_t* f, uint rate_cps)
{
	if(getfilesize(cfg, f) < 1)
		return 0;
	if(f->size <= (off_t)rate_cps)
		return 1;
	return f->size / rate_cps;
}

BOOL DLLCALL addfile(scfg_t* cfg, uint dirnum, smbfile_t* file, const uchar* extdesc)
{
	smb_t smb;

	if(smb_open_dir(cfg, &smb, dirnum) != SMB_SUCCESS)
		return FALSE;

	BOOL result = smb_addfile(&smb, file, SMB_SELFPACK, extdesc);
	smb_close(&smb);

	return result;
}

/* 'size' does not include the NUL-terminator */
char* DLLCALL format_filename(const char* fname, char* buf, size_t size, BOOL pad)
{
	size_t fnlen = strlen(fname);
	char* ext = getfext(fname);
	if(ext != NULL) {
		size_t extlen = strlen(ext);
		if(extlen >= size)
			safe_snprintf(buf, size + 1, "%s", fname);
		else {
			fnlen -= extlen;
			if(fnlen > size - extlen)
				fnlen = size - extlen;
			safe_snprintf(buf, size + 1, "%-*.*s%s", pad ? (size - extlen) : 0, fnlen, fname, ext);
		}
	} else	/* no extension */
		snprintf(buf, size + 1, "%s", fname);
	return buf;
}

