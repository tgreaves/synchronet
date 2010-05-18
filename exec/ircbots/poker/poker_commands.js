this.Bot_Commands["DEAL"] = new Bot_Command(0,false,false);
this.Bot_Commands["DEAL"].command = function (target,onick,ouh,srv,lvl,cmd) {
	cmd.shift();
	if (!poker_games[target]) {
		srv.o(target, onick + " just started a new poker hand.  To get "
			+ "in on the action, type '" + get_cmd_prefix() + "DEAL'");
		srv.o(target, "Seats will remain open for the next 60 seconds, "
			+ "or until someone types '" + get_cmd_prefix() + "GO'");
		poker_games[target] = new Poker_Game();
		poker_games[target].users[onick.toUpperCase()]=new Poker_Player();
	} else	if (poker_games[target].users[onick.toUpperCase()]) {
		srv.o(target, onick + ", you're already in the hand. Relax, don't do it.");
	} else {
		poker_games[target].users[onick.toUpperCase()]=new Poker_Player();
		srv.o(target, onick + " has been dealt in for this hand.");
	}
	return;
}

this.Bot_Commands["GO"] = new Bot_Command(0,false,false);
this.Bot_Commands["GO"].command = function (target,onick,ouh,srv,lvl,cmd) {
	cmd.shift();
	if (!poker_games[target]) {
		srv.o(target, "No poker game to 'GO' with. Type '" + get_cmd_prefix() + "DEAL' to "
			+ "start a new one.");
		return;
	} else if(poker_games[target].round>=0) {
		srv.o(target, "This hand has already started.");
		return;
	}
	if(true_array_len(poker_games[target].users)==1) {
		srv.o(target, "At least two players are necessary to start the game.");
		return;
	}
	poker_games[target].round = 0;
	poker_init_hand(target);
	poker_deal_hole_cards(target,srv);
	poker_prompt_player(target,srv);
	return;
}

this.Bot_Commands["FOLD"] = new Bot_Command(0,false,false);
this.Bot_Commands["FOLD"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if (!poker_games[target]) {
		srv.o(target, onick + ", there is no game in progress.");
		return;
	} else	if (!poker_games[target].users[onick.toUpperCase()]) {
		srv.o(target, onick + ", you aren't playing this game.");
		return;
	} else if(poker_games[target].round<0) {
		srv.o(target, onick + ", the game hasn't started yet.");
		return;
	}

	if(!poker_verify_game_status(target,srv,onick)) return;
	
	delete poker_games[target].users[onick.toUpperCase()];
	srv.o(target, onick + " folded their hand.");
	poker_next_turn(target,srv);
	return;
}

this.Bot_Commands["CHECK"] = new Bot_Command(0,false,false);
this.Bot_Commands["CHECK"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if(!poker_verify_game_status(target,srv,onick)) return;
	srv.o(target,onick + " checks.");
	poker_next_turn(target,srv);
	return;
}

this.Bot_Commands["BET"] = new Bot_Command(0,false,false);
this.Bot_Commands["BET"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if(!poker_verify_game_status(target,srv,onick)) return;
	if(!cmd[1]) {
		srv.writeout("NOTICE " + p + " :" + "You must specify an amount to bet!");
		return;
	}
	var poker=poker_games[target];
	if(cmd[1]>poker.users[onick.toUpperCase()].money) {
		srv.writeout("NOTICE " + p + " :" + "You don't have that much money!");
		srv.writeout("NOTICE " + p + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
		return;
	}
	if(cmd[1]<poker.current_bet) {
		srv.writeout("NOTICE " + p + " :" + "You must meet the minimum bet!");
		srv.writeout("NOTICE " + p + " :" + "Minimum bet: $" + poker.current_bet);
		return;
	}
	srv.o(target,onick + " bets $" + cmd[1]);
	poker.users[onick.toUpperCase()].money-=Number(cmd[1]);
	poker.users[onick.toUpperCase()].bet+=Number(cmd[1]);
	poker.current_bet=Number(cmd[1]);
	srv.writeout("NOTICE " + onick + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
	poker_next_turn(target,srv);
	return;
}

this.Bot_Commands["CALL"] = new Bot_Command(0,false,false);
this.Bot_Commands["CALL"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if(!poker_verify_game_status(target,srv,onick)) return;
	var poker=poker_games[target];
	if(poker.current_bet>poker.users[onick.toUpperCase()].money) {
		srv.writeout("NOTICE " + onick + " :" + "You don't have enough to call!");
		srv.writeout("NOTICE " + onick + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
		return;
	}
	srv.o(target,onick + " calls the bet: $" + poker.current_bet);
	poker.users[onick.toUpperCase()].money-=poker.current_bet;
	poker.users[onick.toUpperCase()].bet+=poker.current_bet;
	srv.writeout("NOTICE " + onick + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
	poker_next_turn(target,srv);
	return;
}

this.Bot_Commands["RAISE"] = new Bot_Command(0,false,false);
this.Bot_Commands["RAISE"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if(!poker_verify_game_status(target,srv,onick)) return;
	if(!cmd[1]) {
		srv.writeout("NOTICE " + onick + " :" + "You must specify an amount to raise!");
		return;
	}
	var poker=poker_games[target];
	var bet=Number(cmd[1]);
	if(poker.current_bet+bet>poker.users[onick.toUpperCase()].money) {
		srv.writeout("NOTICE " + onick + " :" + "You don't have that much money!");
		srv.writeout("NOTICE " + onick + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
		return;
	}
	srv.o(target,onick + " raises the bet to $" + poker.current_bet+bet);
	poker.users[onick.toUpperCase()].money-=bet;
	poker.users[onick.toUpperCase()].bet+=bet;
	poker.current_bet+=bet;
	srv.writeout("NOTICE " + onick + " :" + "Balance: $" + poker.users[onick.toUpperCase()].money);
	poker_next_turn(target,srv);
	return;
}

this.Bot_Commands["STATUS"] = new Bot_Command(0,false,false);
this.Bot_Commands["STATUS"].command = function (target,onick,ouh,srv,lvl,cmd) {
	return;
}

this.Bot_Commands["LIST"] = new Bot_Command(0,false,false);
this.Bot_Commands["LIST"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if(!poker_games[target]) {
		srv.o(target,"There is no active game.");
		return;
	}
	var list="";
	for(u in poker_games[target].users) {
		list+=" "+u;
	}
	srv.o(target,"Poker players:" + list);
	return;
}

this.Bot_Commands["SHOW"] = new Bot_Command(0,false,false);
this.Bot_Commands["SHOW"].command = function (target,onick,ouh,srv,lvl,cmd) {
	if (!poker_games[target]) {
		srv.o(target,"There is no active game.");
		return;
	}
	if (!poker_games[target].users[onick.toUpperCase()]) {
		srv.o(target, onick + ", you aren't playing this game.");
		return;
	}
	if(poker_games[target].round<0) {
		srv.o(target, onick + ", the game hasn't started yet.");
		return;
	}

	srv.o(target, onick + " shows: "
		+ poker_show_card(poker_games[target].users[onick.toUpperCase()].cards[0])
		+ poker_show_card(poker_games[target].users[onick.toUpperCase()].cards[1])
	);
}

