#ifndef CROW_INTEGRATION_H__
#define CROW_INTEGRATION_H__

#include "crow.h"
#include <string>
#include <vector>

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() const {return name;}
};

struct ComponentBase : public Named {
	using Named::Named;
	virtual crow::json::wvalue getStatus()=0;
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix);
	virtual std::string generateLayout()=0;
	virtual std::string generateUpdateJS(std::vector<std::string> parent);
};

template<class First, class...Rest>
class ComponentTuple : public ComponentTuple<Rest...> {
	First first;
protected:
	virtual std::string generateChildLayout() override {
		std::string ret = first.generateLayout();
		ret += ComponentTuple<Rest...>::generateChildLayout();
		return ret;
	}
public:
	template<class CFirst, class...CRest>
	ComponentTuple(std::string name, CFirst cf, CRest... cr) : ComponentTuple<Rest...>(name, cr...), first(cf) {}
	template<int N>
	auto& get() {
		if constexpr (N == 0)
			return first;
		else
			return ComponentTuple<Rest...>::template get<N-1>();	
	}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret = ComponentTuple<Rest...>::getStatus();
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentTuple<Rest...>::registerEndpoints(app, endpointPrefix);
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent) override {
		std::string ret = ComponentTuple<Rest...>::generateUpdateJS(parent);
		parent.push_back(this->getName());
		ret += first.generateUpdateJS(parent);
		return ret;
	}
};

template<class First>
class ComponentTuple<First> : public ComponentBase {
	First first;
protected:
	virtual std::string generateChildLayout() {
		return first.generateLayout();
	}
public:
	template<class CFirst>
	ComponentTuple(std::string name, CFirst cf) : ComponentBase(name), first(cf) {}
	template<int N>
	auto& get() {
		static_assert(N == 0);
		return first;
	}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret;
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentBase::registerEndpoints(app, endpointPrefix);
	}
	virtual std::string generateLayout() override {
		std::string ret = "<fieldset id=\"" + this->getName() + "\">\n";
		ret += "<legend>" + this->getName() + "</legend>\n";
		ret += this->generateChildLayout();
		ret += "</fieldset>\n";
		return ret;
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent) override {
		parent.push_back(this->getName());
		return first.generateUpdateJS(parent);
	}
};

template<class T>
struct ReadableValue : public ComponentBase {
	using ComponentBase::ComponentBase;
	virtual T get()=0;
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret;
		ret = std::to_string(get());
		return ret;
	}
	virtual std::string generateLayout() override {
		return "<div id=\"" + this->getName() + "\"></div>\n";
	}
	virtual std::string generateSelector(std::vector<std::string> parent)
	{
		std::string selector;
		for(auto p : parent)
			selector += "#" + p + " > ";
		selector += "#" + this->getName();
		return selector;
	}
	virtual std::string generateEndpoint(std::vector<std::string> parent)
	{
		std::string endpoint;
		for(auto p : parent)
			endpoint += "/" + p;
		endpoint += "/" + this->getName();
		return endpoint;
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent)
	{
		std::string selector = generateSelector(parent);
		std::string endpoint = generateEndpoint(parent);
		return "registerText('" + endpoint + "', '" + selector + "');\n";
	}
};



#endif

