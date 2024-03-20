#ifndef CROW_INTEGRATION_H__
#define CROW_INTEGRATION_H__

namespace crow {
	template<class...> class Crow;
	namespace json {
		class wvalue;
	}
	class request;
}
#include <string>
#include <memory>
#include <functional>

void crow_mustache_set_base(std::string);

class JSONWrapper {
	struct Deleter {
		void operator()(crow::json::wvalue*);
	};
	std::unique_ptr<crow::json::wvalue, Deleter> impl;
public:
	JSONWrapper();
	JSONWrapper(const JSONWrapper&);
	void set(std::string value);
	void set(unsigned int, JSONWrapper);
	void set(std::string id, std::string value);
	void set(std::string id, JSONWrapper value);
	std::string dump();
	crow::json::wvalue& to_wvalue();
	const crow::json::wvalue& to_wvalue() const;
};

class CrowRequest {
	const crow::request* req;
public:
	CrowRequest(const crow::request&);
	std::string url_params_get(std::string) const;
};

std::string crow_mustache_load(std::string, JSONWrapper);


class SimpleApp {
	struct Deleter {
		void operator()(crow::Crow<>*);
	};
	std::unique_ptr<crow::Crow<>, Deleter> impl;
public:
	SimpleApp();
	void route_dynamic(std::string endPoint, std::function<std::string()> exec);
	void route_dynamic(std::string endPoint, std::function<std::string(int)> exec);
	void route_dynamic(std::string endPoint, std::function<std::string(const CrowRequest&)> exec);

	enum LogLevels {Debug};
	void loglevel(LogLevels);

	void run_on_port(unsigned);
	void stop();
};

#endif

