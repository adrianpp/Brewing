#include "web_components.h"

std::string generateSelector(std::string name, std::vector<std::string> parent)
{
	std::string selector;
	for(auto p : parent)
		selector += "#" + p + " > ";
	selector += "#" + name;
	return selector;
}

std::string generateEndpoint(std::string name, std::vector<std::string> parent)
{
	std::string endpoint;
	for(auto p : parent)
		endpoint += "/" + p;
	endpoint += "/" + name;
	return endpoint;
}

std::string generateLayout(TempSensor& t)
{
	return "<div id=\"" + t.getName() + "\"></div>\n"
		"<canvas id=\"" + t.getName() + "_graph\" style=\"width:100%;max-width:700px\"></canvas>\n";
}
void registerEndpoints(TempSensor& t, SimpleApp& app, std::string endpointPrefix)
{
	app.route_dynamic(endpointPrefix+"/"+t.getName()+"/status/<int>",
			[&](std::size_t last){
			JSONWrapper ret;
			auto hist = t.getHistory();
			// dont send too many elements at the same time
			auto max_history = std::min(hist.size(), last+120);

			for(unsigned int i = last; i < max_history; ++i)
			{
				JSONWrapper v;
				v.set("x", std::to_string(hist[i].first));
				v.set("y", std::to_string(hist[i].second));
				ret.set(i, v);
			}
			return ret.dump();
		});
}
std::string generateUpdateJS(TempSensor& t, std::vector<std::string> parent)
{
	std::string selector = generateSelector(t.getName(), parent);
	std::string endpoint = generateEndpoint(t.getName(), parent);
	return "registerGraph('" + endpoint + "', '" + selector + "', '" + selector + "_graph');\n";
}

std::string generateLayout(Button& b)
{
	return "<button id=\"" + b.getName() + "\">" + b.getName() + "</button>\n";
}

void registerEndpoints(Button& b, SimpleApp& app, std::string endpointPrefix)
{
	registerEndpoints(static_cast<ReadableValue<int>&>(b), app, endpointPrefix);
	app.route_dynamic(endpointPrefix+"/"+b.getName()+"/toggle",
			[&](){
				b.set(!b.get());
				return std::string{};
			});
}

std::string generateUpdateJS(Button& b, std::vector<std::string> parent)
{
	std::string selector = generateSelector(b.getName(), parent);
	std::string endpoint = generateEndpoint(b.getName(), parent);
	return "registerButton('" + endpoint + "', '" + selector + "');\n";
}

template<class T>
std::string generateLayout(ReadableValue<T>& r)
{
	return "<div id=\"" + r.getName() + "\"></div>\n";
}
template<class T>
void registerEndpoints(ReadableValue<T>& r, SimpleApp& app, std::string endpointPrefix)
{
	app.route_dynamic(endpointPrefix+"/"+r.getName()+"/status",
			[&](){
				return std::to_string(r.get());
			});
}
template<class T>
std::string generateUpdateJS(ReadableValue<T>& r, std::vector<std::string> parent)
{
	std::string selector = generateSelector(r.getName(), parent);
	std::string endpoint = generateEndpoint(r.getName(), parent);
	return "registerText('" + endpoint + "', '" + selector + "');\n";
}

template<class T>
std::string generateLayout(TargetValue<T>& t)
{
	std::string ret;
	ret += "<div id=\"" + t.getName() + "_label\"></div>\n";
	ret += "<input id=\"" + t.getName() + "\" type='range'/>\n";
	return ret;
}


template<class T>
void registerEndpoints(TargetValue<T>& t, SimpleApp& app, std::string endpointPrefix)
{
	registerEndpoints(static_cast<WriteableValue<T>&>(t), app, endpointPrefix);
	app.route_dynamic(endpointPrefix+"/"+t.getName()+"/set_target",
			[&](const CrowRequest& req){
				std::stringstream ss;
				ss << req.url_params_get("value");
				T v;
				ss >> v;
				t.set(v);
				return std::string{};
			});
}

template<class T>
std::string generateUpdateJS(TargetValue<T>& t, std::vector<std::string> parent)
{
	std::string selector = generateSelector(t.getName(), parent);
	std::string endpoint = generateEndpoint(t.getName(), parent);
	return "registerTargetValue(\'" + endpoint + "\', \'" + selector + "\'," + std::to_string(t.getMin()) + ", " + std::to_string(t.getMax()) + ");\n";
}

//explicit instantiate
#define EXPLICIT_INSTANTIATE(PARAM, TEMPL_TYPE) \
template std::string generateLayout<TEMPL_TYPE>(PARAM<TEMPL_TYPE>&); \
template void registerEndpoints<TEMPL_TYPE>(PARAM<TEMPL_TYPE>&,SimpleApp&,std::string); \
template std::string generateUpdateJS<TEMPL_TYPE>(PARAM<TEMPL_TYPE>&,std::vector<std::string>);
EXPLICIT_INSTANTIATE(ReadableValue, int)
EXPLICIT_INSTANTIATE(ReadableValue, double)
EXPLICIT_INSTANTIATE(TargetValue, int)
EXPLICIT_INSTANTIATE(TargetValue, double)


