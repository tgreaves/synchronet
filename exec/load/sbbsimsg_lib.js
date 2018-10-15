// sbbsimsg_lib.js

// Synchronet inter-bbs instant message library

// $Id$

load("sockdefs.js");	// SOCK_DGRAM

// Read the list of systems into list array
var filename = system.ctrl_dir + "sbbsimsg.lst";
var sys_list = {};
var sock = new Socket(SOCK_DGRAM);

function read_sys_list()
{
	var f = new File(filename);
	if(!f.open("r")) {
		return null;
	}

	var list = f.readAll();
	f.close();
	for(var i in list) {
		if(list[i]==null)
			break;
		var line = list[i].trimLeft();
		if(line.charAt(0)==';')		// comment? 
			continue;

		var word = line.split('\t');
		var host = word[0].trimRight();

		if(host == system.host_name
			|| host == system.inetaddr)		// local system?
			continue;						// ignore

		var ip_addr = word[1];
		var name = word[2];

//		if(js.global.client && ip_addr == client.socket.local_ip_address)
//			continue;
		if(js.global.server && ip_addr == server.interface_ip_address)
			continue;

		this.sys_list[ip_addr] = { host: host, name: name, users: [] };
	}
	
	return this.sys_list;
}

function request_active_users()
{
	var requests_sent = 0;
	for(var i in sys_list) {
		if(!sock.sendto("\r\n", i, IPPORT_SYSTAT))	// Get list of active users
			continue;
		requests_sent++;
	}
	return requests_sent;
}

function find_name(arr, name)
{
	for(var i in arr)
		if(arr[i].name == name)
			return true;
	return false;
}

function parse_active_users(message, logon_callback, logoff_callback)
{
	if(!message)
		return false;
	var sys = sys_list[message.ip_address];
	if(!sys) {
		alert("Unknown system: " + message.ip_address);
		return false;
	}
	
	sys.last_response = time();
	
	var response = message.data.split("\r\n");
	
	// Skip header
	while(response.length && response[0].charAt(0)!='-')
		response.shift();
	if(response.length && response[0].charAt(0)=='-')
		response.shift();	// Delete the separator line
	while(response.length && !response[0].length)
		response.shift();	// Delete any blank lines
	while(response.length && !response[response.length-1].length)
		response.pop();		// Delete trailing blank lines
	
	var old_users = sys.users.slice();
	sys.users = [];

	for(var i in response) {
		if(response[i]=="")
			continue;

		sys.users.push( { 
			name: format("%.25s",response[i]).trimRight(),
			action: response[i].substr(26, 31).trimLeft().trimRight(),
			timeon: response[i].substr(57, 9).trimLeft(),
			age: response[i].substr(67, 3).trimLeft(),
			sex: response[i].substr(71, 3).trimLeft(),
			node: response[i].substr(75, 4).trimLeft(),
			time: time() 
			} );
	}
	
	if(logon_callback) {
		var count = 0;
		for(var i in sys.users) {
			if(!find_name(old_users, sys.users[i].name)) {
				logon_callback(sys.users[i], count ? null : sys);
				count++;
			}
		}
	}
	if(logoff_callback) {
		for(var i in old_users) {
			if(!find_name(sys.users, old_users[i].name))
				logoff_callback(old_users[i], sys);
		}
	}
			
	return true;
}

function receive_active_users()
{
	return sock.recvfrom(0x10000);
}

function poll_systems(sent, interval, timeout, callback)
{
	var replies = 0;
	var begin = new Date();
	for(var loop = 0; replies < sent && new Date().valueOf()-begin.valueOf() < timeout; loop++)
	{
		if(callback)
			callback(loop);
		if(!sock.poll(interval))
			continue;

		var message = receive_active_users();
		if(message == null)
			continue;
		replies++;

		var result = parse_active_users(message);
		if(!result)
			alert("Failed to parse: " + JSON.stringify(message));
	}
	return replies;
}

// Returns true on success, string (error) on failure
function send_msg(dest, msg, from)
{
	var hp = dest.indexOf('@');
	if(hp < 0)
		return "Invalid user";
	var host = dest.slice(hp+1);
	var destuser = dest.substr(0, hp);
	var sock = new Socket();
	if(!sock.connect(host, IPPORT_MSP))
		return "MSP Connection to " + host + " failed with error " + sock.last_error;
	var result = sock.send("B" + destuser + "\0" + /* Dest node +*/"\0" + msg + "\0" + from + "\0\0\0" + system.name + "\0");
	sock.close();
	if(result < 1)
		return "MSP Send to " + host + " failed with error " + sock.last_error;
	return true;
}

// TEST CODE ONLY:
if(this.argc == 1 && argv[0] == "test") {
	
	this.read_sys_list();
	print(JSON.stringify(sys_list, null ,4));
	var sent = request_active_users();
	printf("Requests sent = %d\n", sent);

	var UDP_RESPONSE_TIMEOUT = 10000;

	function logon_callback(sys, user)
	{
		print("User " + user.name + " logged on to " + sys.name);
	}

	function poll_callback(loop)
	{
		printf("%c\r", "/-\\|"[loop%4]);
	}

	var replies = poll_systems(sent, 0.25, UDP_RESPONSE_TIMEOUT, poll_callback);

	printf("Replies received = %d\n", replies);

	print(JSON.stringify(sys_list, null, 4));
}

this;	// Must be last line