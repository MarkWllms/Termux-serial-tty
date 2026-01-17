/*
 *  @file ptyserial.cpp
 *  This example attaches a pseudo-tty to a given USB-UART device
 */
/* This file is part of USBUART Library. http://hutorny.in.ua/projects/usbuart
 *
 * Copyright (C) 2016 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * The USBUART Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License v2
 * as published by the Free Software Foundation;
 *
 * The USBUART Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the USBUART Library; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <cstdio>
#include <cstring>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <pty.h>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include "usbuart.h"

static bool terminated = false;

static void doexit(int signal) {
	terminated = true;
}

void show_err(int err) {
	fprintf(stderr,"err(%d)==%s\n", err, strerror(err));
}
using namespace usbuart;
using namespace std::chrono;

static inline bool is_good(int status) noexcept {
	return status == status_t::alles_gute;
}

static inline bool is_usable(int status) noexcept {
	return	status == (status_t::usb_dev_ok | status_t::read_pipe_ok)  ||
			status == (status_t::usb_dev_ok | status_t::write_pipe_ok) ||
			status ==  status_t::alles_gute;
}

int main(int argc, char** argv) {
	channel chnl = bad_channel;
	int master = -1;

	if( argc < 2 ) {
		fprintf(stderr,"Usage: termux-usb -e '%s  <command> [command arguments]' <usb device address>\n",argv[0]);
		return -1;
	}
	const char* usb_fd_arg_str = argv[argc-1];
	int usb_dev_fd = -1;
    try {
        usb_dev_fd = std::stoi(usb_fd_arg_str);
    } catch (const std::invalid_argument& ia) {
        fprintf(stderr,"Error: Invalid USB file descriptor argument %s. Not a number.\n",usb_fd_arg_str);
        return EXIT_FAILURE;
    } catch (const std::out_of_range& oor) {
        fprintf(stderr,"Error: USB file descriptor argument %s out of range.\n",usb_fd_arg_str);
        return EXIT_FAILURE;
    }

    if (usb_dev_fd < 0) { 
        fprintf(stderr,"Error: Invalid USB file descriptor value parsed: %d (must be non-negative).\n",usb_dev_fd);
        return EXIT_FAILURE;
    }


  char ptyname[256];
  pid_t pid = forkpty(&master, ptyname, NULL, NULL);
  if (pid < 0) {
	perror("Could not create pty");
	return EXIT_FAILURE;
  }
  if (pid == 0) {    	
//child process
    close(master);
    if (argc> 2) {
      argv[argc-1] = NULL;
      execvp(argv[1], const_cast<char *const *>(argv+1));
    }
    wait(NULL);
    exit(0);
  }
// Parent process
  fprintf(stderr, "PTY created: %s\n",ptyname);
  chnl.fd_read = master;
  chnl.fd_write = master;

  context::setloglevel(loglevel_t::warning);

  context ctx;
  int res, status = 0;
  res = ctx.attach(usb_dev_fd, 0, chnl, _115200_8N1n);
  if( res ) {
	fprintf(stderr,"Error %d attaching fd %x\n",
			-res, usb_dev_fd);
	kill(pid, SIGKILL);
	return -res;
  }


	signal(SIGINT, doexit);
	signal(SIGQUIT, doexit);

	int count_down = 10;
	int timeout = 0;

	while(!terminated && (res=ctx.loop(timeout)) >= -error_t::no_channel) {
		if( ! is_usable(status = ctx.status(chnl)) ) break;
		// end loop if child process exits
		if (waitpid(pid, NULL, WNOHANG) == pid) {
                   fprintf(stderr,"Process ended: closing.\n");
                   break;
	    	}
		if( res == -error_t::no_channel || ! is_good(status) ) {
			timeout = 100;
			fprintf(stderr,"Channel error:%d. Status=%d.\n",res,status);
			if( --count_down <= 0 ) break;
		}
	}
	kill(pid, SIGKILL);

	fprintf(stderr,"USB status %d res %d\n", status, res);
	ctx.close(chnl);
	res = ctx.loop(1000);
	if( res < -error_t::no_channel ) {
		fprintf(stderr,"Terminated with error %d\n",-res);
	} else
		res = 0;

	close(master);

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	return res;
}
