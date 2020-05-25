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

#include <chrono>

#include "threadeddb.h"

static void startThreadedDb(ThreadedDb *tdb) {
    tdb->run();
}

ThreadedDb::ThreadedDb() : outstandingCommands(0) {
    thread = new std::thread(startThreadedDb, this);
}

ThreadedDb::~ThreadedDb() {
    cmdQueue.add(new DbCloseCommand());
    cmdQueue.add(nullptr);

    thread->join();
    delete thread;
}

void ThreadedDb::queueCommand(DbCommand *cmd, size_t refreshId) {
    outstandingCommands += cmdQueue.add(cmd, refreshId);
}

void ThreadedDb::checkCallbacks() {
    DbCallback *cb;

    while ((cb = cbQueue.pop(false))) {
        cb->execute();
        delete cb;
        outstandingCommands--;
    }
}

bool ThreadedDb::isProcessing() {
    return outstandingCommands > 0;
}

void ThreadedDb::run() {
    DbCommand *cmd;

    while ((cmd = cmdQueue.pop())) {
        cmd->execute(wsdb, cbQueue);
        delete cmd;

        // Use this to simulate a slow filesystem
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
