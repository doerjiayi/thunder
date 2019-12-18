/*******************************************************************************
 * Project:  CenterServer
 * @file    Comm.hpp
 * @brief    
 * @author   cjy
 * @date:    2015年9月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_NODE_COMM_HPP_
#define SRC_NODE_COMM_HPP_

#include <unordered_set>
#include <string>
#include "util/encrypt/base64.h"
#include "util/http/http_parser.h"
#include "util/json/CJsonObject.hpp"
#include "util/UnixTime.hpp"
#include "dbi/Dbi.hpp"
#include "log4cplus/loggingmacros.h"

#include "NetDefine.hpp"
#include "labor/Labor.hpp"
#include "coor.pb.h"
#include "protocol/oss_sys.pb.h"
#include "CoorError.hpp"
#include "ProtoError.h"
#include "NetError.hpp"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"
#include "cmd/CW.hpp"


DEFINE_INT


#endif /* SRC_NODEREG_COMM_HPP_ */
