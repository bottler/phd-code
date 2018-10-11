#include "JRSqlite.h"
#include "../sqlite/sqlite3.h"

#include <stdexcept>
#include <sstream>
#ifndef LINUX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
using std::vector;
using std::list;
using std::string;

namespace JRSqlite
{

QueryBase::~QueryBase(){
	sqlite3_finalize(m_stmt); //not checking error
}

void Connection::init(int res, const char* location)
{
	if(res!=SQLITE_OK)
	{
		sqlite3_close(m_conn);
		m_conn=NULL;
		std::ostringstream oss;
		oss<<"Could not open connection to \""<<location<<"\" with error number "<<res<<".";
#ifndef LINUX
		OutputDebugStringA(oss.str().c_str());
#endif
		throw std::runtime_error(oss.str());
	}
}

Connection::Connection(const char* location) : m_conn(0){
	int res = sqlite3_open(location, &m_conn);
	init(res, location);
}

/*static*/
Connection Connection::openReadOnly(const char* location)
{
	Connection c;
	int res = sqlite3_open_v2(location, &c.m_conn, SQLITE_OPEN_READONLY, 0);
	c.init(res, location);
	return std::move(c);
}

/*static*/
Connection Connection::openExisting(const char* location)
{
	Connection c;
	int res = sqlite3_open_v2(location, &c.m_conn, SQLITE_OPEN_READWRITE, 0);
	c.init(res, location);
	return std::move(c);
}


/* Sometimes you have a program whose whole purpose is to make a db from scratch, and the output is totally
irrelevant in the case where the program hasn't run to completion. In such a case, journalling, locking, flushing
can just be turned off and our writes can be quick. After filesystem TS, make this fn check location is a new file.*/
/*static*/
Connection Connection::openNewCarefree(const char* location)
{
	Connection c;
	int res = sqlite3_open_v2(location, &c.m_conn, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
	c.init(res, location);
	JRSqlite::Query(c,"PRAGMA locking_mode=OFF").executeAndIgnoreAll();
	JRSqlite::Query(c,"PRAGMA synchronous=OFF").executeAndIgnoreAll();
	return std::move(c);
}

Connection::~Connection(){
	sqlite3_close(m_conn); //don't check for busy
}

void Connection::reportError(const std::string& msg, int res)
{
	std::ostringstream oss;
	if(!msg.empty())
		oss<<msg<<"\n";
	oss<<res<<": ";
	oss<<sqlite3_errstr(res)<<"\n";
	oss<<sqlite3_errmsg(m_conn);
#ifndef LINUX
	OutputDebugStringA(oss.str().c_str());
#endif
	throw std::runtime_error(oss.str());
}

int Connection::nChanges()
{
	return sqlite3_changes(m_conn);
}

__int64 Connection::lastInsertRowid()
{
	return sqlite3_last_insert_rowid(m_conn);
}

QueryBase::QueryBase(Connection& c, const char* query):m_connection(&c){
	int res = sqlite3_prepare_v2(c.m_conn, query, -1, &m_stmt, 0);
	if(res!=SQLITE_OK)
		c.reportError(std::string("Could not prepare statement \"")+query+"\"", res);
}

void QueryBase::bind(int idx, double d)
{
	int res = sqlite3_bind_double(m_stmt, idx, d);
	if(SQLITE_OK!=res)
		m_connection->reportError("Failed to bind",res);
}
void QueryBase::bind(int idx, int i)
{
	int res = sqlite3_bind_int(m_stmt, idx, i);
	if(SQLITE_OK!=res)
		m_connection->reportError("Failed to bind",res);
}
void QueryBase::bind(int idx, const char* s)
{
  //SQLITE_STATIC is probably good enough in all the cases this is used
	int res = sqlite3_bind_text(m_stmt, idx, s,-1,SQLITE_TRANSIENT);
	if(SQLITE_OK!=res)
		m_connection->reportError("Failed to bind",res);
}
void QueryBase::bindBlob(int idx, const char* a, const char* pastEnd)
{
	int res = sqlite3_bind_blob(m_stmt, idx, a, pastEnd-a, SQLITE_TRANSIENT);
	if(SQLITE_OK!=res)
		m_connection->reportError("Failed to bind",res);
}


void ResultlessStmt::exec(){
	int res = sqlite3_step(m_stmt);
	if(SQLITE_DONE!=res)
		m_connection->reportError("The query failed to execute simply",res);
	sqlite3_reset(m_stmt);
	//consider returning #changes
}

Sink1DoubleStmt::Sink1DoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink1DoubleStmt::execute(double p1)
{
	bind(1,p1);
	exec();
}

Sink2DoubleStmt::Sink2DoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink2DoubleStmt::execute(double p1, double p2)
{
	bind(1,p1);
	bind(2, p2);
	exec();
}
Sink1IntStmt::Sink1IntStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink1IntStmt::execute(int p1)
{
	bind(1,p1);
	exec();
}
Sink4IntStmt::Sink4IntStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink4IntStmt::execute(int p1, int p2, int p3, int p4)
{
	bind(1,p1); bind(2, p2);bind(3,p3); bind(4, p4);
	exec();
}
Sink2IntDoubleStmt::Sink2IntDoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink2IntDoubleStmt::execute(int i1, int i2, double p)
{
	bind(1,i1);
	bind(2, i2);
	bind(3, p);
	exec();
}

Sink2Int6DoubleStmt::Sink2Int6DoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink2Int6DoubleStmt::execute(int i1, int i2, double p1, double p2, double p3, double p4, double p5, double p6)
{
	bind(1,i1);
	bind(2, i2);
	bind(3, p1);
	bind(4, p2);
	bind(5, p3);
	bind(6, p4);
	bind(7, p5);
	bind(8, p6);
	exec();
}

Sink4IntDoubleStmt::Sink4IntDoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink4IntDoubleStmt::execute(int i1, int i2, int i3, int i4, double d)
{
	bind(1,i1);
	bind(2, i2);
	bind(3, i3);
	bind(4, i4);
	bind(5, d);
	exec();
}


Sink5IntDoubleStmt::Sink5IntDoubleStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink5IntDoubleStmt::execute(int i1, int i2, int i3, int i4, int i5, double d)
{
	bind(1,i1);
	bind(2, i2);
	bind(3, i3);
	bind(4, i4);
	bind(5, i5);
	bind(6, d);
	exec();
}

SinkAnythingStmt::SinkAnythingStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void SinkAnythingStmt::execute_(int i){
  if(i!=sqlite3_bind_parameter_count(m_stmt))
    throw std::runtime_error("Wrong size given in SinkAnythingStmt");
  exec();
}
void SinkAnythingStmt::execute(const std::vector<D>& d){
  for(size_t s=0; s<d.size(); ++s){
    const D& e = d[s];
    if(e.m_type==D::I_)
      bind(s+1,e.m_i);
    else if(e.m_type==D::D_)
      bind(s+1,e.m_d);
    else if(e.m_type==D::S_)
      bind(s+1,e.m_s);
    else
      throw std::runtime_error("unknown enum type in SinkAnythingStmt");
  }
  execute_(d.size());
}
SinkSourceAnythingStmt::SinkSourceAnythingStmt(Connection& c, const char* q):QueryBase(c,q){}
void SinkSourceAnythingStmt::execute(const vector<D>& d, const vector<D::Type>& outTypes, vector<vector<D>>& out, list<string>& space){
  for(size_t s=0; s<d.size(); ++s){
    const D& e = d[s];
    if(e.m_type==D::I_)
      bind(s+1,e.m_i);
    else if(e.m_type==D::D_)
      bind(s+1,e.m_d);
    else if(e.m_type==D::S_)
      bind(s+1,e.m_s);
    else
      throw std::runtime_error("unknown enum type in SinkSourceAnythingStmt");
  }
  if(d.size()!=sqlite3_bind_parameter_count(m_stmt))
    throw std::runtime_error("Wrong input size given in SinkSourceAnythingStmt");
  const size_t outCols = outTypes.size();
  while(1){
    const int res = sqlite3_step(m_stmt);
    if(res==SQLITE_ROW)
    {
      if(sqlite3_data_count(m_stmt)!=outCols)
	throw std::runtime_error("Wrong output size given in SinkSourceAnythingStmt");
      vector<D> o;
      for(int i=0; i!=outCols; ++i){
	D d;
	d.m_type = outTypes[i];
	switch(outTypes[i]){
	case D::D_:
	  d.m_d=sqlite3_column_double(m_stmt,i);
	  break;
	case D::I_:
	  d.m_i=sqlite3_column_int(m_stmt,i);
	  break;
	case D::S_:
	  const unsigned char* t = sqlite3_column_text(m_stmt,i);
	  space.emplace_back(reinterpret_cast<const char*>(t));
	  d.m_s=space.back().data();
	  break;
	}
	o.push_back(d);
      }
      out.push_back(o);
      continue;
    }
    if(res==SQLITE_DONE)
    {
      sqlite3_reset(m_stmt);
      break;
    }
    m_connection->reportError("Failed",res);
  }
}

Sink2IntBlobStmt::Sink2IntBlobStmt(Connection& c, const char* q):ResultlessStmt(c,q){}
void Sink2IntBlobStmt::execute(int i1, int i2, const char* a, const char* pastEnd)
{
	bind(1,i1);
	bind(2, i2);
	bindBlob(3,a, pastEnd);
	exec();
}

bool Source1DoubleStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_d = (sqlite3_column_double(m_stmt,0));
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}

bool Source2DoubleStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_d1 = (sqlite3_column_double(m_stmt,0));
		m_d2 = (sqlite3_column_double(m_stmt,1));
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}
int SourceSingleIntStmt::execute()
{
	const int res = sqlite3_step(m_stmt);
	if(res!=SQLITE_ROW)
		m_connection->reportError("Failed to get one row",res);
	int o = sqlite3_column_int(m_stmt,0);
	const int res2 = sqlite3_step(m_stmt);
	if(res2!=SQLITE_DONE)
		m_connection->reportError("Perhaps got more than one row",res2);
	sqlite3_reset(m_stmt);
	return o;
}
std::string SourceSingleTextStmt::execute()
{
	const int res = sqlite3_step(m_stmt);
	if(res!=SQLITE_ROW)
		m_connection->reportError("Failed to get one row",res);
	const unsigned char* o = sqlite3_column_text(m_stmt,0);
	const int length = sqlite3_column_bytes(m_stmt,0);
	std::string oo (o, o+length);
	const int res2 = sqlite3_step(m_stmt);
	if(res2!=SQLITE_DONE)
		m_connection->reportError("Perhaps got more than one row",res2);
	sqlite3_reset(m_stmt);
	return oo;
}
bool Source1IntStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_i1 = (sqlite3_column_int(m_stmt,0));
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}
bool Source4IntStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_i1 = (sqlite3_column_int(m_stmt,0));
		m_i2 = (sqlite3_column_int(m_stmt,1));
		m_i3 = (sqlite3_column_int(m_stmt,2));
		m_i4 = (sqlite3_column_int(m_stmt,3));
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}
bool Source1BlobStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_b = (const char*)(sqlite3_column_blob(m_stmt,0));
		m_bytes=sqlite3_column_bytes(m_stmt,0);
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}
bool Source1Int1BlobStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_a = sqlite3_column_int(m_stmt,0);
		m_b = (const char*)(sqlite3_column_blob(m_stmt,1));
		m_bytes=sqlite3_column_bytes(m_stmt,1);
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}
bool Source1BlobSink2IntStmt::get()
{
	const int res = sqlite3_step(m_stmt);
	if(res==SQLITE_ROW)
	{
		m_b = (const char*)(sqlite3_column_blob(m_stmt,0));
		return true;
	}
	if(res==SQLITE_DONE)
	{
		sqlite3_reset(m_stmt);
		return false;
	}
	m_connection->reportError("Failed",res);
	return false;
}

std::string Query::executeAndReportAll()
{
	std::ostringstream oss;
	const int cols = sqlite3_column_count(m_stmt);
	oss<<"column names:";
	for(int i=0; i<cols; ++i)
	{
		oss<<sqlite3_column_name(m_stmt, i)<<(i+1==cols?"\n":",");
	}
	while(1)
	{
		const int res = sqlite3_step(m_stmt);
		if(res==SQLITE_ROW)
		{
			oss<<"row:\n";
			for(int i=0; i<cols; ++i)
			{
				const char * r = (const char *)(sqlite3_column_text(m_stmt,i));
				oss<< (r?r:"<NULL>") <<"\n";
			}
		}
		else if(res==SQLITE_DONE)
		{
			sqlite3_reset(m_stmt);
			break;
		}
		else
			m_connection->reportError("Failed",res);
	}
	return oss.str();
}

void Query::executeAndIgnoreAll()
{
	while(1)
	{
		const int res = sqlite3_step(m_stmt);
	        if(res==SQLITE_DONE)
		{
			sqlite3_reset(m_stmt);
			break;
		}
		else if (res!=SQLITE_ROW)
			m_connection->reportError("Failed",res);
	}
}

Query::Query(Connection& c, const char* q):QueryBase(c,q){}



}
