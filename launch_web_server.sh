make -j --directory cc && make -j --directory python && node web_server.js 2>&1 | tee -a web_server.log
