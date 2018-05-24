make -j --directory cc && make -j --directory python && env PORT=8139 node web_server.js 2>&1 | tee -a web_server.log
