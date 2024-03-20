#ifndef COMPONENTS_H__
#define COMPONENTS_H__

#include <string>
#include <vector>
#include "crow_integration.h"

std::string generateSelector(std::string name, std::vector<std::string> parent);
std::string generateEndpoint(std::string name, std::vector<std::string> parent);

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() const {return name;}
};

struct ComponentBase : public Named {
	using Named::Named;
	virtual JSONWrapper getStatus()=0;
	virtual void registerEndpoints(SimpleApp& app, std::string endpointPrefix)
	{
		app.route_dynamic(endpointPrefix+"/"+this->getName()+"/status",
		[&](){
			return this->getStatus().dump();
		});
	}
	virtual std::string generateLayout()=0;
	virtual std::string generateUpdateJS(std::vector<std::string> parent);
	virtual ~ComponentBase()=default;
};

template<class First, class...Rest>
class ComponentTuple : public ComponentTuple<Rest...> {
	First first;
protected:
	std::string generateChildLayout() override {
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
	JSONWrapper getStatus() override {
		JSONWrapper ret = ComponentTuple<Rest...>::getStatus();
		ret.set(first.getName(), first.getStatus());
		return ret;
	}
	void registerEndpoints(SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentTuple<Rest...>::registerEndpoints(app, endpointPrefix);
	}
	std::string generateUpdateJS(std::vector<std::string> parent) override {
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
	JSONWrapper getStatus() override {
		JSONWrapper ret;
		ret.set(first.getName(), first.getStatus());
		return ret;
	}
	void registerEndpoints(SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentBase::registerEndpoints(app, endpointPrefix);
	}
	std::string generateLayout() override {
		std::string ret = "<fieldset id=\"" + this->getName() + "\">\n";
		ret += "<legend>" + this->getName() + "</legend>\n";
		ret += this->generateChildLayout();
		ret += "</fieldset>\n";
		return ret;
	}
	std::string generateUpdateJS(std::vector<std::string> parent) override {
		parent.push_back(this->getName());
		return first.generateUpdateJS(parent);
	}
};

template<class T>
struct ReadableValue : public ComponentBase {
	using ComponentBase::ComponentBase;
	virtual T get()=0;
	JSONWrapper getStatus() override {
		JSONWrapper ret;
		ret.set(std::to_string(get()));
		return ret;
	}
	std::string generateLayout() override {
		return "<div id=\"" + this->getName() + "\"></div>\n";
	}
	std::string generateUpdateJS(std::vector<std::string> parent) override
	{
		std::string selector = generateSelector(this->getName(), parent);
		std::string endpoint = generateEndpoint(this->getName(), parent);
		return "registerText('" + endpoint + "', '" + selector + "');\n";
	}
};


#endif

