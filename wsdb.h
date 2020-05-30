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

#ifndef WSDB_H
#define WSDB_H

#include <sqlite3.h>
#include <stdint.h>
#include <string>
#include <vector>

class WsdbCallback {
public:
    virtual ~WsdbCallback() {}

    virtual void callback(int64_t slot, int64_t station, const char *name, int64_t attr);
};

class Wsdb{
public:
    Wsdb();
    ~Wsdb();

    void open(const char *filename);
    void close();

    class Limits {
    public:
        Limits();
        Limits(int64_t yellow, int64_t red) :
            yellow(yellow), red(red) {}

        int64_t yellow;
        int64_t red;
    };

    class StationInfo {
    public:
        StationInfo(std::string name, std::string desc, int64_t flags) :
            name(name), desc(desc), flags(flags) {}

        std::string name;
        std::string desc;
        int64_t flags;
    };

    int64_t getNumStations();
    void setNumStations(int64_t num);
    Limits getLimits();
    void setLimits(Limits &limits);
    void getStationInfo(std::vector<StationInfo> *infoOut);
    void setStationInfo(int64_t station, const StationInfo &info);
    void setStationInfo(const std::vector<StationInfo> &info);

    int insertName(int64_t slot, int64_t station, const char *name, int64_t attr); // 1 on sucess, 0 on error
    void selectNames(int64_t slotStart, int64_t slotStop, int64_t stationStart, int64_t stationStop, WsdbCallback &callback);
    void removeNames(int64_t slotStart, int64_t slotStop, int64_t station);

    static std::string defaultWorkstationName(int64_t station);

private:
    int64_t getParameter(const char *name, int64_t default_val);
    void setParameter(const char *name, int64_t value);

private:
    sqlite3 *db;
    sqlite3_stmt *getParam;
    sqlite3_stmt *setParam;
    sqlite3_stmt *getInfo;
    sqlite3_stmt *setInfo;
    sqlite3_stmt *cleanInfo;
    sqlite3_stmt *insert;
    sqlite3_stmt *select;
    sqlite3_stmt *remove;
};

#endif // WSDB_H
