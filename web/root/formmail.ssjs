// Basic FormMail module

// $Id$

// Requires query values: to, from, and subject
// Message body text is contained in the post data

load("sbbsdefs.js");

var redir = http_request.query.redir;
if(!redir)
	redir = http_request.header.referer;

function results(level, text)
{
	log(level,text);
	writeln("<html>");
	writeln("<head>");
	writeln("<title>Sending e-mail</title>");

	if(level>LOG_WARNING) {
		writeln('<meta http-equiv="Pragma" content="no-cache">');
		writeln('<meta http-equiv="expires" content="0">');
		writeln('<meta http-equiv="refresh" content="3; URL=' 
			+ redir +'">');
	}
	writeln("</head>");

	writeln("<body>");
	writeln("<center>");
	if(level<=LOG_WARNING) {
		writeln("!ERROR: ");
	}
	writeln(text);
	if(level>LOG_WARNING) {
		writeln("<p>");
		writeln("Returning to <i>" + redir + "</i>");
	}
	writeln("</body>");
	writeln("</html>");
	exit();
}

if(http_request.query.to==undefined
    || http_request.query.from==undefined
    || http_request.query.subject==undefined)
	results(LOG_ERR,"mising query field");

if(http_request.post_data==undefined)
	results(LOG_ERR,"mising post data");

var msgbase=new MsgBase("mail");
if(!msgbase.open())
	results(LOG_ERR,format("%s opening mail base", msgbase.error));

var hdr = { from: http_request.query.from,
			to: http_request.query.to,
			subject: http_request.query.subject };

hdr.to_net_type=netaddr_type(hdr.to);
if(hdr.to_net_type!=NET_NONE)
	hdr.to_net_addr=hdr.to;
else {
	var usrnum=system.matchuser(hdr.to);
	if(usrnum==0)
		results(LOG_ERR,"bad local username specified: " + hdr.to);
	hdr.to_ext=usrnum;
}

if(!msgbase.save_msg(hdr,client,http_request.post_data))
	results(LOG_ERR,format("%s saving message", msgbase.error));

results(LOG_INFO,"E-mail sent to <i>" + hdr.to + "</i> successfully.");
