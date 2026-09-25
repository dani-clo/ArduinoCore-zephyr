/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

/* Monica provides TLS clients; omit the server handshake to fit the loader. */
#undef MBEDTLS_SSL_SRV_C
