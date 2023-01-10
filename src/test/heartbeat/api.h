/*
 * \brief  Abstraction for our REST API.
 * \author Johannes Schlatow
 * \date   2022-09-06
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _API_H_
#define _API_H_

/* Genode includes */
#include <util/string.h>
#include <base/log.h>

/* 3rd-party includes */
#include <json.hpp>

/* local includes */
#include "communication.hpp"

class Api
{
	public:

		using Server_uri     = Genode::String<32>;

	private:

		using json = nlohmann::json;

		std::string  server_uri;

		template <typename FUNC>
		long _performRequest(json & payload, FUNC && fn)
		{
			try {
				auto response = fn();

				long code = std::get<0>(response);
				payload   = std::get<1>(response);

				return code;
			} catch (std::runtime_error &e) {
				Genode::error("API request failed: ", e.what()); }

			return -1;
		}

	public:
		Api(Server_uri const & uri)
		: server_uri(uri.string())
		{
			initCurl();
		}

		bool getHeartbeat()
		{
			json result;

			std::string endpoint = server_uri + "/heartbeat";

			return (200 == _performRequest(result, [&] () { return getRequest(endpoint); } ));
		}

		bool postHeartbeat(unsigned id)
		{
			json result;
			json payload;
			payload["id"]  = id;

			std::string endpoint = server_uri + "/heartbeat";

			return (200 == _performRequest(result, [&] () { return postRequest(endpoint, payload); } ));
		}
};


#endif /* _API_H_ */
