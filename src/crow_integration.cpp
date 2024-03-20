#include "crow_integration.h"
#define CROW_MAIN
#include "crow.h"

void crow_mustache_set_base(std::string base)
{
	crow::mustache::set_base(base);
}

void JSONWrapper::Deleter::operator()(crow::json::wvalue* wv)
{
	delete wv;
}
JSONWrapper::JSONWrapper() : impl(new crow::json::wvalue)
{
}
JSONWrapper::JSONWrapper(const JSONWrapper& rhs) : impl(new crow::json::wvalue(*rhs.impl))
{
}
void JSONWrapper::set(std::string value)
{
	*impl = value;
}
void JSONWrapper::set(unsigned int index, JSONWrapper value)
{
	(*impl)[index] = crow::json::wvalue(*value.impl);
}
void JSONWrapper::set(std::string id, std::string value)
{
	(*impl)[id] = value;
}
void JSONWrapper::set(std::string id, JSONWrapper value)
{
	(*impl)[id] = crow::json::wvalue(*value.impl);
}
std::string JSONWrapper::dump()
{
	return impl->dump();
}
crow::json::wvalue& JSONWrapper::to_wvalue()
{
	return *impl;
}
const crow::json::wvalue& JSONWrapper::to_wvalue() const
{
	return *impl;
}

CrowRequest::CrowRequest(const crow::request& req) : req(&req)
{
}
std::string CrowRequest::url_params_get(std::string param) const
{
	return req->url_params.get(param);
}

std::string crow_mustache_load(std::string file, JSONWrapper ctx)
{
	return crow::mustache::load(file).render(ctx.to_wvalue());
}


void SimpleApp::Deleter::operator()(crow::Crow<>* app)
{
	delete app;
}
SimpleApp::SimpleApp() : impl(new crow::SimpleApp) 
{
}
void SimpleApp::route_dynamic(std::string endPoint, std::function<std::string()> exec)
{
	impl->route_dynamic(std::move(endPoint))(exec);
}
void SimpleApp::route_dynamic(std::string endPoint, std::function<std::string(int)> exec)
{
	impl->route_dynamic(std::move(endPoint))(exec);
}
void SimpleApp::route_dynamic(std::string endPoint, std::function<std::string(const CrowRequest&)> exec)
{
	impl->route_dynamic(std::move(endPoint))([=](const crow::request& req) {
			return exec(CrowRequest(req));
		});
}

void SimpleApp::loglevel(SimpleApp::LogLevels level)
{
	if( level == SimpleApp::Debug )
		impl->loglevel(crow::LogLevel::Debug);
}

void SimpleApp::run_on_port(unsigned p)
{
	impl->port(p).run();
}
void SimpleApp::stop()
{
	impl->stop();
}

