/* Synchronet QWK replay (REP) packet unpacking routine */

/* $Id$ */

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
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include "sbbs.h"
#include "qwk.h"

/****************************************************************************/
/* Unpacks .REP packet, 'repfile' is the path and filename of the packet    */
/****************************************************************************/
bool sbbs_t::unpack_rep(char* repfile)
{
	char	str[MAX_PATH+1],fname[MAX_PATH+1];
	char	rep_fname[MAX_PATH+1];
	char	msg_fname[MAX_PATH+1];
	char 	tmp[512];
	char	inbox[MAX_PATH+1];
	char	block[QWK_BLOCK_LEN];
	int 	file;
	uint	i,j,k,lastsub=INVALID_SUB;
	uint	blocks;
	uint	usernum;
	long	l,size,misc;
	ulong	n;
	ulong	ex;
	ulong	errors = 0;
	node_t	node;
	FILE*	rep;
	FILE*	fp;
	DIR*	dir;
	DIRENT*	dirent;
	smbmsg_t	msg;
	str_list_t	headers=NULL;
	str_list_t	voting=NULL;
	str_list_t	ip_can=NULL;
	str_list_t	host_can=NULL;
	str_list_t	subject_can=NULL;
	str_list_t	twit_list=NULL;
	const char* hostname;
	const char* AttemptedToUploadREPpacket="Attempted to upload REP packet";

	memset(&msg,0,sizeof(msg));

	if(repfile!=NULL) {
		delfiles(cfg.temp_dir,ALLFILES);
		SAFECOPY(rep_fname,repfile);
	} else
		SAFEPRINTF2(rep_fname,"%s%s.rep",cfg.temp_dir,cfg.sys_id);
	if(!fexistcase(rep_fname)) {
		bputs(text[QWKReplyNotReceived]);
		logline(LOG_NOTICE,"U!",AttemptedToUploadREPpacket);
		logline(LOG_NOTICE,nulstr,"REP file not received");
		return(false); 
	}
	for(k=0;k<cfg.total_fextrs;k++)
		if(!stricmp(cfg.fextr[k]->ext,useron.tmpext) && chk_ar(cfg.fextr[k]->ar,&useron,&client))
			break;
	if(k>=cfg.total_fextrs)
		k=0;
	ex=EX_STDOUT;
	if(online!=ON_REMOTE)
		ex|=EX_OFFLINE;
	i=external(cmdstr(cfg.fextr[k]->cmd,rep_fname,ALLFILES,NULL),ex);
	if(i) {
		bputs(text[QWKExtractionFailed]);
		logline(LOG_NOTICE,"U!",AttemptedToUploadREPpacket);
		logline(LOG_NOTICE,nulstr,"Extraction failed");
		return(false); 
	}
	SAFEPRINTF2(msg_fname,"%s%s.msg",cfg.temp_dir,cfg.sys_id);
	if(!fexistcase(msg_fname)) {
		bputs(text[QWKReplyNotReceived]);
		logline(LOG_NOTICE,"U!",AttemptedToUploadREPpacket);
		logline(LOG_NOTICE,nulstr,"MSG file not received");
		return(false); 
	}
	if((rep=fnopen(&file,msg_fname,O_RDONLY))==NULL) {
		errormsg(WHERE,ERR_OPEN,msg_fname,O_RDONLY);
		return(false); 
	}
	size=(long)filelength(file);

	SAFEPRINTF(fname,"%sHEADERS.DAT",cfg.temp_dir);
	if(fexistcase(fname)) {
		lprintf(LOG_DEBUG, "Reading %s", fname);
		FILE* fp;
		set_qwk_flag(QWK_HEADERS);
		if((fp=fopen(fname,"r")) == NULL)
			errormsg(WHERE,ERR_OPEN,fname,0);
		else {
			headers=iniReadFile(fp);
			fclose(fp);
		}
		remove(fname);
	}
	SAFEPRINTF(fname,"%sVOTING.DAT",cfg.temp_dir);
	if(fexistcase(fname)) {
		if(useron.rest&FLAG('V'))
			bputs(text[R_Voting]);
		else {
			lprintf(LOG_DEBUG, "Reading %s", fname);
			FILE* fp;
			set_qwk_flag(QWK_VOTING);
			if((fp=fopen(fname,"r")) == NULL)
				errormsg(WHERE,ERR_OPEN,fname,0);
			else {
				voting=iniReadFile(fp);
				fclose(fp);
#ifdef _DEBUG
				for(uint u=0; voting[u]; u++)
					lprintf(LOG_DEBUG, "VOTING.DAT: %s", voting[u]);
#endif
			}
		}
		remove(fname);
	}

	fread(block,QWK_BLOCK_LEN,1,rep);
	if(strnicmp((char *)block,cfg.sys_id,strlen(cfg.sys_id))) {
		iniFreeStringList(headers);
		iniFreeStringList(voting);
		fclose(rep);
		bputs(text[QWKReplyNotReceived]);
		logline(LOG_NOTICE,"U!",AttemptedToUploadREPpacket);
		logline(LOG_NOTICE,nulstr,"Incorrect QWK BBS ID");
		return(false); 
	}
	logline("U+","Uploaded REP packet");
	/********************/
	/* Process messages */
	/********************/
	bputs(text[QWKUnpacking]);

	ip_can=trashcan_list(&cfg,"ip");
	host_can=trashcan_list(&cfg,"host");
	subject_can=trashcan_list(&cfg,"subject");

	SAFEPRINTF(fname,"%stwitlist.cfg",cfg.ctrl_dir);
	if((fp=fopen(fname,"r"))!=NULL) {
		twit_list=strListReadFile(fp,NULL,128);
		fclose(fp);
	}

	now=time(NULL);
	for(l=QWK_BLOCK_LEN;l<size;l+=blocks*QWK_BLOCK_LEN) {
		if(terminated) {
			bprintf("!Terminated");
			break;
		}

		lncntr=0;					/* defeat pause */
		if(fseek(rep,l,SEEK_SET)!=0) {
			errormsg(WHERE,ERR_SEEK,msg_fname,l);
			errors++;
			break;
		}
		if(fread(block,1,QWK_BLOCK_LEN,rep)!=QWK_BLOCK_LEN) {
			errormsg(WHERE,ERR_READ,msg_fname,(long)ftell(rep));
			errors++;
			break;
		}
		sprintf(tmp,"%.6s",block+116);
		blocks=atoi(tmp);  /* i = number of blocks */
		if(blocks<2) {
			if(block[0] == 'V' && blocks == 1 && voting != NULL) {	/* VOTING DATA */
				if(!qwk_voting(&voting, l, (useron.rest&FLAG('Q')) ? NET_QWK : NET_NONE, /* QWKnet ID : */useron.alias))
					errors++;
				continue;
			}
			SAFEPRINTF3(str,"%s blocks (read '%s' at offset %ld)", msg_fname, tmp, l);
			errormsg(WHERE,ERR_CHK,str,blocks);
			errors++;
			blocks=1;
			continue; 
		}

		long confnum = atol((char *)block+1);

		qwk_new_msg(confnum, &msg, block, /* offset: */l, headers, /* parse_sender_hfields: */useron.rest&FLAG('Q') ? true:false);

		if(cfg.max_qwkmsgage && msg.hdr.when_written.time < (uint32_t)now
			&& (now-msg.hdr.when_written.time)/(24*60*60) > cfg.max_qwkmsgage) {
			SAFEPRINTF2(str,"!Filtering QWK message from %s due to age: %" PRIu64 " days"
				,msg.from
				,(uint64_t)((now-msg.hdr.when_written.time)/(24*60*60))); 
			logline(LOG_NOTICE,"P!",str);
			continue;
		}

		if(findstr_in_list(msg.from_ip,ip_can)) {
			SAFEPRINTF2(str,"!Filtering QWK message from %s due to blocked IP: %s"
				,msg.from
				,msg.from_ip); 
			logline(LOG_NOTICE,"P!",str);
			continue;
		}

		hostname = getHostNameByAddr(msg.from_host);
		if(findstr_in_list(hostname,host_can)) {
			SAFEPRINTF2(str,"!Filtering QWK message from %s due to blocked hostname: %s"
				,msg.from
				,hostname); 
			logline(LOG_NOTICE,"P!",str);
			continue;
		}

		if(findstr_in_list(msg.subj,subject_can)) {
			SAFEPRINTF2(str,"!Filtering QWK message from %s due to filtered subject: %s"
				,msg.from
				,msg.subj); 
			logline(LOG_NOTICE,"P!",str);
			continue;
		}

		if(confnum==0) {						/**********/
			if(useron.rest&FLAG('E')) {         /* E-mail */
				bputs(text[R_Email]);			/**********/
				continue; 
			}

			if(msg.to!=NULL) {
				if(stricmp(msg.to,"NETMAIL")==0) {  /* QWK to FidoNet NetMail */
					qwktonetmail(rep,block,NULL,0);
					continue; 
				}
				if(strchr(msg.to,'@')) {
					qwktonetmail(rep,block,msg.to,0);
					continue; 
				}
				if(!stricmp(msg.to,"SBBS")) {    /* to SBBS, config stuff */
					qwkcfgline(msg.subj,INVALID_SUB);
					continue; 
				}
			}

			if(useron.etoday>=cfg.level_emailperday[useron.level] && !(useron.exempt&FLAG('M'))
				&& !(useron.rest&FLAG('Q'))) {
				bputs(text[TooManyEmailsToday]);
				continue; 
			}
			usernum=0;
			if(msg.to!=NULL) {
				usernum=atoi(msg.to);
				if(usernum>lastuser(&cfg))
					usernum=0;
				if(!usernum)
					usernum=matchuser(&cfg,msg.to,TRUE /* sysop_alias */);
			}
			if(!usernum) {
				bputs(text[UnknownUser]);
				continue; 
			}
			if(usernum==1 && useron.rest&FLAG('S')) {
				bprintf(text[R_Feedback],cfg.sys_op);
				continue; 
			}

			getuserrec(&cfg,usernum,U_MISC,8,str);
			misc=ahtoul(str);
			if(misc&NETMAIL && cfg.sys_misc&SM_FWDTONET) {
				getuserrec(&cfg,usernum,U_NETMAIL,LEN_NETMAIL,str);
				qwktonetmail(rep,block,str,0);
				continue; 
			}

			SAFEPRINTF(smb.file,"%smail",cfg.data_dir);
			smb.retry_time=cfg.smb_retry_time;

			if(lastsub!=INVALID_SUB) {
				smb_close(&smb);
				lastsub=INVALID_SUB; 
			}

			smb.subnum=INVALID_SUB;
			if((k=smb_open(&smb))!=0) {
				errormsg(WHERE,ERR_OPEN,smb.file,k,smb.last_error);
				errors++;
				continue; 
			}

			if(!filelength(fileno(smb.shd_fp))) {
				smb.status.max_crcs=cfg.mail_maxcrcs;
				smb.status.max_msgs=0;
				smb.status.max_age=cfg.mail_maxage;
				smb.status.attr=SMB_EMAIL;
				if((k=smb_create(&smb))!=0) {
					smb_close(&smb);
					errormsg(WHERE,ERR_CREATE,smb.file,k,smb.last_error);
					errors++;
					continue; 
				} 
			}

			if((k=smb_locksmbhdr(&smb))!=0) {
				smb_close(&smb);
				errormsg(WHERE,ERR_LOCK,smb.file,k,smb.last_error);
				errors++;
				continue; 
			}

			if((k=smb_getstatus(&smb))!=0) {
				smb_close(&smb);
				errormsg(WHERE,ERR_READ,smb.file,k,smb.last_error);
				errors++;
				continue; 
			}

			smb_unlocksmbhdr(&smb);

			if(qwk_import_msg(rep, block, blocks
				,/* fromhub: */0,/* subnum: */INVALID_SUB,/* touser: */usernum,&msg)) {

				if(usernum==1) {
					useron.fbacks++;
					logon_fbacks++;
					putuserrec(&cfg,useron.number,U_FBACKS,5
						,ultoa(useron.fbacks,tmp,10)); 
				}
				else {
					useron.emails++;
					logon_emails++;
					putuserrec(&cfg,useron.number,U_EMAILS,5
						,ultoa(useron.emails,tmp,10)); 
				}
				useron.etoday++;
				putuserrec(&cfg,useron.number,U_ETODAY,5
					,ultoa(useron.etoday,tmp,10));
				bprintf(text[Emailed],username(&cfg,usernum,tmp),usernum);
				SAFEPRINTF3(str,"%s sent QWK e-mail to %s #%d"
					,useron.alias,username(&cfg,usernum,tmp),usernum);
				logline("E+",str);
				if(cfg.node_num) {
					for(k=1;k<=cfg.sys_nodes;k++) { /* Tell user, if online */
						getnodedat(k,&node,0);
						if(node.useron==usernum && !(node.misc&NODE_POFF)
							&& (node.status==NODE_INUSE
							|| node.status==NODE_QUIET)) {
							SAFEPRINTF2(str,text[EmailNodeMsg]
								,cfg.node_num,msg.from);
							putnmsg(&cfg,k,str);
							break; 
						} 
					}
				}
				if(cfg.node_num==0 || k>cfg.sys_nodes) {
					SAFEPRINTF(str,text[UserSentYouMail],msg.from);
					putsmsg(&cfg,usernum,str); 
				} 
			} else
				errors++;
			smb_close(&smb);
		}    /* end of email */

				/**************************/
		else {	/* message on a sub-board */
				/**************************/
			if((n=resolve_qwkconf(confnum))==INVALID_SUB) {
				bprintf(text[QWKInvalidConferenceN],confnum);
				SAFEPRINTF2(str,"%s: Invalid QWK conference number %ld",useron.alias,confnum);
				logline(LOG_NOTICE,"P!",str);
				errors++;
				continue; 
			}

			/* if posting, add to new-scan config for QWKnet nodes automatically */
			if(useron.rest&FLAG('Q'))
				subscan[n].cfg|=SUB_CFG_NSCAN;

			if(msg.to!=NULL) {
				if(stricmp(msg.to,"SBBS")==0) {	/* to SBBS, config stuff */
					qwkcfgline(msg.subj,n);
					continue; 
				}
			}

#if 0	/* This stuff isn't really necessary anymore */
			if(!SYSOP && cfg.sub[n]->misc&SUB_QNET) {	/* QWK Netted */
				sprintf(str,"%-25.25s","DROP");         /* Drop from new-scan? */
				if(!strnicmp((char *)block+71,str,25))	/* don't allow post */
					continue;
				sprintf(str,"%-25.25s","ADD");          /* Add to new-scan? */
				if(!strnicmp((char *)block+71,str,25))	/* don't allow post */
					continue; 
			}
#endif

			if(useron.rest&FLAG('Q') && !(cfg.sub[n]->misc&SUB_QNET)) {
				bputs(text[CantPostOnSub]);
				logline(LOG_NOTICE,"P!","Attempted to post QWK message on non-QWKnet sub");
				continue; 
			}

			if(useron.rest&FLAG('P')) {
				bputs(text[R_Post]);
				logline(LOG_NOTICE,"P!","QWK Post attempted");
				continue; 
			}

			if(useron.ptoday>=cfg.level_postsperday[useron.level]
				&& !(useron.rest&FLAG('Q'))) {
				bputs(text[TooManyPostsToday]);
				continue; 
			}

			if(useron.rest&FLAG('N')
				&& cfg.sub[n]->misc&(SUB_FIDO|SUB_PNET|SUB_QNET|SUB_INET)) {
				bputs(text[CantPostOnSub]);
				logline(LOG_NOTICE,"P!","QWK Networked post attempted");
				continue; 
			}

			if(!chk_ar(cfg.sub[n]->post_ar,&useron,&client)) {
				bputs(text[CantPostOnSub]);
				logline(LOG_NOTICE,"P!","QWK Post attempted");
				continue; 
			}

			if((block[0]=='*' || block[0]=='+')
				&& !(cfg.sub[n]->misc&SUB_PRIV)) {
				bputs(text[PrivatePostsNotAllowed]);
				logline(LOG_NOTICE,"P!","QWK Private post attempt");
				continue; 
			}

			if(block[0]=='*' || block[0]=='+'           /* Private post */
				|| cfg.sub[n]->misc&SUB_PONLY) {
				if(msg.to==NULL || !msg.to[0]
					|| stricmp(msg.to,"All")==0) {		/* to blank */
					bputs(text[NoToUser]);				/* or all */
					continue; 
				} 
			}

#if 0	/* This stuff isn't really necessary anymore */
			if(!SYSOP && !(useron.rest&FLAG('Q'))) {
				sprintf(str,"%-25.25s","SYSOP");
				if(!strnicmp((char *)block+21,str,25)) {
					sprintf(str,"%-25.25s",username(&cfg,1,tmp));
					memcpy((char *)block+21,str,25);		/* change from sysop */
				}											/* to user name */
			}
#endif

			/* TWIT FILTER */
			if(findstr_in_list(msg.from,twit_list) || findstr_in_list(msg.to,twit_list)) {
				SAFEPRINTF4(str,"!Filtering QWK post from %s to %s on %s %s"
					,msg.from
					,msg.to
					,cfg.grp[cfg.sub[n]->grp]->sname,cfg.sub[n]->lname);
				logline(LOG_NOTICE,"P!",str);
				continue; 
			}

			if(n!=lastsub) {
				if(lastsub!=INVALID_SUB)
					smb_close(&smb);
				lastsub=INVALID_SUB;
				SAFEPRINTF2(smb.file,"%s%s",cfg.sub[n]->data_dir,cfg.sub[n]->code);
				smb.retry_time=cfg.smb_retry_time;
				smb.subnum=n;
				if((j=smb_open(&smb))!=0) {
					errormsg(WHERE,ERR_OPEN,smb.file,j,smb.last_error);
					errors++;
					continue; 
				}

				if(!filelength(fileno(smb.shd_fp))) {
					smb.status.max_crcs=cfg.sub[n]->maxcrcs;
					smb.status.max_msgs=cfg.sub[n]->maxmsgs;
					smb.status.max_age=cfg.sub[n]->maxage;
					smb.status.attr=cfg.sub[n]->misc&SUB_HYPER ? SMB_HYPERALLOC:0;
					if((j=smb_create(&smb))!=0) {
						smb_close(&smb);
						lastsub=INVALID_SUB;
						errormsg(WHERE,ERR_CREATE,smb.file,j,smb.last_error);
						errors++;
						continue; 
					} 
				}

				if((j=smb_locksmbhdr(&smb))!=0) {
					smb_close(&smb);
					lastsub=INVALID_SUB;
					errormsg(WHERE,ERR_LOCK,smb.file,j,smb.last_error);
					errors++;
					continue; 
				}
				if((j=smb_getstatus(&smb))!=0) {
					smb_close(&smb);
					lastsub=INVALID_SUB;
					errormsg(WHERE,ERR_READ,smb.file,j,smb.last_error);
					errors++;
					continue; 
				}
				smb_unlocksmbhdr(&smb);
				lastsub=n; 
			}

			if(qwk_import_msg(rep, block, blocks
				,/* fromhub: */0,/* subnum: */n,/* touser: */0,&msg)) {
				logon_posts++;
				user_posted_msg(&cfg, &useron, 1);
				bprintf(text[Posted],cfg.grp[cfg.sub[n]->grp]->sname
					,cfg.sub[n]->lname);
				SAFEPRINTF3(str,"%s posted QWK message on %s %s"
					,useron.alias,cfg.grp[cfg.sub[n]->grp]->sname,cfg.sub[n]->lname);
				signal_sub_sem(&cfg,n);
				logline("P+",str); 
				if(!(useron.rest&FLAG('Q')))
					user_event(EVENT_POST);
			} else
				errors++;
		}   /* end of public message */
	}

	qwk_handle_remaining_votes(&voting, (useron.rest&FLAG('Q')) ? NET_QWK : NET_NONE, /* QWKnet ID : */useron.alias);

	update_qwkroute(NULL);			/* Write ROUTE.DAT */

	smb_freemsgmem(&msg);

	iniFreeStringList(headers);
	iniFreeStringList(voting);

	strListFree(&ip_can);
	strListFree(&host_can);
	strListFree(&subject_can);
	strListFree(&twit_list);

	if(lastsub!=INVALID_SUB)
		smb_close(&smb);
	fclose(rep);

	/* QWKE support */
	SAFEPRINTF(fname,"%sTODOOR.EXT",cfg.temp_dir);
	if(fexistcase(fname)) {
		set_qwk_flag(QWK_EXT);
		FILE* fp=fopen(fname,"r");
		char* p;
		if(fp!=NULL) {
			while(!feof(fp)) {
				if(!fgets(str,sizeof(str)-1,fp))
					break;
				if(strnicmp(str,"AREA ",5)==0) {
					p=str+5;
					SKIP_WHITESPACE(p);
					if((n=resolve_qwkconf(atoi(p))) != INVALID_SUB) {
						FIND_WHITESPACE(p);
						SKIP_WHITESPACE(p);
						if(strchr(p,'D'))
							subscan[n].cfg&=~SUB_CFG_NSCAN;
						else if(strchr(p,'a') || strchr(p,'g'))
							subscan[n].cfg|=SUB_CFG_NSCAN;
						else if(strchr(p,'p'))
							subscan[n].cfg|=SUB_CFG_NSCAN|SUB_CFG_YSCAN;
					}
					continue;
				}
				if(strnicmp(str,"RESET ",6)==0) {
					p=str+6;
					SKIP_WHITESPACE(p);
					if((n=resolve_qwkconf(atoi(p))) != INVALID_SUB) {
						FIND_WHITESPACE(p);
						SKIP_WHITESPACE(p);
						/* If the [#ofmessages] is blank then the pointer should be set back to the start of the message base */
						if(*p==0)
							subscan[n].ptr=0;
						else {
							/* otherwise it should be set back [#ofmessages] back from the end of the message base. */
							uint32_t last=0;
							getlastmsg(n,&last,/* time_t* */NULL);
							l=last-atol(p);
							if(l<0)
								l=0;
							subscan[n].ptr=l;
						}
					}
				}
			}
			fclose(fp);
		}
	}

	if(useron.rest&FLAG('Q')) {             /* QWK Net Node */
		if(fexistcase(msg_fname))
			remove(msg_fname);
		SAFEPRINTF(fname,"%sATTXREF.DAT",cfg.temp_dir);
		if(fexistcase(fname))
			remove(fname);

		dir=opendir(cfg.temp_dir);
		while(dir!=NULL && (dirent=readdir(dir))!=NULL) {				/* Extra files */
			// Move files
			SAFEPRINTF2(str,"%s%s",cfg.temp_dir,dirent->d_name);
			if(isdir(str))
				continue;

			// Create directory if necessary
			SAFEPRINTF2(inbox,"%sqnet/%s.in",cfg.data_dir,useron.alias);
			MKDIR(inbox); 

			SAFEPRINTF2(fname,"%s/%s",inbox,dirent->d_name);
			mv(str,fname,1);
			SAFEPRINTF2(str,text[ReceivedFileViaQWK],dirent->d_name,useron.alias);
			putsmsg(&cfg,1,str);
		} 
		if(dir!=NULL)
			closedir(dir);
		SAFEPRINTF(fname,"%sqnet-rep.now",cfg.data_dir);
		ftouch(fname);
	}

	bputs(text[QWKUnpacked]);
	CRLF;
	/**********************************************/
	/* Hang-up now if that's what the user wanted */
	/**********************************************/
	autohangup();

	return errors == 0;
}
