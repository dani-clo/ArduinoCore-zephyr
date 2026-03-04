/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "SocketWrapper.h"
#include "api/Client.h"
#include "unistd.h"
#include "zephyr/sys/printk.h"
#include "ZephyrClient.h"

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
class ZephyrSSLClient : public ZephyrClient {

public:
	int connect(const char *host, uint16_t port) override {
		return connectSSL(host, port, nullptr);
	}

	int connect(const char *host, uint16_t port, const char *cert) {
		return connectSSL(host, port, cert);
	}

	// Connect with separate IP and hostname for SNI (useful when DNS doesn't work)
	int connect(const char *ip_address, uint16_t port, const char *sni_hostname, const char *cert) {
		return connectSSL(ip_address, port, cert, sni_hostname);
	}
};
#endif
