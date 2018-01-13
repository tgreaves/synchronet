load('sbbsdefs.js');
load('frame.js');
load('tree.js');
load('scrollbar.js');
load('event-timer.js');
load('ansiedit.js');

const sauce_lib = load({}, 'sauce_lib.js');
const avatar_lib = load({}, 'avatar_lib.js');

const BORDER = [ BLUE, LIGHTBLUE, CYAN, LIGHTCYAN, WHITE ];
const TITLE = 'Avatar Settings';
const TITLE_COLOR = WHITE;
const QUIT_TEXT = 'Press Q to quit';
const EXCLUDE_FILES = /\.\d+\.bin$/;

Frame.prototype.drawBorder = function (color, title) {
	this.pushxy();
	var theColor = color;
	if (Array.isArray(color)) {
		var sectionLength = Math.round(this.width / color.length);
	}
	for (var y = 1; y <= this.height; y++) {
		for (var x = 1; x <= this.width; x++) {
			if (x > 1 && x < this.width && y > 1 && y < this.height) continue;
			var msg;
			this.gotoxy(x, y);
			if (y == 1 && x == 1) {
				msg = ascii(218);
			} else if (y == 1 && x == this.width) {
				msg = ascii(191);
			} else if (y == this.height && x == 1) {
				msg = ascii(192);
			} else if (y == this.height && x == this.width) {
				msg = ascii(217);
			} else if (x == 1 || x == this.width) {
				msg = ascii(179);
			} else {
				msg = ascii(196);
			}
			if (typeof sectionLength != 'undefined') {
				if (x == 1) {
					theColor = color[0];
				} else if (x % sectionLength == 0 && x < this.width) {
					theColor = color[x / sectionLength];
				} else if (x == this.width) {
					theColor = color[color.length - 1];
				}
			}
			this.putmsg(msg, theColor);
		}
	}
	if (typeof title == 'object') {
		this.gotoxy(title.x, title.y);
		this.attr = title.attr;
		this.putmsg(ascii(180) + title.text + ascii(195));
	}
	this.popxy();
}

Frame.prototype.nest = function (paddingX, paddingY) {
	if (typeof this.parent === 'undefined') return;
	const xOffset = (typeof paddingX === 'number' ? paddingX : 0);
	const yOffset = (typeof paddingY === 'number' ? paddingY : 0);
	this.x = this.parent.x + xOffset;
	this.y = this.parent.y + yOffset;
	this.width = this.parent.width - (xOffset * 2);
	this.height = this.parent.height - (yOffset * 2);
}

Frame.prototype.blit = function (bin, w, h, x, y, str, sc) {
	var o = 0; // offset into 'bin'
	for (var yy = 0; yy < h; yy++) {
		for (var xx = 0; xx < w; xx++) {
			this.setData(x + xx, y + yy, bin.substr(o, 1), bin.substr(o + 1, 1).charCodeAt(0) || BG_BLACK);
			o = o + 2;
		}
	}
	if (typeof str !== 'undefined') {
		// get fancy and center this later
		str = str.substr(0, 10);
        var o = Math.floor((w - str.length) / 2);
		for (var n = 0; n < str.length; n++) {
			this.setData(x + n + o, y + h, str.substr(n, 1), sc);
		}
	}
}

function bury_cursor() {
	console.gotoxy(console.screen_columns, console.screen_rows);
}

function download_avatar() {
	const user_avatar = avatar_lib.read_localuser(user.number);
	if (user_avatar) {
		const fn = system.temp_dir + system.qwk_id.toLowerCase() + '.avatar.bin';
		var f = new File(fn);
		f.open('wb');
		f.write(base64_decode(user_avatar.data));
		f.close();
		const sauce = {
			title : user.alias + ' avatar',
			author : user.alias,
			group : system.name,
			datatype : sauce_lib.defs.datatype.bin,
			tinfo1 : avatar_lib.defs.width,
			tinfo2 : avatar_lib.defs.height,
			tinfo3 : 0,
			tinfo4 : 0,
			comment : ['']
		};
		sauce_lib.write(fn, sauce);
		bbs.send_file(fn);
		file_remove(fn);
		return true;
	} else {
		return false;
	}
}

function upload_avatar() {
	const fn = system.temp_dir + format('avatar-%04d.bin', user.number);
	bbs.receive_file(fn);
	const success = avatar_lib.import_file(user.number, fn, 0);
	file_remove(fn);
	return success;
}

function remove_avatar() {
	if(console.noyes("Remove your avatar"))
		return false;
	return avatar_lib.remove_localuser(user.number);
}

function CollectionBrowser(filename, parent_frame) {

	const frames = {
		parent : parent_frame,
		container : null,
		highlight : null,
		scrollbar : null
	};

	const collection = {
		title : '',
		descriptions : [],
		count : 0
	};

	const state = {
		cols : 0,
		rows : 0,
		selected : 0
	};

	function parse_sauce() {
		const sauce = sauce_lib.read(filename);
		collection.title = sauce.title;
		collection.descriptions = sauce.comment;
		collection.count = Math.floor(sauce.rows / avatar_lib.defs.height);
	}

	function draw_collection(offset) {

		const f = new File(filename);
		f.open('rb');
		const avatars = f.read();
		f.close();

		var x = 1, y = 2;
		for (var a = 0; a < collection.count; a++) {
			frames.container.blit(avatars.substr(a * avatar_lib.size, avatar_lib.size), avatar_lib.defs.width, avatar_lib.defs.height, x, y, collection.descriptions[a], WHITE);
			x += avatar_lib.defs.width + 2;
			if (x + avatar_lib.defs.width >= frames.container.x + frames.container.width) {
				x = 1;
				y += avatar_lib.defs.height + 2;
			}
		}

		highlight();

	}

	function highlight() {
		// Column and row of this avatar in the list
		var col = state.selected % state.cols;
		var row = Math.floor(state.selected / state.cols);
		// Actual x, y coordinates of avatar graphic within frames.container
		var x = 1 + (col * avatar_lib.defs.width) + (2 * col);
		var y = 2 + (row * avatar_lib.defs.height) + (2 * row);
		if (y - 1 + avatar_lib.defs.height + 3 > frames.container.height) {
			frames.container.scrollTo(0, y - 1);
		} else if (y - 1 < frames.container.offset.y) {
			frames.container.scrollTo(0, y - 1);
		}
		frames.highlight.moveTo(frames.container.x + x - 1, frames.container.y + y - 1 - frames.container.offset.y);
	}

	function flashy_flashy() {
		for (var n = 0; n < 5; n++) {
			frames.highlight.close();
			frames.highlight.cycle();
			bury_cursor();
			mswait(75);
			frames.highlight.open();
			frames.highlight.cycle();
			bury_cursor();
			mswait(75);
		}
	}

	this.open = function () {

		parse_sauce();

		frames.container = new Frame(1, 1, 1, 1, WHITE, frames.parent);
		frames.container.nest(1, 1);
		frames.container.gotoxy(1, 1);
		frames.container.center(collection.title);
		frames.container.checkbounds = false;
		frames.container.v_scroll = true;

		frames.highlight = new Frame(frames.container.x + 1, frames.container.y + 1, avatar_lib.defs.width + 2, avatar_lib.defs.height + 3, WHITE, frames.container);
		frames.highlight.transparent = true;
		frames.highlight.drawBorder(BORDER);

		state.cols = Math.floor((frames.container.width - 3) / (avatar_lib.defs.width + 2));
		state.rows = Math.floor((frames.container.height - 2) / (avatar_lib.defs.height + 2));

		frames.scrollbar = new ScrollBar(frames.container);

		if (frames.parent.is_open) frames.container.open();
		draw_collection();

	}

	this.getcmd = function (cmd) {
		var ret = null;
		switch (cmd.toLowerCase()) {
			case KEY_LEFT:
				if (state.selected > 0) {
					state.selected--;
					highlight();
				}
				break;
			case KEY_RIGHT:
				if (state.selected < collection.count - 1) {
					state.selected++;
					highlight();
				}
				break;
			case KEY_UP:
				if (state.selected - state.cols >= 0) {
					state.selected -= state.cols;
					highlight();
				}
				break;
			case KEY_DOWN:
				if (state.selected + state.cols < collection.count) {
					state.selected += state.cols;
					highlight();
				}
				break;
			case '\r':
			case '\n':
				flashy_flashy();
				ret = state.selected;
				break;
			case 'q':
				ret = -1;
				break;
			default:
				break;
		}
		return ret;
	}

	this.cycle = function () {
		frames.scrollbar.cycle();
	}

	this.close = function () {
		frames.highlight.delete();
		frames.container.delete();
	}

}

function CollectionLister(dir, parent_frame) {

	const frames = {
		container : null,
		tree : null,
		info : null,
		parent : parent_frame
	};

	const state = {
		tree : null,
		cb : null,
		collection : null
	};

	function display_collection_info(sauce) {
		frames.info.clear();
		frames.info.putmsg('Author: ' + (sauce.author.length ? sauce.author : 'Unknown') + '\r\n');
		frames.info.putmsg('Group: ' + (sauce.group.length ? sauce.group : 'Unknown') + '\r\n');
		frames.info.putmsg('Avatars: ' + Math.floor(sauce.rows / avatar_lib.defs.height) + '\r\n');
		frames.info.putmsg('ICE Colors: ' + (sauce.ice_color ? 'Yes' : 'No') + '\r\n');
		frames.info.putmsg('Updated: ' + sauce.date.toLocaleDateString());
	}

	this.open = function () {

		frames.container = new Frame(1, 1, 1, 1, WHITE, parent_frame);
		frames.container.transparent = true;
		frames.container.nest(1, 1);
		frames.container.center('Avatar Collections');

		frames.tree = new Frame(
			frames.container.x,
			frames.container.y + 2,
			Math.floor((frames.container.width - 2) / 2),
			frames.container.height - 3,
			0,
			frames.container
		);

		frames.info = new Frame(
			frames.tree.x + frames.tree.width + 1,
			frames.container.y + 2,
			frames.container.width - frames.tree.width - 2,
			5,
			WHITE,
			frames.container
		);

		if (frames.parent.is_open) frames.container.open();

		state.tree = new Tree(frames.tree, 'Avatar collections');
		state.tree.colors.fg = WHITE;
		state.tree.colors.bg = BG_BLACK;
		state.tree.colors.lfg = WHITE;
		state.tree.colors.lbg = BG_BLUE;
		state.tree.colors.kfg = LIGHTCYAN;
		var first_sauce = null;
		directory(dir + '/*.bin').forEach(
			function (e, i) {
				if (e.search(EXCLUDE_FILES) > -1) return;
				const sauce = sauce_lib.read(e);
				if (!sauce) return;
				const ti = state.tree.addItem(
					sauce.title.length ? sauce.title : 'Unknown',
					function () {
						state.collection = e;
						state.cb = new CollectionBrowser(e, frames.parent);
						state.cb.open();
					}
				);
				ti.sauce = sauce;
				if (first_sauce === null) first_sauce = sauce;
			}
		);
		if (first_sauce !== null) display_collection_info(first_sauce);
		state.tree.open();

	}

	function get_avatar(i) {
		const f = new File(state.collection);
		f.open('rb');
		const contents = f.read();
		f.close();
		return contents.substr(i * avatar_lib.size, avatar_lib.size);
	}

	this.getcmd = function (cmd) {
		if (state.cb !== null) {
			var ret = state.cb.getcmd(cmd);
			if (typeof ret == 'number') {
				if (ret >= 0) {
					// set user avatar here
					var obj = {
						created : new Date(),
						updated : new Date(),
						data : base64_encode(get_avatar(ret))
					};
					avatar_lib.write_localuser(user.number, obj);
				}
				state.cb.close();
				state.cb = null;
				state.collection = null;
			}
		} else {
			if (cmd.toLowerCase() == 'q') {
				return false;
			} else if (state.tree.getcmd(cmd)) {
				frames.info.clear();
				display_collection_info(state.tree.currentItem.sauce);
			}
		}
		return true;
	}

	this.cycle = function () {
		if (state.cb !== null) state.cb.cycle();
	}

	this.close = function () {
		state.tree.close();
		frames.info.delete();
		frames.tree.delete();
		frames.container.delete();
	}

}

function AvatarEditor(parent_frame, on_exit) {

	const self = this;

	const frames = {
		parent : parent_frame,
		container : null
	};

	var editor = null;

	function _save() {
		const fn = system.temp_dir + format('avatar-%04d.bin', user.number);
		editor.save_bin(fn);
		const success = avatar_lib.import_file(user.number, fn, 0);
		file_remove(fn);
	}

	function save_and_exit() {
		_save();
		on_exit();
	}

    function flip_x() {
        editor.flip_x();
        console.ungetstr('\t');
    }

    function flip_y() {
        editor.flip_y();
        console.ungetstr('\t');
    }

	this.open = function () {
		frames.container = new Frame(1, 1, 1, 1, BG_BLUE|WHITE, frames.parent);
		frames.container.nest();
		frames.container.putmsg('Avatar Editor - Press TAB for menu');
		if (frames.parent.is_open) frames.container.open();
		editor = new ANSIEdit(
			{	x : frames.container.x,
				y : frames.container.y + 1,
				width : frames.container.width,
				height : frames.container.height - 1,
				canvas_width : 10,
				canvas_height : 6,
				parentFrame : frames.container
			}
		);
		editor.open();
        editor.menu.addItem('Flip X', flip_x);
        editor.menu.addItem('Flip Y', flip_y);
		editor.menu.addItem('Save', _save);
		editor.menu.addItem('Exit', on_exit);
		editor.menu.addItem('Save & Exit', save_and_exit);

		const user_avatar = avatar_lib.read_localuser(user.number);
		if (user_avatar) {
			const bin = base64_decode(user_avatar.data);
			var o = 0; // offset into 'bin'
			for (var yy = 0; yy < avatar_lib.defs.height; yy++) {
				for (var xx = 0; xx < avatar_lib.defs.width; xx++) {
					editor.putChar(
						{	x : xx,
							y : yy,
							ch : bin.substr(o, 1),
							attr : bin.substr(o + 1, 1).charCodeAt(0) || BG_BLACK
						}
					)
					o = o + 2;
				}
			}
		}
	}

	this.getcmd = function (cmd) {
		editor.getcmd(cmd);
	}

	this.cycle = function () {
		editor.cycle();
	}

	this.close = function () {
		editor.close();
		frames.container.delete();
	}

}

function MainMenu(parent_frame) {

	const user_fname = avatar_lib.localuser_fname(user.number);

	const frames = {
		parent : parent_frame,
		tree : null,
		info : null,
		user_avatar : null
	};

	const state = {
		ae : null,
		cl : null,
		tree : null,
		timer : new Timer(),
		utime : file_exists(user_fname) ? file_date(user_fname) : -1
	};

	function load_user_avatar() {
		var user_avatar = avatar_lib.read_localuser(user.number);
		if (user_avatar) {
			frames.user_avatar.clear();
			frames.user_avatar.drawBorder(BORDER);
			frames.user_avatar.blit(
				base64_decode(user_avatar.data),
				avatar_lib.defs.width, avatar_lib.defs.height, 1, 1,
				'My Avatar', WHITE
			);
		}
	}

	function test_user_file() {
		if (file_exists(user_fname)) {
			const utime = file_date(user_fname);
			if (utime > state.utime) {
				state.utime = utime;
				load_user_avatar();
			}
		}
	}

	this.open = function () {

		frames.tree = new Frame(
			frames.parent.x + 1,
			frames.parent.y + 3,
			Math.floor((frames.parent.width - 2) / 2),
			5,
			0,
			frames.parent
		);

		frames.info = new Frame(
			frames.tree.x + frames.tree.width + 1,
			frames.parent.y + 3,
			frames.parent.width - frames.tree.width - 4,
			5,
			WHITE,
			frames.parent
		);
		frames.info.word_wrap = true;
		frames.info.putmsg(
			'Your avatar will be displayed alongside messages ' +
			'you have posted and files you have uploaded.'
		);

		frames.user_avatar = new Frame(
			frames.parent.x + frames.parent.width - 1 - avatar_lib.defs.width - 2,
			frames.parent.y + frames.parent.height - 1 - avatar_lib.defs.height - 3,
			avatar_lib.defs.width + 2,
			avatar_lib.defs.height + 3,
			15,
			frames.parent
		);

		load_user_avatar();

		if (frames.parent.is_open) {
			frames.tree.open();
			frames.info.open();
			frames.user_avatar.open();
		}

		state.tree = new Tree(frames.tree, 'Avatar collections');
		state.tree.colors.fg = WHITE;
		state.tree.colors.bg = BG_BLACK;
		state.tree.colors.lfg = WHITE;
		state.tree.colors.lbg = BG_BLUE;
		state.tree.colors.kfg = LIGHTCYAN;
		state.tree.addItem(
			'Select an avatar', function () {
				state.tree.close();
				state.cl = new CollectionLister(avatar_lib.local_library(), frames.parent);
				state.cl.open();
			}
		);
		state.tree.addItem(
			'Upload an avatar', function () {
				console.clear(WHITE);
				console.putmsg('Your avatar must be 10 x 6 characters in size and saved in binary format.\r\n');
				if (upload_avatar()) {
					console.putmsg('Your avatar has been updated.');
				} else {
					console.putmsg('An error was encountered.  Your avatar has nto been updated.');
				}
				console.clear(LIGHTGRAY);
				frames.parent.invalidate();
			}
		);
		state.tree.addItem(
			'Download your avatar', function () {
				console.clear(WHITE);
				console.putmsg(
					'Avatars are ' +
					avatar_lib.defs.width + ' x ' + avatar_lib.defs.height +
					' characters in size, and are sent in binary format.\r\n'
				);
				download_avatar();
				console.clear(LIGHTGRAY);
				frames.parent.invalidate();
			}
		);
		state.tree.addItem(
			'Edit your avatar', function () {
				// placeholder { x y w h canvas_x canvas_y, canvas_w, canvas_h }
				state.ae = new AvatarEditor(
					frames.parent,
					function () {
						state.ae.close();
						state.ae = null;
					}
				);
				state.ae.open();
			}
		);
		state.tree.addItem(
			'Remove your avatar',  function () {
				console.clear(WHITE);
				if(remove_avatar())
					frames.user_avatar.clear();
				console.clear(LIGHTGRAY);
				frames.parent.invalidate();
			}
		);
		state.tree.open();

		state.timer.addEvent(2000, true, test_user_file);

	}

	this.getcmd = function (cmd) {
		if (state.ae !== null) {
			state.ae.getcmd(cmd);
		} else if (state.cl !== null) {
			if (!state.cl.getcmd(cmd)) {
				state.cl.close();
				delete state.cl;
				state.cl = null;
				state.tree.open();
			}
		} else if (cmd.toLowerCase() == 'q') {
			return false;
		} else {
			state.tree.getcmd(cmd);
		}
		return true;
	}

	this.cycle = function () {
		state.timer.cycle();
		if (state.ae !== null) {
			state.ae.cycle();
		} else if (state.cl !== null) {
			state.cl.cycle();
		}
	}

	this.close = function () {
		state.tree.close();
		frames.user_avatar.delete();
		frames.tree.delete();
	}

}

var sys_status, console_attr;

function init() {
	sys_status = bbs.sys_status;
	console_attr = console.attributes;
	bbs.sys_status|=SS_MOFF;
	console.clear(LIGHTGRAY);
}

function main() {

	const frame = new Frame(1, 1, console.screen_columns, console.screen_rows, 0);
	frame.transparent = true;
	frame.v_scroll = true;
	frame.drawBorder(BORDER, { x : 5, y : 1, attr : TITLE_COLOR, text: TITLE });
	frame.gotoxy(frame.x + frame.width - 20, frame.y + frame.height - 1);
	frame.putmsg(ascii(180) + QUIT_TEXT + ascii(195));
	frame.open();

	const menu = new MainMenu(frame);
	menu.open();
	while (true) {
		var i = console.inkey(K_NONE);
		if (i !== '' && !menu.getcmd(i)) break;
		menu.cycle();
		if (frame.cycle()) bury_cursor();
	}
	menu.close();
	frame.close();

}

function clean_up() {
	bbs.sys_status = sys_status;
	console.attributes = console_attr;
	console.clear();
}

init();
main();
clean_up();
