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
    console.log("- Time = " + myDate.toLocaleString( ))
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
    for (var name in params) {
        console.log("- Request params['" + name + "']=" + params[name]);
        response.write(name + "=" + params[name] + " ");
        response.write("\n");
    }
    var need_update = params["update"];
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
        if (!mark) {
            response.write("### DONE.\n");
            response.end();
            return;
        }
    }
    response.write("### Start optimization ...\n");
    console.log("- Start optimization ...");
    var get_engine_cmd = function(eng) {
        var uniq_rundir = "run_" + mark + "/" + Date.now() + "_" + crypto.randomBytes(3).toString('hex') + "/";
        var cmd_ln = ""
            + "ln -sf ../../" + COST_CACHE_FILE + "; " + "ln -sf ../../" + PROB_CACHE_FILE + "; "
            + "ln -sf ../../" + COST_PROB_CC_CACHE_FILE + "; "
        // Python command
        var cmd_python_cc_flags = "";  // prepare for C++ version
        var cmd_python_run = "/usr/bin/time python -O ../../python/ " + cmd_python_cc_flags + " "
            + "--order-file='http://139.198.5.125/OwnLogistics/api/own/orders?mark=" + mark + "' "
            + "--vehicle-file='http://139.198.5.125/OwnLogistics/api/own/vehicles?mark=" + mark + "' "
            + "--plan-upload-api='http://139.198.5.125/OwnLogistics/api/own/routes/result?mark=" + mark + "' ";
        // C++(CC) command
        var cmd_cc_run =  "/usr/bin/time ../../cc/vsp 1 "  // multi-process by default
            + "'http://139.198.5.125/OwnLogistics/api/own/orders?mark=" + mark + "' "
            + "'http://139.198.5.125/OwnLogistics/api/own/vehicles?mark=" + mark + "' "
            + "'http://139.198.5.125/OwnLogistics/api/own/routes/result?mark=" + mark + "' ";
        // Select a run command
        var cmd_run = cmd_python_run;  // default using Python version
        if (!eng && (eng[0] === 'c' || eng[0] === 'C'))
            cmd_run = cmd_cc_run;
        else
            console.log("** Warning: Unknown engine version: " + eng + "! Use Python instead.");
        var cmd =
            "export rundir=" + uniq_rundir + "; OUTDIR=$rundir; mkdir -p $OUTDIR; cd $OUTDIR; "
            + cmd_ln + cmd_run
            + "> stdout.log; echo RUNDIR=$rundir ";
    };
    var cmd = get_engine_cmd(params["engine"]);
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
            console.log("- Finish optimization.\n");
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

