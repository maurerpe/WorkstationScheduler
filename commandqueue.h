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

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <condition_variable>
#include <mutex>
#include <vector>

template <class T>
class CommandQueue {
private:
    class QueuedItem {
  public:
        QueuedItem *prev;
        QueuedItem *next;
        T *data;
        size_t id;
    };
  
    std::mutex mutex;
    std::condition_variable cond;

    QueuedItem *first;
    QueuedItem *last;
    std::vector<QueuedItem *> refresh;
    bool readerWaiting;
  
    void append(T *data, size_t id) {
        QueuedItem *qi = new QueuedItem();
        qi->prev = last;
        qi->next = nullptr;
        qi->data = data;
        qi->id   = id;

        if (last != nullptr)
            last->next = qi;

        last = qi;
        if (first == nullptr)
            first = qi;
    }

    void remove(QueuedItem *qi) {
        if (qi->prev)
            qi->prev->next = qi->next;
        else
            first = qi->next;

        if (qi->next)
            qi->next->prev = qi->prev;
        else
            last = qi->prev;

        if (qi->id != 0 && qi->id <= refresh.size() && refresh[qi->id-1] == qi)
            refresh[qi->id-1] = nullptr;

        delete qi;
    }

    int purgeRefresh(size_t id) {
        QueuedItem *qi;

        if (id == 0 || id > refresh.size() || (qi = refresh[id - 1]) == nullptr)
            return 0;

        delete qi->data;
        remove(qi);

        return 1;
    }

    void refreshAddLast() {
        size_t id = last->id;

        if (id == 0)
            return;

        if (refresh.size() < id)
            refresh.resize(id, nullptr);

        refresh[id - 1] = last;
    }

public:
    CommandQueue() : first(nullptr), last(nullptr), readerWaiting(false) {}

    ~CommandQueue() {
    }

    int add(T *data, size_t refreshId = 0) {
        std::unique_lock<std::mutex> lock(mutex);

        int num = purgeRefresh(refreshId);
        append(data, refreshId);
        refreshAddLast();

        if (readerWaiting)
            cond.notify_one();

        return 1 - num;
    }
  
    T *pop(bool block = true) {
        std::unique_lock<std::mutex> lock(mutex);
    
        if (!block && first == nullptr)
            return nullptr;

        readerWaiting = true;
        while (first == nullptr)
            cond.wait(lock);
        readerWaiting = false;

        T *data = first->data;
        remove(first);
    
        return data;
    }
};

#endif // COMMANDQUEUE_H
