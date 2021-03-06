/*
** 2006 March 18
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
*/

#include "SQLite.h"

#include <stdio.h>

using namespace std;

namespace SQLite
{
	Stmt::Stmt(DB& aOwner, const string &aQuery)
		: owner(aOwner), query(aQuery), stmt(NULL)
	{
		prepare();
	}
#ifdef __BORLANDC__
	Stmt::Stmt(DB& aOwner, const AnsiString &aQuery)
		: owner(aOwner), query(aQuery.c_str()), stmt(NULL)
	{
		prepare();
	}
#endif
	Stmt::Stmt(const Stmt &c)
		: owner(c.owner), query(c.query), stmt(NULL)
	{
		prepare();
	}
	Stmt::~Stmt(void)
	{
		finalize();
	}
	Stmt& Stmt::operator=(const Stmt &c)
	{
		owner = c.owner;
		query = c.query;
		finalize();
		prepare();
		return *this;
	}
	void Stmt::prepare()
	{			
		done = ok = result = false;
		const char *pTail = NULL;
		if (sqlite3_prepare_v2(owner.ctx, query.c_str(), (int)query.length(), &stmt, &pTail) != SQLITE_OK)
		{
			throw Exception(owner.ctx);
		}
		ok = true;	
		if (pTail)
		{
			tail.assign(pTail);
		}
	}
#ifndef NDEBUG
#	define CHKTHROW if (!ok) { throw Exception("Statement unprepared!"); }
#else
#	define CHKTHROW
#endif
#define SQLOK(x) if (SQLITE_OK != x) throw Exception(owner.ctx);
	void Stmt::check()
	{
		CHKTHROW
		if (sqlite3_expired(stmt))
		{
			throw Exception("Statement expired");
		}
	}
	void Stmt::reset()
	{
		result = done = false;
		CHKTHROW;
		SQLOK(sqlite3_reset(stmt));
	}
	void Stmt::finalize()
	{
		if (ok)
		{
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
		result = done = ok = false;
	}
	void Stmt::bind(unsigned idx)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_null(stmt, idx));
	}
	void Stmt::bind(unsigned idx, int value)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_int(stmt, idx, value));
	}
	void Stmt::bind(unsigned idx, const __int64& value)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_int64(stmt, idx, value));
	}
	void Stmt::bind(unsigned idx, const double& value)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_double(stmt, idx, value));
	}
	void Stmt::bind(unsigned idx, const string& value)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_text(stmt, idx, value.c_str(), (int)value.length(), SQLITE_TRANSIENT));
	}
#ifdef __BORLANDC__
	void Stmt::bind(unsigned idx, const AnsiString& value)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_text(stmt, idx, value.c_str(), value.Length(), SQLITE_TRANSIENT));
	}
#endif
	void Stmt::bind(unsigned idx, const void *value, unsigned length)
	{
		CHKTHROW;
		SQLOK(sqlite3_bind_blob(stmt, idx, value, length, SQLITE_TRANSIENT));
	}

	unsigned Stmt::getColumnCount()
	{
		CHKTHROW
		int rv = sqlite3_column_count(stmt);
		if (rv < 0)
		{
			throw Exception(rv);
		}
		return rv;
	}

	bool Stmt::next()
	{
		CHKTHROW
		if (done)
		{
			throw Exception("Statement already done!");
		}
		int err = sqlite3_step(stmt);
		if (err == SQLITE_DONE)
		{
			return (result = !(done = true));
		}
		else if (err == SQLITE_ROW)
		{
			return (result = true);
		}
#if 0
		FILE *fp = fopen("sqlite.except.log", "a");
		fprintf(
			fp,
			"[\n[%s}\n%d: %s\n]\n",
			query.c_str(),
			sqlite3_errcode(owner.ctx),
			sqlite3_errmsg(owner.ctx)
		);
		fclose(fp);
#endif
		throw Exception(owner.ctx);
	}
	void Stmt::executeMany(DataItr &dp, Trans::TransactionType aType)
	{
		Trans trans(owner, aType);
		while(dp.next())
		{
			dp.bind(*this);
			execute();
		}
		finalize();
		trans.commit();
	}
	Data Stmt::value(unsigned idx)
	{
		CHKTHROW
		if (done)
		{
			throw Exception("Statement already done!");
		}
		else if (!result)
		{
			throw Exception("No Result!");
		}
		return Data(this, idx);
	}

	Finalizer::Finalizer(Stmt& aStmt)
	: stmt(aStmt)
	{
		try {
			stmt.execute();
#ifdef __BORLANDC__
		} catch (SQLite::Exception &ex) {
#else
		} catch (SQLite::Exception) {
#endif
			try {
				stmt.finalize();
#ifdef __BORLANDC__
			} catch (SQLite::Exception &ex) {
#else
			} catch (SQLite::Exception) {
#endif
			}
			throw;
		}
	}
	Finalizer::~Finalizer()
	{
		try {
			stmt.finalize();
#ifdef __BORLANDC__
		} catch (SQLite::Exception &ex) {
#else
		} catch (SQLite::Exception) {
#endif
		}
	}

	Resetter::Resetter(Stmt& aStmt)
	: stmt(aStmt)
	{
	}
	Resetter::~Resetter()
	{
		try {
			stmt.reset();
#ifdef __BORLANDC__
		} catch (SQLite::Exception &ex) {
#else
		} catch (SQLite::Exception) {
#endif
		}
	}
}
