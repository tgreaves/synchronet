load("sbbsdefs.js");
load("lockfile.js");

var doorscan_dir='.';
try { throw barfitty.barf(barf) } catch(e) { doorscan_dir=e.fileName }
doorscan_dir=doorscan_dir.replace(/[\/\\][^\/\\]*$/,'');
doorscan_dir=backslash(doorscan_dir);

function LockedOpen(filename, fmode)
{
	var f=new File(filename);
	var suffix='';
	var mode='';
	var wr=false;

	if(fmode.search(/b/)!=-1)
		suffix+='b';
	if(fmode.search(/\+/)!=-1) {
		suffix+= '+';
		wr=true;
	}

	/* TODO: Possible race here in mode */
	var m=fmode.match(/[rwa]/);
	if(m==null)
		throw("Unknown file mode "+fmode);
	switch(m[0]) {
		case 'r':
			mode='r'+suffix;
			if(wr) {
				if(!f.exists)
					mode='w'+suffix;
			}
			break;
		default:
			wr=true;
			mode=m[0]+suffix;
			break;
	}

	if(!Lock(f.name, system.node_num, wr, 5))
		throw("Unable to lock "+f.name);

	if(!f.open(mode, true)) {
		Unlock(f.name);
		throw("Unable to open "+f.name);
	}
	return(f);
}

function Display()
{
	this.ANSI=Display_ANSI;
	this.ASCII=Display_ASCII;
	this.ASC=Display_ASC;
}

function Display_ANSI(filename)
{
	/* If the user doesn't support ANSI, return false... */
	if(!(console.term_supports(USER_ANSI)))
		return(false);
	return(this.ASCII(filename));
}

function Display_ASCII(filename)
{
	var f=LockedOpen(filename, "rb");
	var txt;

	while((txt=f.read())!=undefined) {
		txt=f.read();
		if(txt==undefined || txt=='')
			break;
		console.write(txt);
	}
	return(true);
}

function Display_ASC(filename)
{
	console.printfile(filename);
	return(true);
}

/*
 * This is like Display, but for news files...
 * the idea is that this *could* display only news since the last time you
 * started the door or something.
 */
function NewsDisplay()
{
	this.ANSI=NewsDisplay_ANSI;
	this.ASCII=NewsDisplay_ASCII;
	this.ASC=NewsDisplay_ASC;
}

function NewsDisplay_ANSI(filename, since)
{
	new Display().ANSI(filename);
}

function NewsDisplay_ASCII(filename, since)
{
	new Display().ASCII(filename);
}

function NewsDisplay_ASC(filename, since)
{
	new Display().ASC(filename);
}

function DoorConfig(leaveopen)
{
	if(leaveopen==undefined)
		leaveopen=false;
	this.file=LockedOpen(doorscan_dir+"doors.ini", "r+");

	var sections=this.file.iniGetAllObjects();
	if(sections==undefined)
		sections=new Array();

	this.door=new Object();
	for(var i in sections) {
		if(sections[i].installed != undefined)
			sections[i].installed=new Date(sections[i].installed);
		if(sections[i].lastRan != undefined)
			sections[i].lastRan=new Date(sections[i].lastRan);
		if(sections[i].lastExit != undefined)
			sections[i].lastExit=new Date(sections[i].lastExit);
		this.door[sections[i].name]=sections[i];
	}
	this.global=this.file.iniGetObject();
	this.skipSection=new Object();
	if(this.global.skipSections != undefined) {
		var sections=this.global.skipSections.split(/,/);
		for(var sec in sections) {
			this.skipSection[sections[sec]]=true;
		}
		delete this.global.skipSections;
	}

	var updated=false;
	for(var door in xtrn_area.prog) {
		if(this.door[door]==undefined) {
			this.door[door]={name:door, installed:new Date(), runCount:0}
			sections.push(this.door[door]);
			updated=true;
		}
	}
	if(updated) {
		this.file.iniSetObject(null,this.global);
		this.file.iniSetAllObjects(sections);
	}

	this.save=DoorConfig_save;
	if(!leaveopen) {
		this.file.close();
		Unlock(this.file.name);
	}
}

function DoorConfig_save()
{
	if(!this.file.is_open)
		this.file=LockedOpen(this.file.name, "r+");

	var sections=new Array();

	for(var door in this.door)
		sections.push(this.door[door]);

	var sections=new Array();
	for(var section in this.skipSection) {
		if(this.skipSection[section])
			sections.push(section);
	}
	this.global.skipSections=sections.join(',');

	this.file.iniSetObject(null,this.global);
	delete this.global.skipSections;

	this.file.iniSetAllObjects(sections);

	this.file.close();
	Unlock(this.file.name);
	return(true);
}

function UserConfig(unum, leaveopen)
{
	var parsefile=true;

	if(leaveopen==undefined)
		leaveopen=false;
	this.door=new Object();
	this.user_number=unum;
	this.save=UserConfig_save;
	this.addxtrn=UserConfig_addxtrn;
	this.configure=UserConfig_configure;

	if(unum==undefined) {
		this.file=LockedOpen(doorscan_dir+"defaults.ini", "r+");
	}
	else {
		this.file=LockedOpen(format("%suser/%04u.doorscan",system.data_dir,unum), "r+");
		if(this.file.length==0) {
			var defaults=new UserConfig();

			this.global=eval(defaults.global.toSource());
			this.door=eval(defaults.door.toSource());
			parsefile=false;
		}
	}

	if(parsefile) {
		var sections=this.file.iniGetAllObjects();
		if(sections==undefined)
			sections=new Array();

		for(var i in sections) {
			if(sections[i].lastRan != undefined)
				sections[i].lastRan=new Date(sections[i].lastRan);
			if(sections[i].lastExit != undefined)
				sections[i].lastExit=new Date(sections[i].lastExit);
			this.door[sections[i].name]=sections[i];
		}
		this.global=this.file.iniGetObject();
	}

	if(this.global.lastScan==undefined) {
		this.global.lastScan=new Date();
	}
	else {
		this.global.lastScan=new Date(this.global.lastScan);
	}

	if(!leaveopen) {
		this.file.close();
		Unlock(this.file.name);
	}
}

function UserConfig_save()
{
	if(!this.file.is_open)
		this.file=LockedOpen(this.file.name, "r+");
	var sections=new Array();

	for(var door in this.door)
		sections.push(this.door[door]);

	this.file.iniSetObject(null,this.global);
	this.file.iniSetAllObjects(sections);

	this.file.close();
	Unlock(this.file.name);
	return(true);
}

function doScan()
{
	var dcfg=new DoorConfig();
	var ucfg=new UserConfig(user.number);
	var dsp=new Display();
	var door;
	var tmp;

	/* First, look for new doors */
	for(door in dcfg.door) {
		if(dcfg.skipSection[door]!=undefined && dcfg.skipSection[door])
			continue;
		if(dcfg.door[door].skip != undefined && dcfg.door[door].skip)
			continue;
		if(dcfg.door[door].installed > ucfg.global.lastScan) {
			/* This door is NEW! */
			
			/* If the user can't run it, don't display it. */
			if(!xtrn_area.prog[door].can_run)
				continue;

			tmp=false;
			if(dcfg.door[door].ad != undefined) {
				/* Assume ANSI */
				if(dcfg.door[door].adType==undefined)
					tmp=dsp.ANSI(dcfg.door[door].ad);
				else {
					if(dsp[dcfg.door[door].adType] == undefined)
						log("doorscan WARNING ad type "+dcfg.door[door].adType+" for door "+door+" does not have a display method.");
					else
						tmp=dsp[dcfg.door[door].adType](dcfg.door[door].ad);
				}
			}
			if(tmp==false) {
				/* No ad... just display a message... */
				console.attributes=YELLOW;
				console.writeln("New external: "+xtrn_area.prog[door].name+" in the "+xtrn_area.sec[xtrn_area.prog[door].sec_code].name+" section.");
				console.crlf();
			}
			if(ucfg.door[door]==undefined && ucfg.global.addNew)
				ucfg.door[door].addxtrn(door);
		}
	}

	/*
	 * Next, look for doors which have been played since either
	 * your last scan or the last time you left the game, whichever is
	 * newer
	 */
	for(door in ucfg.door) {
		if(dcfg.skipSection[door]!=undefined && dcfg.skipSection[door])
			continue;
		if(dcfg.door[door].skip != undefined && dcfg.door[door].skip)
			continue;
		tmp=ucfg.global.lastScan;
		if(ucfg.door[door].lastExit != undefined && ucfg.door[door].lastExit > tmp)
			tmp=ucfg.door[door].lastExit;
		if(dcfg.door[door].lastRan != undefined && dcfg.door[door].lastRan > tmp) {
			/* Yes, this has been played... */

			/* News File */
			if(!ucfg.door[door].skipNews) {
				if(dcfg.door[door].news != undefined) {
					/*
					 * If the news file has not been updated, don't bother
					 * Some doors only update the news during maintenance
					 */
					
					if(new Date(file_date(dcfg.door[door].news)*1000) >= tmp) {
						/* Assume ANSI */
						if(dcfg.door[door].newsType==undefined)
							dsp.ANSI(dcfg.door[door].news);
						else {
							if(dsp[dcfg.door[door].newsType] == undefined)
								log("doorscan WARNING News type "+dcfg.door[door].newsType+" for door "+door+" does not have a display method.");
							else
								dsp[dcfg.door[door].newsType](dcfg.door[door].news);
						}
					}
				}
			}

			if(!ucfg.door[door].skipScores) {
				if(dcfg.door[door].scores != undefined) {
					/*
					 * If the Scores file has not been updated, don't bother
					 * Some doors only update the Scores during maintenance
					 */
					
					if(new Date(file_date(dcfg.door[door].scores)*1000) >= tmp) {
						/* Assume ANSI */
						if(dcfg.door[door].scoresType==undefined)
							dsp.ANSI(dcfg.door[door].scores);
						else {
							if(dsp[dcfg.door[door].scoresType] == undefined)
								log("doorscan WARNING Scores type "+dcfg.door[door].scoresType+" for door "+door+" does not have a display method.");
							else
								dsp[dcfg.door[door].scoresType](dcfg.door[door].scores);
						}
					}
				}
			}

			if(!ucfg.door[door].skipRunCount) {
				if(ucfg.door[door].lastRunCount != undefined) {
					console.attributes=LIGHTCYAN;
					console.writeln(xtrn_area.prog[door].name+" in the "+xtrn_area.sec[xtrn_area.prog[door].sec_code].name+" section has been ran "+(dcfg.door[door].runCount-ucfg.door[door].lastRunCount)+" times since you last played");
				}
			}
			// TODO: List how many users have played and possible who (needs more logging)
		}
	}
	ucfg=new UserConfig(user.number, true);
	ucfg.global.lastScan=new Date();
	ucfg.save();
}

function UserConfig_addxtrn(xtrn)
{
	if(this.door[xtrn] == undefined) {
		this.door[xtrn]=new Object();
		this.door[xtrn].name=xtrn;
		if(this.global.defaultSkipScores)
			this.door[xtrn].skipScores=true;
		if(this.global.defaultSkipNews)
			this.door[xtrn].skipNews=true;
		if(ucfg.global.defaultSkipRunCount)
			this.door[xtrn].skipRunCount=true;
	}
}

function runXtrn(xtrn)
{
	if(xtrn_area.prog[xtrn]==undefined)
		throw("Unknown external: "+xtrn);
	if(!xtrn_area.prog[xtrn].can_run)
		throw("User "+user.name+" is not allowed to run "+xtrn);

	var now=new Date();
	var dcfg=new DoorConfig(true);

	dcfg.door[xtrn].lastRan=now;
	dcfg.door[xtrn].runCount++;
	dcfg.save();

	var ucfg=new UserConfig(user.number, true);
	if(ucfg.door[xtrn] == undefined) {
		if(!this.global.noAutoScan)
			ucfg.addxtrn(xtrn);
	}

	if(ucfg.door[xtrn] != undefined)
		ucfg.door[xtrn].lastRan=now;

	ucfg.save();

	bbs.exec_xtrn(xtrn);

	dcfg=new DoorConfig(true);

	dcfg.door[xtrn].lastExit=now;
	dcfg.save();

	ucfg=new UserConfig(user.number, true);
	if(ucfg.door[xtrn] != undefined) {
		ucfg.door[xtrn].lastExit=now;
		ucfg.door[xtrn].lastRunCount=dcfg.door[xtrn].runCount;
	}

	ucfg.save();
}

function UserConfig_configure()
{
	var dcfg=new DoorConfig();

	/*
	 * User settings
	 * Per Door: 
	 *		skipNews			Does not display the configured news file
	 *		skipScores			Does not display the configured scores file
	 *		skipRunCount		Does not display the number of times ran
	 * Globals:
	 *		lastScan			Date/Time of last scan
	 *		addNew				New doors should be added to the scan config
	 *		noAutoScan			Do not add doors which ara ran to the scan
	 *		defaultSkipNews		New entries should set skipNews
	 *		defaultSkipScores	New entries should set skipScores
	 *		defaultSkipRunCount	New entries should set skipRunCount
	 */

	/*
	 * Default scan config - new UserConfig(null)
	 */

	// TODO: User configuration
}

function sysop_config()
{
	/*
	 * Door Scan configuration
	 * Per Door:
	 *		ad					Path to ad file
	 *		atType				Ad file type
	 *		score				Path to score file
	 *		scoreType			Score file type
	 *		news				Path to News file
	 *		newsType			News file type
	 *		skip				Do not include this door in scans
	 * Globals:
	 * Top-level:
	 *		skipSection			Object which contains bool properties
	 *							If the bool property is true, the door with the
	 *							same internal code as the property name will
	 *							not be included in scans.
	 */
}

for(i in argv) {
	switch(argv[i].toLowerCase()) {
		case 'scan':
			doScan();
			break;
		case 'run':
			if(i+1<argc)
				runXtrn(argv[++i].toLowerCase());
			else
				throw("XTRN code not included on command-line!");
			break;
		case 'test':
			var dcfg=new DoorConfig();
			dcfg.save();
			var ucfg=new UserConfig(user.number);
			ucfg.save();
			break;
		case 'config':
			new UserConfig(user.number).configure();
			break;
		case 'sysconfig':
			// TODO: Sysop configuration
			break;
		case 'rank':
			// TODO: Door popularity rankings
			break;
	}
}
