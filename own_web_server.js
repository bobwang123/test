var express = require('express');
var app = express();
var http = require('http');
var url = require('url');
var util = require('util');
var fs = require("fs")
const child_process = require('child_process');

var PORT = 8139

var get_client_ip = function(req) {
    var ip = req.headers['x-forwarded-for'] ||
    req.ip ||
    req.connection.remoteAddress ||
    req.socket.remoteAddress ||
    req.connection.socket.remoteAddress || '';
    if(ip.split(',').length>0){
            ip = ip.split(',')[0]
    }
    return ip;
};

var server = http.createServer(function (request, response) {
    var myDate = new Date();
    console.log("- Time = " + myDate.toLocaleString( ))
    var params = url.parse(request.url, true).query;
    for (var name in params) {
	    console.log("- Request params[" + name + "]=" + params[name]);
    }
    var client_ip = get_client_ip(request)
    if (!params["simulateCode"]) {
	console.log("- Ignore illegal request from IP=" + client_ip);
	console.log("- Refused to respond the illegal request!")
	return ;
    } else {
	console.log("- Get a legal request from IP=" + client_ip);
    }
    response.writeHead(200, {'Content-Type': 'text/plain:charset=utf-8'});
    response.write("### Arguments \n")
    response.write("### Start optimization ...\n");
    console.log("Start optimization ...");
    for (var name in params) {
	    console.log("Request params[" + name + "]=" + params[name]);
	    response.write(name + "=" + params[name]);
	    response.write("\n");
    }
    var uniq_rundir = "run_" + params["simulateCode"] + "/" + Date.now() + "_" + Math.ceil(Math.random()*1e10) + "/";
    // "curl http://139.198.5.125/OwnLogistics/api/own/conf?mark=test0927 -o conf.json; "
    var cmd =
	"export rundir=" + uniq_rundir + "; OUTDIR=$rundir; mkdir -p $OUTDIR; cd $OUTDIR; "
	+ "/usr/bin/time python -O ../.. "
	+ "--cost='http://139.198.5.125/OwnLogistics/api/own/city/cost" + "' "
	+ "--order-occur-prob='http://139.198.5.125/OwnLogistics/api/own/city/statistics?mark=" + params["simulateCode"] + "' "
	+ "--order-file='http://139.198.5.125/OwnLogistics/api/own/orders?mark=" + params["simulateCode"] + "' "
	+ "--vehicle-file='http://139.198.5.125/OwnLogistics/api/own/vehicles?mark=" + params["simulateCode"] + "' "
	+ "--plan-upload-api='http://139.198.5.125/OwnLogistics/api/own/routes/result?mark=" + params["simulateCode"] + "' "
	+ "> stdout.log; echo RUNDIR=$rundir ";
    //var cmd = "sleep 140; echo sleep 140 done > not_close_api.log"; // DEBUG
    var workerProcess = child_process.exec(cmd,
	function (error, stdout, stderr) {
	    if (error) {
		console.log(error.stack);
		console.log('Error code: '+ error.code);
		console.log('Signal received: '+ error.signal);
	    }
	    console.log('stdout: ' + stdout);
	    console.log('stderr: ' + stderr);
	});
    workerProcess.on('exit', function (exitCode) {
	    console.log("Done. exitCode=" + exitCode);
	    response.write("### DONE.\n");
	    response.end();
    });
}).listen(PORT);

server.setTimeout(0); // https://zhidao.baidu.com/question/2077282925889752628.html

console.log('Server running at http://127.0.0.1:' + PORT);
console.log('Ctrl+C to exit ...');
