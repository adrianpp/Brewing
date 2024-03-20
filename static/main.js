var miss_count = 0;
function countedJSON(endpoint, func) {
	if( miss_count < 5 )
	{
		$("#downStatus").html("");
		$.getJSON(endpoint, func)
			.fail(function(){miss_count++;})
			.done(function(){miss_count=0;});
	}
	else if ( !$("#downStatus").dialog("isOpen") )
	{
		$("#downStatus").html("Server appears down!");
		$("#downStatus").css("color", "red");
		$("#downStatus").dialog("open");
	}
}
function updateText(endpoint, selector) {
	countedJSON(endpoint+"/status", function(data) {
		const path = endpoint.split("/");
		$(selector).html(path[path.length-1] + " : " + data);
	});
}
function updateButton(endpoint, selector) {
	countedJSON(endpoint+"/status", function(data) {
		if( !$.isNumeric(data) || data == 0 )
			$(selector).css('color','red');
		else
			$(selector).css('color','green');
	});
}
function updateTargetValue(endpoint, selector) {
	countedJSON(endpoint+"/status", function(data) {
		$(selector).prop("value", data);
		const path = endpoint.split("/");
		$(selector+"_label").html(path[path.length-1] + "_target : " + data);
		$(selector+"_label").css('color','black');
	});
}
function updateGraph(chart, endpoint, selectorText, selectorGraph) {
	countedJSON(endpoint+"/status/"+(chart.data.labels.length-1), function(data) {
		for (e in data)
		{
			if( !data[e] )
				continue;
			chart.data.labels.push(data[e].x);
			chart.data.datasets[0].data.push(data[e].y);
			chart.update();
			const path = endpoint.split("/");
			$(selectorText).html(path[path.length-1] + " : " + data[e].y);
		}
	});
}
function registerText(endpoint, selector) {
	setInterval(function(){ updateText(endpoint, selector); }, 1000);
}
function registerButton(endpoint, selector) {
	var updateFunc = function(){updateButton(endpoint, selector);};
	$(selector).click(function(){$.get(endpoint + "/toggle"); setTimeout(updateFunc,100);});
	setInterval(updateFunc, 1000);
}
function registerTargetValue(endpoint, selector, MinValue, MaxValue) {
	$(selector).prop('min', MinValue);
	$(selector).prop('max', MaxValue);
	var updateFunc = function(){updateTargetValue(endpoint,selector);};
	var setFunc = function(){$.get(endpoint+"/set_target?value="+$(selector).prop('value')); setTimeout(updateFunc,100);};
	$(selector).change(setFunc);
	$(selector).css('width', '100%');
	const path = endpoint.split("/");
	var changedFunc = function(){$(selector+"_label").css('color','red').html(path[path.length-1]+"_target : " + $(selector).prop('value'));};
	$(selector).on('input',changedFunc);
	setInterval(updateFunc, 1000);
}
function registerGraph(endpoint, selectorText, selectorGraph) {
	var chart = new Chart(document.querySelector(selectorGraph),
		{
			type: "line",
			data: {
				labels: [0],
				datasets: [{
					label: selectorText,
					data: [0]
				}]
			},
			options: {
			}
		});
	setInterval(function(){ updateGraph(chart, endpoint, selectorText, selectorGraph); }, 2000);
}

