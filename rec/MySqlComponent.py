import pymysql

from ConfigComponent import CommConfig

class MySqlComponent:

    host = None
    user = None
    passwd = None
    db = None
    port = 15811

    def __init__(self):
        conf = CommConfig().getConfig()
        self.host = conf.get("db", "host")
        self.user = conf.get("db", "user")
        self.passwd = conf.get("db", "passwd")
        self.db = conf.get("db", "db")
        # self.port = CommConfig.getConfig().get("db", "port")

    def setDbConfig(self, host, user, passwd, db, port=3306):
        self.host = host
        self.user = user
        self.passwd = passwd
        self.db = db
        self.port = port

    def querySql(self, sql):
        conn = self.getconn()
        cur = conn.cursor()
        cur.execute(sql)
        self.closeConn(conn)
        result = cur.fetchall()
        cur.close()
        return result

    def executeSql(self, sql, args=None):
        conn = self.getconn()
        cur = conn.cursor()
        execute = cur.execute(sql, args)
        cur.close()
        conn.commit();
        self.closeConn(conn)
        return execute

    def executeMany(self, sql, args=None):
        conn = self.getconn()
        cur = conn.cursor()
        execute = cur.executemany(sql, args)
        cur.close()
        conn.commit();
        self.closeConn(conn)
        return execute

    def callProc(self, proc, args=()):
        conn = self.getconn()
        cur = conn.cursor()
        cur.callproc(proc, args)
        result = cur.fetchall()
        cur.close()
        conn.commit();
        self.closeConn(conn)
        return result

    def getconn(self, ):
        return pymysql.connect(
            host=self.host,
            port=self.port,
            user=self.user,
            passwd=self.passwd,
            db=self.db,
            charset='utf8'
        )

    def closeConn(self, conn):
        if conn != None:
            conn.close()


            # result = query("select * from cities")
            #
            # print  result
