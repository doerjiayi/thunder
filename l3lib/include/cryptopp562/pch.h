#ifndef CRYPTOPP_PCH_H
#define CRYPTOPP_PCH_H

#ifdef CRYPTOPP_GENERATE_X64_MASM

	#include "../../../l3lib/include/cryptopp562/cpu.h"

#else

	#include "../../../l3lib/include/cryptopp562/config.h"

	#ifdef USE_PRECOMPILED_HEADERS
		#include "../../../l3lib/include/cryptopp562/simple.h"
		#include "../../../l3lib/include/cryptopp562/secblock.h"
		#include "../../../l3lib/include/cryptopp562/misc.h"
		#include "../../../l3lib/include/cryptopp562/smartptr.h"
	#endif

#endif

#endif
