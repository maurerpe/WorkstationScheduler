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

#ifndef DBCOMMAND_H
#define DBCOMMAND_H

#include <list>

#include "commandqueue.h"
#include "wsdb.h"

// Abstract classes //////////////////////////////////////////////////////////

class DbCallback {
public:
    DbCallback();
    virtual ~DbCallback();

    virtual void execute();
};

class DbCommand {
public:
    DbCommand();
    virtual ~DbCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);
};

// DbNopCommand /////////////////////////////////////////////////////////////

class DbNopCommand : public DbCommand {
public:
    DbNopCommand(DbCallback *cb);
    virtual ~DbNopCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    std::unique_ptr<DbCallback> callback;
};

// DbOpenCommand ////////////////////////////////////////////////////////////

class DbOpenCallback : public DbCallback {
public:
    void prepare(std::string preErrorMsg);

protected:
    std::string errorMsg;
};

class DbOpenCommand : public DbCommand {
public:
    DbOpenCommand(std::string filename, DbOpenCallback *cb);
    virtual ~DbOpenCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    std::string filename;
    std::unique_ptr<DbOpenCallback> callback;
};

// DbCloseCommand ////////////////////////////////////////////////////////////

class DbCloseCommand : public DbCommand {
public:
    DbCloseCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);
};

// DbGetStationsInfoCommand/////////////////////////////////////////////////

class DbGetStationInfoCallback : public DbCallback {
public:
    std::vector<Wsdb::StationInfo> *prepare(const Wsdb::Limits &preLimits);

protected:
    std::vector<Wsdb::StationInfo> info;
    Wsdb::Limits limits;
};

class DbGetStationInfoCommand : public DbCommand {
public:
    DbGetStationInfoCommand(DbGetStationInfoCallback *cb);
    virtual ~DbGetStationInfoCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    std::unique_ptr<DbGetStationInfoCallback> callback;
};

// DbSetStationInfoCommand /////////////////////////////////////////

class DbSetStationInfoCommand : public DbCommand {
public:
    DbSetStationInfoCommand(const std::vector<Wsdb::StationInfo> &info, const Wsdb::Limits &limits);

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    std::vector<Wsdb::StationInfo> info;
    Wsdb::Limits limits;
};

// DbInsertNameCommand ////////////////////////////////////////////////////

class DbInsertNameCallback : public DbCallback {
public:
    DbInsertNameCallback(int64_t *bookCount);

    void prepare(int64_t num);

protected:
    int64_t *bookCount;
};

class DbInsertNameCommand : public DbCommand {
public:
    DbInsertNameCommand(int64_t slotStart, int64_t slotStop, int64_t station, std::string name, int64_t attr, DbInsertNameCallback *cb);
    virtual ~DbInsertNameCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    int64_t slotStart;
    int64_t slotStop;
    int64_t station;
    std::string name;
    int64_t attr;
    std::unique_ptr<DbInsertNameCallback> callback;
};

// DbSelectNamesCommand ///////////////////////////////////////////////////

class DbSelectNamesCallback : public DbCallback {
public:
    virtual ~DbSelectNamesCallback();

    void prepare(int64_t slot, int64_t station, const std::string &name, int64_t attr);

    class Datum {
    public:
        Datum(int64_t slot, int64_t station, const std::string &name, int64_t attr);
        int64_t slot;
        int64_t station;
        std::string name;
        int64_t attr;
    };

protected:
    std::list<Datum *> data;
};

class DbSelectNamesCommand : public DbCommand {
public:
    DbSelectNamesCommand(int64_t slotStart, int64_t slotStop, int64_t stationStart, int64_t stationStop, DbSelectNamesCallback *cb);
    virtual ~DbSelectNamesCommand();

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    int64_t slotStart;
    int64_t slotStop;
    int64_t stationStart;
    int64_t stationStop;
    std::unique_ptr<DbSelectNamesCallback> callback;
};

// DbRemoveNamesCommand //////////////////////////////////////////////////

class DbRemoveNamesCommand : public DbCommand {
public:
    DbRemoveNamesCommand(int64_t slotStart, int64_t slotStop, int64_t station);

    virtual void execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue);

protected:
    int64_t slotStart;
    int64_t slotStop;
    int64_t station;
};

#endif // DBCOMMAND_H
