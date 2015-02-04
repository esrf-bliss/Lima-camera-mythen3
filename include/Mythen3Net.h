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

#ifndef MYTHEN3NET_H_
#define MYTHEN3NET_H_

#include <netinet/in.h>
#include "lima/Debug.h"

using namespace std;

namespace lima {
namespace Mythen3 {

class Mythen3Net {
DEB_CLASS_NAMESPC(DebModCamera, "Mythen3Net", "Mythen3");

public:
	Mythen3Net();
	~Mythen3Net();

	void sendCmd(string cmd, uint8_t* value, int len);
	void connectToServer (const string hostname, int port);
	void disconnectFromServer();

private:
	mutable Cond m_cond;
	bool m_connected;					// true if connected
	int m_sock;							// socket for commands */
	struct sockaddr_in m_remote_addr;	// address of remote server */
};

} // namespace Mythen3
} // namespace lima

#endif /* MYTHEN3NET_H_ */
