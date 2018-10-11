#ifndef JRSQLITE_H
#define JRSQLITE_H

#include <string>
#include <tuple>
#include <iosfwd>
#include <type_traits> //just for enable_if
#include <vector>
#include <list>

#ifdef LINUX
#define __int64 long long
#endif

struct sqlite3;
struct sqlite3_stmt;

namespace JRSqlite
{


class Connection;
class QueryBase {
	QueryBase(const QueryBase&);//not copyable
	QueryBase& operator=(const QueryBase&);//not copyable

protected:
	Connection * const m_connection;
	sqlite3_stmt* m_stmt;
	QueryBase(Connection& c, const char* query);
	~QueryBase();
	void bind(int idx, double d);
	void bind(int idx, int i);
	void bind(int idx, const char* s);
	void bindBlob(int idx, const char* a, const char* pastEnd);
};

class Connection
{
	friend class QueryBase;
	sqlite3* m_conn;
	void init(int res, const char* location);
	Connection():m_conn(0){}
	Connection(Connection&);//not copyable
	Connection& operator=(Connection&);//not copyable
public:
	Connection(const char* location);
	Connection(Connection&& c):m_conn(c.m_conn){c.m_conn=0;}
	static Connection openExisting(const char* location); //will be RO if not writable on disk
	static Connection openReadOnly(const char* location);
	static Connection openNewCarefree(const char* location); //location should be a new file (not checked), no transactions
	~Connection();
	void reportError(const std::string& msg, int res);
	int nChanges();
	__int64 lastInsertRowid();
};

class ResultlessStmt : public QueryBase
{
protected:
	ResultlessStmt(Connection& c, const char* q):QueryBase(c,q){}
	void exec();
};


class SimpleStmt : public ResultlessStmt
{
public:
	SimpleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
	void execute(){exec();}
};

class Sink1DoubleStmt : public ResultlessStmt
{
public:
	Sink1DoubleStmt(Connection& c, const char* q);
	void execute(double);
};
class Sink2DoubleStmt : public ResultlessStmt
{
public:
	Sink2DoubleStmt(Connection& c, const char* q);
	void execute(double, double);
};
class Sink1IntStmt : public ResultlessStmt
{
public:
	Sink1IntStmt(Connection& c, const char* q);
	void execute(int);
};class Sink4IntStmt : public ResultlessStmt
{
public:
	Sink4IntStmt(Connection& c, const char* q);
	void execute(int,int,int,int);
};
class Sink2IntDoubleStmt : public ResultlessStmt
{
public:
	Sink2IntDoubleStmt(Connection& c, const char* q);
	void execute(int i1, int i2, double p);
};
class Sink2Int6DoubleStmt : public ResultlessStmt
{
public:
	Sink2Int6DoubleStmt(Connection& c, const char* q);
	void execute(int i1, int i2, double, double, double, double, double, double);
};

class Sink2IntBlobStmt : public ResultlessStmt
{
public:
	Sink2IntBlobStmt(Connection& c, const char* q);
	void execute(int, int, const char* a, const char* pastEnd);
};

class Sink4IntDoubleStmt : public ResultlessStmt
{
public:
	Sink4IntDoubleStmt(Connection& c, const char* q);
	void execute(int, int, int, int, double);
};

class Sink5IntDoubleStmt : public ResultlessStmt
{
public:
	Sink5IntDoubleStmt(Connection& c, const char* q);
	void execute(int, int, int, int, int, double);
};

class SinkAnythingStmt : public ResultlessStmt
{
public:
	SinkAnythingStmt(Connection& c, const char* q);
	//t must be a tuple of the right # of double, int, const char*	
	template<typename T>
	void executeTuple(const T& t){
	  constexpr int s = std::tuple_size<T>::value;
	  e<T,s>(t);
	  execute_(s);
	}

	struct D{
	  enum {I_, D_, S_} m_type;
	  union {int m_i; double m_d; const char* m_s;};
	};
	void execute(const std::vector<D>& d);
	
private:
	void execute_(int i);
	template<typename T, int i>
	  std::enable_if<(i>0)> e(const T& t){
	  bind(i,std::get<i-1>(t));
	  e<T,i-1>(t);
	}
	template<typename T, int i>
	  std::enable_if<i==0> e(const T& t){
	  bind(1,std::get<0>(t));
	}
};

class SinkSourceAnythingStmt : public QueryBase
{
public:
	SinkSourceAnythingStmt(Connection& c, const char* q);
	struct D{
	  enum Type {I_, D_, S_} m_type;
	  union {int m_i; double m_d; const char* m_s;};
	};
	//space is used for the storing string results. 
	void execute(const std::vector<D>& d, 
		     const std::vector<D::Type>& outTypes,
		     std::vector<std::vector<D>>& out, 
		     std::list<std::string>& space);
};

class Source1DoubleStmt : public QueryBase
{
protected:
	double m_d;
	bool get();
public:
	Source1DoubleStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T>
	void execute(T&& t){while(get())t(m_d);}
};

//caller is assumed to know length of blob, unless executeWithSize is used.
class Source1BlobStmt : public QueryBase
{
protected:
	const char* m_b;
	int m_bytes;
	bool get();
public:
	Source1BlobStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T> void execute(T&& t){while(get())t(m_b);}
	template<class T> void executeWithSize(T&& t){while(get())t(m_b,m_bytes);}
};
class Source1Int1BlobStmt : public QueryBase
{
protected:
	int m_a;
	const char* m_b;
	int m_bytes;
	bool get();
public:
	Source1Int1BlobStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T> void execute(T&& t){while(get())t(m_a,m_b);}
	template<class T> void executeWithSize(T&& t){while(get())t(m_a,m_b,m_bytes);}
};
class Source1BlobSink1IntStmt : protected Source1BlobStmt
{
public:
	Source1BlobSink1IntStmt(Connection& c, const char* q):Source1BlobStmt(c,q){}
	template<class T> void execute(int a, T&& t){bind(1,a);while(get())t(m_b);}
	template<class T> void executeWithSize(int a, T&& t){bind(1,a);while(get())t(m_b,m_bytes);}
};
class Source1BlobSink2IntStmt : public QueryBase
{
	const char* m_b;
	bool get();
public:
	Source1BlobSink2IntStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T> void execute(int a, int b, T&& t){bind(1,a);bind(2,b); while(get())t(m_b);}
};

class Source1DoubleSink2IntStmt : private Source1DoubleStmt
{
public:
	Source1DoubleSink2IntStmt(Connection& c, const char* q):Source1DoubleStmt(c,q){}
	template<class T>
	void execute(int a, int b, T&& t){bind(1,a);bind(2,b); while(get())t(m_d);}
};

class Source2DoubleStmt : public QueryBase
{
protected:
	double m_d1, m_d2;
	bool get();
public:
	Source2DoubleStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T>
	void execute(T&& t){while(get())t(m_d1, m_d2);}
};
class Source2DoubleSink2IntStmt : private Source2DoubleStmt{
public:
	Source2DoubleSink2IntStmt(Connection& c, const char* q):Source2DoubleStmt(c,q){}
	template<class T>
	void execute(int a, int b, T&& t){bind(1,a);bind(2,b); while(get())t(m_d1, m_d2);}
};
class SourceSingleIntStmt : private QueryBase{
public:
	SourceSingleIntStmt(Connection& c, const char* q):QueryBase(c,q){}
	int execute();
};
class SourceSingleTextStmt : private QueryBase{
public:
	SourceSingleTextStmt(Connection& c, const char* q):QueryBase(c,q){}
	std::string execute();
};

class Source1IntStmt : private QueryBase{
	int m_i1;
	bool get();
public:
	Source1IntStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T>
	void execute(T&& t){while(get())t(m_i1);}
};

class Source4IntStmt : private QueryBase{
	int m_i1, m_i2, m_i3, m_i4;
	bool get();
public:
	Source4IntStmt(Connection& c, const char* q):QueryBase(c,q){}
	template<class T>
	void execute(T&& t){while(get())t(m_i1, m_i2, m_i3, m_i4);}
};

class Query : public QueryBase
{
public:
	Query(Connection& c, const char* q);
	std::string executeAndReportAll();
	void executeAndIgnoreAll();
};

}
#endif
