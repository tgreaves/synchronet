dk.console.remote_io.sock = new Socket(true, dk.connection.socket);
if (!dk.console.remote_io.sock.is_connected)
	throw("Unable to create socket object");

dk.console.remote_io.print = function(string) {
	this.sock.write(string.replace(/\xff/g, "\xff\xff"));
};

load("ansi_console.js");

var socket_input_queue = load(true, "socket_input.js", dk.connection.socket);
js.on_exit("socket_input_queue.write(''); socket_input_queue.poll(0x7fffffff);");
