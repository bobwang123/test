var express = require('express');
var app = express();
var http = require('http');
var url = require('url');
var util = require('util');
var fs = require("fs")
var crypto = require("crypto");
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
    console.log("- Start Time = " + myDate.toLocaleString( ))
    var params = url.parse(request.url, true).query;
    var client_ip = get_client_ip(request)
    if (!params["simulateCode"] && !params["update"]) {
        console.log("- Refuse to respond illegal request from IP=" + client_ip);
        return; // drop illegal requests
    } else {
        // mark legal requests
        console.log("- Get a legal request from IP=" + client_ip);
    }
    response.writeHead(200, {'Content-Type': 'text/plain:charset=utf-8'});
    response.write("### Arguments \n")
    // dump arguments
    for (var name in params) {
        console.log("- Request params['" + name + "']=" + params[name]);
        response.write(name + "=" + params[name] + " ");
        response.write("\n");
    }
    // update cost/prob info
    var need_update = Number(params["update"]);
    var mark = params["simulateCode"];
    if (need_update) {
        response.write("### Updating cost matrix and probability matrix ...\n");
        try {
            var update_stdout = child_process.execFileSync("make",
                ["-j2", "--ignore-errors", "-f", "Makefile-update-db"])
            console.log('stdout: ' + update_stdout);
            console.log("- Finish updating cost and probability matrix.\n");
            response.write("### Finish updating cost and probability matrix.\n");
            response.write("### Not run because no simulateCode is specified.\n");
        } catch (e) {
            console.error("- Error caught by catch block: ", e);
            response.write("### FAILED to update cost and probability matrix.\n");
        }
	// update only without running opimization
        if (!mark) {
            response.write("### DONE.\n");
            response.end();
            var myDate = new Date();
            console.log("- End Time = " + myDate.toLocaleString( ))
            return;
        }
    }
    var engine = function (eng) {
        return (eng && (eng[0] === 'c' || eng[0] === 'C')) ? "C++": "Python";
    }(params["engine"]);
    response.write("### Start optimization ...\n");
    console.log("- Start optimization ...");
    // Ansyc run
    var workerProcess = child_process.execFile("make",
	["--ignore-errors", "-f", "Makefile-run", "MARK="+mark, "ENGINE="+engine],
	function (error, stdout, stderr) {
	    console.log('stdout: ' + stdout);
	    console.log('stderr: ' + stderr);
	    if (error) {
		console.log(error.stack);
		console.log('Error code: '+ error.code);
		console.log('Signal received: '+ error.signal);
		console.log("- Optimization FAILED!\n");
		response.write("### Optimization FAILED!\n");
	    } else {
		console.log("- Finish optimization.\n");
		response.write("### Finish optimization.\n");
	    }
	    response.end();
	    var myDate = new Date();
	    console.log("- End Time = " + myDate.toLocaleString( ))
	});
    // workerProcess.on('exit', function (exitCode) {
    // Ansyc postprocess
    //});
}).listen(PORT);

server.setTimeout(0); // https://zhidao.baidu.com/question/2077282925889752628.html

console.log('Server running at http://127.0.0.1:' + PORT);
console.log('Ctrl+C to exit ...');

