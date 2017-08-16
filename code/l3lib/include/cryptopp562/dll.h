#ifndef CRYPTOPP_DLL_H
#define CRYPTOPP_DLL_H

#if !defined(CRYPTOPP_IMPORTS) && !defined(CRYPTOPP_EXPORTS) && !defined(CRYPTOPP_DEFAULT_NO_DLL)
#ifdef CRYPTOPP_CONFIG_H
#error To use the DLL version of Crypto++, this file must be included before any other Crypto++ header files.
#endif
#define CRYPTOPP_IMPORTS
#endif

#include "../../../l3lib/include/cryptopp562/aes.h"
#include "../../../l3lib/include/cryptopp562/cbcmac.h"
#include "../../../l3lib/include/cryptopp562/ccm.h"
#include "../../../l3lib/include/cryptopp562/cmac.h"
#include "../../../l3lib/include/cryptopp562/channels.h"
#include "../../../l3lib/include/cryptopp562/des.h"
#include "../../../l3lib/include/cryptopp562/dh.h"
#include "../../../l3lib/include/cryptopp562/dsa.h"
#include "../../../l3lib/include/cryptopp562/ec2n.h"
#include "../../../l3lib/include/cryptopp562/eccrypto.h"
#include "../../../l3lib/include/cryptopp562/ecp.h"
#include "../../../l3lib/include/cryptopp562/files.h"
#include "../../../l3lib/include/cryptopp562/fips140.h"
#include "../../../l3lib/include/cryptopp562/gcm.h"
#include "../../../l3lib/include/cryptopp562/hex.h"
#include "../../../l3lib/include/cryptopp562/hmac.h"
#include "../../../l3lib/include/cryptopp562/modes.h"
#include "../../../l3lib/include/cryptopp562/mqueue.h"
#include "../../../l3lib/include/cryptopp562/nbtheory.h"
#include "../../../l3lib/include/cryptopp562/osrng.h"
#include "../../../l3lib/include/cryptopp562/pkcspad.h"
#include "../../../l3lib/include/cryptopp562/pssr.h"
#include "../../../l3lib/include/cryptopp562/randpool.h"
#include "../../../l3lib/include/cryptopp562/rsa.h"
#include "../../../l3lib/include/cryptopp562/rw.h"
#include "../../../l3lib/include/cryptopp562/sha.h"
#include "../../../l3lib/include/cryptopp562/skipjack.h"
#include "../../../l3lib/include/cryptopp562/trdlocal.h"

#ifdef CRYPTOPP_IMPORTS

#ifdef _DLL
// cause CRT DLL to be initialized before Crypto++ so that we can use malloc and free during DllMain()
#ifdef NDEBUG
#pragma comment(lib, "msvcrt")
#else
#pragma comment(lib, "msvcrtd")
#endif
#endif

#pragma comment(lib, "cryptopp")

#endif		// #ifdef CRYPTOPP_IMPORTS

#include <new>	// for new_handler

NAMESPACE_BEGIN(CryptoPP)

#if !(defined(_MSC_VER) && (_MSC_VER < 1300))
using std::new_handler;
#endif

typedef void * (CRYPTOPP_API * PNew)(size_t);
typedef void (CRYPTOPP_API * PDelete)(void *);
typedef void (CRYPTOPP_API * PGetNewAndDelete)(PNew &, PDelete &);
typedef new_handler (CRYPTOPP_API * PSetNewHandler)(new_handler);
typedef void (CRYPTOPP_API * PSetNewAndDelete)(PNew, PDelete, PSetNewHandler);

NAMESPACE_END

#endif
