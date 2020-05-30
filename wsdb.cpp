//////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Paul Maurer
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include <stdexcept>
#include <sstream>

#include "wsdb.h"

#define DEFAULT_STATIONS "10"

class ResetOnExit {
private:
    sqlite3_stmt *stmt;

public:
    ResetOnExit(sqlite3_stmt *stmt) : stmt(stmt) {}
    ~ResetOnExit() {sqlite3_reset(stmt);}
};

void WsdbCallback::callback(int64_t, int64_t, const char *, int64_t) {
}

Wsdb::Wsdb() :
    db(nullptr),
    getParam(nullptr),
    setParam(nullptr),
    getInfo(nullptr),
    setInfo(nullptr),
    cleanInfo(nullptr),
    insert(nullptr),
    select(nullptr),
    remove(nullptr) {
}

Wsdb::~Wsdb() {
    close();
}

void Wsdb::open(const char *filename) {
    close();

    if (sqlite3_open(filename, &db) != SQLITE_OK)
        throw std::runtime_error("Could not open database: " + std::string(sqlite3_errmsg(db)));

    char *errStr;
    if (sqlite3_exec(db, "create table if not exists reservations (slot int not null, station int not null, name text, attr int, primary key (slot, station)) without rowid;", nullptr, nullptr, &errStr) != SQLITE_OK) {
        std::string err(errStr);
        close();
        throw std::runtime_error("Could not create table reservations: " + err);
    }

    if (sqlite3_exec(db, "create table if not exists descriptions (station int primary key not null, name text, desc text, flags int) without rowid;", nullptr, nullptr, &errStr) != SQLITE_OK) {
        std::string err(errStr);
        close();
        throw std::runtime_error("Could not create table descriptions: " + err);
    }

    if (sqlite3_exec(db, "create table if not exists parameters (name text primary key, value int) without rowid;", nullptr, nullptr, &errStr) != SQLITE_OK) {
        std::string err(errStr);
        close();
        throw std::runtime_error("Could not create table parameters: " + err);
    }

    if (sqlite3_exec(db, "insert or ignore into parameters (name, value) values (\"numStations\", " DEFAULT_STATIONS ");", nullptr, nullptr, &errStr) != SQLITE_OK) {
        std::string err(errStr);
        close();
        throw std::runtime_error("Could not set default number of stations: " + err);
    }

    if (sqlite3_prepare_v2(db, "select (value) from parameters where name = ?;", -1, &getParam, nullptr) != SQLITE_OK) {
        close();
        throw std::runtime_error("Could not prepare getParam statement: " + std::string(sqlite3_errmsg(db)));
    }

    if (sqlite3_prepare_v2(db, "insert or replace into parameters (name, value) values (?, ?);", -1, &setParam, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare setParam statement: " + err);
    }

    if (sqlite3_prepare_v2(db, "select * from descriptions;", -1, &getInfo, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare getInfo statement: " + std::string(sqlite3_errmsg(db)));
    }

    if (sqlite3_prepare_v2(db, "insert or replace into descriptions (station, name, desc, flags) values (?, ?, ?, ?);", -1, &setInfo, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare setInfo statement: " + err);
    }

    if (sqlite3_prepare_v2(db, "delete from descriptions where station >= ? or (name is null and desc is null and flags = 0);", -1, &cleanInfo, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare cleanInfo statement: " + err);
    }

    if (sqlite3_prepare_v2(db, "select * from reservations where slot between ? and ? and station between ? and ?;", -1, &select, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare select statement: " + err);
    }

    if (sqlite3_prepare_v2(db, "insert or fail into reservations (slot, station, name, attr) values (?, ?, ?, ?);", -1, &insert, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare insert statement: " + err);
    }

    if (sqlite3_prepare_v2(db, "delete from reservations where slot between ? and ? and station = ?;", -1, &remove, nullptr) != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        close();
        throw std::runtime_error("Could not prepare remove statement: " + err);
    }
}

void Wsdb::close() {
    if (getParam)
        sqlite3_finalize(getParam);

    if (setParam)
        sqlite3_finalize(setParam);

    if (getInfo)
        sqlite3_finalize(getInfo);

    if (setInfo)
        sqlite3_finalize(setInfo);

    if (cleanInfo)
        sqlite3_finalize(cleanInfo);

    if (insert)
        sqlite3_finalize(insert);

    if (select)
        sqlite3_finalize(select);

    if (remove)
        sqlite3_finalize(remove);

    if (db)
        sqlite3_close(db);

    getParam  = nullptr;
    setParam  = nullptr;
    getInfo   = nullptr;
    setInfo   = nullptr;
    cleanInfo = nullptr;
    insert    = nullptr;
    select    = nullptr;
    remove    = nullptr;
    db        = nullptr;
}

int64_t Wsdb::getNumStations() {
    return getParameter("numStations", 0);
}

void Wsdb::setNumStations(int64_t num) {
    setParameter("numStations", num);
}

void Wsdb::getStationInfo(std::vector<Wsdb::StationInfo> *infoOut) {
    if (infoOut == nullptr)
        return;
    infoOut->clear();

    int64_t num = getNumStations();

    for (int64_t count = 0; count < num; count++)
        infoOut->push_back(StationInfo(defaultWorkstationName(count), "", 0));

    if (getInfo == nullptr)
        return;

    ResetOnExit roe(getInfo);

    while (sqlite3_step(getInfo) == SQLITE_ROW) {
        int64_t station = sqlite3_column_int64(getInfo, 0);
        if (station < 0 || station >= num)
            continue;
        if (sqlite3_column_type(getInfo, 1) != SQLITE_NULL) {
            const char *name = (const char *) sqlite3_column_text(getInfo, 1);
            if (*name != '\0')
                (*infoOut)[static_cast<size_t> (station)].name = name;
        }

        if (sqlite3_column_type(getInfo, 2) != SQLITE_NULL)
            (*infoOut)[static_cast<size_t> (station)].desc = (const char *) sqlite3_column_text(getInfo, 2);

        (*infoOut)[static_cast<size_t> (station)].flags = sqlite3_column_int64(getInfo, 3);
    }
}

void Wsdb::setStationInfo(int64_t station, const StationInfo &info) {
    if (setInfo == nullptr)
        return;

    ResetOnExit roe(setInfo);

    if (sqlite3_bind_int64(setInfo, 1, station) != SQLITE_OK)
        return;

    if (info.name.size() > 0) {
        if (sqlite3_bind_text(setInfo, 2, info.name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)
            return;
    } else {
        if (sqlite3_bind_null(setInfo, 2) != SQLITE_OK)
            return;
    }

    if (info.desc.size() > 0) {
        if (sqlite3_bind_text(setInfo, 3, info.desc.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)
            return;
    } else {
        if (sqlite3_bind_null(setInfo, 3) != SQLITE_OK)
            return;
    }

    sqlite3_bind_int64(setInfo, 4, info.flags);

    sqlite3_step(setInfo);
}

void Wsdb::setStationInfo(const std::vector<StationInfo> &info) {
    size_t numSize = info.size();
    if (numSize > INT64_MAX)
        numSize = INT64_MAX;
    int64_t num = static_cast<int64_t>(numSize);
    setNumStations(num);

    for (int64_t count = 0; count < num; count++)
        setStationInfo(count, info[static_cast<size_t>(count)]);

    if (cleanInfo == nullptr)
        return;

    ResetOnExit roe(cleanInfo);

    if (sqlite3_bind_int64(cleanInfo, 1, num) != SQLITE_OK)
        return;

    sqlite3_step(cleanInfo);
}

int Wsdb::insertName(int64_t slot, int64_t station, const char *name, int64_t attr) {
    if (insert == nullptr)
        return 0;

    ResetOnExit roe(insert);

    if (sqlite3_bind_int64(insert, 1, slot) != SQLITE_OK)
        return 0;

    if (sqlite3_bind_int64(insert, 2, station) != SQLITE_OK)
        return 0;

    if (sqlite3_bind_text(insert, 3, name, -1, SQLITE_STATIC) != SQLITE_OK)
        return 0;

    if (sqlite3_bind_int64(insert, 4, attr) != SQLITE_OK)
        return 0;

    return sqlite3_step(insert) == SQLITE_DONE;
}

void Wsdb::selectNames(int64_t slotStart, int64_t slotStop, int64_t stationStart, int64_t stationStop, WsdbCallback &callback) {
    if (select == nullptr)
        return;

    ResetOnExit row(select);

    if (sqlite3_bind_int64(select, 1, slotStart) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(select, 2, slotStop) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(select, 3, stationStart) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(select, 4, stationStop) != SQLITE_OK)
        return;

    while (sqlite3_step(select) == SQLITE_ROW) {
        callback.callback(sqlite3_column_int64(select, 0),
                          sqlite3_column_int64(select, 1),
                          (const char *) sqlite3_column_text(select, 2),
                          sqlite3_column_int64(select, 3));
    }
}

void Wsdb::removeNames(int64_t slotStart, int64_t slotStop, int64_t station) {
    if (remove == nullptr)
        return;

    ResetOnExit roe(remove);

    if (sqlite3_bind_int64(remove, 1, slotStart) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(remove, 2, slotStop) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(remove, 3, station) != SQLITE_OK)
        return;

    sqlite3_step(remove);
}

std::string Wsdb::defaultWorkstationName(int64_t station) {
    std::stringstream str;

    str << "Workstation " << (station + 1);

    return str.str();
}

int64_t Wsdb::getParameter(const char *name, int64_t default_val) {
    if (getParam == nullptr)
        return default_val;

    ResetOnExit roe(getParam);

    if (sqlite3_bind_text(getParam, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
        return default_val;

    if (sqlite3_step(getParam) != SQLITE_ROW)
        return default_val;

    if (sqlite3_column_type(getParam, 0) != SQLITE_INTEGER)
        return default_val;

    return sqlite3_column_int64(getParam, 0);
}

void Wsdb::setParameter(const char *name, int64_t value) {
    if (setParam == nullptr)
        return;

    ResetOnExit roe(setParam);

    if (sqlite3_bind_text(setParam, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
        return;

    if (sqlite3_bind_int64(setParam, 2, value) != SQLITE_OK)
        return;

    sqlite3_step(setParam);
}
