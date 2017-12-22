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

var COST_CACHE_FILE = "cost.http_api.json";
var PROB_CACHE_FILE = "probability.http_api.json";
var COST_PROB_CC_CACHE_FILE = "cost_prob.cc.json";

var server = http.createServer(function (request, response) {
    var myDate = new Date();
    console.log("- Time = " + myDate.toLocaleString( ))
    var params = url.parse(request.url, true).query;
    var client_ip = get_client_ip(request)
    if (!params["simulateCode"] && !params["update"]) {
        console.log("- Ignore illegal request from IP=" + client_ip);
        console.log("- Refused to respond the illegal request!")
        return ;
    } else {
        console.log("- Get a legal request from IP=" + client_ip);
    }
    response.writeHead(200, {'Content-Type': 'text/plain:charset=utf-8'});
    response.write("### Arguments \n")
    for (var name in params) {
        console.log("- Request params['" + name + "']=" + params[name]);
        response.write(name + "=" + params[name] + " ");
        response.write("\n");
    }
    var need_update = params["update"];
    var mark = params["simulateCode"];
    if (need_update) {
        response.write("### Updating cost matrix and probability matrix ...\n");
        var cost_api = "http://139.198.5.125/OwnLogistics/api/own/city/cost";
        var prob_api = "http://139.198.5.125/OwnLogistics/api/own/city/statistics";
        var curl_cfg = " --verbose "; // " --silent --show-error ";
        var cmd_cost = "echo curl-cost && /usr/bin/time curl" + curl_cfg + cost_api + " -o " + COST_CACHE_FILE;
        var cmd_prob = "echo curl-prob && /usr/bin/time curl" + curl_cfg + prob_api + " -o " + PROB_CACHE_FILE;
        // cmd to create COST_PROB_CC_CACHE_FILE, i.e. cost_prob.cc.json
        var cmd_cost_prob_cc = "/usr/bin/time python python/cost.py " + COST_CACHE_FILE + " " + PROB_CACHE_FILE;
        var cmd = cmd_cost + ";\n" + cmd_prob + ";\n" + cmd_cost_prob_cc + "; true";
        console.log(cmd);
        var update_stdout = child_process.execSync(cmd);
        console.log('stdout: ' + update_stdout);
        console.log("- Finish updating cost and probability matrix.");
        response.write("### Finish updating cost and probability matrix.\n");
        if (!mark) {
            response.write("### Not run because no simulateCode is specified.\n");
            response.write("### DONE.\n");
            response.end();
            return;
        }
    }
    response.write("### Start optimization ...\n");
    console.log("- Start optimization ...");
    var cmd_python_cc_flags = "";  // prepare for C++ version
    var cmd_python = "/usr/bin/time python -O ../../python/ " + cmd_python_cc_flags + " "
        + "--order-file='http://139.198.5.125/OwnLogistics/api/own/orders?mark=" + mark + "' "
        + "--vehicle-file='http://139.198.5.125/OwnLogistics/api/own/vehicles?mark=" + mark + "' "
        + "--plan-upload-api='http://139.198.5.125/OwnLogistics/api/own/routes/result?mark=" + mark + "' ";
    var uniq_rundir = "run_" + mark + "/" + Date.now() + "_" + crypto.randomBytes(3).toString('hex') + "/";
    var cmd =
        "export rundir=" + uniq_rundir + "; OUTDIR=$rundir; mkdir -p $OUTDIR; cd $OUTDIR; "
        + "ln -sf ../../" + COST_CACHE_FILE + "; " + "ln -sf ../../" + PROB_CACHE_FILE + "; "
        + "ln -sf ../../" + COST_PROB_CC_CACHE_FILE + "; "
        + cmd_python
        + "> stdout.log; echo RUNDIR=$rundir ";
    console.log(cmd);
    var workerProcess = child_process.exec(cmd,
        function (error, stdout, stderr) {
            if (error) {
                console.log(error.stack);
                console.log('Error code: '+ error.code);
                console.log('Signal received: '+ error.signal);
            }
            console.log('stdout: ' + stdout);
            console.log('stderr: ' + stderr);
            console.log("- Finish optimization.");
            response.write("### Finish optimization.\n");
            response.end();
        });
    // workerProcess.on('exit', function (exitCode) {
        // Ansyc postprocess
    //});
}).listen(PORT);

server.setTimeout(0); // https://zhidao.baidu.com/question/2077282925889752628.html

console.log('Server running at http://127.0.0.1:' + PORT);
console.log('Ctrl+C to exit ...');

