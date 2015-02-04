//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>

#include "Mythen3Net.h"
#include "lima/ThreadUtils.h"
#include "lima/Exceptions.h"
#include "lima/Debug.h"

using namespace std;
using namespace lima;
using namespace lima::Mythen3;


Mythen3Net::Mythen3Net() {
	DEB_CONSTRUCTOR();
	// Ignore the sigpipe we get we try to send quit to
	// dead server in disconnect, just use error codes
	struct sigaction pipe_act;
	sigemptyset(&pipe_act.sa_mask);
	pipe_act.sa_flags = 0;
	pipe_act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &pipe_act, 0);
	m_connected = false;
	m_sock = -1;
}

Mythen3Net::~Mythen3Net() {
	DEB_DESTRUCTOR();
}

void Mythen3Net::connectToServer(const string hostname, int port) {
	DEB_MEMBER_FUNCT();
	struct hostent *host;
	struct protoent *protocol;
	int opt;

	if (m_connected) {
		THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Already connected to server";
	}
	if ((host = gethostbyname(hostname.c_str())) == 0) {
		endhostent();
		THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Can't get gethostbyname";
	}
	if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		endhostent();
		THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Can't create socket";
	}
	m_remote_addr.sin_family = host->h_addrtype;
	m_remote_addr.sin_port = htons (port);
	size_t len = host->h_length;
	memcpy(&m_remote_addr.sin_addr.s_addr, host->h_addr, len);
	endhostent();
	if (connect(m_sock, (struct sockaddr *) &m_remote_addr, sizeof(struct sockaddr_in)) == -1) {
		close(m_sock);
		THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Connection to server refused. Is the server running?";
	}
	protocol = getprotobyname("tcp");
	if (protocol == 0) {
		THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Can't get protocol TCP";
	} else {
		opt = 1;
		if (setsockopt(m_sock, protocol->p_proto, TCP_NODELAY, (char *) &opt, 4) < 0) {
			THROW_HW_ERROR(Error) << "Mythen3Net::connectToServer(): Can't set socket options";
		}
	}
	endprotoent();
	m_connected = true;
}

void Mythen3Net::disconnectFromServer() {
	DEB_MEMBER_FUNCT();
	if (m_connected) {
		shutdown(m_sock, 2);
		close(m_sock);
		m_connected = false;
	}
}

void Mythen3Net::sendCmd(string cmd, uint8_t* recvBuf, int len) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Mythen3Net::sendCmd(" << cmd << ")";
	AutoMutex aLock(m_cond.mutex());

	if (!m_connected) {
		THROW_HW_ERROR(Error) << "Mythen3Net::sendCmd(): not connected";
	}
	if (write(m_sock, cmd.c_str(), strlen(cmd.c_str())) <= 0) {
		THROW_HW_ERROR(Error) << "Mythen3Net::sendCmd(): write to socket error";
	}
	uint8_t* buffer;
	buffer = recvBuf;
	int total = 0;
	int count;
	while (len > 0 && count != 4) {
		if ((count = read(m_sock, buffer, len)) < 0) {
			THROW_HW_ERROR(Error) << "Mythen3Net::sendCmd(): read from socket error";
		}
		buffer += count;
		len -= count;
		total += count;
		DEB_TRACE() << "Mythen3Net::sendCmd(): read " << count << " bytes, total " << total;
	}
	DEB_TRACE() << "Mythen3Net::sendCmd(): total bytes read" << total;
}
