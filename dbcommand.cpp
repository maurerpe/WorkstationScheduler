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

#include <exception>

#include "dbcommand.h"

// Abstract classes //////////////////////////////////////////////////////////

DbCallback::DbCallback() {
}

DbCallback::~DbCallback() {
}

void DbCallback::execute() {
}

DbCommand::DbCommand() {
}

DbCommand::~DbCommand() {
}

void DbCommand::execute(Wsdb &, CommandQueue<DbCallback> &cbQueue) {
    cbQueue.add(new DbCallback());
}

// DbNopCommand ///////////////////////////////////////////////////////////////

DbNopCommand::DbNopCommand(DbCallback *cb) : callback(cb) {
}

DbNopCommand::~DbNopCommand() {
}

void DbNopCommand::execute(Wsdb &, CommandQueue<DbCallback> &cbQueue) {
    cbQueue.add(callback.release());
}

// DbOpenCommand //////////////////////////////////////////////////////////////

void DbOpenCallback::prepare(std::string preErrorMsg) {
    errorMsg = preErrorMsg;
}

DbOpenCommand::DbOpenCommand(std::string filename, DbOpenCallback *cb) : filename(filename), callback(cb) {
}

DbOpenCommand::~DbOpenCommand() {
}

void DbOpenCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    try {
        wsdb.open(filename.c_str());
    } catch (std::exception &e) {
        callback->prepare(e.what());
        cbQueue.add(callback.release());
        return;
    }

    cbQueue.add(new DbCallback());
}

// DbCloseCommand ////////////////////////////////////////////////////////////

DbCloseCommand::DbCloseCommand() {
}

void DbCloseCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    wsdb.close();
    cbQueue.add(new DbCallback());
}

// DbGetStationsNamesCommand/////////////////////////////////////////////////

std::vector<Wsdb::StationInfo> *DbGetStationInfoCallback::prepare() {
    return &info;
}

DbGetStationInfoCommand::DbGetStationInfoCommand(DbGetStationInfoCallback *cb) : callback(cb) {
}

DbGetStationInfoCommand::~DbGetStationInfoCommand() {
}

void DbGetStationInfoCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    wsdb.getStationInfo(callback->prepare());
    cbQueue.add(callback.release());
}

// DbSetStationInfoCommand //////////////////////////////////////////

DbSetStationInfoCommand::DbSetStationInfoCommand(const std::vector<Wsdb::StationInfo> &info) : info(info) {
}

void DbSetStationInfoCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    wsdb.setStationInfo(info);
    cbQueue.add(new DbCallback());
}

// DbInsertNameCommand ////////////////////////////////////////////////////

DbInsertNameCallback::DbInsertNameCallback(int64_t *bookCount) : bookCount(bookCount) {
}

void DbInsertNameCallback::prepare(int64_t num) {
    *bookCount += num;
}

DbInsertNameCommand::DbInsertNameCommand(int64_t slotStart, int64_t slotStop, int64_t station, std::string name, int64_t attr, DbInsertNameCallback *cb) :
   slotStart(slotStart), slotStop(slotStop), station(station), name(name), attr(attr), callback(cb) {
}

DbInsertNameCommand::~DbInsertNameCommand() {
}

void DbInsertNameCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    for (int64_t slot = slotStart; slot <= slotStop; slot++)
        callback->prepare(wsdb.insertName(slot, station, name.c_str(), attr));
    cbQueue.add(callback.release());
}

// DbSelectNamesCommand //////////////////////////////////////////////////

DbSelectNamesCallback::Datum::Datum(int64_t slot, int64_t station, const std::string &name, int64_t attr) :
   slot(slot), station(station), name(name), attr(attr) {
}

void DbSelectNamesCallback::prepare(int64_t slot, int64_t station, const std::string &name, int64_t attr) {
    data.push_back(new Datum(slot, station, name, attr));
}

DbSelectNamesCommand::DbSelectNamesCommand(int64_t slotStart, int64_t slotStop, int64_t stationStart, int64_t stationStop, DbSelectNamesCallback *cb) :
    slotStart(slotStart), slotStop(slotStop), stationStart(stationStart), stationStop(stationStop), callback(cb) {
}

DbSelectNamesCommand::~DbSelectNamesCommand() {
}

class DbWsdbCallback : public WsdbCallback {
public:
    DbWsdbCallback(DbSelectNamesCallback *cb) : cb(cb) {}

    virtual void callback(int64_t slot, int64_t station, const char *name, int64_t attr);

private:
    DbSelectNamesCallback *cb;
};

void DbWsdbCallback::callback(int64_t slot, int64_t station, const char *name, int64_t attr) {
    cb->prepare(slot, station, std::string(name), attr);
}

void DbSelectNamesCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    DbWsdbCallback cb(callback.get());

    wsdb.selectNames(slotStart, slotStop, stationStart, stationStop, cb);
    cbQueue.add(callback.release());
}

// DbRemoveNamesCommand ///////////////////////////////////////////////////

DbRemoveNamesCommand::DbRemoveNamesCommand(int64_t slotStart, int64_t slotStop, int64_t station) :
    slotStart(slotStart), slotStop(slotStop), station(station) {
}

void DbRemoveNamesCommand::execute(Wsdb &wsdb, CommandQueue<DbCallback> &cbQueue) {
    wsdb.removeNames(slotStart, slotStop, station);
    cbQueue.add(new DbCallback());
}
